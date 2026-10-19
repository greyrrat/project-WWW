#include "app.h"

/* The DHT11 fails if it is read again too soon. Every caller (measure, LCD, maintenance)
   goes through here, so a read closer than DHT_MIN_MS to the last one returns the last result. */
#define DHT_MIN_MS  2000U

int8_t DHT11_Read(int16_t *temperature, int16_t *humidity)
{
  static uint32_t last_ms = 0U;
  static int8_t   last_r = -1;
  static int16_t  last_t = 0, last_h = 0;
  static bool     first = true;
  uint8_t d[5] = { 0, 0, 0, 0, 0 };
  int8_t r;
  uint32_t now = HAL_GetTick();

  if (!first && (now - last_ms) < DHT_MIN_MS)
  {
    if (last_r == 1) { *temperature = last_t; *humidity = last_h; }
    return last_r;
  }
  first = false;
  last_ms = now;

  DHT_OUTPUT();
  DHT_LOW();
  HAL_Delay(20);                 /* start signal (interrupts still enabled) */
  __disable_irq();
  DHT_HIGH();
  delay_us(30);
  DHT_INPUT();
  r = Dht_Exchange(d);
  __enable_irq();
  DHT_OUTPUT();                  /* back to idle: line high */
  DHT_HIGH();

  if (r == 1 && ((d[0] + d[1] + d[2] + d[3]) & 0xFF) != d[4]) r = -6;
  if (r == 1) { last_h = d[0]; last_t = d[2]; *humidity = last_h; *temperature = last_t; }
  last_r = r;
  return r;
}
