/* Milestone 1: blink LD2 (green) and print over USART2 (ST-LINK virtual COM port). No RTOS. */
#include "app.h"
#include "main.h"
#include <stdio.h>

extern UART_HandleTypeDef huart2;

/* syscalls.c routes printf() -> _write() -> __io_putchar(). */
int __io_putchar(int ch)
{
  uint8_t c = (uint8_t)ch;
  HAL_UART_Transmit(&huart2, &c, 1, HAL_MAX_DELAY);
  return ch;
}

void app_run_blinky(void)
{
  setvbuf(stdout, NULL, _IONBF, 0); /* print immediately, no buffering */
  printf("\r\nBMS H5 test firmware (m1-blinky-uart)\r\n");

  for (uint32_t n = 0;; n++) {
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin); /* toggles every 500 ms */
    if (n % 2 == 0) printf("BMS H5 alive %lu\r\n", (unsigned long)(n / 2));
    HAL_Delay(500);
  }
}
