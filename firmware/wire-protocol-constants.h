#pragma once

#define TWI_CMD_NONE 0x00
#define TWI_CMD_VERSION 0x01
#define TWI_CMD_KEYSCAN_INTERVAL 0x02
#define TWI_CMD_LED_SET_ALL_TO 0x03
#define TWI_CMD_LED_SET_ONE_TO 0x04
#define TWI_CMD_COLS_USE_PULLUPS 0x05
#define TWI_CMD_LED_SPI_FREQUENCY 0x06
#define TWI_CMD_LED_GLOBAL_BRIGHTNESS 0x07

#define LED_SPI_FREQUENCY_4MHZ      0x07
#define LED_SPI_FREQUENCY_2MHZ      0x06
#define LED_SPI_FREQUENCY_1MHZ      0x05
#define LED_SPI_FREQUENCY_512KHZ    0x04
#define LED_SPI_FREQUENCY_256KHZ    0x03
#define LED_SPI_FREQUENCY_128KHZ    0x02
#define LED_SPI_FREQUENCY_64KHZ     0x01
#define LED_SPI_OFF                 0x00


// a 4MHz SPI update frequency is the fastest we can drive the SPI
// bus on an ATTiny88. That lets us update 32 LEDs in 1.6ms (Code compiled at -O3)
// That's a bit faster than our 400KHz max I2C speed.
#define LED_SPI_FREQUENCY_DEFAULT LED_SPI_FREQUENCY_4MHZ


#define TWI_CMD_LED_BASE 0x80

#define TWI_REPLY_NONE 0x00
#define TWI_REPLY_KEYDATA 0x01


/* Measured timings
 * Interval of 1 = 62us
 * Interval of 3=  0.1245ms
 * interval of 5 = 0.18ms
 * Interval of 7 = 0.245ms
 * Interval of 10 = 0.3415ms
 * Interval of 50 = 1.585ms
 *
 * Default to a minimum of 0.25ms or so between reads
 * That lets us take about 20 readings during a 'regular' 5ms debounce
 */
#define KEYSCAN_INTERVAL_DEFAULT 7


#define DEBOUNCE_CYCLES 20

