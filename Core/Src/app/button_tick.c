#include "app.h"
#include "features.h"

/* Called every millisecond: debounce + 5 s hold detection */
void Button_Tick(Button_t *b, uint32_t now)
{
#if !USE_BUTTONS
  (void)b; (void)now;
  return;                          /* buttons disabled in features.h */
#endif
  if (b->edge_pending && (now - b->last_edge) >= DEBOUNCE_MS)
  {
    uint8_t p = (HAL_GPIO_ReadPin(b->port, b->pin) == GPIO_PIN_RESET) ? 1U : 0U;
    b->edge_pending = 0U;
    if (p && !b->pressed)
    {
      b->pressed = 1U; b->press_start = now; b->long_fired = 0U;
    }
    else if (!p && b->pressed)
    {
      b->pressed = 0U;
      if (!b->long_fired) Evt_Set(b->evt_short);
    }
  }
  if (b->pressed && !b->long_fired && (now - b->press_start) >= LONG_PRESS_MS)
  {
    b->long_fired = 1U;
    Evt_Set(b->evt_long);
  }
}
