#include "app.h"
#include "features.h"

void Maintenance_Print(void)
{
  int16_t t = 0, h = 0;
  uint32_t lx = 0U;
  char ts[16] = "NA", hs[16] = "NA", ls[24] = "NA", clk[16] = "--:--:--";
  char tl[8] = "--", hl[8] = "--", ll[12] = "--", lcd_s[40];
  DateTime_t dt;

  if (Rtc_Get(&dt)) snprintf(clk, sizeof(clk), "%02d:%02d:%02d", (int)dt.hour, (int)dt.min, (int)dt.sec);
  if (USE_TEMP_HUM && DHT11_Read(&t, &h) == 1)
  {
    snprintf(ts, sizeof(ts), "%d C", (int)t);
    snprintf(hs, sizeof(hs), "%d %%", (int)h);
    snprintf(tl, sizeof(tl), "%dC", (int)t);
    snprintf(hl, sizeof(hl), "%d%%", (int)h);
  }
  if (USE_LIGHT && BH1750_ReadLux(&lx))
  {
    snprintf(ls, sizeof(ls), "%lu lx", (unsigned long)(lx / 100U));
    snprintf(ll, sizeof(ll), "%lu", (unsigned long)(lx / 100U));
  }
  printf("[MAINT %s] Temp=%s | Hum=%s | Lum=%s | Press=NA | GPS=NA\r\n", clk, ts, hs, ls);

  snprintf(lcd_s, sizeof(lcd_s), "%s %s L%s", tl, hl, ll);
  Lcd_ShowLine(1, lcd_s);
}
