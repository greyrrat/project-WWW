#include "app.h"

void sd_deselect(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
  spi_rw(0xFF);
}
