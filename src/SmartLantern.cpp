// src/SmartLantern.cpp

#include "SmartLantern.h"

#include "leds/effects/RainbowEffect.h"
#include "leds/effects/FireEffect.h"
#include "leds/effects/MatrixEffect.h"
#include "leds/effects/GradientEffect.h"
#include "leds/effects/WaterfallEffect.h"
#include "leds/effects/CodeRedEffect.h"
#include "leds/effects/RegalEffect.h"
#include "leds/effects/RainbowTranceEffect.h"
#include "leds/effects/PartyFireEffect.h"
#include "leds/effects/TemperatureColorEffect.h"
#include "leds/effects/CandleFlickerEffect.h"
#include "leds/effects/AuraEffect.h"
#include "leds/effects/FutureEffect.h"
#include "leds/effects/FutureRainbowEffect.h"
#include "leds/effects/RgbPatternEffect.h"
#include "leds/effects/EmeraldCityEffect.h"
#include "leds/effects/SuspendedFireEffect.h"
#include "leds/effects/SuspendedPartyFireEffect.h"
#include "leds/effects/LustEffect.h"
#include "leds/effects/PartyCycleEffect.h"
#include "leds/effects/DarkEnergyEffect.h"

SmartLantern::SmartLantern() :
    buttonFeedback(leds),
    isPowerOn(false),
    isAutoOn(false),
    currentMode(MODE_AMBIENT),
    currentEffect(0),
    tempButtonState(0),
    lightButtonState(0),
    isWindingDown(false),
    windDownPosition(0),
    lastWindDownTime(0),
    powerButtonPressTime(0),
    lowLightStartTime(0),
    autoOnTime(0),
    // Initialize button hold timing variables
    tempButtonPressTime(0),
    lightButtonPressTime(0),
    modeButtonPressTime(0),
    effectButtonPressTime(0),
    // Initialize button hold state tracking variables
    tempButtonToggled(false),
    lightButtonToggled(false),
    modeButtonToggled(false),
    effectButtonToggled(false)
{
    // Initialize the effects vector structure
    effects.resize(5); // One vector for each mode (0-4)

    // Create special effects that need direct access
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
    auto splitRainbowGradient = new GradientEffect(
    leds,
    Gradient(),                                      // Core off
    GradientEffect::createFirstHalfRainbowGradient(), // Inner: Red to Cyan
    GradientEffect::createSecondHalfRainbowGradient(), // Outer: Cyan to Red
    Gradient()                                       // Ring off
);
    auto fireEffect = new FireEffect(leds);
    auto darkenergyEffect = new DarkEnergyEffect(leds);
    auto suspendedFireEffect = new SuspendedFireEffect(leds);
    auto matrixEffect = new MatrixEffect(leds);
    auto waterfallEffect = new WaterfallEffect(leds);
    auto coreGrowEffect = new CodeRedEffect(leds);
    auto technoOrangeEffect = new RegalEffect(leds);
    auto rainbowTranceEffect = new RainbowTranceEffect(leds);
    auto partyFireEffect = new PartyFireEffect(leds);
    auto rgbPatternEffect = new RgbPatternEffect(leds);
    auto suspendedPartyFireEffect = new SuspendedPartyFireEffect(leds);
    auto lustEffect = new LustEffect(leds);
    auto partyRippleEffect = new AuraEffect(leds,
        false,   // Core on
        true,   // Inner on
        true,   // Outer on
        false    // Ring of
    );
    auto futureEffect = new FutureEffect(leds);
    auto futureRainbowEffect = new FutureRainbowEffect(leds);
    // Create the Emerald City effect
    auto emeraldCityEffect = new EmeraldCityEffect(leds);

    // Store a reference to the fire effect for temperature override
    fireEffectPtr = fireEffect;

    // Solid color effects for ambient mode
    auto candleEffect = new CandleFlickerEffect(leds);

    auto incandescent = new TemperatureColorEffect(
        leds,
        2700,   // Warm incandescent
        false,  // Core off
        true,   // Inner on
        true,   // Outer on (with fade)
        false   // Ring off
    );

    auto daylight = new TemperatureColorEffect(
        leds,
        5500,   // Natural daylight
        false,  // Core off
        true,   // Inner on
        true,   // Outer on (with fade)
        false   // Ring off
    );

    effects[MODE_AMBIENT].push_back(incandescent);
    effects[MODE_AMBIENT].push_back(daylight);
    effects[MODE_AMBIENT].push_back(candleEffect);

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
        GradientEffect::createCoreChristmasGradient(),
        GradientEffect::reverseGradient(GradientEffect::createOuterChristmasGradient()),
        GradientEffect::createOuterChristmasGradient(),
        Gradient()
    );

    effects[MODE_GRADIENT].push_back(sunsetGradient);
    effects[MODE_GRADIENT].push_back(purpleBlueOpposingGradient);
    effects[MODE_GRADIENT].push_back(splitRainbowGradient);
    effects[MODE_GRADIENT].push_back(christmasGradient);

    // MODE_ANIMATED
    effects[MODE_ANIMATED].push_back(darkenergyEffect); // Full rainbow effect
    effects[MODE_ANIMATED].push_back(suspendedFireEffect);
    effects[MODE_ANIMATED].push_back(waterfallEffect); // Waterfall effect
    effects[MODE_ANIMATED].push_back(rainbowEffectNoCore); // Rainbow effect
    effects[MODE_ANIMATED].push_back(partyRippleEffect);

    // MODE_PARTY - Create party cycle effect and individual effects
    Serial.println("=== CREATING PARTY EFFECTS ===");

    // MODE_PARTY - Create party cycle effect and individual effects
    Serial.println("=== CREATING PARTY EFFECTS ===");

    // Create vector of individual party effects for cycling
    std::vector<Effect*> partyEffectsForCycling;
    partyEffectsForCycling.push_back(coreGrowEffect);
    partyEffectsForCycling.push_back(lustEffect);
    partyEffectsForCycling.push_back(emeraldCityEffect);
    partyEffectsForCycling.push_back(rainbowTranceEffect);
    partyEffectsForCycling.push_back(rgbPatternEffect);
    partyEffectsForCycling.push_back(futureEffect);
    partyEffectsForCycling.push_back(rainbowEffect);
    partyEffectsForCycling.push_back(technoOrangeEffect);
    partyEffectsForCycling.push_back(futureRainbowEffect);
    partyEffectsForCycling.push_back(matrixEffect);
    //partyEffectsForCycling.push_back(partyFireEffect);
    partyEffectsForCycling.push_back(suspendedPartyFireEffect);


    Serial.println("Created " + String(partyEffectsForCycling.size()) + " individual party effects");

    // Create the party cycle effect
    auto partyCycleEffect = new PartyCycleEffect(leds, partyEffectsForCycling);

    // Add party cycle effect as the FIRST option in party mode
    effects[MODE_PARTY].push_back(partyCycleEffect);

    // Add all individual party effects after the cycle effect
    for (Effect* effect : partyEffectsForCycling) {
        effects[MODE_PARTY].push_back(effect);
    }

    Serial.println("Party mode has " + String(effects[MODE_PARTY].size()) + " total effects (cycle + individuals)");
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

    // Set power on immediately
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

    // Update brightness based on TOF sensor (only when powered on)
    if (isPowerOn) {
        updateBrightnessFromTOF();
    }

    // Process user inputs
    processTouchInputs();

    // Handle auto on/off based on light sensor
    handleAutoLighting();

    // IMPORTANT: Check if button feedback is active before running effects
    bool feedbackActive = buttonFeedback.isFeedbackActive();

    // Tell ALL effects to skip ring updates when button feedback is showing
    if (feedbackActive) {
        // Set skipRing flag on ALL effects in ALL modes to prevent ring conflicts
        for (auto &modeEffects : effects) {
            for (auto effect : modeEffects) {
                if (effect != nullptr) {
                    effect->setSkipRing(true);
                }
            }
        }
        // Also set it on the fire effect pointer used for temperature override
        if (fireEffectPtr != nullptr) {
            fireEffectPtr->setSkipRing(true);
        }
    } else {
        // Clear skipRing flag on ALL effects when no button feedback
        for (auto &modeEffects : effects) {
            for (auto effect : modeEffects) {
                if (effect != nullptr) {
                    effect->setSkipRing(false);
                }
            }
        }
        // Also clear it on the fire effect pointer
        if (fireEffectPtr != nullptr) {
            fireEffectPtr->setSkipRing(false);
        }
    }

    // If we're in wind-down mode, handle that instead of normal effects
    if (isWindingDown) {
        updateWindDown();
    } else {
        // Update the current effect normally
        updateEffects();
    }

    // Update button feedback AFTER effects are drawn
    // This ensures button feedback timing is managed correctly
    buttonFeedback.update();
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

    // Reset the new effect to its initial state
    if (!effects[currentMode].empty() && currentEffect < effects[currentMode].size()) {
        effects[currentMode][currentEffect]->reset();
    }
}

