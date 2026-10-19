#include "app.h"

#if EMPTY_MAIN_LOOP
void SWPMI1_IRQHandler(void)   { App_Task(); }
#endif
