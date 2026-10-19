#include "app.h"

/* ========================= SD CARD (SPI1) ============================= */
uint8_t spi_rw(uint8_t tx)
{
  uint8_t rx = 0xFF;
  HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 100);
  return rx;
}
