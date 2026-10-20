/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Worldwide Weather Watcher - NUCLEO-L476RG (80 MHz, MSI + PLL)
  *
  * Wiring
  *   RGB LED          A0 (PA0) red, A1 (PA1) green, A2 (PA4) blue   (pins in features.h)
  *   DHT11            A3 (PB0)
  *   BH1750 + LCD     I2C1: SCL D15 (PB8), SDA D14 (PB9)
  *   SD card (SPI1)   SCK D13 (PA5), MISO D12 (PA6), MOSI D11 (PA7), CS D10 (PB6)
  *   RED button       A5 (PC0) to GND        GREEN button = blue board button (PC13)
  *   Serial port      ST-Link virtual COM, 115200 8N1
  *
  * Buttons
  *   RED held at start-up      -> CONFIGURATION
  *   RED short press           -> measure now and save to the SD
  *   GREEN short press         -> print the check list again
  *   RED 5 s                   -> MAINTENANCE (and back)
  *   GREEN 5 s (in STANDARD)   -> ECONOMY ; RED 5 s in ECONOMY -> STANDARD
  *
  * Nothing ever waits more than about 1 s: sensors are read once every 2 s
  * (no retry loop), the measurement line uses the last good values.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "fatfs.h"

/* USER CODE BEGIN Includes */
#include "features.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
/* USER CODE END Includes */

/* ============================================================================ */
/*                               TYPES & SETTINGS                               */
/* ============================================================================ */

typedef enum { MODE_STANDARD = 0, MODE_CONFIGURATION, MODE_MAINTENANCE, MODE_ECONOMY } SystemMode_t;

/* Error flags, in the priority order of the LED table */
#define ERR_RTC          (1U << 0)
#define ERR_GPS          (1U << 1)
#define ERR_SENSOR       (1U << 2)   /* sensor not answering (2 measurements in a row) */
#define ERR_DATA         (1U << 3)   /* sensor value out of range                     */
#define ERR_SD_FULL      (1U << 4)
#define ERR_SD           (1U << 5)   /* SD access / write error                        */

#define FW_VERSION       "3.0.0"
#define LOT_NUMBER       "LOT-2026-L476RG"

#define READ_PERIOD_MS   2000U       /* sensors read + printed every 2 s           */
#define FIRST_LOG_MS     5000U       /* first measurement line 5 s after start-up  */
#define DEBOUNCE_MS      30U
#define LONG_PRESS_MS    5000U
#define CONFIG_TIMEOUT_MS (30UL * 60UL * 1000UL)

/* Buttons */
#define BTN_RED_PIN      GPIO_PIN_0  /* A5 */
#define BTN_RED_PORT     GPIOC
#define BTN_GREEN_PIN    GPIO_PIN_13 /* blue board button */
#define BTN_GREEN_PORT   GPIOC

/* RGB LED: all on port A, pins chosen in features.h */
#define LED_PORT         GPIOA
#if LED_COMMON_ANODE
#define LED_ON           GPIO_PIN_RESET
#define LED_OFF          GPIO_PIN_SET
#else
#define LED_ON           GPIO_PIN_SET
#define LED_OFF          GPIO_PIN_RESET
#endif

/* Settings kept in flash: bank 2 page 255 = last 2 KB page */
#define CONFIG_MAGIC          0x57575733UL
#define FLASH_CONFIG_ADDR     0x080FF800UL

typedef struct {
  uint32_t magic;
  int32_t  log_interval;      /* minutes, 1..1440, default 10      */
  int32_t  file_max_size;     /* bytes, 256..1048576, default 2048 */
  int32_t  timeout;           /* seconds, 1..300, default 30       */
  int32_t  lumin;             /* 0 / 1                             */
  int32_t  lumin_low;         /* 0..1023, default 255              */
  int32_t  lumin_high;        /* 0..1023, default 768              */
  int32_t  temp_air;          /* 0 / 1                             */
  int32_t  min_temp_air;      /* -40..85, default -10              */
  int32_t  max_temp_air;      /* -40..85, default 60               */
  int32_t  hygr;              /* 0 / 1                             */
  int32_t  hygr_mint;         /* -40..85, default 0                */
  int32_t  hygr_maxt;         /* -40..85, default 50               */
  int32_t  pressure;          /* 0 / 1, default 0 (no sensor)      */
  int32_t  pressure_min;      /* 300..1100, default 850            */
  int32_t  pressure_max;      /* 300..1100, default 1080           */
  uint32_t reserved;
  uint32_t checksum;
} SystemConfig_t;             /* 72 bytes = 9 double-words */

typedef struct { const char *name; uint8_t off; int32_t min, max, def; } Param_t;

#define P(n, f, lo, hi, d) { n, (uint8_t)offsetof(SystemConfig_t, f), lo, hi, d }
static const Param_t params[] = {
  P("LOG_INTERVAL",  log_interval,   1, 1440,     10),
  P("FILE_MAX_SIZE", file_max_size,  256, 1048576, 2048),
  P("TIMEOUT",       timeout,        1, 300,      30),
  P("LUMIN",         lumin,          0, 1,        1),
  P("LUMIN_LOW",     lumin_low,      0, 1023,     255),
  P("LUMIN_HIGH",    lumin_high,     0, 1023,     768),
  P("TEMP_AIR",      temp_air,       0, 1,        1),
  P("MIN_TEMP_AIR",  min_temp_air,  -40, 85,     -10),
  P("MAX_TEMP_AIR",  max_temp_air,  -40, 85,      60),
  P("HYGR",          hygr,           0, 1,        1),
  P("HYGR_MINT",     hygr_mint,     -40, 85,      0),
  P("HYGR_MAXT",     hygr_maxt,     -40, 85,      50),
  P("PRESSURE",      pressure,       0, 1,        0),
  P("PRESSURE_MIN",  pressure_min,   300, 1100,   850),
  P("PRESSURE_MAX",  pressure_max,   300, 1100,   1080),
};
#define PARAM_COUNT (sizeof(params) / sizeof(params[0]))

/* SD card commands (SPI mode) */
#define SD_CMD0    (0)
#define SD_CMD8    (8)
#define SD_CMD13   (13)
#define SD_CMD16   (16)
#define SD_CMD17   (17)
#define SD_CMD24   (24)
#define SD_CMD55   (55)
#define SD_CMD58   (58)
#define SD_ACMD41  (0x80 + 41)

/* ============================================================================ */
/*                                  VARIABLES                                   */
/* ============================================================================ */
I2C_HandleTypeDef  hi2c1;
SPI_HandleTypeDef  hspi1;
UART_HandleTypeDef huart2;

static uint32_t cyc_per_us = 80U;

static SystemMode_t current_mode  = MODE_STANDARD;
static SystemMode_t previous_mode = MODE_STANDARD;
static volatile uint32_t err_flags = 0U;
static SystemConfig_t sys_cfg __attribute__((aligned(8)));
static bool rtc_ok = false;

static uint32_t next_read_tick = 0U;
static uint32_t next_log_tick = 0U;
static uint32_t last_config_activity_tick = 0U;
static uint8_t  dht_fail_count = 0U, lux_fail_count = 0U;

/* Latest readings (taken every READ_PERIOD_MS) */
static int16_t  rd_t = 0, rd_h = 0;
static uint32_t rd_lux_x100 = 0U;
static uint32_t rd_th_tick = 0U, rd_lux_tick = 0U;   /* time of the last GOOD reading, 0 = never */
__attribute__((unused)) static int8_t rd_th_res = 0;                      /* last DHT11 result, 1 = ok              */
__attribute__((unused)) static bool rd_lux_ok = false;

/* Buttons */
typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  bool     stable;            /* debounced state, true = pressed */
  bool     last_raw;
  uint32_t change_tick;
  uint32_t press_tick;
  bool     long_done;
} Button_t;
static Button_t btn_red   = { BTN_RED_PORT,   BTN_RED_PIN,   false, false, 0U, 0U, false };
static Button_t btn_green = { BTN_GREEN_PORT, BTN_GREEN_PIN, false, false, 0U, 0U, false };

/* Serial commands */
#define RX_BUFFER_SIZE 96
static uint8_t rx_byte = 0;
static char rx_line_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_line_idx = 0;
static volatile bool rx_line_ready = false;

