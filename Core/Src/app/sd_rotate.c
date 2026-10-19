#include "app.h"

/* The full _0 file becomes the next free revision: _1 .. _9 then _A .. _Z (35 per day).
   A new _0 is then started. If all are used, _0 keeps growing (no data is lost). */
void SD_Rotate(const char *tag, const char *path0)
{
  static const char revs[] = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  char pn[32];
  FILINFO fno;
  (void)tag;
  for (uint8_t i = 0U; revs[i] != '\0'; i++)
  {
    size_t r;
    strncpy(pn, path0, sizeof(pn) - 1U);
    pn[sizeof(pn) - 1U] = '\0';
    r = strlen(pn) - 5U;                         /* the '0' just before ".LOG" */
    pn[r] = revs[i];
    if (f_stat(pn, &fno) == FR_NO_FILE)
    {
      if (f_rename(path0, pn) == FR_OK) DBG("[SD] Fichier plein -> %s\r\n", pn + 3);
      return;
    }
  }
}
