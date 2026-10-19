#include "app.h"

/* Reads the last GGA sentence. Returns 1 = position in out, 0 = GPS talks but no fix yet,
   -1 = nothing received during TIMEOUT (wires / power). */
int8_t GPS_Read(char *out, size_t n)
{
  char g[GPS_LINE_MAX];
  char *f[16];
  char lat[16], lon[16];
  uint8_t nf = 0U, sum = 0U;
  uint32_t last;
  char *p, *star;

  __disable_irq();
  memcpy(g, gps_gga, sizeof(g));
  last = gps_last_ms;
  __enable_irq();
  g[GPS_LINE_MAX - 1U] = '\0';

  if (last == 0U || (HAL_GetTick() - last) > ((uint32_t)sys_cfg.timeout * 1000UL)) return -1;
  if (g[0] != '$') return 0;

  /* checksum: XOR of everything between '$' and '*' */
  star = strchr(g, '*');
  if (star == NULL) return 0;
  for (p = g + 1; p < star; p++) sum ^= (uint8_t)*p;
  if (strtoul(star + 1, NULL, 16) != sum) return 0;
  *star = '\0';

  /* split on commas (keeps empty fields) */
  p = g;
  while (p != NULL && nf < 16U) { f[nf++] = p; p = strchr(p, ','); if (p) *p++ = '\0'; }

  /* $GPGGA,time,lat,N,lon,E,fix,sats,hdop,alt,M,... */
  if (nf < 10U || f[6][0] == '\0' || f[6][0] == '0') return 0;            /* no fix */
  if (!Gps_ParseCoord(f[2], f[3][0], lat, sizeof(lat))) return 0;
  if (!Gps_ParseCoord(f[4], f[5][0], lon, sizeof(lon))) return 0;

  {
    char alt[8] = "?";
    size_t i = 0U;
    while (f[9][i] != '\0' && f[9][i] != '.' && i < sizeof(alt) - 1U) { alt[i] = f[9][i]; i++; }
    if (i > 0U) alt[i] = '\0';
    snprintf(out, n, "%s %s %sm", lat, lon, alt);
  }
  return 1;
}
