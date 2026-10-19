#include "app.h"

void Mode_OnGreenLong(void)
{
  if (app_mode == MODE_STANDARD) Mode_Set(MODE_ECONOMY);
}
