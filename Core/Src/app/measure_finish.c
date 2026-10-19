#include "app.h"
#include "features.h"

void Measure_Finish(void)
{
  char t_s[16] = "NA", h_s[16] = "NA", l_s[28] = "NA", p_s[16] = "NA", g_s[40] = "NA";
  char tag[12], line[LOG_LINE_MAX];
  bool data_err = false;
  DateTime_t dt;
  bool rtc_valid;
  uint32_t now;

  meas.active = false;
  now = HAL_GetTick();
  next_measure_ms = meas.start_ms + Interval_ms();
  if ((int32_t)(now - next_measure_ms) >= 0) next_measure_ms = now + 1000U;

  /* "after 2 measurements in timeout, the sensor is in error" */
  if (meas.dht_state == 2U) { if (dht_fail < 255U) dht_fail++; } else dht_fail = 0U;
  if (meas.lux_state == 2U) { if (lux_fail < 255U) lux_fail++; } else lux_fail = 0U;

  if (meas.dht_state == 1U)
  {
    if (sys_cfg.temp_air)
    {
      snprintf(t_s, sizeof(t_s), "%d C", (int)meas.t);
      if (meas.t < sys_cfg.min_temp_air || meas.t > sys_cfg.max_temp_air) data_err = true;
    }
    /* HYGR_MINT / HYGR_MAXT are AIR TEMPERATURE limits: outside them humidity is ignored (NA) */
    if (sys_cfg.hygr && meas.t >= sys_cfg.hygr_mint && meas.t <= sys_cfg.hygr_maxt)
      snprintf(h_s, sizeof(h_s), "%d %%", (int)meas.h);
  }

  if (meas.lux_state == 1U)
  {
    uint16_t v = Lux_Scale(meas.lux_x100 / 100U);
    const char *lvl = (v < sys_cfg.lumin_low) ? "LOW" : ((v > sys_cfg.lumin_high) ? "HIGH" : "MID");
    snprintf(l_s, sizeof(l_s), "%u (%s)", (unsigned)v, lvl);      /* low/mid/high is a class, not an error */
  }

#if HAVE_PRESSURE
  {
    int32_t hpa;
    if (sys_cfg.pressure && Pressure_Read(&hpa))
    {
      snprintf(p_s, sizeof(p_s), "%ld hPa", (long)hpa);
      if (hpa < sys_cfg.pressure_min || hpa > sys_cfg.pressure_max) data_err = true;
    }
  }
#endif
#if USE_GPS
  {
    static uint8_t eco_toggle = 0U;
    bool skip = (app_mode == MODE_ECONOMY) && ((eco_toggle++ & 1U) != 0U);   /* economy: every 2nd time */
    if (!skip)
    {
      if (GPS_Read(g_s, sizeof(g_s)) < 0) Err_Set(ERR_GPS); else Err_Clear(ERR_GPS);   /* no fix = NA, no error */
    }
  }
#endif

  if (dht_fail >= 2U || lux_fail >= 2U) Err_Set(ERR_SENS_ACC); else Err_Clear(ERR_SENS_ACC);
  if (data_err) Err_Set(ERR_SENS_DATA); else Err_Clear(ERR_SENS_DATA);

  rtc_valid = Rtc_Get(&dt);
  if (!rtc_valid) { Err_Set(ERR_RTC); memset(&dt, 0, sizeof(dt)); } else Err_Clear(ERR_RTC);

  snprintf(tag, sizeof(tag), "%02d%02d%02d", (int)dt.year, (int)dt.month, (int)dt.day);
  snprintf(line, sizeof(line), "20%02d-%02d-%02d %02d:%02d:%02d ; Temp=%s ; Hum=%s ; Lum=%s ; Press=%s ; GPS=%s\r\n",
           (int)dt.year, (int)dt.month, (int)dt.day, (int)dt.hour, (int)dt.min, (int)dt.sec,
           t_s, h_s, l_s, p_s, g_s);
  {
    char tl[8] = "--", hl[8] = "--", ll[8] = "--", lcd_s[32];
    if (meas.dht_state == 1U && sys_cfg.temp_air) snprintf(tl, sizeof(tl), "%dC", (int)meas.t);
    if (h_s[0] != 'N') snprintf(hl, sizeof(hl), "%d%%", (int)meas.h);
    if (meas.lux_state == 1U) snprintf(ll, sizeof(ll), "%u", (unsigned)Lux_Scale(meas.lux_x100 / 100U));
    snprintf(lcd_s, sizeof(lcd_s), "%s %s L%s", tl, hl, ll);
    snprintf(lcd_vals, sizeof(lcd_vals), "%.16s", lcd_s);
    Lcd_Refresh(true);
  }
  DBG("[LOG] %s", line);
  SD_LogLine(tag, line);
}
