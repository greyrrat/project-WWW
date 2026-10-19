#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "fatfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================== SETTINGS ============================== */
#define FW_VERSION               "1.0.0"
#define LOT_NUMBER               "LOT-2026-001"

#define DEBUG_PRINT              1   /* 1 = print debug lines on the serial port            */
#define EMPTY_MAIN_LOOP          1   /* 1 = while(1) is empty, work runs from interrupts     */
                                     /* 0 = main loop calls App_Task() itself                */
#define QUICK_LOG_BUTTON         1   /* 1 = short press on RED logs a measurement right away */
#define ECONOMY_RED_TO_STANDARD  1   /* 1 = RED 5 s in ECONOMY -> STANDARD, 0 -> MAINTENANCE */
#define HAVE_PRESSURE            0   /* pressure sensor not built yet                        */

#define DEBOUNCE_MS              30U
#define LONG_PRESS_MS            5000U
#define CONFIG_TIMEOUT_MS        (30UL * 60UL * 1000UL)
#define LED_ORANGE_GREEN_ON      2U  /* orange = red + green on 2 ticks out of 5             */
#define LED_LEVEL_ON             GPIO_PIN_RESET /* swap both if your LED is common-anode    */
#define LED_LEVEL_OFF            GPIO_PIN_SET 

#if DEBUG_PRINT
#define DBG(...)  printf(__VA_ARGS__)
#else
#define DBG(...)  ((void)0)
#endif

/* ============================== HARDWARE ============================== */
#define BTN_RED_PORT     GPIOC
#define BTN_RED_PIN      GPIO_PIN_0     /* A5, wired to GND            */
#define BTN_GREEN_PORT   GPIOC
#define BTN_GREEN_PIN    GPIO_PIN_13    /* blue button of the board    */

#define LED_R_PORT       GPIOA
#define LED_R_PIN        GPIO_PIN_4
#define LED_G_PORT       GPIOA
#define LED_G_PIN        GPIO_PIN_1
#define LED_B_PORT       GPIOA
#define LED_B_PIN        GPIO_PIN_0

#define CONFIG_MAGIC       0x57575732UL
#define FLASH_CONFIG_ADDR  0x080FF800UL      /* last 2 KB page of the 1 MB flash */

#define RX_MAX             96U
#define GPS_LINE_MAX       100U
#define GPS_BAUD           9600U   /* NEO-6M / NEO-7M / NEO-M8N default */
#define LOG_LINE_MAX       180U
#define LOG_QUEUE_LEN      8U

#define SD_SPI_SLOW        SPI_BAUDRATEPRESCALER_256  /* 312 kHz: the card must start below 400 kHz */
#define SD_SPI_FAST        SPI_BAUDRATEPRESCALER_32   /* 2.5 MHz after start-up (safe with jumper wires) */
#define SD_RETRY_MS        15000U   /* lines waiting in the queue: new SD try every 15 s       */
#define LCD_REFRESH_MS     1000U    /* LCD redrawn every second                                 */
#define LCD_LIVE_MS        5000U    /* STANDARD/ECONOMY: sensors read for the LCD every 5 s     */

/* Events handled by App_Task() */
#define EVT_RED_SHORT    (1UL << 0)
#define EVT_RED_LONG     (1UL << 1)
#define EVT_GREEN_SHORT  (1UL << 2)
#define EVT_GREEN_LONG   (1UL << 3)
#define EVT_UART_LINE    (1UL << 4)
#define EVT_TICK         (1UL << 5)

/* Error flags (priority = order of the spec table) */
#define ERR_RTC          (1UL << 0)
#define ERR_GPS          (1UL << 1)
#define ERR_SENS_ACC     (1UL << 2)
#define ERR_SENS_DATA    (1UL << 3)
#define ERR_SD_FULL      (1UL << 4)
#define ERR_SD_ACC       (1UL << 5)

/* LED colours: bit0 = red, bit1 = green, bit2 = blue */
enum { C_OFF = 0, C_RED = 1, C_GREEN = 2, C_YELLOW = 3, C_BLUE = 4, C_WHITE = 7, C_ORANGE = 8 };

/* ---- DHT11 on PB0 (A3) ---- */
#define DHT_IN()      ((GPIOB->IDR & GPIO_PIN_0) != 0U)
#define DHT_LOW()     (GPIOB->BSRR = ((uint32_t)GPIO_PIN_0 << 16))
#define DHT_HIGH()    (GPIOB->BSRR = (uint32_t)GPIO_PIN_0)
#define DHT_OUTPUT()  (GPIOB->MODER = (GPIOB->MODER & ~(3UL << 0)) | (1UL << 0))
#define DHT_INPUT()   (GPIOB->MODER &= ~(3UL << 0))

