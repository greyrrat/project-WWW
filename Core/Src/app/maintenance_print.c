#include "app.h"
#include "features.h"

void Maintenance_Print(void)
{
  int16_t t = 0, h = 0;
  uint32_t lx = 0U;
  char ts[16] = "NA", hs[16] = "NA", ls[24] = "NA", clk[16] = "--:--:--", gs[40] = "OFF";
  char tl[8] = "--", hl[8] = "--", ll[12] = "--", lcd_s[40];
  DateTime_t dt;
  int8_t dr;

  if (Rtc_Get(&dt)) snprintf(clk, sizeof(clk), "%02d:%02d:%02d", (int)dt.hour, (int)dt.min, (int)dt.sec);

  if (!(USE_TEMP_HUM && tog.temp)) { strcpy(ts, "OFF"); strcpy(hs, "OFF"); }
  else if ((dr = DHT11_Read(&t, &h)) != 1)
  {
    snprintf(ts, sizeof(ts), "NA (err %d)", (int)dr);   /* -1 = no answer: check PB0 (A3), VCC, GND */
  }
  else
  {
    snprintf(ts, sizeof(ts), "%d C", (int)t);
    snprintf(hs, sizeof(hs), "%d %%", (int)h);
    snprintf(tl, sizeof(tl), "%dC", (int)t);
    snprintf(hl, sizeof(hl), "%d%%", (int)h);
  }

  if (!(USE_LIGHT && tog.light)) strcpy(ls, "OFF");
  else if (BH1750_ReadLux(&lx))
  {
    snprintf(ls, sizeof(ls), "%lu lx", (unsigned long)(lx / 100U));
    snprintf(ll, sizeof(ll), "%lu", (unsigned long)(lx / 100U));
  }

#if USE_GPS
  {
    int8_t g = GPS_Read(gs, sizeof(gs));
    if (g == 0)      strcpy(gs, "NO FIX (wait / go outside)");
    else if (g < 0)  strcpy(gs, "NO DATA (check wires)");
  }
#endif
  printf("[MAINT %s] Temp=%s | Hum=%s | Lum=%s | Press=NA | GPS=%s\r\n", clk, ts, hs, ls, gs);

  snprintf(lcd_s, sizeof(lcd_s), "%s %s L%s", tl, hl, ll);
  snprintf(lcd_vals, sizeof(lcd_vals), "%.16s", lcd_s);      /* shown by Lcd_Refresh */
}
