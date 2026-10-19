#include "app.h"

/* ======================== PARAMETERS IN FLASH ========================= */
uint32_t Config_Checksum(const SystemConfig_t *c)
{
  const uint32_t *w = (const uint32_t *)c;
  uint32_t sum = 0x1234ABCDUL;
  for (size_t i = 0; i < (sizeof(SystemConfig_t) / 4U) - 1U; i++) sum = (sum * 31U) ^ w[i];
  return sum;
}
