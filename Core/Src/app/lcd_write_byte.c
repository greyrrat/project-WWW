#include "app.h"
#include "features.h"

/* PCF8574 pins: bit0 = RS, bit1 = RW, bit2 = E, bit3 = backlight, bit4..7 = D4..D7 */
#define LCD_BL  0x08U
#define LCD_EN  0x04U

/* Sends a command (rs = 0) or a character (rs = 1) in two 4-bit halves */
void Lcd_WriteByte(uint8_t value, uint8_t rs)
{
  uint8_t r  = rs ? 0x01U : 0x00U;
  uint8_t hi = (uint8_t)((value & 0xF0U) | LCD_BL | r);
  uint8_t lo = (uint8_t)(((value << 4) & 0xF0U) | LCD_BL | r);

  Lcd_Expander(hi | LCD_EN);  Lcd_Expander(hi);
  Lcd_Expander(lo | LCD_EN);  Lcd_Expander(lo);
}
