#include "app.h"

/* ============================ MEASUREMENT ============================= */
uint32_t Interval_ms(void)
{
  uint32_t ms = (uint32_t)sys_cfg.log_interval * 60000UL;
  if (app_mode == MODE_ECONOMY) ms *= 2U;
  return ms;
}