/* SD card */
static uint8_t sd_is_hc = 0;
static uint8_t sd_fail_step = 0;
static volatile DSTATUS sd_stat = STA_NOINIT;
static FATFS sd_fs;
static FIL sd_file;
static bool sd_mounted = false;
static char sd_day_dir[20] = "";

/* LCD */
__attribute__((unused)) static uint8_t lcd_addr = 0x27;
__attribute__((unused)) static bool lcd_ready = false;
static char lcd_vals[17] = "";

/* ============================================================================ */
/*                                 PROTOTYPES                                   */
/* ============================================================================ */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);

static void App_Init(void);
static void App_Run(void);
static void Boot_Checks(void);
static void Lcd_Refresh(bool force);
static bool SD_MountFS(void);
static void SD_UnmountFS(void);

/* ============================================================================ */
/*                       PRINTF, MICROSECOND DELAY                              */
/* ============================================================================ */
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 10);
  return ch;
}

static void DWT_Init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  cyc_per_us = SystemCoreClock / 1000000U;
  if (cyc_per_us == 0U) cyc_per_us = 80U;
}

static inline void delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT, ticks = us * cyc_per_us;
  while ((DWT->CYCCNT - start) < ticks) { }
}

static bool Due(uint32_t now, uint32_t t) { return (int32_t)(now - t) >= 0; }

/* ============================================================================ */
/*                                   RGB LED                                    */
/* ============================================================================ */
static void Led_SetHardware(bool r, bool g, bool b)
{
#if USE_LED
  HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN,   r ? LED_ON : LED_OFF);
  HAL_GPIO_WritePin(LED_PORT, LED_GREEN_PIN, g ? LED_ON : LED_OFF);
  HAL_GPIO_WritePin(LED_PORT, LED_BLUE_PIN,  b ? LED_ON : LED_OFF);
#else
  (void)r; (void)g; (void)b;
#endif
}

/* Error (highest priority first) alternates red with the colour at 1 Hz, else mode colour */
static void Led_Update(void)
{
  uint32_t now = HAL_GetTick(), ph = now % 1000U, f = err_flags;
  bool red_half = (ph < 500U), red_third = (ph < 333U);

  if      (f & ERR_RTC)     { if (red_half)  Led_SetHardware(1, 0, 0); else Led_SetHardware(0, 0, 1); }
  else if (f & ERR_GPS)     { if (red_half)  Led_SetHardware(1, 0, 0); else Led_SetHardware(1, 1, 0); }
  else if (f & ERR_SENSOR)  { if (red_half)  Led_SetHardware(1, 0, 0); else Led_SetHardware(0, 1, 0); }
  else if (f & ERR_DATA)    { if (red_third) Led_SetHardware(1, 0, 0); else Led_SetHardware(0, 1, 0); }
  else if (f & ERR_SD_FULL) { if (red_half)  Led_SetHardware(1, 0, 0); else Led_SetHardware(1, 1, 1); }
  else if (f & ERR_SD)      { if (red_third) Led_SetHardware(1, 0, 0); else Led_SetHardware(1, 1, 1); }
  else
  {
    switch (current_mode)
    {
      case MODE_STANDARD:      Led_SetHardware(0, 1, 0); break;               /* green  */
      case MODE_CONFIGURATION: Led_SetHardware(1, 1, 0); break;               /* yellow */
      case MODE_ECONOMY:       Led_SetHardware(0, 0, 1); break;               /* blue   */
      case MODE_MAINTENANCE:   Led_SetHardware(1, (now % 4U) == 0U, 0); break; /* orange = red + 25 % green */
      default:                 Led_SetHardware(0, 0, 0); break;
    }
  }
}

static void Led_Test(void)
{
#if USE_LED
  printf("[LED] Test : la LED doit etre ROUGE, puis VERTE, puis BLEUE\r\n");
  Led_SetHardware(1, 0, 0); HAL_Delay(400);
  Led_SetHardware(0, 1, 0); HAL_Delay(400);
  Led_SetHardware(0, 0, 1); HAL_Delay(400);
  Led_SetHardware(0, 0, 0);
#endif
}

/* ============================================================================ */
/*                           SETTINGS IN FLASH                                  */
/* ============================================================================ */
static int32_t *Cfg_Field(SystemConfig_t *c, uint8_t off) { return (int32_t *)((uint8_t *)c + off); }

static uint32_t Config_Checksum(const SystemConfig_t *c)
{
  const uint32_t *w = (const uint32_t *)c;
  uint32_t sum = 0x1234ABCDUL;
  for (size_t i = 0; i < (sizeof(SystemConfig_t) / 4U) - 1U; i++) sum = (sum * 31U) ^ w[i];
  return sum;
}

static bool Config_PairsOk(const SystemConfig_t *c)
{
  return (c->lumin_low <= c->lumin_high) && (c->min_temp_air <= c->max_temp_air) &&
         (c->hygr_mint <= c->hygr_maxt) && (c->pressure_min <= c->pressure_max);
}

static void Config_RestoreDefaults(void)
{
  memset(&sys_cfg, 0, sizeof(sys_cfg));
  sys_cfg.magic = CONFIG_MAGIC;
  for (size_t i = 0; i < PARAM_COUNT; i++) *Cfg_Field(&sys_cfg, params[i].off) = params[i].def;
}

static bool Config_Save(void)
{
  uint64_t buf[sizeof(SystemConfig_t) / 8U];
  const size_t n = sizeof(SystemConfig_t) / 8U;
  FLASH_EraseInitTypeDef er = { 0 };
  uint32_t page_error = 0;
  bool ok;

  sys_cfg.magic = CONFIG_MAGIC;
  sys_cfg.reserved = 0U;
  sys_cfg.checksum = Config_Checksum(&sys_cfg);
  memcpy(buf, &sys_cfg, sizeof(buf));

  er.TypeErase = FLASH_TYPEERASE_PAGES;
  er.Banks = FLASH_BANK_2;
  er.Page = 255;
  er.NbPages = 1;

  HAL_FLASH_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
  ok = (HAL_FLASHEx_Erase(&er, &page_error) == HAL_OK);
  for (size_t k = 1U; ok && k <= n; k++)          /* magic (word 0) LAST: a power cut leaves the page invalid */
  {
    size_t i = (k == n) ? 0U : k;
    ok = (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, FLASH_CONFIG_ADDR + 8U * i, buf[i]) == HAL_OK);
  }
  HAL_FLASH_Lock();
  return ok && (memcmp((const void *)FLASH_CONFIG_ADDR, &sys_cfg, sizeof(sys_cfg)) == 0);
}

static void Config_LoadOrDefault(void)
{
  const SystemConfig_t *f = (const SystemConfig_t *)FLASH_CONFIG_ADDR;
  bool ok = (f->magic == CONFIG_MAGIC) && (f->checksum == Config_Checksum(f)) && Config_PairsOk(f);
  for (size_t i = 0; ok && i < PARAM_COUNT; i++)
  {
    int32_t v = *Cfg_Field((SystemConfig_t *)f, params[i].off);
    ok = (v >= params[i].min && v <= params[i].max);
  }
  if (ok) { memcpy(&sys_cfg, f, sizeof(sys_cfg)); printf("[CONFIG] Parametres charges depuis la Flash\r\n"); }
  else
  {
    Config_RestoreDefaults();
    printf("[CONFIG] Premiere utilisation : valeurs par defaut %s\r\n", Config_Save() ? "sauvegardees" : "(ERREUR Flash)");
  }
}

/* ============================================================================ */
/*                       INTERNAL RTC (32 kHz crystal, registers)               */
/* ============================================================================ */
static uint8_t bcd2bin(uint8_t v) { return (uint8_t)((v >> 4) * 10U + (v & 0x0FU)); }
static uint32_t bin2bcd(uint32_t v) { return ((v / 10U) << 4) | (v % 10U); }

static bool RTC_WaitSync(void)
{
  uint32_t t0 = HAL_GetTick();
  RTC->WPR = 0xCAU; RTC->WPR = 0x53U;
  RTC->ISR &= ~RTC_ISR_RSF;
  RTC->WPR = 0xFFU;
  while ((RTC->ISR & RTC_ISR_RSF) == 0U) if ((HAL_GetTick() - t0) > 1000U) return false;
  return true;
}