/* ---- SD card commands ---- */
#define SD_CMD0    (0)
#define SD_CMD8    (8)
#define SD_CMD13   (13)
#define SD_CMD16   (16)
#define SD_CMD17   (17)
#define SD_CMD24   (24)
#define SD_CMD55   (55)
#define SD_CMD58   (58)
#define SD_ACMD41  (0x80 + 41)

/* ================================ TYPES =============================== */
typedef enum { MODE_STANDARD = 0, MODE_CONFIG, MODE_MAINTENANCE, MODE_ECONOMY } SystemMode_t;

typedef struct {
  uint32_t magic;
  int32_t  log_interval;    /* minutes            */
  int32_t  file_max_size;   /* bytes              */
  int32_t  timeout;         /* seconds            */
  int32_t  lumin;
  int32_t  lumin_low;
  int32_t  lumin_high;
  int32_t  temp_air;
  int32_t  min_temp_air;
  int32_t  max_temp_air;
  int32_t  hygr;
  int32_t  hygr_mint;
  int32_t  hygr_maxt;
  int32_t  pressure;
  int32_t  pressure_min;
  int32_t  pressure_max;
  uint32_t reserved;
  uint32_t checksum;
} SystemConfig_t;

typedef struct { uint8_t year, month, day, weekday, hour, min, sec; } DateTime_t;

typedef struct { const char *name; uint16_t off; int32_t min; int32_t max; } Param_t;

typedef struct { char tag[12]; char text[LOG_LINE_MAX]; } LogEntry_t;

/* Test switches, changed at run time with USE_xxx=0/1 (RAM only, back to features.h after a reset) */
typedef struct { bool led, sd, light, temp, lcd; } Toggles_t;

typedef struct {
  bool     active;
  uint8_t  dht_state;      /* 0 pending, 1 ok, 2 timeout, 3 disabled */
  uint8_t  lux_state;
  uint32_t start_ms;
  uint32_t next_try_ms;
  int16_t  t, h;
  uint32_t lux_x100;
} Meas_t;

typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  volatile uint32_t last_edge;
  volatile uint8_t  edge_pending;
  uint8_t  pressed;
  uint8_t  long_fired;
  uint32_t press_start;
  uint32_t evt_short;
  uint32_t evt_long;
} Button_t;

#define PARAM_COUNT  15U      /* number of entries in params[] (checked in app_globals.c) */

/* ============================== GLOBALS =============================== */
/* (defined in app_globals.c) */
extern I2C_HandleTypeDef  hi2c1;
extern SPI_HandleTypeDef  hspi1;
extern UART_HandleTypeDef huart2;

extern volatile SystemMode_t app_mode;
extern SystemMode_t prev_mode;
extern volatile uint32_t error_flags;
extern volatile uint32_t app_events;
extern SystemConfig_t sys_cfg __attribute__((aligned(8)));
extern uint32_t cycles_per_us;
extern bool rtc_ok;
extern uint32_t pwm_phase;
extern uint32_t tick_div;

extern uint32_t next_measure_ms;
extern uint32_t next_maint_ms;
extern uint32_t config_activity_ms;
extern uint8_t dht_fail;
extern uint8_t lux_fail;
extern Meas_t meas;

extern uint8_t rx_byte;
extern char rx_line[RX_MAX];
extern volatile uint8_t rx_len;
extern volatile uint8_t rx_ready;
extern volatile uint8_t rx_overflow;

extern uint8_t sd_is_hc;
extern volatile DSTATUS sd_stat;
extern FATFS sd_fs;
extern FIL sd_file;
extern bool sd_mounted;
extern uint8_t sd_fail_step;
extern LogEntry_t log_q[LOG_QUEUE_LEN];
extern uint8_t q_head, q_count;
extern uint32_t next_sd_retry_ms;
extern char gps_gga[GPS_LINE_MAX];          /* last $GPGGA / $GNGGA sentence received */
extern volatile uint32_t gps_last_ms;       /* time of the last sentence (0 = never)   */
extern Toggles_t tog;
extern char lcd_vals[17];          /* last readings shown on LCD line 2 */

extern Button_t btn_red;
extern Button_t btn_green;
extern const Param_t params[PARAM_COUNT];

/* =================== EVENTS, ERRORS, TASK WAKE-UP ===================== */
/* Tiny functions kept inline in this header (fast, and delay_us is timing critical) */
#if EMPTY_MAIN_LOOP
static inline void App_Wake(void) { NVIC_SetPendingIRQ(SWPMI1_IRQn); }   /* software interrupt */
#else
static inline void App_Wake(void) { }
#endif

static inline void Evt_Set(uint32_t e)
{
  __atomic_fetch_or(&app_events, e, __ATOMIC_SEQ_CST);
  App_Wake();
}
static inline uint32_t Evt_Take(void)
{
  return __atomic_exchange_n(&app_events, 0U, __ATOMIC_SEQ_CST);
}
static inline void Err_Set(uint32_t f)   { error_flags |= f; }
static inline void Err_Clear(uint32_t f) { error_flags &= ~f; }

