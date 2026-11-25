#ifndef APP_H
#define APP_H

/* Milestone 1: called from main() after the CubeMX peripheral init. Never returns,
 * so the FreeRTOS scheduler is not started in this milestone. */
void app_run_blinky(void);

#endif
