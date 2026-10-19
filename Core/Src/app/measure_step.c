#include "app.h"

/* Non-blocking: tries again every 2.1 s (the DHT11 needs >= 2 s between reads) until TIMEOUT */
void Measure_Step(void)
{
  uint32_t now = HAL_GetTick();
  bool expired;

  if (!meas.active || (int32_t)(now - meas.next_try_ms) < 0) return;
  expired = (now - meas.start_ms) >= ((uint32_t)sys_cfg.timeout * 1000UL);

  if (meas.dht_state == 0U)
  {
    if (DHT11_Read(&meas.t, &meas.h) == 1) meas.dht_state = 1U;
    else if (expired)                      meas.dht_state = 2U;
  }
  if (meas.lux_state == 0U)
  {
    if (BH1750_ReadLux(&meas.lux_x100)) meas.lux_state = 1U;
    else if (expired)                   meas.lux_state = 2U;
  }

  if (meas.dht_state != 0U && meas.lux_state != 0U) Measure_Finish();
  else meas.next_try_ms = HAL_GetTick() + 2100U;
}
