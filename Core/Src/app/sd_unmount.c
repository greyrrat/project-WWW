#include "app.h"

void SD_Unmount(void)
{
  if (sd_mounted) { (void)f_mount(NULL, "0:", 0); }
  sd_mounted = false;
  sd_day_dir[0] = '\0';                         /* the card may be changed: check folders again */
  sd_stat = STA_NOINIT;
}