static bool RTC_IsSet(void) { return rtc_ok && (RTC->ISR & RTC_ISR_INITS) != 0U; }

/* Keeps the time through RESET / flashing: if the RTC already runs, nothing is touched */
static void RTC_Start(void)
{
  RCC_OscInitTypeDef osc = { 0 };
  RCC_PeriphCLKInitTypeDef pclk = { 0 };
  bool lse;

  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  if ((RCC->BDCR & RCC_BDCR_RTCEN) && (RTC->ISR & RTC_ISR_INITS)) { rtc_ok = RTC_WaitSync(); return; }

  osc.OscillatorType = RCC_OSCILLATORTYPE_LSE;
  osc.LSEState = RCC_LSE_ON;
  lse = (HAL_RCC_OscConfig(&osc) == HAL_OK);
  if (!lse)
  {
    printf("[RTC] Quartz 32 kHz absent : horloge interne (moins precise)\r\n");
    memset(&osc, 0, sizeof(osc));
    osc.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    osc.LSIState = RCC_LSI_ON;
    (void)HAL_RCC_OscConfig(&osc);
  }
  pclk.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  pclk.RTCClockSelection = lse ? RCC_RTCCLKSOURCE_LSE : RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK) return;
  __HAL_RCC_RTC_ENABLE();
  rtc_ok = RTC_WaitSync();
}

static bool RTC_Write(bool set_tr, uint32_t tr, bool set_dr, uint32_t dr)
{
  uint32_t t0 = HAL_GetTick();
  if (!rtc_ok) return false;
  RTC->WPR = 0xCAU; RTC->WPR = 0x53U;
  RTC->ISR = 0xFFFFFFFFU;                         /* init mode */
  while ((RTC->ISR & RTC_ISR_INITF) == 0U)
    if ((HAL_GetTick() - t0) > 1000U) { RTC->WPR = 0xFFU; return false; }
  if (set_tr) RTC->TR = tr;
  if (set_dr) RTC->DR = dr;
  RTC->ISR &= ~RTC_ISR_INIT;
  RTC->WPR = 0xFFU;
  return RTC_WaitSync();
}

static uint8_t Weekday(uint32_t y, uint32_t m, uint32_t d)   /* 1 = Monday ... 7 = Sunday */
{
  static const uint8_t t[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  uint32_t w;
  if (m < 3U) y--;
  w = (y + y / 4U - y / 100U + y / 400U + t[m - 1U] + d) % 7U;
  return (uint8_t)((w == 0U) ? 7U : w);
}

static bool RTC_SetTime(uint8_t h, uint8_t m, uint8_t s)
{
  return RTC_Write(true, (bin2bcd(h) << 16) | (bin2bcd(m) << 8) | bin2bcd(s), false, 0U);
}

static bool RTC_SetDate(uint8_t month, uint8_t day, uint8_t year)   /* year 0..99 */
{
  uint32_t wd = Weekday(2000U + year, month, day);
  return RTC_Write(false, 0U, true, (bin2bcd(year) << 16) | (wd << 13) | (bin2bcd(month) << 8) | bin2bcd(day));
}

static bool RTC_SetWeekday(uint8_t wd)
{
  return RTC_Write(false, 0U, true, (RTC->DR & ~(7UL << 13)) | ((uint32_t)wd << 13));
}

typedef struct { uint8_t year, month, day, hour, min, sec; } DateTime_t;

static bool RTC_Get(DateTime_t *d)
{
  uint32_t tr, dr;
  memset(d, 0, sizeof(*d));
  if (!rtc_ok) { err_flags |= ERR_RTC; return false; }
  tr = RTC->TR;                                   /* TR first: locks DR until it is read */
  dr = RTC->DR;
  d->hour  = bcd2bin((uint8_t)((tr >> 16) & 0x3FU));
  d->min   = bcd2bin((uint8_t)((tr >> 8) & 0x7FU));
  d->sec   = bcd2bin((uint8_t)(tr & 0x7FU));
  d->year  = bcd2bin((uint8_t)((dr >> 16) & 0xFFU));
  d->month = bcd2bin((uint8_t)((dr >> 8) & 0x1FU));
  d->day   = bcd2bin((uint8_t)(dr & 0x3FU));
  if (d->month < 1U || d->month > 12U || d->day < 1U || d->day > 31U) { err_flags |= ERR_RTC; return false; }
  err_flags &= ~ERR_RTC;
  return true;
}

/* ============================================================================ */
/*                                 SENSORS                                      */
/* ============================================================================ */

/* ---------- DHT11 on A3 (PB0) ---------- */
#define DHT_IN()   ((GPIOB->IDR & GPIO_PIN_0) != 0U)

/* waits while the line is at 'level'; false after max_us */
static bool Dht_While(bool level, uint32_t max_us)
{
  uint32_t t0 = DWT->CYCCNT, lim = max_us * cyc_per_us;
  while (DHT_IN() == level) if ((DWT->CYCCNT - t0) > lim) return false;
  return true;
}

/* 1 = ok, -1 no answer (wiring), -2..-5 cut during transfer, -6 bad checksum.
   The DHT11 fails when read more often than every 2 s: a read sooner returns the last result. */
__attribute__((unused)) static int8_t DHT11_Read(int16_t *temperature, int16_t *humidity)
{
  static uint32_t last_tick = 0;
  static int8_t   last_res = -1;
  static int16_t  last_t = 0, last_h = 0;
  static bool     once = false;
  uint8_t data[5] = { 0 };
  int8_t res = 1;
  uint32_t now = HAL_GetTick();

  if (once && (now - last_tick) < 2000U)
  {
    if (last_res == 1) { *temperature = last_t; *humidity = last_h; }
    return last_res;
  }
  once = true;
  last_tick = now;

  GPIOB->MODER = (GPIOB->MODER & ~0x3UL) | 0x1UL;    /* PB0 output */
  GPIOB->BSRR = (uint32_t)GPIO_PIN_0 << 16;          /* low 20 ms = start */
  HAL_Delay(20);
  __disable_irq();                                   /* about 4 ms, timing critical */
  GPIOB->BSRR = GPIO_PIN_0;
  delay_us(30);
  GPIOB->MODER &= ~0x3UL;                            /* PB0 input (pull-up) */

  if      (!Dht_While(true, 100U))  res = -1;        /* sensor pulls low  */
  else if (!Dht_While(false, 100U)) res = -2;        /* 80 us low         */
  else if (!Dht_While(true, 100U))  res = -3;        /* 80 us high        */
  for (int i = 0; res == 1 && i < 40; i++)
  {
    uint32_t t0;
    if (!Dht_While(false, 80U)) { res = -4; break; }  /* 50 us low before each bit */
    t0 = DWT->CYCCNT;
    if (!Dht_While(true, 100U)) { res = -5; break; }  /* high: 26 us = 0, 70 us = 1 */
    if ((DWT->CYCCNT - t0) > 45U * cyc_per_us) data[i / 8] |= (uint8_t)(1U << (7 - (i % 8)));
  }
  __enable_irq();
  GPIOB->MODER = (GPIOB->MODER & ~0x3UL) | 0x1UL;    /* back to output, high = idle */
  GPIOB->BSRR = GPIO_PIN_0;

  if (res == 1 && (uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) res = -6;
  if (res == 1) { last_h = (int16_t)data[0]; last_t = (int16_t)data[2]; *humidity = last_h; *temperature = last_t; }
  last_res = res;
  return res;
}

/* ---------- BH1750 light sensor (I2C 0x23) ---------- */
__attribute__((unused)) static uint8_t BH1750_Init(void)
{
  uint8_t cmd = 0x01;                                /* power on */
  if (HAL_I2C_Master_Transmit(&hi2c1, (0x23 << 1), &cmd, 1, 20) != HAL_OK) return 0;
  cmd = 0x10;                                        /* continuous high resolution */
  if (HAL_I2C_Master_Transmit(&hi2c1, (0x23 << 1), &cmd, 1, 20) != HAL_OK) return 0;
  return 1;
}

__attribute__((unused)) static uint8_t BH1750_ReadLux(uint32_t *lux_x100)
{
  uint8_t rx[2] = { 0 };
  if (HAL_I2C_Master_Receive(&hi2c1, (0x23 << 1), rx, 2, 20) != HAL_OK) return 0;
  *lux_x100 = ((uint32_t)((rx[0] << 8) | rx[1]) * 1000U) / 12U;   /* lux = raw / 1.2 */
  return 1;
}

/* the spec uses a 0..1023 scale: 1 lux = 1 unit, capped at 1023 */
static uint16_t Lux_Scale(uint32_t lux_x100)
{
  uint32_t lux = lux_x100 / 100U;
  return (lux > 1023U) ? 1023U : (uint16_t)lux;
}

static const char *Lux_Class(uint16_t v)
{
  return (v < sys_cfg.lumin_low) ? "LOW" : ((v > sys_cfg.lumin_high) ? "HIGH" : "MID");
}

/* Reads every sensor switched on, ONCE (no waiting loop). A failed read keeps the last
   good value and its time: the measurement uses TIMEOUT to decide if it is still valid. */
static void Sensors_Read(void)
{
  uint32_t now = HAL_GetTick();
#if USE_TEMP_HUM
  {
    int16_t t = 0, h = 0;
    rd_th_res = DHT11_Read(&t, &h);
    if (rd_th_res == 1) { rd_t = t; rd_h = h; rd_th_tick = now | 1U; }
  }
#endif
#if USE_LIGHT
  {
    uint32_t lx;
    rd_lux_ok = (BH1750_ReadLux(&lx) != 0U);
    if (rd_lux_ok) { rd_lux_x100 = lx; rd_lux_tick = now | 1U; }
  }
#endif
  (void)now;
}

/* ============================================================================ */
/*                          LCD 16x2 (PCF8574 backpack)                         */
/* ============================================================================ */
#if USE_LCD
#define LCD_BL  0x08U
#define LCD_EN  0x04U

static void Lcd_Byte(uint8_t v, uint8_t rs)         /* one byte = 4 I2C bytes in one transfer */
{
  uint8_t hi = (uint8_t)((v & 0xF0U) | LCD_BL | rs), lo = (uint8_t)(((v << 4) & 0xF0U) | LCD_BL | rs);
  uint8_t b[4] = { (uint8_t)(hi | LCD_EN), hi, (uint8_t)(lo | LCD_EN), lo };
  if (!lcd_ready) return;                            /* stop at the first failure: no long wait */
  if (HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(lcd_addr << 1), b, 4, 5) != HAL_OK) lcd_ready = false;
}

static bool Lcd_Init(void)
{
  static const uint8_t cand[2] = { 0x27U, 0x3FU };
  bool found = false;
  for (uint8_t i = 0U; i < 2U && !found; i++)
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(cand[i] << 1), 2, 10) == HAL_OK) { lcd_addr = cand[i]; found = true; }
  if (!found) return false;
  lcd_ready = true;
  HAL_Delay(50);
  for (uint8_t i = 0U; i < 4U && lcd_ready; i++)    /* wake-up: 0x30 x3, then 0x20 = 4-bit mode */
  {
    uint8_t d = (uint8_t)(((i < 3U) ? 0x30U : 0x20U) | LCD_BL);
    uint8_t b[2] = { (uint8_t)(d | LCD_EN), d };
    if (HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(lcd_addr << 1), b, 2, 5) != HAL_OK) lcd_ready = false;
    HAL_Delay(5);
  }
  Lcd_Byte(0x28U, 0U);                               /* 4-bit, 2 lines */
  Lcd_Byte(0x0CU, 0U);                               /* display on, no cursor */
  Lcd_Byte(0x01U, 0U);                               /* clear */
  HAL_Delay(2);
  Lcd_Byte(0x06U, 0U);
  return lcd_ready;
}

