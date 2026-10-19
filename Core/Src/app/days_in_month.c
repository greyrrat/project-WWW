#include "app.h"

uint8_t DaysInMonth(long y, long m)
{
  static const uint8_t d[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))) return 29U;
  return d[m - 1];
}
