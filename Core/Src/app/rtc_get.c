#include "app.h"

bool Rtc_Get(DateTime_t *d)
{
  uint32_t tr, dr;
  if (!rtc_ok) return false;
  tr = RTC->TR;                 /* time first, then date */
  dr = RTC->DR;
  d->hour    = bcd2bin((uint8_t)((tr >> 16) & 0x3FU));
  d->min     = bcd2bin((uint8_t)((tr >> 8) & 0x7FU));
  d->sec     = bcd2bin((uint8_t)(tr & 0x7FU));
  d->year    = bcd2bin((uint8_t)((dr >> 16) & 0xFFU));
  d->month   = bcd2bin((uint8_t)((dr >> 8) & 0x1FU));
  d->day     = bcd2bin((uint8_t)(dr & 0x3FU));
  d->weekday = (uint8_t)((dr >> 13) & 0x7U);
  return (d->month >= 1U && d->month <= 12U && d->day >= 1U && d->day <= 31U);
}
