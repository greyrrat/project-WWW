#include "app.h"

/* STATUS command */
void Tog_Print(void)
{
  printf("USE_LED=%d ; USE_SD=%d ; USE_LIGHT=%d ; USE_TEMP_HUM=%d ; USE_LCD=%d\r\n",
         (int)tog.led, (int)tog.sd, (int)tog.light, (int)tog.temp, (int)tog.lcd);
  printf("SD %s ; %u ligne(s) en attente ; erreurs=0x%02lX\r\n",
         sd_mounted ? "montee" : "non montee", (unsigned)q_count, (unsigned long)error_flags);
}
