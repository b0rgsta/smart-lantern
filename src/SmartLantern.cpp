// src/SmartLantern.cpp

#include "SmartLantern.h"

#include "leds/effects/StartupEffect.h"
#include "leds/effects/TrailsEffect.h"
#include "leds/effects/RainbowEffect.h"
#include "leds/effects/FireEffect.h"
#include "leds/effects/MatrixEffect.h"
#include "leds/effects/AcceleratingTrailsEffect.h"
#include "leds/effects/SolidColorEffect.h"
#include "leds/effects/GradientEffect.h"

SmartLantern::SmartLantern() : isPowerOn(false),
                               isAutoOn(false),
                               currentMode(MODE_AMBIENT),
                               currentEffect(0),
                               tempButtonState(0),
                               lightButtonState(0),
                               powerButtonPressTime(0),
                               lowLightStartTime(0),
                               autoOnTime(0) {
    // Initialize the effects vector structure
    effects.resize(5); // One vector for each mode (0-4)

    // Create special effects that need direct access
    startupEffect = new StartupEffect(leds);
    fireEffectPtr = new FireEffect(leds);

    // Call helper to initialize all effects
    initializeEffects();
}

SmartLantern::~SmartLantern() {
    // Clean up all effects
    for (auto &modeEffects: effects) {
        for (auto effect: modeEffects) {
            delete effect;
        }
    }
    // The fireEffectPtr is just a reference, it's already deleted in the loop above
}

void SmartLantern::initializeEffects() {
    // Create the effect instances
    auto startupEffect = new StartupEffect(leds);
    auto trailsEffect = new TrailsEffect(leds);
    auto rainbowEffect = new RainbowEffect(leds);
    auto fireEffect = new FireEffect(leds);
    auto matrixEffect = new MatrixEffect(leds);
    auto acceleratingTrailsEffect = new AcceleratingTrailsEffect(leds);

    // Store a reference to the fire effect for temperature override
    fireEffectPtr = fireEffect;

    // Solid color effects for ambient mode
    auto coolWhiteEffect = new SolidColorEffect(
        leds,
        SolidColorEffect::COLOR_NONE, // Core
        SolidColorEffect::COLD_WHITE, // Inner (off)
        SolidColorEffect::COLD_WHITE, // Outer (off)
        SolidColorEffect::COLOR_NONE // Ring
    );

    auto whiteEffect = new SolidColorEffect(
        leds,
        SolidColorEffect::COLOR_NONE, // Core
        SolidColorEffect::NATURAL_WHITE, // Inner (off)
        SolidColorEffect::NATURAL_WHITE, // Outer (off)
        SolidColorEffect::COLOR_NONE // Ring
    );

    auto warmWhiteEffect = new SolidColorEffect(
        leds,
        SolidColorEffect::COLOR_NONE, // Core
        SolidColorEffect::WARM_WHITE, // Inner (off)
        SolidColorEffect::WARM_WHITE, // Outer (off)
        SolidColorEffect::COLOR_NONE // Ring
        );

    effects[MODE_AMBIENT].push_back(coolWhiteEffect); // Cool white
    effects[MODE_AMBIENT].push_back(whiteEffect); // Pure white
    effects[MODE_AMBIENT].push_back(warmWhiteEffect); // Warm white

    // Gradient effects for gradient mode
    // 1. Purple-Blue opposing gradients (inner purple→blue, outer blue→purple, others off)
    auto purpleBlueOpposingGradient = new GradientEffect(
        leds,
        Gradient(), // Core off
        GradientEffect::createPurpleToBlueGradient(),
        GradientEffect::createBlueToPurpleGradient(),
        Gradient()  // Ring off
    );

    // 2. Rainbow but flipped directions for inner and outer
    auto rainbowGradient = GradientEffect::createRainbowGradient();
    auto reversedRainbowGradient = GradientEffect::reverseGradient(rainbowGradient);
    auto opposingRainbowGradient = new GradientEffect(
        leds,
        Gradient(), // Core off
        rainbowGradient,
        reversedRainbowGradient,
        Gradient()  // Ring off
    );

    // 3. Sunset gradient on all strips
    auto sunsetGradient = new GradientEffect(
        leds,
        Gradient(),
        GradientEffect::createSunsetGradient(),
        GradientEffect::reverseGradient(GradientEffect::createSunsetGradient()),
        Gradient()
    );

    // 4. Easter gradient on all strips
    auto easterGradient = new GradientEffect(
        leds,
        Gradient(),
        GradientEffect::createEasterGradient(),
        GradientEffect::reverseGradient(GradientEffect::createEasterGradient()),
        Gradient()

    );

    // 5. Christmas gradient on all strips
    auto christmasGradient = new GradientEffect(
        leds,
        Gradient(),
        GradientEffect::createChristmasGradient(),
        GradientEffect::reverseGradient(GradientEffect::createChristmasGradient()),
        Gradient()
    );

    effects[MODE_GRADIENT].push_back(purpleBlueOpposingGradient);
    effects[MODE_GRADIENT].push_back(opposingRainbowGradient);
    effects[MODE_GRADIENT].push_back(sunsetGradient);
    effects[MODE_GRADIENT].push_back(easterGradient);
    effects[MODE_GRADIENT].push_back(christmasGradient);

    // MODE_ANIMATED
    effects[MODE_ANIMATED].push_back(fireEffect); // Fire effect
    effects[MODE_ANIMATED].push_back(trailsEffect); // Trails effect
    effects[MODE_ANIMATED].push_back(matrixEffect); // Matrix effect
    effects[MODE_ANIMATED].push_back(acceleratingTrailsEffect); // Accelerating trails
    effects[MODE_ANIMATED].push_back(rainbowEffect); // Rainbow effect

    // MODE_PARTY
    effects[MODE_PARTY].push_back(rainbowEffect); // Rainbow
    effects[MODE_PARTY].push_back(trailsEffect); // Trails
    effects[MODE_PARTY].push_back(matrixEffect); // Matrix
    effects[MODE_PARTY].push_back(acceleratingTrailsEffect); // Accelerating trails
}

