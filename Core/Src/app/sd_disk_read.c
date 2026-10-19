#include "app.h"

DRESULT SD_Disk_Read(BYTE pdrv, BYTE *buf, DWORD sector, UINT count)
{
  uint8_t tok;
  uint32_t t;
  (void)pdrv;
  if (sd_stat & STA_NOINIT) return RES_NOTRDY;

  while (count--)
  {
    uint32_t addr = sd_is_hc ? sector : sector * 512;
    if (sd_cmd(SD_CMD17, addr) != 0) { sd_deselect(); return RES_ERROR; }

    t = HAL_GetTick();
    do { tok = spi_rw(0xFF); } while (tok == 0xFF && (HAL_GetTick() - t) < 200);
    if (tok != 0xFE) { sd_deselect(); return RES_ERROR; }

    for (int i = 0; i < 512; i++) buf[i] = spi_rw(0xFF);
    spi_rw(0xFF); spi_rw(0xFF);
    sd_deselect();

    buf += 512;
    sector++;
  }
  return RES_OK;
}
