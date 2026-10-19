#include "app.h"

/* After a power cut: puts back the last saved date/time (late by the time the board was off) */
bool Rtc_Restore(void)
{
  int16_t i = Rtc_LastRec();
  uint64_t rec;
  if (i < 0) return false;
  rec = ((const uint64_t *)FLASH_TIME_ADDR)[i];
  return Rtc_Write(true, (uint32_t)rec, true, (uint32_t)(rec >> 32) & 0x00FFFFFFUL);
}
