#ifndef SETTINGS_H
#define SETTINGS_H

// LCD temperature value in Celsius or Fahrenheit
// 0 - Celsius
// 1 - Fahrenheit
#define CONF_LCD_TEMP_C_OR_F 0

// Advertisement temperature value in Celsius or Fahrenheit
// 0 - Celsius
// 1 - Fahrenheit
#define CONF_ADV_TEMP_C_OR_F 0

// Blinking smiley - toggle LCD smiley face on each iteration
// 0 - Not blinking
// 1 - Blinking
#define CONF_LCD_BLINKING_SMILEY 0

// Show confort smiley
// 0 - Do not show the comfort smiley
// 1 - Comfort smiley functionality similar to the stock FW
#define CONF_LCD_SHOW_COMFORT_SMILEY 0

// Show battery level on LCD on every other iteration
// 0 - Do not show battery level; indicator always shows humidity
// 1 - Alternate humidity and battery levels on LCD
#define CONF_LCD_BATTERY_INDICATOR 0

// Advertising format
// ADV_FORMAT_CUSTOM_ATC - custom format known from ATC firmware
// ADV_FORMAT_MI - stock Mi firmware format
#define ADV_FORMAT_CUSTOM_ATC 1
#define ADV_FORMAT_MI 2
#define CONF_ADV_FORMAT ADV_FORMAT_CUSTOM_ATC

// Advertisement interval - number of main loop iterations between
// advertisement payload updates
#define CONF_ADV_ITERATIONS 6

// Measurement interval - number of main loop iterations between sensor
// measurements
#define CONF_MEASUREMENT_ITERATIONS 10

// Temperature and humidity offsets - values that will be added to the sensor
// measurements. Use to callibrate the sensors if needed.
// Units: temperature: 0.1C; humidity: 0.1%.
#define CONF_TEMP_OFFSET 0
#define CONF_HUMI_OFFSET 0

// Instant advertisement payload update thresholds. If the sensor value delta
// is larger than this, the measurement triggers instant payload update.
// Units: temperature: 0.1C; humidity: 0.1%.
#define CONF_TEMP_ALARM 5
#define CONF_HUMI_ALARM 5

// BLE advertisement interval as given to the Telink BLE stack
#define CONF_ADV_INTERVAL 3000

#endif
