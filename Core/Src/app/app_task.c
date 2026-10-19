#include "app.h"
#include "features.h"

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
      Live_Print();                              /* readings on LCD + serial every 3 s */
      /* lines still waiting (SD error): try again every SD_RETRY_MS */
      if (USE_SD && q_count > 0U && !meas.active && (int32_t)(now - next_sd_retry_ms) >= 0)
      {
        next_sd_retry_ms = now + SD_RETRY_MS;
        DBG("[SD] Nouvel essai : %u ligne(s) en attente\r\n", (unsigned)q_count);
        SD_Flush();
      }
      Gps_SyncClock(now);
      Rtc_Save(false);                           /* date/time to flash every 5 min */
      break;

    case MODE_MAINTENANCE:
      if ((int32_t)(now - next_maint_ms) >= 0)
      {
        next_maint_ms = now + 2000U;
        Maintenance_Print();
      }
      Gps_SyncClock(now);
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

  Lcd_Refresh(false);                            /* redraws the screen once per second */
}