static void Lcd_Line(uint8_t row, const char *txt, char shown[17])   /* sends only if changed */
{
  char b[17];
  snprintf(b, sizeof(b), "%-16.16s", txt);
  if (!lcd_ready || strcmp(b, shown) == 0) return;
  Lcd_Byte((uint8_t)(0x80U | (row ? 0x40U : 0x00U)), 0U);
  for (uint8_t i = 0U; i < 16U; i++) Lcd_Byte((uint8_t)b[i], 1U);
  if (lcd_ready) memcpy(shown, b, 17U); else shown[0] = '\0';
}
#endif

/* line 1 = mode + clock, line 2 = readings (alternating with the error every 2 s) */
static void Lcd_Refresh(bool force)
{
#if USE_LCD
  static const char *const modes[4] = { "STANDARD", "CONFIG", "MAINT", "ECONOMY" };
  static const char *const errs[6]  = { "RTC", "GPS", "CAPTEUR", "DONNEE", "SD PLEINE", "SD" };
  static uint32_t next_tick = 0U, retry_tick = 0U, full_tick = 0U;
  static char shown[2][17];
  char l0[17], l1[17], clk[12] = "--:--";
  const char *err = NULL;
  uint32_t now = HAL_GetTick();
  DateTime_t dt;

  if (!force && !Due(now, next_tick)) return;
  next_tick = now + 1000U;
  if (!lcd_ready)                                    /* unplugged or glitch: look again every 5 s */
  {
    if (!Due(now, retry_tick)) return;
    retry_tick = now + 5000U;
    if (!Lcd_Init()) return;
    memset(shown, 0, sizeof(shown));
  }
  if (Due(now, full_tick)) { full_tick = now + 30000U; memset(shown, 0, sizeof(shown)); }

  if (RTC_IsSet() && RTC_Get(&dt)) snprintf(clk, sizeof(clk), "%02u:%02u", dt.hour, dt.min);
  snprintf(l0, sizeof(l0), "%-10.10s %.5s", modes[current_mode & 3], clk);
  for (int i = 0; i < 6; i++) if (err_flags & (1U << i)) { err = errs[i]; break; }
  if (current_mode == MODE_CONFIGURATION)          snprintf(l1, sizeof(l1), "Serial 115200");
  else if (err != NULL && ((now / 2000U) & 1U))   snprintf(l1, sizeof(l1), "ERR %s", err);
  else                                             snprintf(l1, sizeof(l1), "%s", lcd_vals[0] ? lcd_vals : "-- -- --");
  Lcd_Line(0, l0, shown[0]);
  Lcd_Line(1, l1, shown[1]);
#else
  (void)force;
#endif
}

/* ============================================================================ */
/*                             SD CARD (SPI1)                                   */
/* ============================================================================ */
static uint8_t spi_rw(uint8_t tx)
{
  uint8_t rx = 0xFF;
  HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 10);
  return rx;
}

static void spi_speed(uint32_t presc)
{
  __HAL_SPI_DISABLE(&hspi1);
  MODIFY_REG(hspi1.Instance->CR1, SPI_CR1_BR, presc);
  hspi1.Init.BaudRatePrescaler = presc;
  __HAL_SPI_ENABLE(&hspi1);
}

static void sd_deselect(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
  spi_rw(0xFF);
}

static uint8_t sd_wait_ready(uint32_t ms)
{
  uint32_t t = HAL_GetTick();
  uint8_t r;
  do { r = spi_rw(0xFF); } while (r != 0xFF && (HAL_GetTick() - t) < ms);
  return (r == 0xFF);
}

static uint8_t sd_select(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
  spi_rw(0xFF);
  if (sd_wait_ready(500)) return 1;
  sd_deselect();
  return 0;
}

static uint8_t sd_cmd(uint8_t cmd, uint32_t arg)
{
  uint8_t r, n;
  if (cmd & 0x80)
  {
    cmd &= 0x7F;
    r = sd_cmd(SD_CMD55, 0);
    if (r > 1) return r;
  }
  sd_deselect();
  if (cmd == SD_CMD0) { HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET); spi_rw(0xFF); }
  else if (!sd_select()) return 0xFF;

  spi_rw(0x40 | cmd);
  spi_rw((uint8_t)(arg >> 24)); spi_rw((uint8_t)(arg >> 16)); spi_rw((uint8_t)(arg >> 8)); spi_rw((uint8_t)arg);
  spi_rw((cmd == SD_CMD0) ? 0x95 : ((cmd == SD_CMD8) ? 0x87 : 0x01));
  n = 10;
  do { r = spi_rw(0xFF); } while ((r & 0x80) && --n);
  return r;
}

