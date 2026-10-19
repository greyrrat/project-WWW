#ifndef FEATURES_H
#define FEATURES_H

#include "app.h"

/* ====================================================================== */
/*  ENABLE / DISABLE LIST :  1 = used,  0 = ignored                       */
/*  Change a number, rebuild, flash. Nothing else to touch.               */
/* ====================================================================== */
#define USE_LED            1   /* RGB status LED                           */
#define USE_SD             1   /* SD card logging                          */
#define USE_LIGHT          1   /* BH1750 light sensor                      */
#define USE_TEMP_HUM       1   /* DHT11 temperature + humidity             */
#define USE_LCD            1   /* 16x2 I2C LCD                             */
#define USE_BUTTONS        1   /* RED and GREEN buttons                    */
#define I2C_SCAN_AT_BOOT   1   /* print every I2C address found at start   */
#define USE_WATCHDOG       1   /* restarts the board if the program gets stuck (16 s) */
#define SD_TEST_AT_BOOT    1   /* write + read back SDTEST.TXT at start, prints OK / ERROR */
#define USE_GPS            1   /* GPS on USART1: GPS TX -> PA10 (D2), GPS RX -> PA9 (D8) */
/* Pressure: see HAVE_PRESSURE in app.h                                   */

/* ================================ MODES =============================== */
/* STANDARD always exists. 0 = the button that opens this mode does nothing */
#define USE_MODE_CONFIG    1   /* RED held at power-up -> CONFIGURATION    */
#define USE_MODE_MAINT     1   /* RED 5 s -> MAINTENANCE                   */
#define USE_MODE_ECO       1   /* GREEN 5 s -> ECONOMY                     */

/* ============================== LCD (I2C) ============================= */
extern uint8_t lcd_addr;       /* found automatically: 0x27 or 0x3F        */
extern bool    lcd_ready;

void Lcd_Expander(uint8_t data);
void Lcd_WriteByte(uint8_t value, uint8_t rs);
bool Lcd_Init(void);
void Lcd_Clear(void);
void Lcd_SetCursor(uint8_t col, uint8_t row);
void Lcd_Print(const char *s);
void Lcd_ShowLine(uint8_t row, const char *text);   /* writes one padded 16-char line */
void Lcd_LiveRead(void);                            /* live values for the LCD (not logged) */
void Lcd_Refresh(bool force);                       /* redraws mode, clock, readings (every 1 s) */

void I2c_Scan(void);

#endif /* FEATURES_H */
