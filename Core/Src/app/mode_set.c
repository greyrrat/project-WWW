#include "app.h"
#include "features.h"

/* ============================ MODES / TASK ============================ */
void Mode_Set(SystemMode_t m)
{
  static const char *const names[4] = { "STANDARD", "CONFIGURATION", "MAINTENANCE", "ECONOMIE" };
  SystemMode_t old = app_mode;
  uint32_t now = HAL_GetTick();

  if (m == old) return;
  if ((m == MODE_CONFIG && !USE_MODE_CONFIG) || (m == MODE_MAINTENANCE && !USE_MODE_MAINT) ||
      (m == MODE_ECONOMY && !USE_MODE_ECO))
  {
    DBG("[MODE] %s desactive dans features.h\r\n", names[(int)m]);
    return;
  }
  meas.active = false;
  app_mode = m;

  if (m == MODE_MAINTENANCE)
  {
    SD_Unmount();                                /* the card can be removed safely */
    Err_Clear(ERR_SD_FULL | ERR_SD_ACC);
    next_maint_ms = now;
  }
  else if (m == MODE_CONFIG)
  {
    config_activity_ms = now;
  }
  else if (old == MODE_CONFIG)
  {
    next_measure_ms = now + 3000U;
  }
  DBG("[MODE] %s\r\n", names[(int)m]);
  lcd_vals[0] = '\0';                           /* old readings are not shown in the new mode */
  Lcd_Refresh(true);
}