static DSTATUS SD_Disk_Init(BYTE pdrv)
{
  static bool powered = false;
  uint8_t ocr[4], r = 0xFF, i, ok = 0;
  uint32_t t;
  (void)pdrv;

  sd_fail_step = 0;
  spi_speed(SPI_BAUDRATEPRESCALER_256);              /* start-up below 400 kHz */
  if (!powered) { HAL_Delay(100); powered = true; }
  sd_is_hc = 0;
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
  for (i = 0; i < 10; i++) spi_rw(0xFF);             /* 80 clocks with CS high */

  for (i = 0; i < 10 && r != 1; i++) { r = sd_cmd(SD_CMD0, 0); if (r != 1) HAL_Delay(5); }
  if (r != 1) sd_fail_step = 1;                      /* no answer at all: wiring / power */
  else
  {
    t = HAL_GetTick();
    if (sd_cmd(SD_CMD8, 0x1AA) == 1)
    {
      for (i = 0; i < 4; i++) ocr[i] = spi_rw(0xFF);
      if (ocr[2] != 0x01 || ocr[3] != 0xAA) sd_fail_step = 2;
      else
      {
        do { r = sd_cmd(SD_ACMD41, 1UL << 30); } while (r != 0 && (HAL_GetTick() - t) < 1000);
        if (r != 0) sd_fail_step = 3;
        else if (sd_cmd(SD_CMD58, 0) != 0) sd_fail_step = 4;
        else
        {
          for (i = 0; i < 4; i++) ocr[i] = spi_rw(0xFF);
          sd_is_hc = (ocr[0] & 0x40) ? 1 : 0;
          ok = (sd_is_hc || sd_cmd(SD_CMD16, 512) == 0) ? 1 : 0;
          if (!ok) sd_fail_step = 6;
        }
      }
    }
    else
    {
      do { r = sd_cmd(SD_ACMD41, 0); } while (r != 0 && (HAL_GetTick() - t) < 1000);
      ok = (r == 0 && sd_cmd(SD_CMD16, 512) == 0) ? 1 : 0;
      if (!ok) sd_fail_step = 5;
    }
  }
  sd_deselect();
  if (ok) spi_speed(SPI_BAUDRATEPRESCALER_32);       /* 2.5 MHz once the card is ready */
  sd_stat = ok ? 0 : STA_NOINIT;
  return sd_stat;
}

static DSTATUS SD_Disk_Status(BYTE pdrv) { (void)pdrv; return sd_stat; }

static DRESULT SD_Disk_Read(BYTE pdrv, BYTE *buf, DWORD sector, UINT count)
{
  (void)pdrv;
  if (sd_stat & STA_NOINIT) return RES_NOTRDY;
  while (count--)
  {
    uint8_t tok;
    uint32_t t = HAL_GetTick();
    if (sd_cmd(SD_CMD17, sd_is_hc ? sector : sector * 512) != 0) { sd_deselect(); return RES_ERROR; }
    do { tok = spi_rw(0xFF); } while (tok == 0xFF && (HAL_GetTick() - t) < 200);
    if (tok != 0xFE) { sd_deselect(); return RES_ERROR; }
    for (int i = 0; i < 512; i++) buf[i] = spi_rw(0xFF);
    spi_rw(0xFF); spi_rw(0xFF);
    sd_deselect();
    buf += 512;
    sector++;
  }
  return RES_OK;
}

static DRESULT SD_Disk_Write(BYTE pdrv, const BYTE *buf, DWORD sector, UINT count)
{
  (void)pdrv;
  if (sd_stat & STA_NOINIT) return RES_NOTRDY;
  while (count--)
  {
    if (sd_cmd(SD_CMD24, sd_is_hc ? sector : sector * 512) != 0) { sd_deselect(); return RES_ERROR; }
    spi_rw(0xFF);
    spi_rw(0xFE);
    for (int i = 0; i < 512; i++) spi_rw(buf[i]);
    spi_rw(0xFF); spi_rw(0xFF);
    if ((spi_rw(0xFF) & 0x1F) != 0x05 || !sd_wait_ready(500)) { sd_deselect(); return RES_ERROR; }
    sd_deselect();
    if (sd_cmd(SD_CMD13, 0) != 0 || spi_rw(0xFF) != 0) { sd_deselect(); return RES_ERROR; }   /* written ok? */
    sd_deselect();
    buf += 512;
    sector++;
  }
  return RES_OK;
}

static DRESULT SD_Disk_Ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  (void)pdrv;
  if (sd_stat & STA_NOINIT) return RES_NOTRDY;
  switch (cmd)
  {
    case CTRL_SYNC:       if (sd_select()) { sd_deselect(); return RES_OK; } return RES_ERROR;
    case GET_SECTOR_SIZE: *(WORD *)buff = 512; return RES_OK;
    case GET_BLOCK_SIZE:  *(DWORD *)buff = 1;  return RES_OK;
    default:              return RES_PARERR;
  }
}

/* ONE quick attempt (about 1 s max if the card does not answer) */
static bool SD_MountFS(void)
{
  FRESULT fr = FR_DISK_ERR;
  if (sd_mounted) return true;
  if (SD_Disk_Init(0) == 0) fr = f_mount(&sd_fs, "0:", 1);
  if (fr != FR_OK) { printf("[SD] Carte non disponible (etape %u, f_mount=%d)\r\n", sd_fail_step, (int)fr); return false; }
  sd_mounted = true;
  return true;
}

static void SD_UnmountFS(void)
{
  if (sd_mounted) (void)f_mount(NULL, "0:", 0);
  sd_mounted = false;
  sd_stat = STA_NOINIT;
  sd_day_dir[0] = '\0';                             /* the card may be changed: check folders again */
}

static void SD_Fail(uint32_t flag) { err_flags |= flag; SD_UnmountFS(); }

/* Folder of the day: 0:/2026/10/20 (or 0: if USE_FOLDERS is 0) */
static bool SD_DayDir(const DateTime_t *dt, char *dir, size_t n)
{
#if USE_FOLDERS
  char d[20];
  snprintf(d, sizeof(d), "0:/20%02u/%02u/%02u", dt->year, dt->month, dt->day);
  if (strcmp(d, sd_day_dir) != 0)                   /* create the 3 folders once per day */
  {
    for (uint8_t cut = 7U; cut <= 13U; cut += 3U)   /* "0:/2026", "0:/2026/10", "0:/2026/10/20" */
    {
      char c = d[cut];
      FRESULT fr;
      d[cut] = '\0';
      fr = f_mkdir(d);
      d[cut] = c;
      if (fr != FR_OK && fr != FR_EXIST) { printf("[SD] Dossier impossible (%d)\r\n", (int)fr); return false; }
    }
    memcpy(sd_day_dir, d, sizeof(sd_day_dir));
  }
  snprintf(dir, n, "%s", sd_day_dir);
#else
  (void)dt;
  snprintf(dir, n, "0:");
#endif
  return true;
}

/* Writes one line in YYMMDD_0.LOG. When FILE_MAX_SIZE would be passed, _0 is renamed to the
   next free revision (_1.._9 then _A.._Z: 8.3 names) and a new _0 starts. */
