#include "app.h"
#include "features.h"

/* ============================ MODES / TASK ============================ */
void Mode_Set(SystemMode_t m)
{
  static const char *const names[4] = { "STANDARD", "CONFIGURATION", "MAINTENANCE", "ECONOMIE" };
  SystemMode_t old = app_mode;
  uint32_t now = HAL_GetTick();

  if (m == old) return;
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
  Lcd_ShowLine(0, names[(int)m]);
  Lcd_ShowLine(1, "");
}
