#include "app.h"

/* The HAL calls one of these depending on its version: all three do the same */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)         { Button_Edge(GPIO_Pin); }
