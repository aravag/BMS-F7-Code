/* Milestone 3: micro-ROS task only.
 * USART2 is now owned by micro-ROS, so there is no printf on it any more. */
#include "app.h"
#include "cmsis_os2.h"
#include "ros_node.h"

void app_start(void)
{
  /* micro-ROS needs > 10 kB of stack (micro_ros_stm32cubemx_utils README). */
  const osThreadAttr_t ros_attr = { .name = "ros", .stack_size = 3000 * 4, .priority = osPriorityNormal };
  osThreadNew(ros_task, NULL, &ros_attr);
}
