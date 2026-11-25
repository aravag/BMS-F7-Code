/* micro-ROS node "bms_node" (transport: USART2 = ST-LINK virtual COM port, via it_transport.c).
 *   publishes  /bms/battery_state      sensor_msgs/BatteryState  1 Hz
 *   subscribes /bms/cmd_output_enable  std_msgs/Bool             -> output_enabled (shown on LD2 while connected)
 * Agent state machine: waiting -> available (create entities) -> connected (ping every 1 s)
 * -> disconnected (destroy entities) -> waiting, so the agent can be restarted without resetting the board. */
#include "ros_node.h"
#include "bms_data.h"
#include "main.h"
#include "cmsis_os2.h"

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>
#include <sensor_msgs/msg/battery_state.h>
#include <std_msgs/msg/bool.h>

extern UART_HandleTypeDef huart2;

/* From micro_ros_stm32cubemx_utils/extra_sources */
bool cubemx_transport_open(struct uxrCustomTransport *transport);
bool cubemx_transport_close(struct uxrCustomTransport *transport);
size_t cubemx_transport_write(struct uxrCustomTransport *transport, uint8_t *buf, size_t len, uint8_t *err);
size_t cubemx_transport_read(struct uxrCustomTransport *transport, uint8_t *buf, size_t len, int timeout, uint8_t *err);
void *microros_allocate(size_t size, void *state);
void microros_deallocate(void *pointer, void *state);
void *microros_reallocate(void *pointer, size_t size, void *state);
void *microros_zero_allocate(size_t number_of_elements, size_t size_of_element, void *state);

volatile bool ros_connected = false;
volatile bool ros_setup_failed = false;
volatile bool output_enabled = true;

static rclc_support_t support;
static rcl_node_t node;
static rcl_publisher_t battery_pub;
static rcl_subscription_t cmd_sub;
static rcl_timer_t publish_timer;
static rclc_executor_t executor;

static sensor_msgs__msg__BatteryState battery_msg;
static float cell_voltage_buf[BMS_CELL_COUNT];
static std_msgs__msg__Bool cmd_msg;

static void set_string(rosidl_runtime_c__String *s, char *text, size_t len)
{
  s->data = text;
  s->size = len;
  s->capacity = len + 1;
}

/* Point the message's string/sequence fields at static buffers (no heap use per message). */
static void battery_msg_init(void)
{
  static char frame_id[] = "bms";
  static char empty[] = "";
  set_string(&battery_msg.header.frame_id, frame_id, sizeof(frame_id) - 1);
  set_string(&battery_msg.location, empty, 0);
  set_string(&battery_msg.serial_number, empty, 0);
  battery_msg.cell_voltage.data = cell_voltage_buf;
  battery_msg.cell_voltage.size = BMS_CELL_COUNT;
  battery_msg.cell_voltage.capacity = BMS_CELL_COUNT;
  battery_msg.power_supply_status = sensor_msgs__msg__BatteryState__POWER_SUPPLY_STATUS_DISCHARGING;
  battery_msg.power_supply_health = sensor_msgs__msg__BatteryState__POWER_SUPPLY_HEALTH_GOOD;
  battery_msg.power_supply_technology = sensor_msgs__msg__BatteryState__POWER_SUPPLY_TECHNOLOGY_LION;
  battery_msg.present = true;
}

static void publish_timer_cb(rcl_timer_t *timer, int64_t last_call_time)
{
  (void)last_call_time;
  if (timer == NULL) return;

  battery_snapshot_t b;
  bms_data_get(&b);

  int64_t now_ns = rmw_uros_epoch_nanos(); /* agent-synchronised time */
  battery_msg.header.stamp.sec = (int32_t)(now_ns / 1000000000LL);
  battery_msg.header.stamp.nanosec = (uint32_t)(now_ns % 1000000000LL);
  battery_msg.voltage = b.voltage;
  battery_msg.current = b.current;
  battery_msg.charge = b.charge;
  battery_msg.capacity = b.capacity;
  battery_msg.design_capacity = b.capacity;
  battery_msg.percentage = b.percentage;
  battery_msg.temperature = b.temperature;
  for (int i = 0; i < BMS_CELL_COUNT; i++) cell_voltage_buf[i] = b.cell_voltage[i];

  /* A dropped sample is fine: ros_task's ping notices a dead link. */
  rcl_ret_t rc = rcl_publish(&battery_pub, &battery_msg, NULL);
  (void)rc;
}

