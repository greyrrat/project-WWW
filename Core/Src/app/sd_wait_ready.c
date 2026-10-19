#include "app.h"

uint8_t sd_wait_ready(uint32_t ms)
{
  uint32_t t = HAL_GetTick();
  uint8_t r;
  do { r = spi_rw(0xFF); } while (r != 0xFF && (HAL_GetTick() - t) < ms);
  return (r == 0xFF);
}
