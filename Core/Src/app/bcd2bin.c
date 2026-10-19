#include "app.h"

/* ===================== INTERNAL RTC (registers) ======================= */
uint8_t bcd2bin(uint8_t v) { return (uint8_t)(((v >> 4) * 10U) + (v & 0x0FU)); }
