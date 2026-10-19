#include "app.h"

char sd_day_dir[16] = "";                        /* "0:/2026/10/19" once created ("" = check again) */

/* Creates 0:/20YY, 0:/20YY/MM and 0:/20YY/MM/DD for tag "YYMMDD" (only once per day) */
bool SD_MakeDirs(const char *tag)
{
  char d[16];
  snprintf(d, sizeof(d), "0:/20%.2s/%.2s/%.2s", tag, tag + 2, tag + 4);
  if (strcmp(d, sd_day_dir) == 0) return true;   /* already done today */

  for (uint8_t cut = 7U; cut <= 13U; cut += 3U)  /* "0:/2026" then "0:/2026/10" then "0:/2026/10/19" */
  {
    char c = d[cut];
    FRESULT fr;
    d[cut] = '\0';
    fr = f_mkdir(d);
    d[cut] = c;
    if (fr != FR_OK && fr != FR_EXIST) { DBG("[SD] Dossier %s : erreur %d\r\n", d, (int)fr); return false; }
  }
  memcpy(sd_day_dir, d, sizeof(sd_day_dir));
  DBG("[SD] Dossier %s\r\n", d + 3);
  return true;
}
