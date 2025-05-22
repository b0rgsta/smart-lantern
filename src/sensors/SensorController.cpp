#include "SensorController.h"

SensorController::SensorController() :
    currentTouchState(0),
    previousTouchState(0),
    lastTempReadTime(0),
    cachedTemperature(25.0), // Default reasonable values
    cachedHumidity(50.0),
    lastLightDebugTime(0),    // Initialize the debug timer
    tofDebugEnabled(false),   // TOF debugging starts disabled
    tofInitialized(false),    // Track if TOF sensor initialized properly
    lastTOFDebugTime(0),      // Initialize TOF debug timer
    lastValidDistance(-1),    // No valid distance yet
    consecutiveFailures(0)    // No failures yet
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

    // Initialize TOF sensor with detailed debugging
    Serial.println("Initializing VL53L0X TOF sensor...");
    if (!tofSensor.begin()) {
        Serial.println("ERROR: VL53L0X not found! Check wiring:");
        Serial.println("  - VCC to 3.3V");
        Serial.println("  - GND to GND");
        Serial.println("  - SDA to GPIO 2");
        Serial.println("  - SCL to GPIO 1");
        Serial.print("  - I2C Address should be 0x");
        Serial.println(TOF_I2C_ADDR, HEX);

        tofInitialized = false;
        allSensorsInitialized = false;
    } else {
        Serial.println("SUCCESS: VL53L0X TOF sensor initialized!");

        // Test a quick measurement to make sure it's really working
        VL53L0X_RangingMeasurementData_t measure;
        tofSensor.rangingTest(&measure, false);

        if (measure.RangeStatus != 4) {
            Serial.print("Initial TOF reading: ");
            Serial.print(measure.RangeMilliMeter);
            Serial.println(" mm");
            tofInitialized = true;
        } else {
            Serial.println("WARNING: TOF sensor responds but readings are out of range");
            tofInitialized = true; // Still consider it initialized
        }
    }

    // Initialize light sensor pin
    pinMode(LIGHT_SENSOR_PIN, INPUT);
    Serial.println("Light sensor initialized on pin " + String(LIGHT_SENSOR_PIN));

    // Print summary
    Serial.println("=== SENSOR INITIALIZATION SUMMARY ===");
    Serial.print("Touch Sensor (MPR121): "); Serial.println(touchSensor.begin(MPR121_I2C_ADDR) ? "OK" : "FAILED");
    Serial.print("Temperature Sensor (AHT10): "); Serial.println(tempSensor.begin() ? "OK" : "FAILED");
    Serial.print("Gyroscope (BMI160): "); Serial.println(err == 0 ? "OK" : "FAILED");
    Serial.print("TOF Sensor (VL53L0X): "); Serial.println(tofInitialized ? "OK" : "FAILED");
    Serial.print("Light Sensor: OK (Pin "); Serial.print(LIGHT_SENSOR_PIN); Serial.println(")");

    return allSensorsInitialized;
}

void SensorController::update() {
    // Always update touch sensor (needed for user input)
    updateTouchSensor();

    // Update IMU regularly as it's needed for lantern orientation
    updateIMU();

    // Check if it's time to update temperature
    unsigned long currentTime = millis();
    if (currentTime - lastTempReadTime >= tempReadInterval) {
        updateTemperatureSensor();
        lastTempReadTime = currentTime;
    }

    // Update TOF sensor less frequently too, every ~500ms should be plenty
    static unsigned long lastTofUpdate = 0;
    if (currentTime - lastTofUpdate >= 500) {
        updateTOF();
        lastTofUpdate = currentTime;
    }

    // Print TOF debugging info if enabled
    if (tofDebugEnabled && (currentTime - lastTOFDebugTime >= tofDebugInterval)) {
        printTOFStatus();
        lastTOFDebugTime = currentTime;
    }
}

void SensorController::updateTouchSensor() {
    previousTouchState = currentTouchState;
    currentTouchState = touchSensor.touched();
}

void SensorController::updateTemperatureSensor() {
    sensors_event_t humidity, temp;
    tempSensor.getEvent(&humidity, &temp);

    // Store values in cache
    cachedTemperature = temp.temperature;
    cachedHumidity = humidity.relative_humidity;
}

void SensorController::updateIMU() {
    imu.update();
    imu.getAccel(&accelData);
    imu.getGyro(&gyroData);
}

void SensorController::updateTOF() {
    // Only try to update if sensor was initialized properly
    if (!tofInitialized) {
        return;
    }

    // This method doesn't do much right now, but it's here for future expansion
    // The actual reading happens in getDistance() when needed
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
    // Return cached value instead of reading every time
    return cachedTemperature;
}

