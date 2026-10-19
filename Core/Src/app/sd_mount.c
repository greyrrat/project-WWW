#include "app.h"

/* ---- Mount / unmount (3 attempts: the card can be slow to answer) ---- */
bool SD_Mount(void)
{
  if (sd_mounted) return true;
  for (uint8_t attempt = 1U; attempt <= 3U; attempt++)
  {
    FRESULT fr = FR_DISK_ERR;
    if (SD_Disk_Init(0) == 0U) fr = f_mount(&sd_fs, "0:", 1);
    if (fr == FR_OK)
    {
      sd_mounted = true;
      DBG("[SD] Carte montee\r\n");
      return true;
    }
    DBG("[SD] Essai %u/3 echoue (etape %u, f_mount=%d)\r\n", (unsigned)attempt, (unsigned)sd_fail_step, (int)fr);
    if (fr == FR_NO_FILESYSTEM) break;      /* not FAT32: retrying is useless */
    HAL_Delay(200);
  }
  return false;
}
