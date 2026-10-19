#include "app.h"

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  uint8_t c;
  if (huart->Instance != USART2) return;
  c = rx_byte;
  if (!rx_ready)
  {
    if (c == '\r' || c == '\n')
    {
      if (rx_len > 0U && !rx_overflow)
      {
        rx_line[rx_len] = '\0';
        rx_ready = 1U;
        Evt_Set(EVT_UART_LINE);
      }
      rx_len = 0U;
      rx_overflow = 0U;
    }
    else if (rx_len < (RX_MAX - 1U)) rx_line[rx_len++] = (char)c;
    else rx_overflow = 1U;
  }
  HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}
