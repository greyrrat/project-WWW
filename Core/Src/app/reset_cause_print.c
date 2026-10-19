#include "app.h"

/* Prints why the board restarted, then clears the flags.
   "BROWNOUT" or "WATCHDOG" here means something went wrong (power dip, or the program got stuck). */
void Reset_CausePrint(void)
{
  uint32_t csr = RCC->CSR;
  const char *why = "power-up / unknown";

  if      (csr & RCC_CSR_IWDGRSTF) why = "WATCHDOG (program was stuck)";
  else if (csr & RCC_CSR_BORRSTF)  why = "BROWNOUT (power dropped: SD/GPS/LCD current?)";
  else if (csr & RCC_CSR_SFTRSTF)  why = "software";
  else if (csr & RCC_CSR_PINRSTF)  why = "RESET button / flashing";
  printf("[RESET] cause : %s\r\n", why);
  RCC->CSR |= RCC_CSR_RMVF;                      /* clear the flags for next time */
}
