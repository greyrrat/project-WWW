#include "app.h"

/* NMEA "4807.0381" + 'N' (or "01131.0000" + 'E') -> "48.11730N". No float used. */
bool Gps_ParseCoord(const char *s, char hemi, char *out, size_t n)
{
  long whole = 0, frac = 0;
  int fd = 0;

  if (*s < '0' || *s > '9') return false;
  while (*s >= '0' && *s <= '9') { whole = whole * 10 + (*s - '0'); s++; }
  if (*s == '.') s++;
  while (*s >= '0' && *s <= '9') { if (fd < 4) { frac = frac * 10 + (*s - '0'); fd++; } s++; }
  while (fd < 4) { frac *= 10; fd++; }                        /* minutes with 4 decimals */

  {
    long deg  = whole / 100;
    long min4 = (whole % 100) * 10000 + frac;                 /* minutes x 10000 */
    long dec5 = (min4 * 10) / 60;                             /* degrees x 100000 */
    snprintf(out, n, "%ld.%05ld%c", deg, dec5, hemi);
  }
  return true;
}
