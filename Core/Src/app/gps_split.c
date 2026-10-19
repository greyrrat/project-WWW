#include "app.h"

/* Copies a GPS sentence, checks its checksum and splits it on commas (empty fields kept).
   Returns the number of fields, 0 if the sentence is bad. */
uint8_t Gps_Split(const char *src, char *g, char **f, uint8_t max)
{
  char *p, *star;
  uint8_t nf = 0U, sum = 0U;

  __disable_irq();
  memcpy(g, src, GPS_LINE_MAX);
  __enable_irq();
  g[GPS_LINE_MAX - 1U] = '\0';

  star = strchr(g, '*');
  if (g[0] != '$' || star == NULL || star[1] == '\0' || star[2] == '\0') return 0U;
  for (p = g + 1; p < star; p++) sum ^= (uint8_t)*p;
  if (strtoul(star + 1, NULL, 16) != sum) return 0U;
  *star = '\0';
  for (p = g; p != NULL && nf < max; ) { f[nf++] = p; p = strchr(p, ','); if (p != NULL) *p++ = '\0'; }
  return nf;
}
