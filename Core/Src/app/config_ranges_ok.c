#include "app.h"

bool Config_RangesOk(const SystemConfig_t *c)
{
  for (size_t i = 0; i < PARAM_COUNT; i++)
  {
    int32_t v = *(const int32_t *)((const uint8_t *)c + params[i].off);
    if (v < params[i].min || v > params[i].max) return false;
  }
  return Config_PairsOk(c);
}