static inline void delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * cycles_per_us;
  while ((DWT->CYCCNT - start) < ticks) { }
}

/* ============================ PROTOTYPES ============================== */
/* printf, microsecond delay */
int      __io_putchar(int ch);
void     DWT_Init(void);

/* RGB LED */
void     Led_Show(uint8_t color);
void     Led_Tick(uint32_t now);

/* Buttons */
void     Button_Init(Button_t *b, uint32_t now);
void     Button_Edge(uint16_t pin);
void     Button_Tick(Button_t *b, uint32_t now);

/* Parameters in flash */
uint32_t Config_Checksum(const SystemConfig_t *c);
void     Config_Defaults(SystemConfig_t *c);
bool     Config_PairsOk(const SystemConfig_t *c);
bool     Config_RangesOk(const SystemConfig_t *c);
bool     Config_Save(void);
void     Config_Load(void);

/* Internal RTC */
uint8_t  bcd2bin(uint8_t v);
uint8_t  bin2bcd(uint8_t v);
uint8_t  Weekday(uint16_t y, uint8_t m, uint8_t d);
bool     Rtc_WaitSync(void);
bool     Rtc_Write(bool set_tr, uint32_t tr, bool set_dr, uint32_t dr);
bool     Rtc_SetTime(uint8_t h, uint8_t m, uint8_t s);
bool     Rtc_SetDate(uint8_t y, uint8_t mo, uint8_t d);
bool     Rtc_SetWeekday(uint8_t wd);
bool     Rtc_Get(DateTime_t *d);
bool     Rtc_Setup(void);

/* Sensors */
int8_t   Dht_Exchange(uint8_t d[5]);
int8_t   DHT11_Read(int16_t *temperature, int16_t *humidity);
uint8_t  BH1750_Init(void);
uint8_t  BH1750_ReadLux(uint32_t *lux_x100);
uint16_t Lux_Scale(uint32_t lux);
void     Gps_Init(void);
bool     Gps_ParseCoord(const char *s, char hemi, char *out, size_t n);
int8_t   GPS_Read(char *out, size_t n);   /* 1 = position, 0 = no fix yet, -1 = GPS not answering */
#if HAVE_PRESSURE
bool     Pressure_Read(int32_t *hpa);
#endif

/* SD card (SPI1) */
uint8_t  spi_rw(uint8_t tx);
void     Spi_SetPrescaler(uint32_t presc);
void     sd_deselect(void);
uint8_t  sd_wait_ready(uint32_t ms);
uint8_t  sd_select(void);
uint8_t  sd_cmd(uint8_t cmd, uint32_t arg);
DSTATUS  SD_Disk_Init(BYTE pdrv);
DSTATUS  SD_Disk_Status(BYTE pdrv);
DRESULT  SD_Disk_Read(BYTE pdrv, BYTE *buf, DWORD sector, UINT count);
DRESULT  SD_Disk_Write(BYTE pdrv, const BYTE *buf, DWORD sector, UINT count);
DRESULT  SD_Disk_Ioctl(BYTE pdrv, BYTE cmd, void *buff);
bool     SD_Mount(void);
void     SD_Unmount(void);
void     SD_Fail(uint32_t flag);
void     SD_Rotate(const char *tag, const char *path0);
bool     SD_AppendEntry(const LogEntry_t *e);
void     Q_Push(const char *tag, const char *text);
void     SD_Flush(void);
void     SD_LogLine(const char *tag, const char *text);
bool     SD_SelfTest(void);

/* Measurement */
uint32_t Interval_ms(void);
void     Measure_Start(uint32_t now);
void     Measure_Finish(void);
void     Measure_Step(void);
void     Maintenance_Print(void);

/* Serial commands (CLI) */
char    *Trim(char *s);
bool     CiEq(const char *a, const char *b);
bool     ParseNum(const char **p, long *out);
bool     ParseSep(const char **p, char c);
bool     ParseLong(const char *s, long *out);
uint8_t  DaysInMonth(long y, long m);
void     Cli_Reply(bool ok);
void     Cli_PrintParams(void);
void     Cli_Execute(char *line);
bool     Cli_Test(char *line);
void     Tog_Print(void);

/* Modes / task */
void     Mode_Set(SystemMode_t m);
void     Mode_OnRedLong(void);
void     Mode_OnGreenLong(void);
void     App_Task(void);
void     App_Tick1ms(void);
void     Tick_Init(void);
void     App_Init(void);
void     Reset_CausePrint(void);
void     Watchdog_Init(void);
static inline void Watchdog_Kick(void) { IWDG->KR = 0xAAAAU; }

/* Interrupt handlers */
void     EXTI0_IRQHandler(void);
void     USART2_IRQHandler(void);
void     TIM6_DAC_IRQHandler(void);
void     USART1_IRQHandler(void);
#if EMPTY_MAIN_LOOP
void     SWPMI1_IRQHandler(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* APP_H */
