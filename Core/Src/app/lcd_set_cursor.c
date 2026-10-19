#include "app.h"
#include "features.h"

void Lcd_SetCursor(uint8_t col, uint8_t row)       /* col 0..15, row 0..1 */
{
  static const uint8_t offset[2] = { 0x00U, 0x40U };
  Lcd_WriteByte((uint8_t)(0x80U | (col + offset[row & 1U])), 0);
}
