#include "app.h"
#include "features.h"

void SD_LogLine(const char *tag, const char *text)
{
#if !USE_SD
  (void)tag; (void)text;
  return;                                        /* SD disabled in features.h */
#endif
  if (app_mode == MODE_MAINTENANCE) return;      /* the card must stay untouched in maintenance */
  Q_Push(tag, text);
  SD_Flush();
}
