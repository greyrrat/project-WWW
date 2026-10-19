#include "app.h"
#include "features.h"

void Measure_Start(uint32_t now)
{
  memset(&meas, 0, sizeof(meas));
  meas.active = true;
  meas.start_ms = now;
  meas.next_try_ms = now;
  meas.dht_state = (USE_TEMP_HUM && (sys_cfg.temp_air || sys_cfg.hygr)) ? 0U : 3U;
  meas.lux_state = (USE_LIGHT && sys_cfg.lumin) ? 0U : 3U;
}
