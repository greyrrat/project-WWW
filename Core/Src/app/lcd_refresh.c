#include "app.h"
#include "features.h"

/* Redraws the LCD (called every 100 ms by App_Task, works once per second, or now if force):
     line 1 = mode + clock HH:MM
     line 2 = last readings, alternating every 2 s with the current error
   Only changed lines are rewritten. Everything is rewritten every 30 s (clears glitches).
   If the LCD stops answering (unplugged, I2C glitch) it is searched again every 5 s. */
void Lcd_Refresh(bool force)
{
#if USE_LCD
  static const char *const modes[4] = { "STANDARD", "CONFIG", "MAINT", "ECONOMY" };
  static const char *const errs[6]  = { "RTC", "GPS", "SENSOR", "DATA", "SD FULL", "SD" };
  static uint32_t next_ms = 2000U;               /* boot screen stays visible about 2 s */
  static uint32_t next_retry_ms = 0U;
  static uint32_t next_full_ms = 0U;
  static char shown[2][17];
  char l0[17], l1[17], clk[12] = "--:--";
  const char *err = NULL;
  DateTime_t dt;
  uint32_t now = HAL_GetTick();

  if (!force && (int32_t)(now - next_ms) < 0) return;
  next_ms = now + LCD_REFRESH_MS;

  if (!lcd_ready)
  {
    if ((int32_t)(now - next_retry_ms) < 0) return;
    next_retry_ms = now + 5000U;
    lcd_ready = Lcd_Init();
    if (!lcd_ready) return;
    DBG("[LCD] retrouve a 0x%02X\r\n", (unsigned)lcd_addr);
    memset(shown, 0, sizeof(shown));
  }
  if ((int32_t)(now - next_full_ms) >= 0)           /* force = redraw now (changed lines only) */
  {
    next_full_ms = now + 30000U;
    memset(shown, 0, sizeof(shown));             /* forget what is on screen: rewrite both lines */
  }

  if (Rtc_Get(&dt)) snprintf(clk, sizeof(clk), "%02u:%02u", (unsigned)dt.hour, (unsigned)dt.min);
  snprintf(l0, sizeof(l0), "%-10.10s %.5s", modes[(int)app_mode & 3], clk);

  for (int i = 0; i < 6; i++) if (error_flags & (1UL << i)) { err = errs[i]; break; }

  if (app_mode == MODE_CONFIG)              snprintf(l1, sizeof(l1), "Serial 115200");
  else if (err != NULL && ((now / 2000U) & 1U)) snprintf(l1, sizeof(l1), "ERR %s", err);
  else if (lcd_vals[0] != '\0')             snprintf(l1, sizeof(l1), "%s", lcd_vals);
  else if (meas.active)                     snprintf(l1, sizeof(l1), "Mesure...");
  else                                      snprintf(l1, sizeof(l1), "-- -- --");

  if (strcmp(shown[0], l0) != 0) { Lcd_ShowLine(0, l0); strcpy(shown[0], l0); }
  if (strcmp(shown[1], l1) != 0) { Lcd_ShowLine(1, l1); strcpy(shown[1], l1); }
  if (!lcd_ready) memset(shown, 0, sizeof(shown));   /* a write failed: redo everything next time */
#else
  (void)force;
#endif
}
