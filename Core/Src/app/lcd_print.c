#include "app.h"
#include "features.h"

void Lcd_Print(const char *s)
{
  while (*s != '\0') { Lcd_WriteByte((uint8_t)*s, 1); s++; }
}
