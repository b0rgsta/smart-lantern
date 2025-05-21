#ifndef SENSOR_CONTROLLER_H
#define SENSOR_CONTROLLER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <Adafruit_AHTX0.h>
#include <FastIMU.h>
#include <Adafruit_VL53L0X.h>
#include "Config.h"

class SensorController {
public:
    SensorController();

    bool begin();
    void update();

    // Touch sensor methods
    bool isTouched(int channel) const;
    bool isNewTouch(int channel) const;
    bool isNewRelease(int channel) const;

    // Temperature sensor methods
    float getTemperature();
    float getHumidity();

    // Gyroscope/accelerometer methods
    bool isUpsideDown() const;
    AccelData getAccelData() { return accelData; }
    GyroData getGyroData() { return gyroData; }

    // Light sensor methods
    static int getLightLevel();

    // Time of flight sensor methods
    int getDistance();

private:
    Adafruit_MPR121 touchSensor;
    Adafruit_AHTX0 tempSensor;
    BMI160 imu;
    calData calibrationData;
    AccelData accelData;
    GyroData gyroData;
    Adafruit_VL53L0X tofSensor;

    uint16_t currentTouchState;
    uint16_t previousTouchState;

    unsigned long lastTempReadTime;
    const unsigned long tempReadInterval = 20000; // 20 seconds between temp reads
    float cachedTemperature;
    float cachedHumidity;

    // Private helper functions
    void updateTouchSensor();
    void updateTemperatureSensor();
    void updateIMU();
    void updateTOF();
};

#endif // SENSOR_CONTROLLER_H