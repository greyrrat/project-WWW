#include "app.h"

/* ============================== RGB LED =============================== */
void Led_Show(uint8_t color)
{
  uint8_t m = color;
  if (color == C_ORANGE)
    m = (uint8_t)(1U | (((pwm_phase % 5U) < LED_ORANGE_GREEN_ON) ? 2U : 0U));  /* soft PWM on green */
  HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN, (m & 1U) ? LED_LEVEL_ON : LED_LEVEL_OFF);
  HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN, (m & 2U) ? LED_LEVEL_ON : LED_LEVEL_OFF);
  HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN, (m & 4U) ? LED_LEVEL_ON : LED_LEVEL_OFF);
}