void SmartLantern::begin() {
    Serial.println("Smart Lantern Initializing...");

    // Initialize I2C bus
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // Initialize LED controller
    leds.begin();

    // Initialize sensors
    if (!sensors.begin()) {
        Serial.println("WARNING: Some sensors failed to initialize");
    }

    // Initialize preferences for persistent storage
    preferences.begin("lantern", false); // "lantern" is the namespace

    // Load saved settings with sensible defaults
    LanternMode savedMode = (LanternMode) preferences.getUChar("mode", MODE_AMBIENT);
    int savedEffect = preferences.getUChar("effect", 0);
    tempButtonState = preferences.getUChar("tempBtn", 0);
    lightButtonState = preferences.getUChar("lightBtn", 0);

    // Start with power off until startup completes
    isPowerOn = false;

    // Show startup animation first
    this->startupEffect->reset(); // Reset the startup effect first
    while (!this->startupEffect->isComplete()) {
        this->startupEffect->update();
        delay(20);
    }

    // Now set the power on and restore previous mode/effect (or defaults)
    isPowerOn = true;

    // Set mode and effect (default to Ambient white if no valid saved data)
    if (savedMode >= MODE_OFF && savedMode <= MODE_PARTY) {
        currentMode = savedMode;
    } else {
        currentMode = MODE_AMBIENT; // Default to Ambient
    }

    if (savedEffect >= 0 && savedEffect <= 4) {
        currentEffect = savedEffect;
    } else {
        currentEffect = 0; // Default to first effect (white for Ambient mode)
    }

    Serial.println("Smart Lantern Ready!");
    Serial.print("Restored mode: ");
    Serial.print(currentMode);
    Serial.print(", effect: ");
    Serial.println(currentEffect);
}

void SmartLantern::update() {
    // Update sensors
    sensors.update();

    // Process user inputs
    processTouchInputs();

    // Handle auto on/off based on light sensor
    handleAutoLighting();

    // Update the current effect
    updateEffects();
}

void SmartLantern::setMode(LanternMode mode) {
    if (mode != currentMode) {
        currentMode = mode;
        currentEffect = 0; // Reset effect when mode changes

        // Save to persistent storage
        preferences.putUChar("mode", currentMode);
        preferences.putUChar("effect", currentEffect);

        String modeNames[] = {"OFF", "AMBIENT", "GRADIENT", "ANIMATED", "PARTY"};
        Serial.println("Mode changed to: " + modeNames[currentMode]);
    }
}

void SmartLantern::nextEffect() {
    // Effects are mode-dependent, so number will vary
    // For simplicity, we'll cycle through 0-4 for all modes
    unsigned int numOfEffects = effects[currentMode].size();
    currentEffect = (currentEffect + 1) % numOfEffects;

    // Save effect to persistent storage
    preferences.putUChar("effect", currentEffect);

    Serial.println("Effect changed to: " + effects[currentMode][currentEffect]->getName());
}

void SmartLantern::nextMode() {
    // Cycle through modes 1-4 (skipping MODE_OFF)
    currentMode = static_cast<LanternMode>((currentMode % (effects.size() - 1)) + 1);
    currentEffect = 0; // Reset effect when mode changes

    // Save mode and effect to persistent storage
    preferences.putUChar("mode", currentMode);
    preferences.putUChar("effect", currentEffect);

    String modeNames[] = {"OFF", "AMBIENT", "GRADIENT", "ANIMATED", "PARTY"};
    Serial.println("Mode changed to: " + modeNames[currentMode]);
    Serial.println("Effect reset to: " + effects[currentMode][currentEffect]->getName());
}

