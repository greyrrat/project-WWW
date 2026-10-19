#include "app.h"

/* ---- BH1750 light sensor on I2C1 ---- */
uint8_t BH1750_Init(void)
{
  uint8_t cmd = 0x01;
  if (HAL_I2C_Master_Transmit(&hi2c1, (0x23U << 1), &cmd, 1, 100) != HAL_OK) return 0;
  cmd = 0x10;
  if (HAL_I2C_Master_Transmit(&hi2c1, (0x23U << 1), &cmd, 1, 100) != HAL_OK) return 0;
  HAL_Delay(180);
  return 1;
}
