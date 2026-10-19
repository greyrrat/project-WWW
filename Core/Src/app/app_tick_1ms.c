#include "app.h"

/* Called every millisecond by TIM6 */
void App_Tick1ms(void)
{
  uint32_t now = HAL_GetTick();
  Button_Tick(&btn_red, now);
  Button_Tick(&btn_green, now);
  Led_Tick(now);
  if (++tick_div >= 100U) { tick_div = 0U; Evt_Set(EVT_TICK); }
}
