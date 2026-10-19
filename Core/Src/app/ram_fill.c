#include "app.h"

/* Fills the free RAM (between the variables and the stack) with a pattern at start-up.
   Ram_Print() then finds how deep the stack really went. */
void Ram_Fill(void)
{
  extern uint32_t _ebss;                         /* end of the variables (linker script) */
  uint32_t *p = &_ebss;
  uint32_t *stop = (uint32_t *)__get_MSP() - 256;  /* keep 1 KB below the stack in use */
  while (p < stop) *p++ = RAM_FILL;
}
