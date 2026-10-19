#include "app.h"

bool Rtc_SetWeekday(uint8_t wd)
{
  return Rtc_Write(false, 0U, true, (RTC->DR & ~(7UL << 13)) | ((uint32_t)wd << 13));
}
