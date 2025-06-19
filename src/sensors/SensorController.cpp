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

bool SensorController::initializeMPR121WithRetries() {
    const int MAX_RETRIES = 5;           // Try up to 5 times
    const int RETRY_DELAY_MS = 500;      // Wait 500ms between retries
    const int STABILIZATION_DELAY_MS = 100; // Wait for sensor to stabilize after init

    Serial.println("=== INITIALIZING MPR121 TOUCH SENSOR ===");

    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        Serial.print("MPR121 initialization attempt ");
        Serial.print(attempt);
        Serial.print(" of ");
        Serial.println(MAX_RETRIES);

        // Small delay before each attempt (except first)
        if (attempt > 1) {
            delay(RETRY_DELAY_MS);
        }

        // Try to initialize the sensor
        if (touchSensor.begin(MPR121_I2C_ADDR)) {
            Serial.println("✓ MPR121 hardware initialization successful");

            // Give the sensor time to stabilize
            delay(STABILIZATION_DELAY_MS);

            // Verify the sensor is actually working by reading touch states
            uint16_t testTouch = touchSensor.touched();
            Serial.print("Initial touch state reading: 0x");
            Serial.println(testTouch, HEX);

            // Configure sensor sensitivity (optional - helps with reliability)
            configureMPR121Sensitivity();

            // Do a few test reads to ensure stability
            bool stable = true;
            for (int i = 0; i < 3; i++) {
                uint16_t touch1 = touchSensor.touched();
                delay(10);
                uint16_t touch2 = touchSensor.touched();

                // Check if readings are consistent (allowing for natural touch changes)
                if (touch1 == 0xFFFF || touch2 == 0xFFFF) {
                    Serial.println("WARNING: Sensor returning invalid data");
                    stable = false;
                    break;
                }
            }

            if (stable) {
                Serial.println("✓ MPR121 touch sensor initialized and verified!");
                return true;
            } else {
                Serial.println("✗ MPR121 sensor unstable, retrying...");
            }
        } else {
            Serial.print("✗ MPR121 initialization failed on attempt ");
            Serial.println(attempt);
        }
    }

    Serial.println("ERROR: MPR121 failed to initialize after all retries!");
    Serial.println("Check wiring:");
    Serial.println("  - VCC to 3.3V");
    Serial.println("  - GND to GND");
    Serial.println("  - SDA to GPIO 2");
    Serial.println("  - SCL to GPIO 1");
    Serial.print("  - I2C Address should be 0x");
    Serial.println(MPR121_I2C_ADDR, HEX);

    return false;
}

void SensorController::configureMPR121Sensitivity() {
    // Configure touch and release thresholds for better reliability
    // Lower touch threshold = more sensitive
    // Higher release threshold = less likely to false trigger

    for (uint8_t channel = 0; channel < 12; channel++) {
        // The Adafruit MPR121 library only takes 2 parameters: touch and release
        touchSensor.setThresholds(12,  // Touch threshold (lower = more sensitive)
                                 6);   // Release threshold (should be lower than touch)
    }

    Serial.println("✓ MPR121 sensitivity configured");
}

bool SensorController::recoverMPR121() {
    Serial.println("Attempting MPR121 recovery...");

    // Try a simple re-initialization
    if (touchSensor.begin(MPR121_I2C_ADDR)) {
        delay(100); // Stabilization delay
        configureMPR121Sensitivity();

        // Test if it's working
        uint16_t testTouch = touchSensor.touched();
        if (testTouch != 0xFFFF) {
            Serial.println("✓ MPR121 recovery successful");
            return true;
        }
    }

    Serial.println("✗ MPR121 recovery failed");
    return false;
}

bool SensorController::begin() {
    bool allSensorsInitialized = true;
    bool criticalSensorsOK = true;  // Track only critical sensors

    Serial.println("=== INITIALIZING SENSORS ===");

    // Initialize I2C bus with a small delay to ensure stability
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    delay(100); // Give I2C bus time to stabilize

    // Initialize MPR121 touch sensor (CRITICAL) - using robust method
    if (!initializeMPR121WithRetries()) {
        Serial.println("CRITICAL: MPR121 touch sensor failed to initialize!");
        allSensorsInitialized = false;
        criticalSensorsOK = false;  // Touch sensor is critical
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
        Serial.println("ERROR: VL53L0X not found!");
        Serial.println("Check wiring:");
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
    Serial.println(criticalSensorsOK ? "OK" : "FAILED");
    Serial.print("Temperature Sensor (AHT10): ");
    Serial.println(tempSensor.begin() ? "OK" : "FAILED");
    Serial.print("Gyroscope (BMI160): ");
    Serial.println(err == 0 ? "OK" : "FAILED");
    Serial.print("TOF Sensor (VL53L0X): ");
    Serial.println(tofInitialized ? "OK" : "FAILED");
    Serial.print("Light Sensor: OK (Pin ");
    Serial.print(LIGHT_SENSOR_PIN);
    Serial.println(")");

    // Start the sensor task only if critical sensors are working
    if (criticalSensorsOK) {
        Serial.println("Starting sensor task on core 0...");
        sensorTaskRunning = true;

        // Create the sensor task on core 0
        BaseType_t result = xTaskCreatePinnedToCore(
            sensorTaskWrapper,      // Function to run
            "SensorTask",           // Task name
            4096,                   // Stack size (bytes)
            this,                   // Parameter to pass to function
            1,                      // Task priority (1 = low priority)
            &sensorTaskHandle,      // Task handle
            0                       // Core 0 (core 1 runs main loop)
        );

        if (result != pdPASS) {
            Serial.println("ERROR: Failed to create sensor task!");
            sensorTaskRunning = false;
            criticalSensorsOK = false;
        } else {
            Serial.println("✓ Sensor task started successfully on core 0");
        }
    } else {
        Serial.println("ERROR: Critical sensors failed - cannot start sensor task!");
    }

    return criticalSensorsOK;  // Return true if critical sensors are working
}

// Static wrapper function required by FreeRTOS
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
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay = 100Hz update rate
    }

    // Clean up when task is ending
    Serial.println("Sensor task ending on core 0");
    vTaskDelete(NULL); // Delete this task
}

