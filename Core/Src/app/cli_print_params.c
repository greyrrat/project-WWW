#include "app.h"

void Cli_PrintParams(void)
{
  DateTime_t dt;
  for (size_t i = 0; i < PARAM_COUNT; i++)
    printf("%s=%ld\r\n", params[i].name, (long)*(const int32_t *)((const uint8_t *)&sys_cfg + params[i].off));
  if (Rtc_Get(&dt))
    printf("DATE=%02d,%02d,20%02d ; CLOCK=%02d:%02d:%02d\r\n",
           (int)dt.month, (int)dt.day, (int)dt.year, (int)dt.hour, (int)dt.min, (int)dt.sec);
  else
    printf("RTC: lecture impossible\r\n");
}
