#ifndef FEATURES_H
#define FEATURES_H

/* ====================================================================== */
/*  ON / OFF SWITCHES :  1 = used,  0 = ignored. Change, Build, flash.    */
/*  A part set to 0 is never touched: no check, no reading, no error.     */
/* ====================================================================== */
#define USE_LED            1   /* RGB LED on A0 / A1 / A2                         */
#define USE_SD             1   /* SD card (0 = log lines only on the serial port) */
#define USE_LIGHT          1   /* BH1750 light sensor (I2C D15/D14)               */
#define USE_TEMP_HUM       1   /* DHT11 temperature + humidity (A3)               */
#define USE_LCD            1   /* 16x2 LCD with I2C backpack (I2C D15/D14)        */

#define USE_FOLDERS        1   /* 1 = SD files in YEAR/MONTH/DAY folders,
                                  0 = all files at the root of the card           */

/* ================================ RGB LED ============================= */
/* At start-up the LED shows RED, then GREEN, then BLUE (also printed).
   If a colour is wrong, swap the pins here.                             */
#define LED_RED_PIN        GPIO_PIN_0   /* A0 = PA0 */
#define LED_GREEN_PIN      GPIO_PIN_4   /* A1 = PA1 */
#define LED_BLUE_PIN       GPIO_PIN_1   /* A2 = PA4 */
#define LED_COMMON_ANODE   0            /* 0 = LED common leg on GND, 1 = on 3.3V */

#endif /* FEATURES_H */
