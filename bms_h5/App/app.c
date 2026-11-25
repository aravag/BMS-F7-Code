/* Milestone 4: BMS test node.
 *   battery_task  samples the (fake) battery every 100 ms into bms_data
 *   ros_task      micro-ROS: publishes bms_data, receives commands
 *   led_task      LD2 (the only user LED):
 *                   slow blink (0.5 s) = looking for the agent
 *                   fast blink (0.1 s) = micro-ROS setup failed, retrying
 *                   connected: on/off  = last /bms/cmd_output_enable value (starts on) */
#include "app.h"
#include "main.h"
#include "cmsis_os2.h"
#include "bms_data.h"
#include "battery_source.h"
#include "ros_node.h"

static void battery_task(void *arg)
{
  (void)arg;
  battery_snapshot_t b;
  uint32_t next = osKernelGetTickCount();
  for (;;) {
    battery_source_read(&b);
    bms_data_update(&b);
    next += 100;
    osDelayUntil(next);
  }
}

static void led_task(void *arg)
{
  (void)arg;
  for (uint32_t n = 0;; n++) {
    if (ros_setup_failed) {
      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    } else if (ros_connected) {
      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, output_enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
    } else if (n % 5 == 0) {
      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    }
    osDelay(100);
  }
}

void app_init(void)
{
  battery_source_init();
}

void app_start(void)
{
  bms_data_init(); /* creates a mutex: needs the RTOS initialised, so not in app_init() */

  const osThreadAttr_t battery_attr = { .name = "battery", .stack_size = 512 * 4, .priority = osPriorityNormal };
  const osThreadAttr_t led_attr = { .name = "led", .stack_size = 256 * 4, .priority = osPriorityLow };
  /* micro-ROS needs > 10 kB of stack (micro_ros_stm32cubemx_utils README). */
  const osThreadAttr_t ros_attr = { .name = "ros", .stack_size = 3000 * 4, .priority = osPriorityNormal };
  osThreadNew(battery_task, NULL, &battery_attr);
  osThreadNew(led_task, NULL, &led_attr);
  osThreadNew(ros_task, NULL, &ros_attr);
}
