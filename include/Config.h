//
// Created by Andrew Borg on 20/5/2025.
//

#ifndef CONFIG_H
#define CONFIG_H

// Pin Definitions
#define LED_STRIP_CORE_PIN    7  // Connected to ESP32 GPIO 7
#define LED_STRIP_INNER_PIN   6  // Connected to ESP32 GPIO 6
#define LED_STRIP_OUTER_PIN   4  // Connected to ESP32 GPIO 4
#define LED_STRIP_RING_PIN    5  // Connected to ESP32 GPIO 5
#define LIGHT_SENSOR_PIN      3  // Connected to ESP32 GPIO 3

// I2C Pins (for sensors)
#define I2C_SDA_PIN           2  // Connected to ESP32 GPIO 2
#define I2C_SCL_PIN           1  // Connected to ESP32 GPIO 1

// MPR121 Touch Sensor Channel Definitions
#define TEMP_BUTTON_CHANNEL   0  // Temperature button
#define LIGHT_BUTTON_CHANNEL  1  // Light sensor button
#define POWER_BUTTON_CHANNEL  2  // Power button
#define MODE_BUTTON_CHANNEL   3  // Mode button
#define EFFECT_BUTTON_CHANNEL 4  // Effect button

// LED Strip Definitions
#define NUM_INNER_STRIPS       3     // Number of inner strip sections
#define NUM_OUTER_STRIPS       3     // Number of outer strip sections
#define INNER_LEDS_PER_STRIP   28    // 28 LEDs per inner strip section
#define OUTER_LEDS_PER_STRIP   24    // 24 LEDs per outer strip section

// Total LED counts (calculated)
#define LED_STRIP_CORE_COUNT   142    // Number of LEDs in core strip
#define LED_STRIP_INNER_COUNT  (INNER_LEDS_PER_STRIP * NUM_INNER_STRIPS)  // 84 total inner LEDs
#define LED_STRIP_OUTER_COUNT  (OUTER_LEDS_PER_STRIP * NUM_OUTER_STRIPS)  // 72 total outer LEDs
#define LED_STRIP_RING_COUNT   62    // Number of LEDs in ring strip

// Timing Parameters (in milliseconds)
#define POWER_BUTTON_HOLD_TIME    2000  // 2 seconds to turn off
#define LIGHT_THRESHOLD_TIME      3000  // 3 seconds before auto-on
#define AUTO_OFF_TIME             5000  // 4 hours auto-off time
#define DIMMING_START_TIME        3500  // 3.5 hours (when dimming starts)
#define DIMMING_DURATION          1500  // 30 minutes dimming duration

// Temperature Thresholds (in Celsius)
#define TEMP_THRESHOLD_RED        18.0  // Red LED on at 18°C or below
#define TEMP_THRESHOLD_RED_BLUE   10.0  // Red and Blue LEDs on at 10°C or below
#define TEMP_THRESHOLD_BLUE       5.0   // Blue LED on at 5°C or below

// Light Sensor Thresholds
#define LIGHT_THRESHOLD_HIGH      20   // High sensitivity threshold (very dark)
#define LIGHT_THRESHOLD_MEDIUM    150  // Medium sensitivity threshold (dim)
#define LIGHT_THRESHOLD_LOW       600  // Low sensitivity threshold (moderate)

// TOF Sensor Parameters
#define TOF_MIN_DISTANCE          50   // Minimum distance in mm
#define TOF_MAX_DISTANCE          300  // Maximum distance in mm

// I2C Sensor Addresses
#define MPR121_I2C_ADDR           0x5A  // MPR121 touch sensor
#define AHT10_I2C_ADDR            0x38  // AHT10 temperature/humidity sensor (auto-detected)
#define BMI160_I2C_ADDR           0x68  // BMI160 gyroscope/accelerometer
#define TOF_I2C_ADDR              0x29  // VL53L0X time-of-flight sensor

// Color definitions with names
#define COLOR_RED     0xFF0000  // Pure Red
#define COLOR_GREEN   0x00FF00  // Pure Green
#define COLOR_BLUE    0x0000FF  // Pure Blue
#define COLOR_YELLOW  0xFFFF00  // Yellow
#define COLOR_MAGENTA 0xFF00FF  // Magenta/Purple
#define COLOR_CYAN    0x00FFFF  // Cyan
#define COLOR_WHITE   0xFFFFFF  // White
#define COLOR_ORANGE  0xFF8000  // Orange
#define COLOR_PINK    0xFF0080  // Pink
#define COLOR_LIME    0x80FF00  // Lime
// Note: COLOR_NONE is defined in effect files where needed

#endif //CONFIG_H