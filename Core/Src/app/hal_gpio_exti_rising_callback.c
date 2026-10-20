#include "app.h"

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)  { Button_Edge(GPIO_Pin); }
