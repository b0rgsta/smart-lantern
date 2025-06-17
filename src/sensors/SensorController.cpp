#include "SensorController.h"

SensorController::SensorController() :
    currentTouchState(0),
    previousTouchState(0),
    cachedTemperature(25.0), // Default reasonable values
    cachedHumidity(50.0),
    lastValidDistance(-1),    // No valid distance yet
    consecutiveFailures(0),   // No failures yet
    sensorTaskHandle(nullptr),
    sensorTaskRunning(false),
    lastTempReadTime(0),
    lastLightDebugTime(0),
    tofDebugEnabled(false),   // TOF debugging starts disabled
    tofInitialized(false),    // Track if TOF sensor initialized properly
    lastTOFDebugTime(0)       // Initialize TOF debug timer
{
    // Initialize calibration data to zero
    calibrationData = {0};

    // Create mutexes for thread-safe access to shared data
    touchMutex = xSemaphoreCreateMutex();
    tempMutex = xSemaphoreCreateMutex();
    imuMutex = xSemaphoreCreateMutex();
    tofMutex = xSemaphoreCreateMutex();

    // Check if mutex creation was successful
    if (!touchMutex || !tempMutex || !imuMutex || !tofMutex) {
        Serial.println("ERROR: Failed to create sensor mutexes!");
    }
}

