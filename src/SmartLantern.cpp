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
#include "leds/effects/PartyRippleEffect.h"
#include "leds/effects/WaterfallEffect.h"
#include "leds/effects/CoreGrowEffect.h"
#include "leds/effects/TechnoOrangeEffect.h"
#include "leds/effects/RainbowTranceEffect.h"
#include "leds/effects/PartyFireEffect.h"


SmartLantern::SmartLantern() : isPowerOn(false),
                               isAutoOn(false),
                               currentMode(MODE_AMBIENT),
                               currentEffect(0),
                               tempButtonState(0),
                               lightButtonState(0),
                               powerButtonPressTime(0),
                               lowLightStartTime(0),
                               autoOnTime(0),
                               isWindingDown(false), // Initialize wind-down state
                               windDownPosition(0), // Start wind-down position
                               lastWindDownTime(0), // Initialize timing
                               lastModeChangeTime(0), // Initialize mode debounce timing
                               lastEffectChangeTime(0), // Initialize effect debounce timing
                               buttonFeedback(leds) {
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
    // Create two rainbow effects with different parameters
    // First one: all strips enabled (for party mode)
    auto rainbowEffect = new RainbowEffect(leds);

    // Second one: core and ring disabled (for animated mode)
    auto rainbowEffectNoCore = new RainbowEffect(leds,
        false,  // core disabled
        true,   // inner enabled
        true,   // outer enabled
        false   // ring disabled
    );
    auto fireEffect = new FireEffect(leds);
    auto matrixEffect = new MatrixEffect(leds);
    auto acceleratingTrailsEffect = new AcceleratingTrailsEffect(leds);
    auto partyRippleEffect = new PartyRippleEffect(leds);
    auto waterfallEffect = new WaterfallEffect(leds);
    auto coreGrowEffect = new CoreGrowEffect(leds);
    auto technoOrangeEffect = new TechnoOrangeEffect(leds);
    auto rainbowTranceEffect = new RainbowTranceEffect(leds);
    auto partyFireEffect = new PartyFireEffect(leds);


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
        Gradient() // Ring off
    );

    // 2. Rainbow but flipped directions for inner and outer
    auto rainbowGradient = GradientEffect::createRainbowGradient();
    auto reversedRainbowGradient = GradientEffect::reverseGradient(rainbowGradient);
    auto opposingRainbowGradient = new GradientEffect(
        leds,
        Gradient(),
        rainbowGradient,
        reversedRainbowGradient,
        Gradient() // Ring off
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
    auto bavariaGradient = new GradientEffect(
        leds,
        Gradient(),
        GradientEffect::createBlueToWhiteGradient(),
        GradientEffect::reverseGradient(GradientEffect::createBlueToWhiteGradient()),
        Gradient()

    );

    // 5. Christmas gradient on all strips
    auto christmasGradient = new GradientEffect(
        leds,
        GradientEffect::createInnerChristmasGradient(),
        Gradient(),
        (GradientEffect::createOuterChristmasGradient()),
        Gradient()
    );

    effects[MODE_GRADIENT].push_back(purpleBlueOpposingGradient);
    effects[MODE_GRADIENT].push_back(opposingRainbowGradient);
    effects[MODE_GRADIENT].push_back(sunsetGradient);
    effects[MODE_GRADIENT].push_back(christmasGradient);

    // MODE_ANIMATED
    effects[MODE_ANIMATED].push_back(fireEffect); // Fire effect
    effects[MODE_ANIMATED].push_back(waterfallEffect); // Waterfall effect
    effects[MODE_ANIMATED].push_back(matrixEffect); // Matrix effect
    effects[MODE_ANIMATED].push_back(rainbowEffectNoCore); // Rainbow effect


    // MODE_PARTY
    effects[MODE_PARTY].push_back(rainbowEffect); // Rainbow
    effects[MODE_PARTY].push_back(coreGrowEffect); // Core ripple
    effects[MODE_PARTY].push_back(matrixEffect); // Matrix
    effects[MODE_PARTY].push_back(technoOrangeEffect);
    effects[MODE_PARTY].push_back(rainbowTranceEffect); // Rainbow Trance
    effects[MODE_PARTY].push_back(partyFireEffect); // Party Fire
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

    // Enable TOF debugging for calibration
    // sensors.enableTOFDebugging(true);

    // Initialize preferences for persistent storage
    preferences.begin("lantern", false); // "lantern" is the namespace

    // Load saved settings with sensible defaults
    auto savedMode = static_cast<LanternMode>(preferences.getUChar("mode", MODE_AMBIENT));
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

    // Update button feedback handler
    buttonFeedback.update();

    // Update brightness based on TOF sensor (only when powered on)
    if (isPowerOn) {
        updateBrightnessFromTOF();
    }

    // Process user inputs
    processTouchInputs();

    // Handle auto on/off based on light sensor
    handleAutoLighting();

    // If we're in wind-down mode, handle that instead of normal effects
    if (isWindingDown) {
        updateWindDown();
    } else {
        // Update the current effect normally
        updateEffects();
    }
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
        if (on) {
            // Turning ON - restore previous mode and effect (don't override them)
            isPowerOn = true;
            isWindingDown = false; // Make sure wind-down is off

            // Load saved settings with sensible defaults
            auto savedMode = static_cast<LanternMode>(preferences.getUChar("mode", MODE_AMBIENT));
            int savedEffect = preferences.getUChar("effect", 0);
            tempButtonState = preferences.getUChar("tempBtn", 0);
            lightButtonState = preferences.getUChar("lightBtn", 0);

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

            Serial.print("Power turned ON - restored to mode: ");
            Serial.print(currentMode);
            Serial.print(", effect: ");
            Serial.println(currentEffect);
        } else {
            // Turning OFF - start wind-down animation
            startWindDown();
            Serial.println("Starting power wind-down sequence");
        }
    }
}

