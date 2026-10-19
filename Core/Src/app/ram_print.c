#include "app.h"

/* [RAM] variables + deepest stack seen = used / total */
void Ram_Print(void)
{
  extern uint32_t _sdata, _ebss, _estack;        /* from the linker script */
  const uint32_t *p = &_ebss;
  unsigned long total = (unsigned long)((uint32_t)&_estack - 0x20000000UL);
  unsigned long vars  = (unsigned long)((uint32_t)&_ebss - (uint32_t)&_sdata);
  unsigned long stack, used;

  while (p < &_estack && *p == RAM_FILL) p++;
  stack = (unsigned long)((uint32_t)&_estack - (uint32_t)p);
  used = vars + stack;
  printf("[RAM] variables %lu o + pile max %lu o = %lu o / %lu o (%lu %%), libre %lu o\r\n",
         vars, stack, used, total, (used * 100UL + total - 1UL) / total, total - used);
}
