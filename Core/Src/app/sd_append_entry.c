#include "app.h"

/* Writes one line in  YEAR/MONTH/DAY/YYMMDD_0.LOG  (folders created if missing) */
bool SD_AppendEntry(const LogEntry_t *e)
{
  char p0[32];
  FILINFO fno;
  FRESULT fr, fc;
  UINT bw = 0U;
  size_t len = strlen(e->text);

  if (!SD_Mount()) { Err_Set(ERR_SD_ACC); return false; }
  if (!SD_MakeDirs(e->tag)) { SD_Fail(ERR_SD_ACC); return false; }

  snprintf(p0, sizeof(p0), "%s/%.6s_0.LOG", sd_day_dir, e->tag);
  if (f_stat(p0, &fno) == FR_OK && ((uint32_t)fno.fsize + len) > (uint32_t)sys_cfg.file_max_size)
    SD_Rotate(e->tag, p0);

  fr = f_open(&sd_file, p0, FA_OPEN_ALWAYS | FA_WRITE);
  if (fr != FR_OK) { SD_Fail(ERR_SD_ACC); return false; }

  fr = f_lseek(&sd_file, f_size(&sd_file));
  if (fr == FR_OK) fr = f_write(&sd_file, e->text, len, &bw);
  fc = f_close(&sd_file);
  if (fr != FR_OK || fc != FR_OK) { SD_Fail(ERR_SD_ACC); return false; }
  if (bw != len) { SD_Fail(ERR_SD_FULL); return false; }       /* short write = card full */

  DBG("[SD] Ecrit dans %s\r\n", p0 + 3);
  return true;
}