void SmartLantern::togglePower() {
    setPower(!isPowerOn);
}

void SmartLantern::updateEffects() {
    // Don't update effects if power is off
    if (!isPowerOn && !isAutoOn) return;

    // Tell the current effect whether to skip ring updates
    if (currentMode != MODE_OFF && effects[currentMode].size() > 0) {
        // Ensure currentEffect is within bounds
        if (currentEffect < 0 || currentEffect >= effects[currentMode].size()) {
            currentEffect = 0;
        }

        // Tell effect to skip ring if button feedback is active
        effects[currentMode][currentEffect]->setSkipRing(buttonFeedback.isFeedbackActive());

        // Check for temperature override
        float currentTemp = sensors.getTemperature();
        if (tempButtonState > 0 && currentTemp <= TEMP_THRESHOLD_RED) {
            // Find the fire effect in MODE_ANIMATED
            for (auto effect: effects[MODE_ANIMATED]) {
                auto fireEffect = (FireEffect *) (effect);
                if (fireEffect) {
                    fireEffect->setSkipRing(buttonFeedback.isFeedbackActive());
                    fireEffect->update();
                    return;
                }
            }
        }

        // Update the current effect
        effects[currentMode][currentEffect]->update();
    } else {
        // All LEDs off for MODE_OFF
        leds.clearAll();
    }
}

