#include "app.h"

/* USART1 for the GPS: PA9 = TX (D8) -> GPS RX, PA10 = RX (D2) <- GPS TX. 9600 bauds, receive interrupt. */
void Gps_Init(void)
{
  GPIO_InitTypeDef g = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();

  g.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  g.Mode = GPIO_MODE_AF_PP;
  g.Pull = GPIO_PULLUP;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  g.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &g);

  USART1->CR1 = 0U;
  USART1->BRR = (HAL_RCC_GetPCLK2Freq() + GPS_BAUD / 2U) / GPS_BAUD;
  USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE | USART_CR1_UE;

  HAL_NVIC_SetPriority(USART1_IRQn, 3U, 0U);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}
