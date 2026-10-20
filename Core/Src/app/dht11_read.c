#include "app.h"

int8_t DHT11_Read(int16_t *temperature, int16_t *humidity)
{
  uint8_t d[5] = { 0, 0, 0, 0, 0 };
  int8_t r;

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

  if (r != 1) return r;
  if (((d[0] + d[1] + d[2] + d[3]) & 0xFF) != d[4]) return -6;
  *humidity = d[0];
  *temperature = d[2];
  return 1;
}
