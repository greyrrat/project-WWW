#include "app.h"

void Mode_OnRedLong(void)
{
  switch (app_mode)
  {
    case MODE_STANDARD:
      prev_mode = MODE_STANDARD;
      Mode_Set(MODE_MAINTENANCE);
      break;
    case MODE_ECONOMY:
#if ECONOMY_RED_TO_STANDARD
      Mode_Set(MODE_STANDARD);
#else
      prev_mode = MODE_ECONOMY;
      Mode_Set(MODE_MAINTENANCE);
#endif
      break;
    case MODE_MAINTENANCE:
      Mode_Set(prev_mode);
      break;
    default:
      break;
  }
}
