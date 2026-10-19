#include "app.h"

/* BH1750 gives lux; the spec thresholds use a 0..1023 scale: 1 lux = 1 unit, capped at 1023 */
uint16_t Lux_Scale(uint32_t lux) { return (lux > 1023U) ? 1023U : (uint16_t)lux; }
