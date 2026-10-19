#include "app.h"

/* Independent watchdog: restarts the board if App_Task stops running for about 16 s.
   Paused while the debugger is stopped on a breakpoint. */
void Watchdog_Init(void)
{
  DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP;
  IWDG->KR  = 0xCCCCU;                           /* start (LSI 32 kHz turns on by itself) */
  IWDG->KR  = 0x5555U;                           /* unlock PR / RLR */
  IWDG->PR  = 6U;                                /* 32 kHz / 256 = 125 Hz */
  IWDG->RLR = 2000U;                             /* 2000 / 125 Hz = 16 s */
  while (IWDG->SR != 0U) { }
  IWDG->KR  = 0xAAAAU;                           /* reload */
}
