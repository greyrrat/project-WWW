#include "app.h"
#include "features.h"

/* Looks for the LCD at 0x27 then 0x3F, then starts it in 4-bit mode. Returns false if not found. */
bool Lcd_Init(void)
{
  static const uint8_t cand[2] = { 0x27U, 0x3FU };
  static const uint8_t seq[4]  = { 0x30U, 0x30U, 0x30U, 0x20U };   /* wake-up sequence */
  bool found = false;

  for (uint8_t i = 0U; i < 2U; i++)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(cand[i] << 1), 2, 20) == HAL_OK)
    {
      lcd_addr = cand[i];
      found = true;
      break;
    }
  }
  if (!found) return false;
  lcd_ready = true;                                /* Lcd_Expander sets it false if a write fails */

  HAL_Delay(50);                                   /* the LCD needs time after power-up */
  for (uint8_t i = 0U; i < 4U; i++)
  {
    uint8_t d = (uint8_t)(seq[i] | 0x08U);         /* backlight on */
    Lcd_Expander(d | 0x04U);
    Lcd_Expander(d);
    HAL_Delay(5);
  }

  Lcd_WriteByte(0x28, 0);                          /* 4-bit, 2 lines */
  Lcd_WriteByte(0x0C, 0);                          /* display on, no cursor */
  Lcd_WriteByte(0x01, 0);                          /* clear */
  HAL_Delay(2);
  Lcd_WriteByte(0x06, 0);                          /* cursor moves right */
  return lcd_ready;
}
