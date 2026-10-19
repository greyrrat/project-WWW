#include "app.h"

/* Changes the SPI1 clock: slow for the SD start-up, faster afterwards */
void Spi_SetPrescaler(uint32_t presc)
{
  __HAL_SPI_DISABLE(&hspi1);
  MODIFY_REG(hspi1.Instance->CR1, SPI_CR1_BR, presc);
  hspi1.Init.BaudRatePrescaler = presc;
  __HAL_SPI_ENABLE(&hspi1);
}