bool SensorController::begin() {
    bool allSensorsInitialized = true;
    bool criticalSensorsOK = true;  // Track only critical sensors

    Serial.println("=== INITIALIZING SENSORS ===");

    // Initialize MPR121 touch sensor (CRITICAL)
    if (!touchSensor.begin(MPR121_I2C_ADDR)) {
        Serial.println("MPR121 not found, check wiring!");
        allSensorsInitialized = false;
        criticalSensorsOK = false;  // Touch sensor is critical
    } else {
        Serial.println("✓ MPR121 touch sensor initialized");
    }

    // Initialize AHT10 temperature sensor (NON-CRITICAL)
    if (!tempSensor.begin()) {
        Serial.println("AHT10 not found, check wiring!");
        allSensorsInitialized = false;
        // NOT setting criticalSensorsOK = false because temp sensor is optional
    } else {
        Serial.println("✓ AHT10 temperature sensor initialized");
    }

    // Initialize BMI160 via FastIMU (NON-CRITICAL)
    int err = imu.init(calibrationData, BMI160_I2C_ADDR);
    if (err != 0) {
        Serial.print("BMI160 FastIMU init error: ");
        Serial.println(err);
        allSensorsInitialized = false;
        // NOT setting criticalSensorsOK = false because IMU is optional
    } else {
        Serial.println("✓ BMI160 gyroscope initialized via FastIMU");
    }

    // Initialize TOF sensor (NON-CRITICAL)
    Serial.println("Initializing VL53L0X TOF sensor...");
    if (!tofSensor.begin()) {
        Serial.println("ERROR: VL53L0X not found! Check wiring:");
        Serial.println("  - VCC to 3.3V");
        Serial.println("  - GND to GND");
        Serial.println("  - SDA to GPIO 2");
        Serial.println("  - SCL to GPIO 1");
        Serial.print("  - I2C Address should be 0x");
        Serial.println(TOF_I2C_ADDR, HEX);
        Serial.println("NOTE: System will continue without TOF sensor");

        tofInitialized = false;
        allSensorsInitialized = false;
        // NOT setting criticalSensorsOK = false because TOF is optional
    } else {
        Serial.println("✓ VL53L0X TOF sensor initialized!");

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
    Serial.println("✓ Light sensor initialized on pin " + String(LIGHT_SENSOR_PIN));

    // Print summary
    Serial.println("=== SENSOR INITIALIZATION SUMMARY ===");
    Serial.print("Touch Sensor (MPR121): ");
    Serial.println(touchSensor.begin(MPR121_I2C_ADDR) ? "OK" : "FAILED");
    Serial.print("Temperature Sensor (AHT10): ");
    Serial.println(tempSensor.begin() ? "OK" : "FAILED");
    Serial.print("Gyroscope (BMI160): ");
    Serial.println(err == 0 ? "OK" : "FAILED");
    Serial.print("TOF Sensor (VL53L0X): ");
    Serial.println(tofInitialized ? "OK" : "FAILED");
    Serial.print("Light Sensor: OK (Pin ");
    Serial.print(LIGHT_SENSOR_PIN);
    Serial.println(")");

    // Start sensor task if critical sensors are OK (touch sensor)
    // We don't need ALL sensors to work, just the critical ones
    if (criticalSensorsOK) {
        Serial.println("=== STARTING DUAL-CORE OPERATION ===");
        Serial.println("Core 1: LED effects and main logic");
        Serial.println("Core 0: Sensor processing task");

        if (!allSensorsInitialized) {
            Serial.println("NOTE: Some non-critical sensors failed, but system will continue");
        }

        // Create the sensor task and pin it to core 0
        BaseType_t taskCreated = xTaskCreatePinnedToCore(
            sensorTaskWrapper,      // Function to run
            "SensorTask",           // Task name for debugging
            4096,                   // Stack size (4KB should be plenty)
            this,                   // Parameter to pass (this object)
            1,                      // Priority (1 = low priority, good for background tasks)
            &sensorTaskHandle,      // Task handle
            0                       // Pin to core 0 (core 1 runs main loop)
        );

        if (taskCreated == pdPASS) {
            sensorTaskRunning = true;
            Serial.println("✓ Sensor task created successfully on core 0");
        } else {
            Serial.println("ERROR: Failed to create sensor task!");
            criticalSensorsOK = false;
        }
    } else {
        Serial.println("ERROR: Critical sensors failed - cannot start sensor task!");
    }

    return criticalSensorsOK;  // Return true if critical sensors are working
}

// Static wrapper function required by FreeRTOS
// This just calls the actual member function
void SensorController::sensorTaskWrapper(void* parameter) {
    // Cast the parameter back to SensorController object
    SensorController* sensor = static_cast<SensorController*>(parameter);

    // Call the actual sensor processing function
    sensor->sensorTaskFunction();
}

// This function runs continuously on core 0
void SensorController::sensorTaskFunction() {
    Serial.println("Sensor task started on core 0");

    // Main sensor processing loop
    while (sensorTaskRunning) {
        // Get current time for timing comparisons
        unsigned long currentTime = millis();

        // Always update touch sensor (needed for user input)
        updateTouchSensor();

        // Update IMU regularly as it's needed for lantern orientation
        updateIMU();

        // Check if it's time to update temperature (every 20 seconds)
        if (currentTime - lastTempReadTime >= tempReadInterval) {
            updateTemperatureSensor();
            lastTempReadTime = currentTime;
        }

        // Update TOF sensor every 500ms (plenty fast for hand detection)
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

        // Small delay to prevent task from hogging the CPU
        // This also allows other tasks on core 0 to run if needed
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay = 100Hz update rate
    }

    // Clean up when task is ending
    Serial.println("Sensor task ending on core 0");
    vTaskDelete(NULL); // Delete this task
}

// The main update function now just checks if the sensor task is running
// All actual sensor processing happens on core 0
void SensorController::update() {
    // This function is called from core 1 (main loop)
    // But all the actual sensor processing happens on core 0
    // So we don't need to do anything here except maybe check task health

    // Optional: Check if sensor task is still running
    if (sensorTaskHandle && eTaskGetState(sensorTaskHandle) == eDeleted) {
        Serial.println("WARNING: Sensor task has stopped unexpectedly!");
        sensorTaskRunning = false;
    }
}

void SensorController::stopSensorTask() {
    if (sensorTaskRunning && sensorTaskHandle) {
        Serial.println("Stopping sensor task...");
        sensorTaskRunning = false;

        // Wait a moment for the task to finish cleanly
        vTaskDelay(pdMS_TO_TICKS(100));

        // If task hasn't stopped by itself, force delete it
        if (eTaskGetState(sensorTaskHandle) != eDeleted) {
            vTaskDelete(sensorTaskHandle);
        }

        sensorTaskHandle = nullptr;
        Serial.println("Sensor task stopped");
    }
}

// === THREAD-SAFE ACCESS FUNCTIONS ===
// These functions are called from core 1 but access data updated on core 0

bool SensorController::isTouched(int channel) const {
    bool result = false;
    if (takeMutex(touchMutex, pdMS_TO_TICKS(10))) { // 10ms timeout
        result = (currentTouchState & (1 << channel));
        giveMutex(touchMutex);
    }
    return result;
}

bool SensorController::isNewTouch(int channel) const {
    bool result = false;
    if (takeMutex(touchMutex, pdMS_TO_TICKS(10))) {
        result = (currentTouchState & (1 << channel)) && !(previousTouchState & (1 << channel));
        giveMutex(touchMutex);
    }
    return result;
}

bool SensorController::isNewRelease(int channel) const {
    bool result = false;
    if (takeMutex(touchMutex, pdMS_TO_TICKS(10))) {
        result = !(currentTouchState & (1 << channel)) && (previousTouchState & (1 << channel));
        giveMutex(touchMutex);
    }
    return result;
}

float SensorController::getTemperature() {
    float result = 25.0; // Default fallback
    if (takeMutex(tempMutex, pdMS_TO_TICKS(10))) {
        result = cachedTemperature;
        giveMutex(tempMutex);
    }
    return result;
}

float SensorController::getHumidity() {
    float result = 50.0; // Default fallback
    if (takeMutex(tempMutex, pdMS_TO_TICKS(10))) {
        result = cachedHumidity;
        giveMutex(tempMutex);
    }
    return result;
}

bool SensorController::isUpsideDown() const {
    bool result = false;
    if (takeMutex(imuMutex, pdMS_TO_TICKS(10))) {
        // Z-axis value will be negative when upside down
        result = accelData.accelZ < -0.5;
        giveMutex(imuMutex);
    }
    return result;
}

AccelData SensorController::getAccelData() {
    AccelData result = {0}; // Default fallback
    if (takeMutex(imuMutex, pdMS_TO_TICKS(10))) {
        result = accelData;
        giveMutex(imuMutex);
    }
    return result;
}

GyroData SensorController::getGyroData() {
    GyroData result = {0}; // Default fallback
    if (takeMutex(imuMutex, pdMS_TO_TICKS(10))) {
        result = gyroData;
        giveMutex(imuMutex);
    }
    return result;
}

int SensorController::getDistance() {
    int result = -1; // Default fallback
    if (takeMutex(tofMutex, pdMS_TO_TICKS(10))) {
        result = lastValidDistance;
        giveMutex(tofMutex);
    }
    return result;
}

int SensorController::getBrightnessFromDistance() {
    int distance = getDistance(); // This already uses mutex protection

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

bool SensorController::isHandDetected() {
    int brightness = getBrightnessFromDistance();
    // Hand is detected if we get a valid brightness value (not -1)
    return brightness != -1;
}

// Static function - doesn't need mutex protection since it's a direct hardware read
int SensorController::getLightLevel() {
    return analogRead(LIGHT_SENSOR_PIN);
}

// === SENSOR UPDATE FUNCTIONS (run on core 0) ===

void SensorController::updateTouchSensor() {
    if (takeMutex(touchMutex, pdMS_TO_TICKS(5))) {
        previousTouchState = currentTouchState;
        currentTouchState = touchSensor.touched();
        giveMutex(touchMutex);
    }
}

void SensorController::updateTemperatureSensor() {
    sensors_event_t humidity, temp;
    tempSensor.getEvent(&humidity, &temp);

    if (takeMutex(tempMutex, pdMS_TO_TICKS(5))) {
        // Store values in cache
        cachedTemperature = temp.temperature;
        cachedHumidity = humidity.relative_humidity;
        giveMutex(tempMutex);
    }
}

void SensorController::updateIMU() {
    imu.update();

    AccelData newAccelData;
    GyroData newGyroData;
    imu.getAccel(&newAccelData);
    imu.getGyro(&newGyroData);

    if (takeMutex(imuMutex, pdMS_TO_TICKS(5))) {
        accelData = newAccelData;
        gyroData = newGyroData;
        giveMutex(imuMutex);
    }
}

void SensorController::updateTOF() {
    // Only try to update if sensor was initialized properly
    if (!tofInitialized) {
        return;
    }

    VL53L0X_RangingMeasurementData_t measure;
    tofSensor.rangingTest(&measure, false);

    if (takeMutex(tofMutex, pdMS_TO_TICKS(5))) {
        if (measure.RangeStatus != 4) {
            // Valid measurement
            lastValidDistance = measure.RangeMilliMeter;
            consecutiveFailures = 0; // Reset failure counter
        } else {
            // Invalid measurement
            consecutiveFailures++;
            // Keep lastValidDistance as is, don't overwrite with invalid data
        }
        giveMutex(tofMutex);
    }
}

// === TOF DEBUGGING FUNCTIONS ===

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

void SensorController::printTOFStatus() {
    if (!tofInitialized) {
        Serial.println("TOF: SENSOR NOT INITIALIZED");
        return;
    }

    // Get thread-safe copies of the data
    int distance = -1;
    int failures = 0;

    if (takeMutex(tofMutex, pdMS_TO_TICKS(10))) {
        distance = lastValidDistance;
        failures = consecutiveFailures;
        giveMutex(tofMutex);
    }

    int brightness = getBrightnessFromDistance();

    Serial.print("TOF: ");

    if (distance == -1) {
        Serial.print("OUT OF RANGE");
        if (failures > 1) {
            Serial.print(" (");
            Serial.print(failures);
            Serial.print(" consecutive failures)");
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

// === MUTEX HELPER FUNCTIONS ===

bool SensorController::takeMutex(SemaphoreHandle_t mutex, TickType_t timeout) const {
    return xSemaphoreTake(mutex, timeout) == pdTRUE;
}

void SensorController::giveMutex(SemaphoreHandle_t mutex) const {
    xSemaphoreGive(mutex);
}