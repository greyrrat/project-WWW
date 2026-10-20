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
/* GPS and pressure: see HAVE_GPS and HAVE_PRESSURE in app.h              */

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

void I2c_Scan(void);

#endif /* FEATURES_H */
