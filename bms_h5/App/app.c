/* Milestone 2: two FreeRTOS tasks. One blinks LD2, the other prints over USART2
 * (ST-LINK virtual COM port). Both running at once shows the scheduler works. */
#include "app.h"
#include "main.h"
#include "cmsis_os2.h"
#include <stdio.h>

extern UART_HandleTypeDef huart2;

/* syscalls.c routes printf() -> _write() -> __io_putchar(). */
int __io_putchar(int ch)
{
  uint8_t c = (uint8_t)ch;
  HAL_UART_Transmit(&huart2, &c, 1, HAL_MAX_DELAY);
  return ch;
}

static void led_task(void *arg)
{
  (void)arg;
  for (;;) {
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin); /* toggles every 250 ms */
    osDelay(250);
  }
}

static void print_task(void *arg)
{
  (void)arg;
  setvbuf(stdout, NULL, _IONBF, 0); /* print immediately, no buffering */
  printf("\r\nBMS H5 test firmware (m2-freertos)\r\n");
  for (uint32_t n = 0;; n++) {
    printf("BMS H5 alive %lu (FreeRTOS)\r\n", (unsigned long)n);
    osDelay(1000);
  }
}

void app_start(void)
{
  const osThreadAttr_t led_attr = { .name = "led", .stack_size = 256 * 4, .priority = osPriorityNormal };
  const osThreadAttr_t print_attr = { .name = "print", .stack_size = 512 * 4, .priority = osPriorityNormal };
  osThreadNew(led_task, NULL, &led_attr);
  osThreadNew(print_task, NULL, &print_attr);
}
