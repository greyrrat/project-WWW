#include "app.h"

/* Called from the EXTI interrupts: only remembers that an edge happened */
void Button_Edge(uint16_t pin)
{
  if (pin == BTN_RED_PIN)        { btn_red.last_edge = HAL_GetTick();   btn_red.edge_pending = 1U; }
  else if (pin == BTN_GREEN_PIN) { btn_green.last_edge = HAL_GetTick(); btn_green.edge_pending = 1U; }
}