float SensorController::getHumidity() {
    // Return cached value instead of reading every time
    return cachedHumidity;
}

bool SensorController::isUpsideDown() const{
    // Z-axis value will be negative when upside down
    return accelData.accelZ < -0.5;
}

int SensorController::getLightLevel() {
    return analogRead(LIGHT_SENSOR_PIN);
}

int SensorController::getDistance() {
    // Return -1 immediately if sensor wasn't initialized
    if (!tofInitialized) {
        return -1;
    }

    VL53L0X_RangingMeasurementData_t measure;
    tofSensor.rangingTest(&measure, false);

    if (measure.RangeStatus != 4) {
        // Valid measurement
        lastValidDistance = measure.RangeMilliMeter;
        consecutiveFailures = 0; // Reset failure counter
        return measure.RangeMilliMeter;
    } else {
        // Invalid measurement
        consecutiveFailures++;
        return -1; // Invalid/out of range
    }
}

int SensorController::getBrightnessFromDistance() {
    int distance = getDistance();

    // If no valid reading, return -1 to indicate no hand detected
    if (distance == -1) {
        return -1;
    }

    // Convert mm to cm for easier calculation
    double distanceCm = distance / 10.0;

    // Apply your brightness mapping:
    // 0-10cm = OFF (brightness 0)
    // 10-60cm = 0-100% (linear mapping)
    // 60-80cm = 100% (full brightness)
    // 80cm+ = ignore (return -1)

    if (distanceCm <= 10.0) {
        // 0-10cm: OFF
        return 0;
    }
    if (distanceCm <= 60.0) {
        // 10-60cm: Linear mapping from 0% to 100%
        // At 10cm = 0%, at 60cm = 100%
        double brightness = (distanceCm - 10.0) / (60.0 - 10.0) * 100.0;
        return static_cast<int>(brightness);
    }
    if (distanceCm <= 80.0) {
        // 60-80cm: Full brightness (100%)
        return 100;
    }
    // 80cm+: Ignore (no hand detected)
    return -1;
}

// NEW METHOD: Check if hand is in valid detection range
bool SensorController::isHandDetected() {
    int brightness = getBrightnessFromDistance();
    // Hand is detected if we get a valid brightness value (not -1)
    return brightness != -1;
}

// NEW METHOD: Enable or disable TOF debugging
void SensorController::enableTOFDebugging(bool enable) {
    tofDebugEnabled = enable;
    if (enable) {
        Serial.println("=== TOF DEBUGGING ENABLED ===");
        Serial.println("Distance readings will be printed every second");
        Serial.println("Use this to calibrate your hand positions:");
        Serial.println("  - Hold hand at different distances");
        Serial.println("  - Note the readings for close/far positions");
        Serial.println("  - Check for consistent readings");
        Serial.println("================================");
    } else {
        Serial.println("TOF debugging disabled");
    }
}

// NEW METHOD: Print detailed TOF status and readings
void SensorController::printTOFStatus() {
    if (!tofInitialized) {
        Serial.println("TOF: SENSOR NOT INITIALIZED");
        return;
    }

    // Get a fresh reading
    int distance = getDistance();
    int brightness = getBrightnessFromDistance();

    Serial.print("TOF: ");

    if (distance == -1) {
        Serial.print("OUT OF RANGE");
        if (consecutiveFailures > 1) {
            Serial.print(" (");
            Serial.print(consecutiveFailures);
            Serial.print(" consecutive failures)");
        }
        if (lastValidDistance != -1) {
            Serial.print(" [Last valid: ");
            Serial.print(lastValidDistance);
            Serial.print("mm]");
        }
    } else {
        Serial.print(distance);
        Serial.print("mm (");
        Serial.print(distance / 10.0, 1);
        Serial.print("cm)");

        // Show brightness mapping
        if (brightness == -1) {
            Serial.print(" -> IGNORED (too far)");
        } else {
            Serial.print(" -> Brightness: ");
            Serial.print(brightness);
            Serial.print("%");

            // Give user feedback about distance ranges with brightness info
            if (brightness == 0) {
                Serial.print(" (OFF - too close)");
            } else if (brightness < 50) {
                Serial.print(" (DIM)");
            } else if (brightness < 100) {
                Serial.print(" (BRIGHT)");
            } else {
                Serial.print(" (MAX BRIGHTNESS)");
            }
        }
    }

    Serial.println();
}