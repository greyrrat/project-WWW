#include "app.h"

uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10U) << 4) | (v % 10U)); }
