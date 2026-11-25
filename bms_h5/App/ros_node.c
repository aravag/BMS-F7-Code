/* Milestone 3: micro-ROS node "bms_node" publishing std_msgs/Int32 on /bms/heartbeat at 1 Hz.
 * Transport: USART2 (ST-LINK virtual COM port) using micro_ros_stm32cubemx_utils' it_transport.c.
 * LD2 (only user LED): solid on = waiting for the agent, toggles on every successful publish,
 * fast blink = micro-ROS setup failed. */
#include "ros_node.h"
#include "main.h"
#include "cmsis_os2.h"

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/int32.h>

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

static void fail_forever(void)
{
  for (;;) {
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    osDelay(100);
  }
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
  if (!rcutils_set_default_allocator(&freertos_allocator)) fail_forever();

  /* Wait until the agent answers, so plug/start order doesn't matter. */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
  while (rmw_uros_ping_agent(100, 1) != RMW_RET_OK) {
    osDelay(500);
  }

  rcl_allocator_t allocator = rcl_get_default_allocator();
  rclc_support_t support;
  rcl_node_t node;
  rcl_publisher_t publisher;
  std_msgs__msg__Int32 msg = { .data = 0 };

  if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK ||
      rclc_node_init_default(&node, "bms_node", "", &support) != RCL_RET_OK ||
      rclc_publisher_init_default(&publisher, &node,
                                  ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
                                  "/bms/heartbeat") != RCL_RET_OK) {
    fail_forever();
  }

  for (;;) {
    if (rcl_publish(&publisher, &msg, NULL) == RCL_RET_OK) {
      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    }
    msg.data++;
    osDelay(1000);
  }
}