void SmartLantern::setPower(bool on) {
    if (on && !isPowerOn) {
        // Turning ON
        isPowerOn = true;
        Serial.println("Smart Lantern powered ON");
    } else if (!on && isPowerOn) {
        // Turning OFF - start the wind-down sequence
        startWindDown();
        Serial.println("Smart Lantern powering OFF (wind-down started)");
    }
}

void SmartLantern::togglePower() {
    setPower(!isPowerOn);
}

void SmartLantern::updateBrightnessFromTOF() {
    int distance = sensors.getDistance();

    if (distance != -1 && distance <= 500) {  // 50cm max range
        int brightness;
        if (distance <= 100) {  // 10cm - closest
            brightness = 255; // Max brightness when very close
        } else if (distance >= 500) {  // 50cm - farthest
            brightness = 51; // Min brightness when far (20%)
        } else {
            // Linear mapping: closer distance = higher brightness
            brightness = map(distance, 500, 100, 51, 255);
        }
        leds.setBrightness(brightness);
    }
    // If distance is -1 (no reading), don't change brightness
}

void SmartLantern::updateEffects() {
    // Check for temperature override
    if (tempButtonState > 0) {
        float temperature = sensors.getTemperature();
        bool shouldShowFire = false;

        switch (tempButtonState) {
            case 1: // 18°C or below
                shouldShowFire = (temperature <= 18.0);
                break;
            case 2: // 10°C or below
                shouldShowFire = (temperature <= 10.0);
                break;
            case 3: // 5°C or below
                shouldShowFire = (temperature <= 5.0);
                break;
        }

        if (shouldShowFire) {
            // Override current effect with fire effect
            fireEffectPtr->update();
            return;
        }
    }

    // Normal effect update
    if (currentMode != MODE_OFF && !effects[currentMode].empty()) {
        if (currentEffect < effects[currentMode].size()) {
            effects[currentMode][currentEffect]->update();
        }
    }
}

