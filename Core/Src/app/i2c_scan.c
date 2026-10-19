#include "app.h"
#include "features.h"

/* Prints the address of every device found on the I2C bus */
void I2c_Scan(void)
{
  uint8_t n = 0U;
  printf("[I2C] Scan du bus...\r\n");
  for (uint8_t a = 1U; a < 127U; a++)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(a << 1), 1, 10) == HAL_OK)
    {
      printf("[I2C] Appareil trouve a 0x%02X\r\n", (unsigned)a);
      n++;
    }
  }
  if (n == 0U) printf("[I2C] Aucun appareil : verifier SDA/SCL et l'alimentation\r\n");
}
