#include "app.h"

/* Index of the last saved date/time record in the flash page (-1 = none) */
int16_t Rtc_LastRec(void)
{
  const uint64_t *r = (const uint64_t *)FLASH_TIME_ADDR;
  int16_t i = -1;
  while ((uint16_t)(i + 1) < TIME_REC_COUNT && (uint8_t)(r[i + 1] >> 56) == TIME_REC_TAG) i++;
  return i;
}
