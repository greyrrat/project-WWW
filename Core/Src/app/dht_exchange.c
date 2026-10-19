#include "app.h"

/* ============================= SENSORS ================================ */
/* Timing-critical part: runs with interrupts masked (about 4 ms) */
int8_t Dht_Exchange(uint8_t d[5])
{
  uint32_t t;

  t = 200U; while (DHT_IN())  { if (--t == 0U) return -1; delay_us(1); }   /* sensor pulls low  */
  t = 200U; while (!DHT_IN()) { if (--t == 0U) return -2; delay_us(1); }   /* sensor releases   */
  t = 200U; while (DHT_IN())  { if (--t == 0U) return -3; delay_us(1); }   /* start of bit 0    */

  for (int i = 0; i < 40; i++)
  {
    t = 200U; while (!DHT_IN()) { if (--t == 0U) return -4; delay_us(1); }
    delay_us(40);
    if (DHT_IN()) d[i / 8] |= (uint8_t)(1U << (7 - (i % 8)));
    t = 200U; while (DHT_IN())  { if (--t == 0U) return -5; delay_us(1); }
  }
  return 1;
}
