#include "app.h"

/* ======================= INTERRUPT HANDLERS =========================== */
void EXTI0_IRQHandler(void)    { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0); }     /* RED button (A5) */