void SmartLantern::setPower(bool on) {
    if (on != isPowerOn) {
        isPowerOn = on;

        if (isPowerOn) {
            // Turn on - set to default mode
            currentMode = MODE_ANIMATED;
            leds.setBrightness(77); // 30% brightness
        } else {
            // Turn off
            currentMode = MODE_OFF;
            leds.clearAll();
        }

        Serial.print("Power turned ");
        Serial.println(isPowerOn ? "ON" : "OFF");
    }
}

void SmartLantern::togglePower() {
    setPower(!isPowerOn);
}

void SmartLantern::updateEffects() {
    // Don't update effects if power is off
    if (!isPowerOn && !isAutoOn) return;

    // Check for temperature override
    float currentTemp = sensors.getTemperature();
    if (tempButtonState > 0 && currentTemp <= TEMP_THRESHOLD_RED) {
        // Find the fire effect in MODE_ANIMATED
        for (auto effect: effects[MODE_ANIMATED]) {
            auto fireEffect = (FireEffect *) (effect);
            if (fireEffect) {
                fireEffect->update();
                return;
            }
        }
    }

    // If powered and in a valid mode with effects
    if (currentMode != MODE_OFF && effects[currentMode].size() > 0) {
        // Use the current effect for this mode
        // Ensure currentEffect is within bounds
        if (currentEffect < 0 || currentEffect >= effects[currentMode].size()) {
            currentEffect = 0;
        }

        // Update the current effect
        effects[currentMode][currentEffect]->update();
    } else {
        // All LEDs off for MODE_OFF
        leds.clearAll();
    }
}

void SmartLantern::processTouchInputs() {
    // Check for temperature button
    if (sensors.isNewTouch(TEMP_BUTTON_CHANNEL)) {
        tempButtonState = (tempButtonState + 1) % 4; // Cycle through states 0-3

        // Save temperature button state
        preferences.putUChar("tempBtn", tempButtonState);

        Serial.print("Temperature button state: ");
        Serial.println(tempButtonState);
    }

    // Check for light sensitivity button
    if (sensors.isNewTouch(LIGHT_BUTTON_CHANNEL)) {
        lightButtonState = (lightButtonState + 1) % 4; // Cycle through states 0-3

        // Save light sensitivity button state
        preferences.putUChar("lightBtn", lightButtonState);

        Serial.print("Light sensitivity state: ");
        Serial.println(lightButtonState);
    }


    // Check for power button press
    if (sensors.isNewTouch(POWER_BUTTON_CHANNEL)) {
        powerButtonPressTime = millis(); // Record press time for long-press detection
    }

    // Check for power button release
    if (sensors.isNewRelease(POWER_BUTTON_CHANNEL)) {
        unsigned long pressDuration = millis() - powerButtonPressTime;

        if (pressDuration >= POWER_BUTTON_HOLD_TIME) {
            // Long press - toggle power
            togglePower();
        } else {
            // Short press - turn on if off
            if (!isPowerOn) {
                setPower(true);
            }
        }
    }

    // Check for mode button
    if (sensors.isNewTouch(MODE_BUTTON_CHANNEL) && isPowerOn) {
        nextMode();
    }

    // Check for effect button
    if (sensors.isNewTouch(EFFECT_BUTTON_CHANNEL) && isPowerOn) {
        nextEffect();
    }
}

void SmartLantern::handleAutoLighting() {
    int lightLevel = sensors.getLightLevel();
    bool isLowLight = false;

    // Determine light threshold based on lightButtonState
    switch (lightButtonState) {
        case 1: // High sensitivity
            isLowLight = lightLevel < LIGHT_THRESHOLD_HIGH;
            break;
        case 2: // Medium sensitivity
            isLowLight = lightLevel < LIGHT_THRESHOLD_MEDIUM;
            break;
        case 3: // Low sensitivity
            isLowLight = lightLevel < LIGHT_THRESHOLD_LOW;
            break;
        default: // Off
            isLowLight = false;
    }

    // Handle low light state
    if (isLowLight) {
        if (lowLightStartTime == 0) {
            // Start the timer
            lowLightStartTime = millis();
        } else if (millis() - lowLightStartTime > LIGHT_THRESHOLD_TIME && !isAutoOn) {
            // Timer elapsed, turn on automatically
            isAutoOn = true;
            autoOnTime = millis();
            currentMode = MODE_AMBIENT;
            currentEffect = 0;

            Serial.println("Auto-on triggered due to low light");
        }
    } else {
        // Reset the timer if light level is high
        lowLightStartTime = 0;
    }

    // Handle auto-off timer
    if (isAutoOn) {
        unsigned long elapsedTime = millis() - autoOnTime;

        if (elapsedTime > AUTO_OFF_TIME) {
            // Auto-off time reached
            isAutoOn = false;
            currentMode = MODE_OFF;

            Serial.println("Auto-off triggered");
        } else if (elapsedTime > DIMMING_START_TIME) {
            // Dimming phase
            unsigned long dimTime = elapsedTime - DIMMING_START_TIME;
            uint8_t brightness = 77 - (dimTime * 77 / DIMMING_DURATION);
            leds.setBrightness(brightness);
        }
    }
}