__attribute__((unused)) static bool SD_AppendLog(const DateTime_t *dt, const char *line)
{
  static const char revs[] = "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  char dir[20], p0[44], pn[44];
  FILINFO fno;
  FRESULT fr, fc;
  UINT bw = 0;
  UINT len = (UINT)strlen(line);

  if (!SD_MountFS()) { err_flags |= ERR_SD; return false; }
  if (!SD_DayDir(dt, dir, sizeof(dir))) { SD_Fail(ERR_SD); return false; }
  snprintf(p0, sizeof(p0), "%s/%02u%02u%02u_0.LOG", dir, dt->year, dt->month, dt->day);

  if (f_stat(p0, &fno) == FR_OK && (fno.fsize + len) > (DWORD)sys_cfg.file_max_size)
  {
    size_t r = strlen(p0) - 5U;                       /* the '0' before ".LOG" */
    for (uint8_t i = 0U; revs[i] != '\0'; i++)
    {
      memcpy(pn, p0, sizeof(pn));
      pn[r] = revs[i];
      if (f_stat(pn, &fno) == FR_NO_FILE)
      {
        if (f_rename(p0, pn) == FR_OK) printf("[SD] Fichier plein -> %s\r\n", pn + 3);
        break;
      }
    }
  }

  fr = f_open(&sd_file, p0, FA_OPEN_ALWAYS | FA_WRITE);
  if (fr != FR_OK) { SD_Fail(ERR_SD); return false; }
  fr = f_lseek(&sd_file, f_size(&sd_file));
  if (fr == FR_OK) fr = f_write(&sd_file, line, len, &bw);
  fc = f_close(&sd_file);
  if (fr != FR_OK || fc != FR_OK) { SD_Fail(ERR_SD); return false; }
  if (bw != len) { SD_Fail(ERR_SD_FULL); return false; }       /* short write = card full */
  err_flags &= ~(ERR_SD | ERR_SD_FULL);
  printf("[SD] Ecrit dans %s\r\n", p0 + 3);
  return true;
}

/* ============================================================================ */
/*                        READINGS, MEASUREMENT, CHECKS                         */
/* ============================================================================ */

/* [DATA hh:mm:ss] line on the serial port + LCD, every 2 s */
static void Data_Print(void)
{
  char clk[12] = "--:--:--", ts[16] = "OFF", hs[12] = "OFF", ls[20] = "OFF";
  char tl[8] = "--", hl[8] = "--", ll[8] = "--", s[32];
  DateTime_t dt;

  if (RTC_Get(&dt)) snprintf(clk, sizeof(clk), "%02u:%02u:%02u", dt.hour, dt.min, dt.sec);
#if USE_TEMP_HUM
  if (rd_th_res == 1)
  {
    snprintf(ts, sizeof(ts), "%d C", rd_t);  snprintf(hs, sizeof(hs), "%d %%", rd_h);
    snprintf(tl, sizeof(tl), "%dC", rd_t);   snprintf(hl, sizeof(hl), "%d%%", rd_h);
  }
  else { snprintf(ts, sizeof(ts), "NA (err %d)", rd_th_res); snprintf(hs, sizeof(hs), "NA"); }
#endif
#if USE_LIGHT
  if (rd_lux_ok)
  {
    uint16_t v = Lux_Scale(rd_lux_x100);
    snprintf(ls, sizeof(ls), "%u (%s)", v, Lux_Class(v));
    snprintf(ll, sizeof(ll), "%u", v);
  }
  else snprintf(ls, sizeof(ls), "NA");
#endif
  printf("[DATA %s] Temp=%s | Hum=%s | Lum=%s\r\n", clk, ts, hs, ls);
  snprintf(s, sizeof(s), "%s %s L%s", tl, hl, ll);
  snprintf(lcd_vals, sizeof(lcd_vals), "%.16s", s);
  Lcd_Refresh(true);
}

/* One measurement line, from the latest readings: a sensor with no good reading during the
   last TIMEOUT seconds gives NA, and after 2 measurements like that it is in error. */
static void Log_PerformMeasurement(void)
{
  char ts[16] = "NA", hs[16] = "NA", ls[24] = "NA", line[160];
  uint32_t now = HAL_GetTick(), max_age = (uint32_t)sys_cfg.timeout * 1000UL;
  bool th_on = USE_TEMP_HUM && (sys_cfg.temp_air || sys_cfg.hygr);
  bool lx_on = USE_LIGHT && sys_cfg.lumin;
  bool th_fresh = th_on && rd_th_tick != 0U && (now - rd_th_tick) <= max_age;
  bool lx_fresh = lx_on && rd_lux_tick != 0U && (now - rd_lux_tick) <= max_age;
  bool data_err = false;
  DateTime_t dt;

  dht_fail_count = (th_on && !th_fresh) ? (uint8_t)(dht_fail_count + (dht_fail_count < 255U)) : 0U;
  lux_fail_count = (lx_on && !lx_fresh) ? (uint8_t)(lux_fail_count + (lux_fail_count < 255U)) : 0U;

  if (th_fresh)
  {
    if (sys_cfg.temp_air)
    {
      snprintf(ts, sizeof(ts), "%d C", rd_t);
      if (rd_t < sys_cfg.min_temp_air || rd_t > sys_cfg.max_temp_air) data_err = true;
    }
    /* HYGR_MINT / HYGR_MAXT are AIR TEMPERATURE limits: outside them humidity is NA */
    if (sys_cfg.hygr && rd_t >= sys_cfg.hygr_mint && rd_t <= sys_cfg.hygr_maxt) snprintf(hs, sizeof(hs), "%d %%", rd_h);
  }
  if (lx_fresh)
  {
    uint16_t v = Lux_Scale(rd_lux_x100);
    snprintf(ls, sizeof(ls), "%u (%s)", v, Lux_Class(v));   /* LOW / MID / HIGH is a class, not an error */
  }

  if (dht_fail_count >= 2U || lux_fail_count >= 2U) err_flags |= ERR_SENSOR; else err_flags &= ~ERR_SENSOR;
  if (data_err) err_flags |= ERR_DATA; else err_flags &= ~ERR_DATA;

  (void)RTC_Get(&dt);
  snprintf(line, sizeof(line), "20%02u-%02u-%02u %02u:%02u:%02u ; Temp=%s ; Hum=%s ; Lum=%s ; Press=NA ; GPS=NA\r\n",
           dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec, ts, hs, ls);
  printf("[LOG] %s", line);
#if USE_SD
  if (current_mode == MODE_MAINTENANCE) printf("[SD] MAINTENANCE : rien n'est ecrit sur la carte\r\n");
  else if (!SD_AppendLog(&dt, line))    printf("[SD] ERREUR : ligne non ecrite (LED rouge / blanc)\r\n");
#else
  printf("[SD] USE_SD 0 : ligne affichee seulement\r\n");
#endif
}

static void Ram_Print(void)
{
  extern uint32_t _sdata, _estack;                  /* linker script */
  extern void *_sbrk(ptrdiff_t incr);               /* sysmem.c */
  const uint32_t *p = (const uint32_t *)((((uint32_t)_sbrk(0)) + 3U) & ~3UL);
  unsigned long total = (unsigned long)((uint32_t)&_estack - 0x20000000UL);
  unsigned long used_static = (unsigned long)((uint32_t)p - (uint32_t)&_sdata);
  while (p < &_estack && *p == 0xA5A5A5A5UL) p++;   /* pattern written at start-up = never used */
  {
    unsigned long stack = (unsigned long)((uint32_t)&_estack - (uint32_t)p), used = used_static + stack;
    printf("RAM      : %lu o / %lu o (%lu %%)\r\n", used, total, (used * 100UL + total - 1UL) / total);
  }
}

static void Ram_Fill(void)
{
  extern uint32_t _ebss;
  uint32_t *p = &_ebss, *stop = (uint32_t *)__get_MSP() - 256;   /* 1 KB below the stack in use */
  while (p < stop) *p++ = 0xA5A5A5A5UL;
}

