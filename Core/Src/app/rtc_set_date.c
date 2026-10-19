#include "app.h"

bool Rtc_SetDate(uint8_t y, uint8_t mo, uint8_t d)   /* y = 0..99 */
{
  uint32_t wd = Weekday((uint16_t)(2000U + y), mo, d);
  return Rtc_Write(false, 0U, true,
                   ((uint32_t)bin2bcd(y) << 16) | (wd << 13) | ((uint32_t)bin2bcd(mo) << 8) | bin2bcd(d));
}
