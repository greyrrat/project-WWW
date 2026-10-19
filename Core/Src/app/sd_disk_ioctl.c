#include "app.h"

DRESULT SD_Disk_Ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  (void)pdrv;
  if (sd_stat & STA_NOINIT) return RES_NOTRDY;

  switch (cmd)
  {
    case CTRL_SYNC:
      if (sd_select()) { sd_deselect(); return RES_OK; }
      return RES_ERROR;
    case GET_SECTOR_SIZE:
      *(WORD *)buff = 512;
      return RES_OK;
    case GET_BLOCK_SIZE:
      *(DWORD *)buff = 1;
      return RES_OK;
    default:
      return RES_PARERR;
  }
}
