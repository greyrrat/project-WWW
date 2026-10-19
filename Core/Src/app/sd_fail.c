#include "app.h"

void SD_Fail(uint32_t flag)
{
  Err_Set(flag);
  sd_mounted = false;
  sd_day_dir[0] = '\0';
  sd_stat = STA_NOINIT;
}
