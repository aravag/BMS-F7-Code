#ifndef ROS_NODE_H
#define ROS_NODE_H

#include <stdbool.h>

/* True while a session with the micro-ROS agent is up (read by the LED task). */
extern volatile bool ros_connected;
/* True when micro-ROS setup failed (it keeps retrying). */
extern volatile bool ros_setup_failed;
/* Last value received on /bms/cmd_output_enable (stand-in for the real output switch). */
extern volatile bool output_enabled;

/* FreeRTOS task: talks micro-ROS to the agent over USART2. */
void ros_task(void *argument);

#endif
