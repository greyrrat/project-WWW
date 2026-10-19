#include "app.h"

DRESULT SD_Disk_Write(BYTE pdrv, const BYTE *buf, DWORD sector, UINT count)
{
  (void)pdrv;
  if (sd_stat & STA_NOINIT) return RES_NOTRDY;

  while (count--)
  {
    uint32_t addr = sd_is_hc ? sector : sector * 512;
    if (sd_cmd(SD_CMD24, addr) != 0) { sd_deselect(); return RES_ERROR; }

    spi_rw(0xFF);
    spi_rw(0xFE);
    for (int i = 0; i < 512; i++) spi_rw(buf[i]);
    spi_rw(0xFF); spi_rw(0xFF);

    if ((spi_rw(0xFF) & 0x1F) != 0x05) { sd_deselect(); return RES_ERROR; }   /* card refused the data */
    if (!sd_wait_ready(500)) { sd_deselect(); return RES_ERROR; }            /* still busy after 500 ms */
    sd_deselect();

    /* ask the card if the write really worked (CMD13 = status, 2 bytes must be 0) */
    if (sd_cmd(SD_CMD13, 0) != 0 || spi_rw(0xFF) != 0) { sd_deselect(); return RES_ERROR; }
    sd_deselect();

    buf += 512;
    sector++;
  }
  return RES_OK;
}
