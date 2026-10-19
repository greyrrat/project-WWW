#include "app.h"

/* Both edges of both buttons arrive here */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)         { Button_Edge(GPIO_Pin); }
