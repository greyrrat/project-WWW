#include "app.h"
#include "features.h"

uint8_t lcd_addr = 0x27;
bool    lcd_ready = false;

/* Sends one raw byte to the PCF8574 chip on the back of the LCD.
   If the LCD does not answer, lcd_ready goes false and Lcd_Refresh looks for it again. */
void Lcd_Expander(uint8_t data)
{
  if (HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(lcd_addr << 1), &data, 1, 20) != HAL_OK)
    lcd_ready = false;
}
