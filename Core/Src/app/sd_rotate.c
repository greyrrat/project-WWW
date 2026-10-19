#include "app.h"

/* File names are 8.3 (YYMMDD_R.LOG), so only revisions 1..9 exist */
void SD_Rotate(const char *tag, const char *path0)
{
  char pn[24];
  FILINFO fno;
  for (unsigned rev = 1U; rev <= 9U; rev++)
  {
    snprintf(pn, sizeof(pn), "0:/%s_%u.LOG", tag, rev);
    if (f_stat(pn, &fno) == FR_NO_FILE)
    {
      if (f_rename(path0, pn) == FR_OK) DBG("[SD] Rotation : %s -> %s\r\n", path0, pn);
      return;
    }
  }
  /* all revisions used: keep appending to revision 0 (no data is lost) */
}
