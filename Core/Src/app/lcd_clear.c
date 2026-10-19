#include "app.h"
#include "features.h"

void Lcd_Clear(void)
{
  Lcd_WriteByte(0x01, 0);
  HAL_Delay(2);
}
