#include "app.h"

/* SDTEST command: restarts the card, writes SDTEST.TXT, reads it back and compares */
bool SD_SelfTest(void)
{
  static uint32_t count = 0U;
  char wr[48], rd[48];
  UINT n = 0U;
  FRESULT fr;
  bool ok;
  int len;
  uint32_t t0 = HAL_GetTick();


  SD_Unmount();                                  /* full restart of the card, like at power-up */
  if (!SD_Mount())
  {
    printf("[SDTEST] ERROR: carte absente ou illisible (etape %u)\r\n", (unsigned)sd_fail_step);
    return false;
  }

  count++;
  len = snprintf(wr, sizeof(wr), "SDTEST %lu t=%lu ms\r\n", (unsigned long)count, (unsigned long)t0);

  fr = f_open(&sd_file, "0:/SDTEST.TXT", FA_CREATE_ALWAYS | FA_WRITE);
  if (fr == FR_OK)
  {
    FRESULT fc;
    fr = f_write(&sd_file, wr, (UINT)len, &n);
    fc = f_close(&sd_file);
    if (fr == FR_OK) fr = fc;
    if (fr == FR_OK && n != (UINT)len) fr = FR_DENIED;   /* card full */
  }

  memset(rd, 0, sizeof(rd));
  if (fr == FR_OK)
  {
    fr = f_open(&sd_file, "0:/SDTEST.TXT", FA_READ);
    if (fr == FR_OK) { fr = f_read(&sd_file, rd, sizeof(rd) - 1U, &n); (void)f_close(&sd_file); }
  }

  ok = (fr == FR_OK) && (strcmp(rd, wr) == 0);
  printf("[SDTEST] %s : carte %s, ecriture + relecture en %lu ms (FRESULT=%d)\r\n",
         ok ? "OK" : "ERROR", sd_is_hc ? "SDHC" : "SD", (unsigned long)(HAL_GetTick() - t0), (int)fr);

  if (app_mode != MODE_STANDARD && app_mode != MODE_ECONOMY) SD_Unmount();   /* card stays removable */
  else if (!ok) SD_Fail(ERR_SD_ACC);
  return ok;
}
