#include "app.h"
#include "features.h"

/* Writes one 16-character line (padded / cut). Does nothing if the LCD is off or missing. */
void Lcd_ShowLine(uint8_t row, const char *text)
{
#if USE_LCD
  char buf[17];
  if (!lcd_ready || !tog.lcd) return;
  snprintf(buf, sizeof(buf), "%-16.16s", text);
  Lcd_SetCursor(0, row);
  Lcd_Print(buf);
#else
  (void)row; (void)text;
#endif
}
