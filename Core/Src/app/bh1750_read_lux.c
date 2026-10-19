#include "app.h"

uint8_t BH1750_ReadLux(uint32_t *lux_x100)
{
  uint8_t rx[2] = { 0, 0 };
  uint16_t raw;
  if (HAL_I2C_Master_Receive(&hi2c1, (0x23U << 1), rx, 2, 100) != HAL_OK) return 0;
  raw = (uint16_t)(((uint16_t)rx[0] << 8) | rx[1]);
  *lux_x100 = ((uint32_t)raw * 1000U) / 12U;
  return 1;
}