void SmartLantern::processTouchInputs() {
    // Static variable to track if we've already toggled during this button press
    static bool hasToggled = false;

    // Check for temperature button
    if (sensors.isNewTouch(TEMP_BUTTON_CHANNEL)) {
        tempButtonState = (tempButtonState + 1) % 4; // Cycle through states 0-3

        // Save temperature button state
        preferences.putUChar("tempBtn", tempButtonState);

        // Show visual feedback on ring LEDs
        buttonFeedback.showTemperatureState(tempButtonState);

        Serial.print("Temperature button state: ");
        Serial.println(tempButtonState);
    }

    if (sensors.isNewTouch(LIGHT_BUTTON_CHANNEL)) {
        lightButtonState = (lightButtonState + 1) % 4; // Cycle through states 0-3

        // Save light sensitivity button state
        preferences.putUChar("lightBtn", lightButtonState);

        // Show visual feedback on ring LEDs
        buttonFeedback.showLightState(lightButtonState);

        // Debug output with clear state names
        String stateNames[] = {"OFF", "LOW", "MEDIUM", "HIGH"};
        Serial.print("Light sensitivity changed to: ");
        Serial.print(stateNames[lightButtonState]);
        Serial.print(" (state ");
        Serial.print(lightButtonState);
        Serial.println(")");

        // Reset timer when sensitivity changes
        lowLightStartTime = 0;
        Serial.println("Light sensor timer reset due to sensitivity change");
    }

    // Check for power button press (start of touch)
    if (sensors.isNewTouch(POWER_BUTTON_CHANNEL)) {
        powerButtonPressTime = millis(); // Record press time for hold detection
        hasToggled = false; // Reset toggle flag for new press

        // If lantern is currently OFF, turn it ON with a simple touch
        if (!isPowerOn) {
            setPower(true); // Turn on immediately
            hasToggled = true; // Mark that we've already acted on this press
            Serial.println("Power button touched - turning ON");
        } else {
            // If lantern is ON, we need to wait for a hold to turn it OFF
            Serial.println("Power button pressed - starting hold timer for OFF");
        }
    }

    // Check if power button is currently being held (only matters if lantern is ON)
    if (sensors.isTouched(POWER_BUTTON_CHANNEL) && isPowerOn) {
        unsigned long currentHoldTime = millis() - powerButtonPressTime;

        // Check if we've reached the 2-second threshold for turning OFF
        if (currentHoldTime >= POWER_BUTTON_HOLD_TIME && !hasToggled) {
            setPower(false); // Turn off after 2-second hold
            hasToggled = true; // Prevent multiple toggles during same hold
            Serial.println("Power button held for 2 seconds - turning OFF");
        }
    }

    // Check for power button release (end of touch)
    if (sensors.isNewRelease(POWER_BUTTON_CHANNEL)) {
        unsigned long pressDuration = millis() - powerButtonPressTime;

        // If released before 2 seconds while lantern was ON and haven't toggled, do nothing
        if (pressDuration < POWER_BUTTON_HOLD_TIME && !hasToggled && isPowerOn) {
            Serial.println("Power button released early while ON - no action (need to hold to turn OFF)");
        }

        // Note: hasToggled will be reset on next button press
    }

    // Check for mode button (only works when powered on)
    if (sensors.isNewTouch(MODE_BUTTON_CHANNEL) && isPowerOn) {
        unsigned long currentTime = millis();

        // Check if enough time has passed since last mode change (debouncing)
        if (currentTime - lastModeChangeTime >= BUTTON_DEBOUNCE_TIME) {
            nextMode();
            lastModeChangeTime = currentTime; // Update the last change time

            // Show visual feedback for mode selection
            // We have 4 modes (1-4, skipping MODE_OFF which is 0)
            int displayMode = currentMode - 1; // Convert to 0-based index (MODE_AMBIENT=1 becomes 0, etc.)
            buttonFeedback.showModeSelection(displayMode, 4); // 4 total modes to display
        }
    }

    // Check for effect button (only works when powered on)
    if (sensors.isNewTouch(EFFECT_BUTTON_CHANNEL) && isPowerOn) {
        unsigned long currentTime = millis();

        // Check if enough time has passed since last effect change (debouncing)
        if (currentTime - lastEffectChangeTime >= BUTTON_DEBOUNCE_TIME) {
            nextEffect();
            lastEffectChangeTime = currentTime; // Update the last change time

            // Show visual feedback for effect selection
            // Get the number of effects available for the current mode
            int numEffects = effects[currentMode].size();
            buttonFeedback.showEffectSelection(currentEffect, numEffects);
        }
    }
}

void SmartLantern::handleAutoLighting() {
    // Only do anything if light sensor is active (not OFF)
    if (lightButtonState == 0) {
        return;
    }

    int lightLevel = sensors.getLightLevel();
    int currentThreshold = 0;

    // Get current threshold
    switch (lightButtonState) {
        case 1: // Low sensitivity
            currentThreshold = LIGHT_THRESHOLD_LOW;
            break;
        case 2: // Medium sensitivity
            currentThreshold = LIGHT_THRESHOLD_MEDIUM;
            break;
        case 3: // High sensitivity
            currentThreshold = LIGHT_THRESHOLD_HIGH;
            break;
    }

    bool isDark = lightLevel < currentThreshold;
    unsigned long currentTime = millis();

    if (isDark) {
        // Dark conditions - start timer to turn ON
        if (lowLightStartTime == 0) {
            lowLightStartTime = currentTime;
        } else if (currentTime - lowLightStartTime >= 5000) {
            // 1 minute
            // Turn on if not already on
            if (!isPowerOn) {
                Serial.println("5 seconds of darkness - turning ON");
                setPower(true);
            }
            lowLightStartTime = 0; // Reset timer
        }
    } else {
        // Bright conditions - start timer to turn OFF
        if (lowLightStartTime == 0) {
            lowLightStartTime = currentTime;
        } else if (currentTime - lowLightStartTime >= 5000) {
            // 1 minute
            // Turn off if currently on
            if (isPowerOn) {
                Serial.println("5 seconds of brightness - turning OFF");
                setPower(false);
            }
            lowLightStartTime = 0; // Reset timer
        }
    }
}

