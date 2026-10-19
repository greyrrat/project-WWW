#include "app.h"

bool Rtc_SetAll(const DateTime_t *d)
{
  return Rtc_SetDate(d->year, d->month, d->day) && Rtc_SetTime(d->hour, d->min, d->sec);
}