static void cmd_cb(const void *msgin)
{
  const std_msgs__msg__Bool *m = msgin;
  output_enabled = m->data;
}

static bool create_entities(void)
{
  /* Fresh handles every (re)connect: rcl refuses to init a handle that still looks in use. */
  node = rcl_get_zero_initialized_node();
  battery_pub = rcl_get_zero_initialized_publisher();
  cmd_sub = rcl_get_zero_initialized_subscription();
  publish_timer = rcl_get_zero_initialized_timer();
  executor = rclc_executor_get_zero_initialized_executor();

  rcl_allocator_t allocator = rcl_get_default_allocator();
  if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK) return false;
  if (rclc_node_init_default(&node, "bms_node", "", &support) != RCL_RET_OK) return false;
  if (rclc_publisher_init_default(&battery_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, BatteryState), "/bms/battery_state") != RCL_RET_OK) return false;
  if (rclc_subscription_init_default(&cmd_sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool), "/bms/cmd_output_enable") != RCL_RET_OK) return false;
  if (rclc_timer_init_default(&publish_timer, &support, RCL_MS_TO_NS(1000), publish_timer_cb) != RCL_RET_OK) return false;

  if (rclc_executor_init(&executor, &support.context, 2, &allocator) != RCL_RET_OK) return false;
  if (rclc_executor_add_timer(&executor, &publish_timer) != RCL_RET_OK) return false;
  if (rclc_executor_add_subscription(&executor, &cmd_sub, &cmd_msg, cmd_cb, ON_NEW_DATA) != RCL_RET_OK) return false;

  rmw_uros_sync_session(1000); /* so header.stamp is real time, not time since boot */
  return true;
}

static void destroy_entities(void)
{
  /* The agent may be gone: don't wait for it to confirm each deletion. */
  rmw_context_t *rmw_context = rcl_context_get_rmw_context(&support.context);
  if (rmw_context != NULL) (void)rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

  rcl_ret_t rc; /* results ignored: some of these may never have been created */
  rc = rcl_publisher_fini(&battery_pub, &node);
  rc = rcl_subscription_fini(&cmd_sub, &node);
  rc = rcl_timer_fini(&publish_timer);
  rc = rclc_executor_fini(&executor);
  rc = rcl_node_fini(&node);
  rc = rclc_support_fini(&support);
  (void)rc;
}

void ros_task(void *argument)
{
  (void)argument;

  rmw_uros_set_custom_transport(true, (void *)&huart2,
                                cubemx_transport_open, cubemx_transport_close,
                                (write_custom_func)cubemx_transport_write, /* utils file omits const */
                                cubemx_transport_read);

  rcl_allocator_t freertos_allocator = rcutils_get_zero_initialized_allocator();
  freertos_allocator.allocate = microros_allocate;
  freertos_allocator.deallocate = microros_deallocate;
  freertos_allocator.reallocate = microros_reallocate;
  freertos_allocator.zero_allocate = microros_zero_allocate;
  if (!rcutils_set_default_allocator(&freertos_allocator)) {
    ros_setup_failed = true;
    for (;;) osDelay(1000);
  }

  battery_msg_init();

  enum { WAITING_AGENT, AGENT_AVAILABLE, AGENT_CONNECTED, AGENT_DISCONNECTED } state = WAITING_AGENT;
  uint32_t last_ping = 0;

  for (;;) {
    switch (state) {
    case WAITING_AGENT:
      if (rmw_uros_ping_agent(100, 1) == RMW_RET_OK) state = AGENT_AVAILABLE;
      else osDelay(500);
      break;

    case AGENT_AVAILABLE:
      if (create_entities()) {
        ros_setup_failed = false;
        ros_connected = true;
        last_ping = osKernelGetTickCount();
        state = AGENT_CONNECTED;
      } else {
        ros_setup_failed = true;
        destroy_entities();
        state = WAITING_AGENT;
      }
      break;

    case AGENT_CONNECTED:
      if (osKernelGetTickCount() - last_ping >= 1000) {
        last_ping = osKernelGetTickCount();
        if (rmw_uros_ping_agent(100, 3) != RMW_RET_OK) {
          state = AGENT_DISCONNECTED;
          break;
        }
      }
      rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
      osDelay(10);
      break;

    case AGENT_DISCONNECTED:
      ros_connected = false;
      destroy_entities();
      state = WAITING_AGENT;
      break;
    }
  }
}
