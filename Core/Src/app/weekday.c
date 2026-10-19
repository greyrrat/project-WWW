#include "app.h"

uint8_t Weekday(uint16_t y, uint8_t m, uint8_t d)   /* 1 = Monday ... 7 = Sunday */
{
  static const uint8_t t[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  uint8_t w;
  if (m < 3U) y--;
  w = (uint8_t)((y + (y / 4U) - (y / 100U) + (y / 400U) + t[m - 1U] + d) % 7U);   /* 0 = Sunday */
  return (w == 0U) ? 7U : w;
}
