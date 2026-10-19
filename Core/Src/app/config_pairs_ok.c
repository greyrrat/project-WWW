#include "app.h"

bool Config_PairsOk(const SystemConfig_t *c)
{
  return (c->lumin_low <= c->lumin_high) && (c->min_temp_air <= c->max_temp_air) &&
         (c->hygr_mint <= c->hygr_maxt)  && (c->pressure_min <= c->pressure_max);
}
