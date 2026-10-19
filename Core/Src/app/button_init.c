#include "app.h"
#include "features.h"

/* ============================== BUTTONS =============================== */
void Button_Init(Button_t *b, uint32_t now)
{
#if USE_BUTTONS
  b->pressed = (HAL_GPIO_ReadPin(b->port, b->pin) == GPIO_PIN_RESET) ? 1U : 0U;
#else
  b->pressed = 0U;                 /* buttons disabled in features.h */
#endif
  b->press_start = now;
  b->long_fired = 1U;      /* a button already held at boot must not trigger a long press */
  b->edge_pending = 0U;
}
