#include "SensorController.h"

SensorController::SensorController() : currentTouchState(0),
                                       previousTouchState(0)
// Initialize LED handler with the touch sensor's address
{
    // Initialize calibration data to zero
    calibrationData = {0};
}

bool SensorController::begin() {
    bool allSensorsInitialized = true;

    // Initialize MPR121 touch sensor
    if (!touchSensor.begin(MPR121_I2C_ADDR)) {
        Serial.println("MPR121 not found, check wiring!");
        allSensorsInitialized = false;
    } else {
        Serial.println("MPR121 touch sensor initialized");
    }

    // Initialize AHT10 temperature sensor
    if (!tempSensor.begin()) {
        Serial.println("AHT10 not found, check wiring!");
        allSensorsInitialized = false;
    } else {
        Serial.println("AHT10 temperature sensor initialized");
    }

    // Initialize BMI160 via FastIMU
    int err = imu.init(calibrationData, BMI160_I2C_ADDR);
    if (err != 0) {
        Serial.print("BMI160 FastIMU init error: ");
        Serial.println(err);
        allSensorsInitialized = false;
    } else {
        Serial.println("BMI160 gyroscope initialized via FastIMU");
    }

    // Initialize TOF sensor
    if (!tofSensor.begin()) {
        Serial.println("VL53L0X not found, check wiring!");
        allSensorsInitialized = false;
    } else {
        Serial.println("VL53L0X TOF sensor initialized");
    }

    // Initialize light sensor pin
    pinMode(LIGHT_SENSOR_PIN, INPUT);

    return allSensorsInitialized;
}

void SensorController::update() {
    updateTouchSensor();
    updateTemperatureSensor();
    updateIMU();
    updateTOF();
}

void SensorController::updateTouchSensor() {
    previousTouchState = currentTouchState;
    currentTouchState = touchSensor.touched();
}

void SensorController::updateTemperatureSensor() {
    sensors_event_t humidity, temp;
    tempSensor.getEvent(&humidity, &temp);
    // Data is now available in the events
}

void SensorController::updateIMU() {
    imu.update();
    imu.getAccel(&accelData);
    imu.getGyro(&gyroData);
}

void SensorController::updateTOF() {
    // Update TOF sensor data if needed
    // This might not need to be called every cycle due to timing constraints
}

bool SensorController::isTouched(int channel) const {
    return (currentTouchState & (1 << channel));
}

bool SensorController::isNewTouch(int channel) const {
    return (currentTouchState & (1 << channel)) && !(previousTouchState & (1 << channel));
}

bool SensorController::isNewRelease(int channel) const {
    return !(currentTouchState & (1 << channel)) && (previousTouchState & (1 << channel));
}

float SensorController::getTemperature() {
    sensors_event_t humidity, temp;
    tempSensor.getEvent(&humidity, &temp);
    return temp.temperature;
}

float SensorController::getHumidity() {
    sensors_event_t humidity, temp;
    tempSensor.getEvent(&humidity, &temp);
    return humidity.relative_humidity;
}

bool SensorController::isUpsideDown() const{
    // Z-axis value will be negative when upside down
    return accelData.accelZ < -0.5;
}

int SensorController::getLightLevel() {
    return analogRead(LIGHT_SENSOR_PIN);
}

int SensorController::getDistance() {
    VL53L0X_RangingMeasurementData_t measure;
    tofSensor.rangingTest(&measure, false);

    if (measure.RangeStatus != 4) {
        // If not out of range
        return measure.RangeMilliMeter;
    }
    return -1; // Invalid/out of range
}