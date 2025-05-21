/*
 * SmartLantern.cpp - Implementation of the SmartLantern class
 */

#include "SmartLantern.h"

SmartLantern::SmartLantern() :
  isPowerOn(false),
  isAutoOn(false),
  currentMode(MODE_AMBIENT),
  currentEffect(0),
  tempButtonState(0),
  lightButtonState(0),
  powerButtonPressTime(0),
  lowLightStartTime(0),
  autoOnTime(0),
  currentEffectPtr(nullptr)
{
  // Create effects
  startupEffect = new StartupEffect(leds);
  trailsEffect = new TrailsEffect(leds);
  rainbowEffect = new RainbowEffect(leds);
  fireEffect = new FireEffect(leds);
  
  // Set default effect
  currentEffectPtr = trailsEffect;
}

SmartLantern::~SmartLantern() {
  // Clean up effects
  delete startupEffect;
  delete trailsEffect;
  delete rainbowEffect;
  delete fireEffect;
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

  // Show startup animation
  while (!startupEffect->isComplete()) {
    startupEffect->update();
    delay(20);
  }
  
  Serial.println("Smart Lantern Ready!");
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
    currentEffect = 0;  // Reset effect when mode changes
    
    Serial.print("Mode changed to: ");
    Serial.println(currentMode);
  }
}

void SmartLantern::nextMode() {
  // Cycle through modes 1-4 (skipping MODE_OFF)
  currentMode = static_cast<LanternMode>((currentMode % 4) + 1);
  currentEffect = 0;  // Reset effect when mode changes

  String modeNames[] = {"OFF", "AMBIENT", "GRADIENT", "ANIMATED", "PARTY"};

  Serial.print("Mode changed to: " + modeNames[currentMode]);
}

void SmartLantern::nextEffect() {
  // Effects are mode-dependent, so number will vary
  // For simplicity, we'll cycle through 0-3 for all modes
  currentEffect = (currentEffect + 1) % 4;
  
  Serial.print("Effect changed to: ");
  Serial.println(currentEffect);
}

void SmartLantern::setPower(bool on) {
  if (on != isPowerOn) {
    isPowerOn = on;
    
    if (isPowerOn) {
      // Turn on - set to default mode
      currentMode = MODE_AMBIENT;
      leds.setBrightness(77);  // 30% brightness
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
    fireEffect->update();
    return;
  }
  
  // Otherwise, update the current effect based on mode
  switch (currentMode) {
    case MODE_OFF:
      // All LEDs off
      leds.clearAll();
      break;
      
    case MODE_AMBIENT:
      // Choose effect based on currentEffect
      if (currentEffect == 0) {
        // Solid color
        uint32_t color = leds.getCore().Color(255, 255, 255);  // White
        for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
          leds.getCore().setPixelColor(i, color);
        }
        for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
          leds.getInner().setPixelColor(i, color);
        }
        for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
          leds.getOuter().setPixelColor(i, color);
        }
        for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
          leds.getRing().setPixelColor(i, color);
        }
        leds.showAll();
      } else if (currentEffect == 1) {
        // Dim warm white
        uint32_t warmColor = leds.getCore().Color(255, 200, 100);
        for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
          leds.getCore().setPixelColor(i, warmColor);
        }
        for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
          leds.getInner().setPixelColor(i, warmColor);
        }
        for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
          leds.getOuter().setPixelColor(i, warmColor);
        }
        for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
          leds.getRing().setPixelColor(i, warmColor);
        }
        leds.showAll();
      } else {
        // Other ambient effects...
        rainbowEffect->update();
      }
      break;
      
    case MODE_GRADIENT:
      // Rainbow cycle effect
      rainbowEffect->update();
      break;
      
    case MODE_ANIMATED:
      // Trails effect
      fireEffect->update();
      break;
      
    case MODE_PARTY:
      // Party effects - could be a more rapidly changing rainbow
      if (currentEffect % 2 == 0) {
        rainbowEffect->update();
      } else {
        trailsEffect->update();
      }
      break;
  }
}

// This is just the relevant part of SmartLantern.cpp that needs updating

void SmartLantern::processTouchInputs() {
  // Check for temperature button
  if (sensors.isNewTouch(TEMP_BUTTON_CHANNEL)) {
    tempButtonState = (tempButtonState + 1) % 4;  // Cycle through states 0-3
    Serial.print("Temperature button state: ");
    Serial.println(tempButtonState);
  }

  // Check for light sensitivity button
  if (sensors.isNewTouch(LIGHT_BUTTON_CHANNEL)) {
    lightButtonState = (lightButtonState + 1) % 4;  // Cycle through states 0-3
    Serial.print("Light sensitivity state: ");
    Serial.println(lightButtonState);
  }

  // Check for power button press
  if (sensors.isNewTouch(POWER_BUTTON_CHANNEL)) {
    powerButtonPressTime = millis();  // Record press time for long-press detection
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