#include "app.h"
#include "features.h"

/* STANDARD / ECONOMY: every LCD_LIVE_MS the sensors are read for the LCD and the serial port.
   Nothing is written to the SD here (that is done every LOG_INTERVAL). */
void Live_Print(void)
{
  static uint32_t next_ms = 0U;
  static uint8_t n = 0U;
  static int16_t t __attribute__((unused)) = 0, h __attribute__((unused)) = 0;
  static uint32_t lx __attribute__((unused)) = 0U;
  static bool th_ok __attribute__((unused)) = false, lx_ok __attribute__((unused)) = false;
  char ts[12] = "OFF", hs[12] = "OFF", ls[8] = "OFF", tl[8] = "--", hl[8] = "--", ll[8] = "--", s[32];
  uint32_t now = HAL_GetTick();

  if (meas.active || (int32_t)(now - next_ms) < 0) return;
  next_ms = now + LCD_LIVE_MS;

#if USE_TEMP_HUM
  {
    int16_t a, b;
    if (DHT11_Read(&a, &b) == 1) { t = a; h = b; th_ok = true; }   /* a failed read keeps the last value */
    strcpy(ts, "NA"); strcpy(hs, "NA");
    if (th_ok)
    {
      snprintf(ts, sizeof(ts), "%d C", (int)t);  snprintf(hs, sizeof(hs), "%d %%", (int)h);
      snprintf(tl, sizeof(tl), "%dC", (int)t);   snprintf(hl, sizeof(hl), "%d%%", (int)h);
    }
  }
#endif
#if USE_LIGHT
  {
    uint32_t v;
    if (BH1750_ReadLux(&v)) { lx = v; lx_ok = true; }
    strcpy(ls, "NA");
    if (lx_ok)
    {
      snprintf(ls, sizeof(ls), "%u", (unsigned)Lux_Scale(lx / 100U));
      snprintf(ll, sizeof(ll), "%u", (unsigned)Lux_Scale(lx / 100U));
    }
  }
#endif

  DBG("[LIVE] Temp=%s | Hum=%s | Lum=%s\r\n", ts, hs, ls);
  if (++n >= 10U) { n = 0U; Ram_Print(); }       /* RAM use every 30 s */

  snprintf(s, sizeof(s), "%s %s L%s", tl, hl, ll);
  snprintf(lcd_vals, sizeof(lcd_vals), "%.16s", s);
  Lcd_Refresh(true);                             /* LCD shows the new reading right away */
}