void SmartLantern::startWindDown() {
    isWindingDown = true;
    windDownPosition = 0; // Start from position 0
    lastWindDownTime = millis();

    // Don't change isPowerOn yet - we'll do that when wind-down completes
    Serial.println("Wind-down sequence started");
}

// New method to handle the wind-down animation:
void SmartLantern::updateWindDown() {
    unsigned long currentTime = millis();

    // Control animation speed - update every 10ms for smooth wind-down
    if (currentTime - lastWindDownTime < 10) {
        return; // Not time to update yet
    }

    lastWindDownTime = currentTime;

    // Calculate the maximum position we need to reach
    // We'll wind down all strips simultaneously, so use the longest strip
    int maxPosition = max(max(LED_STRIP_CORE_COUNT, LED_STRIP_INNER_COUNT),
                          max(LED_STRIP_OUTER_COUNT, LED_STRIP_RING_COUNT));

    // Check if wind-down is complete
    if (windDownPosition >= maxPosition) {
        // Wind-down complete - now actually turn off
        isWindingDown = false;
        isPowerOn = false;
        isAutoOn = false; // Also turn off auto-on state
        currentMode = MODE_OFF;
        leds.clearAll(); // Final clear to make sure everything is off
        leds.showAll();

        // Reset brightness in case it was dimmed
        leds.setBrightness(77); // Reset to default brightness

        Serial.println("Wind-down complete - power OFF");
        return;
    }

    // Clear LEDs from the end (working backwards)
    // Core strip - clear from end to start
    if (windDownPosition < LED_STRIP_CORE_COUNT) {
        int clearPos = LED_STRIP_CORE_COUNT - 1 - windDownPosition;
        leds.getCore()[clearPos] = CRGB::Black;
    }

    // Inner strips - clear each segment from end to start
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        if (windDownPosition < INNER_LEDS_PER_STRIP) {
            int clearPos = (segment * INNER_LEDS_PER_STRIP) + (INNER_LEDS_PER_STRIP - 1 - windDownPosition);
            if (clearPos < LED_STRIP_INNER_COUNT) {
                leds.getInner()[clearPos] = CRGB::Black;
            }
        }
    }

    // Outer strips - clear each segment from end to start
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        if (windDownPosition < OUTER_LEDS_PER_STRIP) {
            int clearPos = (segment * OUTER_LEDS_PER_STRIP) + (OUTER_LEDS_PER_STRIP - 1 - windDownPosition);
            if (clearPos < LED_STRIP_OUTER_COUNT) {
                leds.getOuter()[clearPos] = CRGB::Black;
            }
        }
    }

    // Ring strip - clear from end to start
    if (windDownPosition < LED_STRIP_RING_COUNT) {
        int clearPos = LED_STRIP_RING_COUNT - 1 - windDownPosition;
        leds.getRing()[clearPos] = CRGB::Black;
    }

    // Show the changes
    leds.showAll();

    // Move to next position for next update
    windDownPosition++;

    // Debug output every 20 positions to reduce spam
    if (windDownPosition % 20 == 0) {
        Serial.print("Wind-down progress: ");
        Serial.print(windDownPosition);
        Serial.print(" / ");
        Serial.println(maxPosition);
    }
}

void SmartLantern::updateBrightnessFromTOF() {
    // Only apply TOF brightness control when lantern is powered on
    if (!isPowerOn) {
        return;
    }

    // Get brightness from distance sensor
    int tofBrightness = sensors.getBrightnessFromDistance();

    // Only update if we have a valid reading (hand detected in range)
    if (tofBrightness != -1) {
        // Convert 0-100 percentage to 0-255 range for FastLED
        // But keep minimum brightness at ~10% so LEDs don't completely disappear
        int ledBrightness;

        if (tofBrightness == 0) {
            // 0-10cm range: Turn LEDs off completely
            ledBrightness = 0;
        } else {
            // 10-80cm range: Map to 25-255 (10%-100%) to keep LEDs visible
            ledBrightness = map(tofBrightness, 1, 100, 25, 255);
        }

        // Apply the brightness to LED controller
        leds.setBrightness(ledBrightness);

        // Optional: Uncomment for debugging brightness changes
        // Serial.print("TOF Brightness: ");
        // Serial.print(tofBrightness);
        // Serial.print("% -> LED: ");
        // Serial.println(ledBrightness);
    }
    // If tofBrightness == -1 (no hand detected), keep current brightness unchanged
}
