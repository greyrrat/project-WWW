#include "app.h"

uint8_t sd_cmd(uint8_t cmd, uint32_t arg)
{
  uint8_t r, n;

  if (cmd & 0x80)
  {
    cmd &= 0x7F;
    r = sd_cmd(SD_CMD55, 0);
    if (r > 1) return r;
  }

  sd_deselect();
  if (cmd == SD_CMD0)
  {
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
    spi_rw(0xFF);
  }
  else
  {
    if (!sd_select()) return 0xFF;
  }

  spi_rw(0x40 | cmd);
  spi_rw((uint8_t)(arg >> 24));
  spi_rw((uint8_t)(arg >> 16));
  spi_rw((uint8_t)(arg >> 8));
  spi_rw((uint8_t)arg);
  n = 0x01;
  if (cmd == SD_CMD0) n = 0x95;
  if (cmd == SD_CMD8) n = 0x87;
  spi_rw(n);

  n = 10;
  do { r = spi_rw(0xFF); } while ((r & 0x80) && --n);
  return r;
}