/* The check list: at start-up and on a short GREEN press */
static void Boot_Checks(void)
{
  DateTime_t dt;
  Sensors_Read();
  printf("\r\n========== VERIFICATIONS ==========\r\n");
#if USE_LCD
  if (lcd_ready) printf("LCD      : OK (0x%02X)\r\n", lcd_addr); else printf("LCD      : ABSENT (D15/D14, VCC, GND)\r\n");
#else
  printf("LCD      : OFF\r\n");
#endif
#if USE_LIGHT
  if (rd_lux_ok) printf("LUMIERE  : OK (%u)\r\n", Lux_Scale(rd_lux_x100)); else printf("LUMIERE  : ABSENT (D15/D14, VCC, GND)\r\n");
#else
  printf("LUMIERE  : OFF\r\n");
#endif
#if USE_TEMP_HUM
  if (rd_th_res == 1) printf("TEMP/HUM : OK (%d C, %d %%)\r\n", rd_t, rd_h);
  else                printf("TEMP/HUM : PAS DE REPONSE (err %d, fil A3)\r\n", rd_th_res);
#else
  printf("TEMP/HUM : OFF\r\n");
#endif
#if USE_SD
  if (current_mode == MODE_MAINTENANCE) printf("SD       : non verifiee (MAINTENANCE)\r\n");
  else if (SD_MountFS())                printf("SD       : OK (%s)\r\n", sd_is_hc ? "SDHC" : "SD");
  else                                  printf("SD       : ABSENTE (etape %u)\r\n", sd_fail_step);
#else
  printf("SD       : OFF\r\n");
#endif
  if (RTC_IsSet() && RTC_Get(&dt)) printf("HORLOGE  : 20%02u-%02u-%02u %02u:%02u:%02u\r\n", dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
  else                             printf("HORLOGE  : NON REGLEE (mode CONFIG puis NOW=)\r\n");
  Ram_Print();
  printf("Boutons  : ROUGE court = mesure + SD, VERT court = cette liste\r\n");
  printf("===================================\r\n\r\n");
}

/* ============================================================================ */
/*                                MODES                                         */
/* ============================================================================ */
static void Mode_Set(SystemMode_t m)
{
  static const char *const names[4] = { "STANDARD", "CONFIGURATION", "MAINTENANCE", "ECONOMIE" };
  if (m == current_mode) return;
  current_mode = m;
  if (m == MODE_MAINTENANCE)
  {
    SD_UnmountFS();
    err_flags &= ~(ERR_SD | ERR_SD_FULL);
    printf("[SD] Carte demontee : vous pouvez la retirer\r\n");
  }
  if (m == MODE_CONFIGURATION) last_config_activity_tick = HAL_GetTick();
  if (m == MODE_STANDARD || m == MODE_ECONOMY) next_log_tick = HAL_GetTick() + FIRST_LOG_MS;
  printf("[MODE] %s\r\n", names[m]);
  lcd_vals[0] = '\0';
  Lcd_Refresh(true);
}

static uint32_t Interval_ms(void)
{
  uint32_t ms = (uint32_t)sys_cfg.log_interval * 60000UL;
  return (current_mode == MODE_ECONOMY) ? ms * 2U : ms;    /* economy: twice as long */
}

/* ============================================================================ */
/*                         SERIAL COMMANDS (CONFIG mode)                        */
/* ============================================================================ */
static char up(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c; }
static bool CiEq(const char *a, const char *b)
{
  while (*a && up(*a) == up(*b)) { a++; b++; }
  return up(*a) == up(*b);
}
static bool Num(const char **p, long *v)            /* optional sign + digits */
{
  char *end;
  long r = strtol(*p, &end, 10);
  if (end == *p) return false;
  *v = r; *p = end;
  return true;
}
static bool Sep(const char **p, char c) { if (**p != c) return false; (*p)++; return true; }
static uint8_t DaysIn(long y, long m)
{
  static const uint8_t d[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  return (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) ? 29 : d[m - 1];
}
static void Reply(bool ok) { printf(ok ? "OK\r\n" : "ERROR\r\n"); }

static void CLI_ProcessLine(char *line)
{
  static const char *const days[7] = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" };
  char *cmd, *val, *eq, *e;
  long a, b, c, h, m, s;
  const char *p;

  while (*line == ' ' || *line == '\t') line++;
  e = line + strlen(line);
  while (e > line && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = '\0';
  if (*line == '\0') return;
  last_config_activity_tick = HAL_GetTick();
  printf("> %s\r\n", line);

  if (CiEq(line, "RESET"))   { Config_RestoreDefaults(); Reply(Config_Save()); return; }
  if (CiEq(line, "VERSION")) { printf("VERSION=%s ; LOT=%s\r\n", FW_VERSION, LOT_NUMBER); return; }
  if (CiEq(line, "PARAMS"))
  {
    DateTime_t dt;
    for (size_t i = 0; i < PARAM_COUNT; i++) printf("%s=%ld\r\n", params[i].name, (long)*Cfg_Field(&sys_cfg, params[i].off));
    if (RTC_Get(&dt)) printf("DATE=%02u,%02u,20%02u ; CLOCK=%02u:%02u:%02u\r\n", dt.month, dt.day, dt.year, dt.hour, dt.min, dt.sec);
    return;
  }

  eq = strchr(line, '=');
  if (eq == NULL) { printf("ERROR: commande inconnue\r\n"); return; }
  *eq = '\0';
  cmd = line;
  e = eq;
  while (e > cmd && e[-1] == ' ') *--e = '\0';
  val = eq + 1;
  while (*val == ' ') val++;
  p = val;

  if (CiEq(cmd, "NOW"))                              /* NOW=2026-10-20 18:30:00 */
  {
    if (Num(&p, &a) && Sep(&p, '-') && Num(&p, &b) && Sep(&p, '-') && Num(&p, &c) && (Sep(&p, ' ') || Sep(&p, 'T')) &&
        Num(&p, &h) && Sep(&p, ':') && Num(&p, &m) && Sep(&p, ':') && Num(&p, &s) && *p == '\0' &&
        a >= 2000 && a <= 2099 && b >= 1 && b <= 12 && c >= 1 && c <= DaysIn(a, b) &&
        h >= 0 && h <= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59)
    {
      bool ok = RTC_SetDate((uint8_t)b, (uint8_t)c, (uint8_t)(a - 2000)) && RTC_SetTime((uint8_t)h, (uint8_t)m, (uint8_t)s);
      DateTime_t dt;
      if (ok && RTC_Get(&dt)) printf("HORLOGE = 20%02u-%02u-%02u %02u:%02u:%02u\r\n", dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
      Reply(ok);
    }
    else printf("ERROR: format NOW=AAAA-MM-JJ HH:MM:SS\r\n");
    return;
  }
  if (CiEq(cmd, "CLOCK"))                            /* CLOCK=HH:MM:SS */
  {
    if (Num(&p, &h) && Sep(&p, ':') && Num(&p, &m) && Sep(&p, ':') && Num(&p, &s) && *p == '\0' &&
        h >= 0 && h <= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59)
      Reply(RTC_SetTime((uint8_t)h, (uint8_t)m, (uint8_t)s));
    else printf("ERROR: format HH:MM:SS\r\n");
    return;
  }
  if (CiEq(cmd, "DATE"))                             /* DATE=MM,DD,YYYY */
  {
    if (Num(&p, &a) && Sep(&p, ',') && Num(&p, &b) && Sep(&p, ',') && Num(&p, &c) && *p == '\0' &&
        c >= 2000 && c <= 2099 && a >= 1 && a <= 12 && b >= 1 && b <= DaysIn(c, a))
      Reply(RTC_SetDate((uint8_t)a, (uint8_t)b, (uint8_t)(c - 2000)));
    else printf("ERROR: format MM,DD,YYYY (2000..2099)\r\n");
    return;
  }
  if (CiEq(cmd, "DAY"))                              /* DAY=MON..SUN */
  {
    for (uint8_t i = 0; i < 7; i++) if (CiEq(val, days[i])) { Reply(RTC_SetWeekday((uint8_t)(i + 1))); return; }
    printf("ERROR: DAY={MON,TUE,WED,THU,FRI,SAT,SUN}\r\n");
    return;
  }
  if (CiEq(cmd, "LOG_INTERVALL")) cmd = (char *)"LOG_INTERVAL";   /* the spec uses both spellings */

  for (size_t i = 0; i < PARAM_COUNT; i++)
  {
    SystemConfig_t tmp;
    if (!CiEq(cmd, params[i].name)) continue;
    if (!Num(&p, &a) || *p != '\0') { printf("ERROR: nombre attendu\r\n"); return; }
    if (a < params[i].min || a > params[i].max) { printf("ERROR: valeur hors limites (%ld..%ld)\r\n", (long)params[i].min, (long)params[i].max); return; }
    tmp = sys_cfg;
    *Cfg_Field(&tmp, params[i].off) = (int32_t)a;
    if (!Config_PairsOk(&tmp)) { printf("ERROR: min/max incoherents\r\n"); return; }
    sys_cfg = tmp;
    Reply(Config_Save());
    return;
  }
  printf("ERROR: parametre inconnu\r\n");
}

/* ============================================================================ */
/*                       BUTTONS (polled, debounced)                            */
/* ============================================================================ */
/* returns 1 = short press (on release), 2 = 5 s hold (once), 0 = nothing */
static uint8_t Button_Poll(Button_t *bt, uint32_t now)
{
  bool raw = (HAL_GPIO_ReadPin(bt->port, bt->pin) == GPIO_PIN_RESET);   /* to GND = pressed */
  if (raw != bt->last_raw) { bt->last_raw = raw; bt->change_tick = now; }
  if (raw != bt->stable && (now - bt->change_tick) >= DEBOUNCE_MS)
  {
    bt->stable = raw;
    if (raw) { bt->press_tick = now; bt->long_done = false; }
    else if (!bt->long_done) return 1;
  }
  if (bt->stable && !bt->long_done && (now - bt->press_tick) >= LONG_PRESS_MS)
  {
    bt->long_done = true;
    return 2;
  }
  return 0;
}

static void Buttons_Handle(uint32_t now)
{
  uint8_t r = Button_Poll(&btn_red, now), g = Button_Poll(&btn_green, now);

  if (r == 1)
  {
    printf("[BTN] ROUGE : appui court\r\n");
    if (current_mode == MODE_CONFIGURATION) printf("[BTN] (en CONFIGURATION : pas de mesure)\r\n");
    else { Sensors_Read(); Log_PerformMeasurement(); }   /* measure + save to the SD now */
  }
  if (r == 2)
  {
    printf("[BTN] ROUGE : appui 5 s\r\n");
    if (current_mode == MODE_STANDARD)         { previous_mode = MODE_STANDARD; Mode_Set(MODE_MAINTENANCE); }
    else if (current_mode == MODE_ECONOMY)     Mode_Set(MODE_STANDARD);
    else if (current_mode == MODE_MAINTENANCE) Mode_Set(previous_mode);
  }
  if (g == 1) { printf("[BTN] VERT : appui court\r\n"); Boot_Checks(); }
  if (g == 2)
  {
    printf("[BTN] VERT : appui 5 s\r\n");
    if (current_mode == MODE_STANDARD) Mode_Set(MODE_ECONOMY);
  }
}

/* ============================================================================ */
/*                          APPLICATION                                         */
/* ============================================================================ */
static void App_Init(void)
{
  uint32_t now;

  Ram_Fill();
  DWT_Init();
  USER_Driver.disk_initialize = SD_Disk_Init;
  USER_Driver.disk_status     = SD_Disk_Status;
  USER_Driver.disk_read       = SD_Disk_Read;
  USER_Driver.disk_write      = SD_Disk_Write;
  USER_Driver.disk_ioctl      = SD_Disk_Ioctl;

  printf("\r\n\r\n=== WORLDWIDE WEATHER WATCHER v%s ===\r\n", FW_VERSION);
  printf("[FEATURES] LED=%d SD=%d LUMIERE=%d TEMP/HUM=%d LCD=%d DOSSIERS=%d\r\n",
         USE_LED, USE_SD, USE_LIGHT, USE_TEMP_HUM, USE_LCD, USE_FOLDERS);
  Led_Test();
  Config_LoadOrDefault();
  RTC_Start();
#if USE_LIGHT
  (void)BH1750_Init();
#endif
#if USE_LCD
  if (Lcd_Init())
  {
    static char s[17] = "";
    Lcd_Line(0, "Weather Watcher", s);
    s[0] = '\0';
    Lcd_Line(1, "v" FW_VERSION, s);
  }
#endif

  /* RED held at start-up -> CONFIGURATION. The press is marked as already used,
     so letting go of the button does not count as a short press. */
  now = HAL_GetTick();
  btn_red.stable = btn_red.last_raw = (HAL_GPIO_ReadPin(BTN_RED_PORT, BTN_RED_PIN) == GPIO_PIN_RESET);
  btn_green.stable = btn_green.last_raw = (HAL_GPIO_ReadPin(BTN_GREEN_PORT, BTN_GREEN_PIN) == GPIO_PIN_RESET);
  btn_red.long_done = btn_green.long_done = true;
  btn_red.press_tick = btn_green.press_tick = now;
  current_mode = btn_red.stable ? MODE_CONFIGURATION : MODE_STANDARD;

  Boot_Checks();

  HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
  if (current_mode == MODE_CONFIGURATION)
  {
    last_config_activity_tick = HAL_GetTick();
    printf("[MODE] CONFIGURATION\r\n");
    printf("Commandes : NOW=AAAA-MM-JJ HH:MM:SS, VERSION, PARAMS, RESET, LOG_INTERVAL=10 ...\r\n");
  }
  else
  {
    next_log_tick = HAL_GetTick() + FIRST_LOG_MS;
    printf("[MODE] STANDARD\r\n");
  }
  Lcd_Refresh(true);
}

static void App_Run(void)
{
  uint32_t now = HAL_GetTick();

  Buttons_Handle(now);

  if (rx_line_ready)
  {
    char cmd[RX_BUFFER_SIZE];
    memcpy(cmd, rx_line_buffer, sizeof(cmd));
    rx_line_idx = 0;
    rx_line_ready = false;
    if (current_mode == MODE_CONFIGURATION) CLI_ProcessLine(cmd);
    else printf("ERROR: commandes seulement en mode CONFIGURATION (ROUGE appuye au demarrage)\r\n");
  }

  now = HAL_GetTick();
  if (current_mode == MODE_CONFIGURATION)
  {
    if ((now - last_config_activity_tick) >= CONFIG_TIMEOUT_MS)
    {
      printf("[CONFIG] 30 min sans activite : retour en STANDARD\r\n");
      Mode_Set(MODE_STANDARD);
    }
  }
  else
  {
    if (Due(now, next_read_tick))                    /* every 2 s: read + print + LCD */
    {
      next_read_tick = now + READ_PERIOD_MS;
      Sensors_Read();
      Data_Print();
    }
    if (current_mode != MODE_MAINTENANCE && Due(now, next_log_tick))   /* every LOG_INTERVAL */
    {
      next_log_tick = now + Interval_ms();
      Log_PerformMeasurement();
    }
  }

  Lcd_Refresh(false);
  Led_Update();
}

/* ============================================================================ */
/*                          INTERRUPTS / CALLBACKS                              */
/* ============================================================================ */
void USART2_IRQHandler(void) { HAL_UART_IRQHandler(&huart2); }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance != USART2) return;
  if (!rx_line_ready)
  {
    if (rx_byte == '\r' || rx_byte == '\n')
    {
      if (rx_line_idx > 0) { rx_line_buffer[rx_line_idx] = '\0'; rx_line_ready = true; }
    }
    else if (rx_line_idx < RX_BUFFER_SIZE - 1) rx_line_buffer[rx_line_idx++] = (char)rx_byte;
  }
  HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)   /* overrun etc.: restart reception */
{
  if (huart->Instance != USART2) return;
  __HAL_UART_CLEAR_OREFLAG(huart);
  HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}

/* ============================================================================ */
/*                                    MAIN                                      */
/* ============================================================================ */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  SystemCoreClockUpdate();

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_FATFS_Init();

  App_Init();

  while (1)
  {
    App_Run();
  }
}

/* ============================================================================ */
/*                       PERIPHERAL INITIALISATION                              */
/* ============================================================================ */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) Error_Handler();

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) Error_Handler();
}

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10909CEC;                    /* 100 kHz at 80 MHz */
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
  HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE);
  HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0);
}

static void MX_SPI1_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) Error_Handler();

  /* After HAL_SPI_Init (whose MSP may set other pins): SCK on D13 (PA5), MISO on D12 (PA6)
     WITH pull-up (SD cards need it), MOSI on D11 (PA7) */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
  HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);           /* the handler is USART2_IRQHandler above */
  HAL_NVIC_EnableIRQ(USART2_IRQn);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN | LED_GREEN_PIN | LED_BLUE_PIN, LED_OFF);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);           /* DHT11 idle = high */
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);  /* SD not selected */

  /* RGB LED A0 / A1 / A2 */
  GPIO_InitStruct.Pin = LED_RED_PIN | LED_GREEN_PIN | LED_BLUE_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

  /* DHT11 A3 (PB0) */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* SD chip select D10 (PB6) */
  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

  /* Buttons: plain inputs with pull-up, read by App_Run (no interrupt needed) */
  GPIO_InitStruct.Pin = BTN_RED_PIN | BTN_GREEN_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
