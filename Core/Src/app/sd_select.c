#include "app.h"

uint8_t sd_select(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
  spi_rw(0xFF);
  if (sd_wait_ready(500)) return 1;
  sd_deselect();
  return 0;
}