// Note: Using constants from Config.h:
// POWER_BUTTON_HOLD_TIME, LIGHT_THRESHOLD_HIGH, LIGHT_THRESHOLD_MEDIUM, LIGHT_THRESHOLD_LOW
// BUTTON_HOLD_TIME is defined in SmartLantern.h as static const unsigned long BUTTON_HOLD_TIME = 100;

void SmartLantern::processTouchInputs() {
    // Read current touch states
    bool tempTouched = sensors.isTouched(0);     // Temperature button
    bool lightTouched = sensors.isTouched(1);    // Light sensor button
    bool powerTouched = sensors.isTouched(2);    // Power button
    bool modeTouched = sensors.isTouched(3);     // Mode button
    bool effectTouched = sensors.isTouched(4);   // Effect button

    unsigned long currentTime = millis();

    // Handle Temperature Button (0)
    if (tempTouched) {
        if (tempButtonPressTime == 0) {
            tempButtonPressTime = currentTime;  // Start timing
            tempButtonToggled = false;          // Reset toggle state
        }
        // Check if held long enough and hasn't been toggled yet
        if (!tempButtonToggled && (currentTime - tempButtonPressTime >= SmartLantern::BUTTON_HOLD_TIME)) {
            // Toggle temperature button state
            tempButtonState = (tempButtonState + 1) % 4;  // Cycle 0-3
            tempButtonToggled = true;  // Mark as toggled

            // Save to persistent storage
            preferences.putUChar("tempBtn", tempButtonState);

            // Show button feedback using the correct method
            buttonFeedback.showTemperatureState(tempButtonState);

            Serial.print("Temperature button state: ");
            Serial.println(tempButtonState);
        }
    } else {
        tempButtonPressTime = 0;   // Reset timing when released
        tempButtonToggled = false; // Reset toggle state
    }

    // Handle Light Sensor Button (1)
    if (lightTouched) {
        if (lightButtonPressTime == 0) {
            lightButtonPressTime = currentTime;
            lightButtonToggled = false;
        }
        if (!lightButtonToggled && (currentTime - lightButtonPressTime >= SmartLantern::BUTTON_HOLD_TIME)) {
            lightButtonState = (lightButtonState + 1) % 4;  // Cycle 0-3
            lightButtonToggled = true;

            // Save to persistent storage
            preferences.putUChar("lightBtn", lightButtonState);

            // Show button feedback using the correct method
            buttonFeedback.showLightState(lightButtonState);

            Serial.print("Light sensor button state: ");
            Serial.println(lightButtonState);
        }
    } else {
        lightButtonPressTime = 0;
        lightButtonToggled = false;
    }

    // Handle Power Button (2) - Special case with 2-second hold for OFF
    static bool powerHasToggled = false;  // Prevent multiple toggles during single press

    if (powerTouched) {
        if (powerButtonPressTime == 0) {
            powerButtonPressTime = currentTime;
            powerHasToggled = false;  // Reset toggle state for new press
        }

        // If powered OFF and any touch detected, turn ON immediately
        if (!isPowerOn && !powerHasToggled) {
            setPower(true);
            powerHasToggled = true;

            Serial.println("Power button pressed - turning ON");
        }
        // If powered ON and held for 2 seconds, turn OFF
        else if (isPowerOn && !powerHasToggled && (currentTime - powerButtonPressTime >= POWER_BUTTON_HOLD_TIME)) {
            setPower(false);
            powerHasToggled = true;

            Serial.println("Power button held 2 seconds - turning OFF");
        }
    } else {
        // Button released - reset timing
        if (powerButtonPressTime != 0) {
            powerButtonPressTime = 0;

            // Only log if we detected a press but didn't turn OFF
            if (isPowerOn && !powerHasToggled) {
                Serial.println("Power button released (hold 2 seconds to turn OFF)");
            }
        }

        // Note: powerHasToggled will be reset on next button press
    }

    // Handle Mode Button (3)
    if (modeTouched) {
        if (modeButtonPressTime == 0) {
            modeButtonPressTime = currentTime;
            modeButtonToggled = false;
        }
        if (!modeButtonToggled && (currentTime - modeButtonPressTime >= SmartLantern::BUTTON_HOLD_TIME)) {
            if (isPowerOn) {  // Only change mode if powered on
                nextMode();
                modeButtonToggled = true;

                // Show mode feedback using showModeSelection
                buttonFeedback.showModeSelection(currentMode - 1, 4); // Convert to 0-based, 4 total modes
            }
        }
    } else {
        modeButtonPressTime = 0;
        modeButtonToggled = false;
    }

    // Handle Effect Button (4)
    if (effectTouched) {
        if (effectButtonPressTime == 0) {
            effectButtonPressTime = currentTime;
            effectButtonToggled = false;
        }
        if (!effectButtonToggled && (currentTime - effectButtonPressTime >= SmartLantern::BUTTON_HOLD_TIME)) {
            if (isPowerOn) {  // Only change effect if powered on
                nextEffect();
                effectButtonToggled = true;

                // Show effect feedback using showEffectSelectionSmart
                int numEffects = effects[currentMode].size();
                bool isPartyMode = (currentMode == MODE_PARTY);
                buttonFeedback.showEffectSelectionSmart(currentEffect, numEffects, isPartyMode);
            }
        }
    } else {
        effectButtonPressTime = 0;
        effectButtonToggled = false;
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
            // 5 seconds
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
            // 5 seconds
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

    // Inner strips - clear from end to start
    if (windDownPosition < LED_STRIP_INNER_COUNT) {
        int clearPos = LED_STRIP_INNER_COUNT - 1 - windDownPosition;
        leds.getInner()[clearPos] = CRGB::Black;
    }

    // Outer strips - clear from end to start
    if (windDownPosition < LED_STRIP_OUTER_COUNT) {
        int clearPos = LED_STRIP_OUTER_COUNT - 1 - windDownPosition;
        leds.getOuter()[clearPos] = CRGB::Black;
    }

    // Ring strip - clear from end to start
    if (windDownPosition < LED_STRIP_RING_COUNT) {
        int clearPos = LED_STRIP_RING_COUNT - 1 - windDownPosition;
        leds.getRing()[clearPos] = CRGB::Black;
    }

    // Show the current wind-down state
    leds.showAll();

    // Move to next position
    windDownPosition++;
}