void SensorController::update() {
    // This function is called from core 1 (main loop)
    // All actual sensor processing happens on core 0
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

bool SensorController::isTouched(int channel) const {
    bool result = false;
    if (takeMutex(touchMutex, pdMS_TO_TICKS(10))) {
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

    if (distanceCm < 10.0) {
        return -1; // Below 10cm: Ignore (too close)
    }
    if (distanceCm <= 50.0) {
        // 10-50cm: Linear mapping from 0% to 100%
        // At 10cm = 0% brightness, at 50cm = 100% brightness
        double brightness = (distanceCm - 10.0) / (50.0 - 10.0) * 100.0;
        return static_cast<int>(brightness);
    }
    return -1; // 50cm+: Ignore (no hand detected)
}
bool SensorController::isHandDetected() {
    int brightness = getBrightnessFromDistance();
    return brightness != -1; // Hand is detected if we get a valid brightness value
}

int SensorController::getLightLevel() {
    return analogRead(LIGHT_SENSOR_PIN);
}

void SensorController::enableTOFDebugging(bool enable) {
    tofDebugEnabled = enable;
    if (enable) {
        Serial.println("TOF debugging enabled");
    } else {
        Serial.println("TOF debugging disabled");
    }
}

void SensorController::printTOFStatus() {
    if (!tofInitialized) {
        Serial.println("TOF: Not initialized");
        return;
    }

    int distance = getDistance();
    int brightness = getBrightnessFromDistance();
    bool handDetected = isHandDetected();

    Serial.print("TOF: Distance=");
    Serial.print(distance);
    Serial.print("mm, Brightness=");
    Serial.print(brightness);
    Serial.print("%, Hand=");
    Serial.println(handDetected ? "YES" : "NO");
}

// === SENSOR UPDATE FUNCTIONS (run on core 0) ===

void SensorController::updateTouchSensor() {
    static unsigned long lastSuccessfulRead = 0;
    static int consecutiveFailures = 0;
    const int MAX_CONSECUTIVE_FAILURES = 5;
    const unsigned long RECOVERY_TIMEOUT_MS = 30000; // 30 seconds

    if (takeMutex(touchMutex, pdMS_TO_TICKS(10))) {
        // Store previous state before updating
        previousTouchState = currentTouchState;

        // Read current touch state
        uint16_t rawTouch = touchSensor.touched();

        // Check for invalid readings (sensor malfunction indicator)
        if (rawTouch == 0xFFFF) {
            consecutiveFailures++;
            Serial.print("MPR121 read failure #");
            Serial.println(consecutiveFailures);

            // If too many failures, attempt recovery
            if (consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) {
                Serial.println("Too many MPR121 failures, attempting recovery...");

                if (recoverMPR121()) {
                    consecutiveFailures = 0;
                    lastSuccessfulRead = millis();
                } else {
                    // If recovery fails, wait before trying again
                    delay(1000);
                }
            }
        } else {
            // Successful read
            currentTouchState = rawTouch;
            consecutiveFailures = 0;
            lastSuccessfulRead = millis();
        }

        giveMutex(touchMutex);
    }

    // Additional safeguard: if no successful reads for too long, force recovery
    unsigned long timeSinceLastSuccess = millis() - lastSuccessfulRead;
    if (timeSinceLastSuccess > RECOVERY_TIMEOUT_MS) {
        Serial.println("MPR121 timeout - forcing recovery attempt");
        recoverMPR121();
        lastSuccessfulRead = millis(); // Reset timer regardless of success
    }
}

void SensorController::updateTemperatureSensor() {
    sensors_event_t humidity, temp;
    tempSensor.getEvent(&humidity, &temp);

    if (takeMutex(tempMutex, pdMS_TO_TICKS(5))) {
        cachedTemperature = temp.temperature;
        cachedHumidity = humidity.relative_humidity;
        giveMutex(tempMutex);
    }
}

void SensorController::updateIMU() {
    imu.update();

    if (takeMutex(imuMutex, pdMS_TO_TICKS(5))) {
        imu.getAccel(&accelData);
        imu.getGyro(&gyroData);
        giveMutex(imuMutex);
    }
}

void SensorController::updateTOF() {
    if (!tofInitialized) {
        return; // Don't try to read if sensor isn't working
    }

    VL53L0X_RangingMeasurementData_t measure;
    tofSensor.rangingTest(&measure, false);

    if (takeMutex(tofMutex, pdMS_TO_TICKS(5))) {
        if (measure.RangeStatus != 4) { // Status 4 means out of range
            lastValidDistance = measure.RangeMilliMeter;
            consecutiveFailures = 0;
        } else {
            consecutiveFailures++;
            // If too many consecutive failures, invalidate distance
            if (consecutiveFailures > 10) {
                lastValidDistance = -1;
            }
        }
        giveMutex(tofMutex);
    }
}

// === HELPER FUNCTIONS ===

bool SensorController::takeMutex(SemaphoreHandle_t mutex, TickType_t timeout) const {
    if (mutex == nullptr) {
        return false;
    }
    return xSemaphoreTake(mutex, timeout) == pdTRUE;
}

void SensorController::giveMutex(SemaphoreHandle_t mutex) const {
    if (mutex != nullptr) {
        xSemaphoreGive(mutex);
    }
}