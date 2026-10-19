#include "app.h"

static uint8_t two(const char *s) { return (uint8_t)((s[0] - '0') * 10 + (s[1] - '0')); }

/* $GPRMC,hhmmss.ss,A,lat,N,lon,E,speed,course,ddmmyy,... -> local date/time (TIMEZONE_HOURS).
   False if no fresh valid sentence. */
bool Gps_Time(DateTime_t *d)
{
  static const uint8_t mdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  char g[GPS_LINE_MAX];
  char *f[10];
  int h;
  uint8_t dim;

  if (gps_rmc_ms == 0U || (HAL_GetTick() - gps_rmc_ms) > 1500U) return false;
  if (Gps_Split(gps_rmc, g, f, 10U) < 10U || f[2][0] != 'A') return false;
  if (strlen(f[1]) < 6U || strlen(f[9]) != 6U) return false;
  for (uint8_t i = 0U; i < 6U; i++) if (f[1][i] < '0' || f[1][i] > '9' || f[9][i] < '0' || f[9][i] > '9') return false;

  d->hour = two(f[1]); d->min = two(f[1] + 2); d->sec = two(f[1] + 4);
  d->day = two(f[9]);  d->month = two(f[9] + 2); d->year = two(f[9] + 4);
  if (d->month < 1U || d->month > 12U || d->day < 1U) return false;

  h = d->hour + TIMEZONE_HOURS;                  /* UTC -> local time, with day/month/year change */
  dim = (uint8_t)(mdays[d->month - 1U] + ((d->month == 2U && (d->year % 4U) == 0U) ? 1U : 0U));
  if (h >= 24)
  {
    h -= 24;
    if (++d->day > dim) { d->day = 1U; if (++d->month > 12U) { d->month = 1U; d->year++; } }
  }
  else if (h < 0)
  {
    h += 24;
    if (--d->day == 0U)
    {
      if (--d->month == 0U) { d->month = 12U; d->year--; }
      d->day = (uint8_t)(mdays[d->month - 1U] + ((d->month == 2U && (d->year % 4U) == 0U) ? 1U : 0U));
    }
  }
  d->hour = (uint8_t)h;
  return true;
}
