#include "app.h"

void USART2_IRQHandler(void)   { HAL_UART_IRQHandler(&huart2); }
