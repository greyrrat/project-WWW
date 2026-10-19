#include "app.h"

bool Rtc_SetTime(uint8_t h, uint8_t m, uint8_t s)
{
  return Rtc_Write(true, ((uint32_t)bin2bcd(h) << 16) | ((uint32_t)bin2bcd(m) << 8) | bin2bcd(s), false, 0U);
}
