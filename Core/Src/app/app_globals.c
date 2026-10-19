#include "app.h"
#include "features.h"

/* ============================== GLOBALS =============================== */
I2C_HandleTypeDef  hi2c1;
SPI_HandleTypeDef  hspi1;
UART_HandleTypeDef huart2;

volatile SystemMode_t app_mode = MODE_STANDARD;
SystemMode_t prev_mode = MODE_STANDARD;
volatile uint32_t error_flags = 0U;
volatile uint32_t app_events = 0U;
SystemConfig_t sys_cfg __attribute__((aligned(8)));
uint32_t cycles_per_us = 80U;
bool rtc_ok = false;
uint32_t pwm_phase = 0U;
uint32_t tick_div = 0U;

uint32_t next_measure_ms = 0U;
uint32_t next_maint_ms = 0U;
uint32_t config_activity_ms = 0U;
uint8_t dht_fail = 0U;
uint8_t lux_fail = 0U;
Meas_t meas;

uint8_t rx_byte = 0U;
char rx_line[RX_MAX];
volatile uint8_t rx_len = 0U;
volatile uint8_t rx_ready = 0U;
volatile uint8_t rx_overflow = 0U;

uint8_t sd_is_hc = 0U;
volatile DSTATUS sd_stat = STA_NOINIT;
FATFS sd_fs;
FIL sd_file;
bool sd_mounted = false;
uint8_t sd_fail_step = 0U;
LogEntry_t log_q[LOG_QUEUE_LEN];
uint8_t q_head = 0U, q_count = 0U;
uint32_t next_sd_retry_ms = 0U;
char lcd_vals[17] = "";
char gps_gga[GPS_LINE_MAX];
char gps_rmc[GPS_LINE_MAX];
volatile uint32_t gps_rmc_ms = 0U;
volatile uint32_t gps_last_ms = 0U;

Button_t btn_red   = { BTN_RED_PORT,   BTN_RED_PIN,   0U, 0U, 0U, 1U, 0U, EVT_RED_SHORT,   EVT_RED_LONG   };
Button_t btn_green = { BTN_GREEN_PORT, BTN_GREEN_PIN, 0U, 0U, 0U, 1U, 0U, EVT_GREEN_SHORT, EVT_GREEN_LONG };

_Static_assert((sizeof(SystemConfig_t) % 8U) == 0U, "SystemConfig_t must be a multiple of 8 bytes");

#define PARAM_ENTRY(n, f, lo, hi) { n, (uint16_t)offsetof(SystemConfig_t, f), lo, hi }
const Param_t params[PARAM_COUNT] = {
  PARAM_ENTRY("LOG_INTERVAL",  log_interval,   1, 1440),
  PARAM_ENTRY("FILE_MAX_SIZE", file_max_size,  256, 1048576),
  PARAM_ENTRY("TIMEOUT",       timeout,        1, 300),
  PARAM_ENTRY("LUMIN",         lumin,          0, 1),
  PARAM_ENTRY("LUMIN_LOW",     lumin_low,      0, 1023),
  PARAM_ENTRY("LUMIN_HIGH",    lumin_high,     0, 1023),
  PARAM_ENTRY("TEMP_AIR",      temp_air,       0, 1),
  PARAM_ENTRY("MIN_TEMP_AIR",  min_temp_air,  -40, 85),
  PARAM_ENTRY("MAX_TEMP_AIR",  max_temp_air,  -40, 85),
  PARAM_ENTRY("HYGR",          hygr,           0, 1),
  PARAM_ENTRY("HYGR_MINT",     hygr_mint,     -40, 85),
  PARAM_ENTRY("HYGR_MAXT",     hygr_maxt,     -40, 85),
  PARAM_ENTRY("PRESSURE",      pressure,       0, 1),
  PARAM_ENTRY("PRESSURE_MIN",  pressure_min,   300, 1100),
  PARAM_ENTRY("PRESSURE_MAX",  pressure_max,   300, 1100),
};
