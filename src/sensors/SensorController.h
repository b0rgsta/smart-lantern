#ifndef SENSOR_CONTROLLER_H
#define SENSOR_CONTROLLER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <Adafruit_AHTX0.h>
#include <FastIMU.h>
#include <Adafruit_VL53L0X.h>
#include "Config.h"

// FreeRTOS includes for dual-core operation
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

class SensorController {
public:
    SensorController();

    bool begin();
    void update();  // This will now just check if data needs updating
    void stopSensorTask();  // Clean shutdown of sensor task

    // Touch sensor methods
    bool isTouched(int channel) const;
    bool isNewTouch(int channel) const;
    bool isNewRelease(int channel) const;

    // Temperature sensor methods - now thread-safe
    float getTemperature();
    float getHumidity();

    // Gyroscope/accelerometer methods - now thread-safe
    bool isUpsideDown() const;
    AccelData getAccelData();
    GyroData getGyroData();

    // Light sensor methods
    static int getLightLevel();

    // Time of flight sensor methods - now thread-safe
    int getDistance();

    // TOF brightness control methods - now thread-safe
    int getBrightnessFromDistance();  // Returns brightness 0-100 based on distance
    bool isHandDetected();           // Returns true if hand is in valid range

    // TOF debugging methods
    void enableTOFDebugging(bool enable = true);
    void printTOFStatus();
    bool isTOFWorking() const { return tofInitialized; }

private:
    // Hardware sensor objects
    Adafruit_MPR121 touchSensor;
    Adafruit_AHTX0 tempSensor;
    BMI160 imu;
    calData calibrationData;
    Adafruit_VL53L0X tofSensor;

    // === SHARED DATA (protected by mutexes) ===
    // Touch sensor data
    uint16_t currentTouchState;
    uint16_t previousTouchState;
    mutable SemaphoreHandle_t touchMutex;    // Protects touch data (mutable for const functions)

    // Temperature/humidity data
    float cachedTemperature;
    float cachedHumidity;
    mutable SemaphoreHandle_t tempMutex;     // Protects temperature data (mutable for const functions)

    // IMU data
    AccelData accelData;
    GyroData gyroData;
    mutable SemaphoreHandle_t imuMutex;      // Protects IMU data (mutable for const functions)

    // TOF data
    int lastValidDistance;
    int consecutiveFailures;
    mutable SemaphoreHandle_t tofMutex;      // Protects TOF data (mutable for const functions)

    // === CORE 0 TASK MANAGEMENT ===
    TaskHandle_t sensorTaskHandle;   // Handle to the sensor task running on core 0
    bool sensorTaskRunning;          // Flag to control task execution
    static void sensorTaskWrapper(void* parameter);  // Static wrapper for FreeRTOS
    void sensorTaskFunction();       // The actual sensor processing function

    // === TIMING VARIABLES (used only in sensor task) ===
    unsigned long lastTempReadTime;
    const unsigned long tempReadInterval = 20000; // 20 seconds between temp reads
    unsigned long lastLightDebugTime;

    // TOF debugging variables
    bool tofDebugEnabled;
    bool tofInitialized;
    unsigned long lastTOFDebugTime;
    const unsigned long tofDebugInterval = 1000; // Print TOF debug every 1 second

    // === PRIVATE HELPER FUNCTIONS (run on core 0) ===
    void updateTouchSensor();
    void updateTemperatureSensor();
    void updateIMU();
    void updateTOF();

    // Thread-safe helper functions for accessing shared data
    bool takeMutex(SemaphoreHandle_t mutex, TickType_t timeout = portMAX_DELAY) const;
    void giveMutex(SemaphoreHandle_t mutex) const;
};

#endif // SENSOR_CONTROLLER_H