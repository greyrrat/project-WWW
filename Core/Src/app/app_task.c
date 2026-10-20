#include "app.h"

/* All the slow work happens here, never inside a button/UART/timer interrupt */
void App_Task(void)
{
  uint32_t ev = Evt_Take();
  uint32_t now;

  if (ev & EVT_UART_LINE)
  {
    char cmd[RX_MAX];
    memcpy(cmd, rx_line, sizeof(cmd));
    rx_ready = 0U;                               /* the interrupt may receive the next line now */
    if (app_mode == MODE_CONFIG) Cli_Execute(cmd);
    else printf("ERROR: commandes disponibles seulement en mode CONFIGURATION\r\n");
  }

  if (ev & EVT_RED_SHORT)   DBG("[BTN] ROUGE : appui court\r\n");
  if (ev & EVT_GREEN_SHORT) DBG("[BTN] VERT : appui court\r\n");
  if (ev & EVT_RED_LONG)    { DBG("[BTN] ROUGE : appui 5 s\r\n"); Mode_OnRedLong(); }
  if (ev & EVT_GREEN_LONG)  { DBG("[BTN] VERT : appui 5 s\r\n");  Mode_OnGreenLong(); }

#if QUICK_LOG_BUTTON
  if ((ev & EVT_RED_SHORT) && (app_mode == MODE_STANDARD || app_mode == MODE_ECONOMY) && !meas.active)
    next_measure_ms = HAL_GetTick();             /* test: measure and log right now */
#endif

  now = HAL_GetTick();
  switch (app_mode)
  {
    case MODE_STANDARD:
    case MODE_ECONOMY:
      if (!meas.active && (int32_t)(now - next_measure_ms) >= 0) Measure_Start(now);
      Measure_Step();
      break;

    case MODE_MAINTENANCE:
      if ((int32_t)(now - next_maint_ms) >= 0)
      {
        next_maint_ms = now + 2000U;
        Maintenance_Print();
      }
      break;

    case MODE_CONFIG:
      if ((now - config_activity_ms) >= CONFIG_TIMEOUT_MS)
      {
        DBG("[CONFIG] 30 min sans activite : retour en STANDARD\r\n");
        Mode_Set(MODE_STANDARD);
      }
      break;

    default:
      break;
  }
}
