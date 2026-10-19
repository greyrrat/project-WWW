#include "app.h"
#include "features.h"

void App_Init(void)
{
  uint32_t now;

  Ram_Fill();                                    /* first: lets Ram_Print measure the stack */
  DWT_Init();

  USER_Driver.disk_initialize = SD_Disk_Init;
  USER_Driver.disk_status     = SD_Disk_Status;
  USER_Driver.disk_read       = SD_Disk_Read;
  USER_Driver.disk_write      = SD_Disk_Write;
  USER_Driver.disk_ioctl      = SD_Disk_Ioctl;

  printf("\r\n=== Worldwide Weather Watcher v%s ===\r\n", FW_VERSION);
  printf("[INFO] CPU %lu Hz\r\n", (unsigned long)SystemCoreClock);

  printf("[FEATURES] LED=%d SD=%d LIGHT=%d TEMP_HUM=%d LCD=%d GPS=%d | CONFIG=%d MAINT=%d ECO=%d\r\n",
         USE_LED, USE_SD, USE_LIGHT, USE_TEMP_HUM, USE_LCD, USE_GPS, USE_MODE_CONFIG, USE_MODE_MAINT, USE_MODE_ECO);

  Config_Load();

#if I2C_SCAN_AT_BOOT && (USE_LIGHT || USE_LCD)  /* nothing to scan if no I2C device is used */
  I2c_Scan();
#endif

#if USE_LIGHT
  if (BH1750_Init()) DBG("[BH1750] OK\r\n");
  else               DBG("[BH1750] non detecte (verifier PB8/PB9)\r\n");
#endif

#if USE_LCD
  lcd_ready = Lcd_Init();
  if (lcd_ready)
  {
    Lcd_Clear();
    Lcd_ShowLine(0, "Weather Watcher");
    Lcd_ShowLine(1, "v" FW_VERSION);
    DBG("[LCD] OK a 0x%02X\r\n", (unsigned)lcd_addr);
  }
  else DBG("[LCD] non detecte\r\n");
#endif

  if (Rtc_Setup())
  {
    DateTime_t dt;
    if ((RTC->ISR & RTC_ISR_INITS) == 0U)
    {
      if (Rtc_Restore()) DBG("[RTC] Coupure de courant : heure restauree depuis la Flash (en retard : CLOCK= ou GPS)\r\n");
      else               DBG("[RTC] Heure non reglee : utiliser CLOCK= et DATE= en mode configuration\r\n");
    }
    if (Rtc_Get(&dt)) DBG("[RTC] 20%02u-%02u-%02u %02u:%02u:%02u\r\n", (unsigned)dt.year, (unsigned)dt.month,
                          (unsigned)dt.day, (unsigned)dt.hour, (unsigned)dt.min, (unsigned)dt.sec);
  }
  else
  {
    Err_Set(ERR_RTC);
    DBG("[RTC] ERREUR d'acces a l'horloge\r\n");
  }

  now = HAL_GetTick();
  Button_Init(&btn_red, now);
  Button_Init(&btn_green, now);

  if (btn_red.pressed && USE_MODE_CONFIG)        /* RED held at power-up -> CONFIGURATION */
  {
    app_mode = MODE_CONFIG;
    config_activity_ms = now;
    printf("[MODE] CONFIGURATION\r\n");
    printf("[CONFIG] Commandes : VERSION, PARAMS, RESET, CLOCK=HH:MM:SS, DATE=MM,DD,YYYY, DAY=MON, RAM, LOG_INTERVAL=...\r\n");
  }
  else
  {
    app_mode = MODE_STANDARD;
    next_measure_ms = now + 3000U;
    printf("[MODE] STANDARD\r\n");
  }

#if USE_GPS
  Gps_Init();
  DBG("[GPS] USART1 9600 bauds : GPS TX -> PA10 (D2), GPS RX -> PA9 (D8)\r\n");
#endif

#if SD_TEST_AT_BOOT && USE_SD
  (void)SD_SelfTest();                           /* press RESET to test again */
#endif

  HAL_NVIC_SetPriority(USART2_IRQn, 3U, 0U);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
  HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

#if EMPTY_MAIN_LOOP
  HAL_NVIC_SetPriority(SWPMI1_IRQn, 14U, 0U);    /* software interrupt that runs App_Task(), lowest priority */
  HAL_NVIC_EnableIRQ(SWPMI1_IRQn);
  SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;           /* empty main loop: the CPU sleeps after every interrupt */
#endif
  Tick_Init();
  Ram_Print();
}
