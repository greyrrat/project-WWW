#include "app.h"

/* GPS bytes arrive here one by one. Only the GGA sentence (position) is kept. */
void USART1_IRQHandler(void)
{
  static char line[GPS_LINE_MAX];
  static uint8_t len = 0U;
  uint32_t isr = USART1->ISR;

  if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE))
    USART1->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NCF;   /* clear errors, keep going */

  if (isr & USART_ISR_RXNE)
  {
    char c = (char)USART1->RDR;
    if (c == '$') len = 0U;                                         /* start of a sentence */
    if (c == '\r' || c == '\n')
    {
      if (len > 6U) gps_last_ms = HAL_GetTick();                    /* the GPS is talking */
      if (len > 6U && line[3] == 'G' && line[4] == 'G' && line[5] == 'A')
      {
        line[len] = '\0';
        memcpy(gps_gga, line, (size_t)len + 1U);
      }
      len = 0U;
    }
    else if (len < GPS_LINE_MAX - 1U) line[len++] = c;
  }
}
