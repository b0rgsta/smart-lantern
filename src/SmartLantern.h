// src/SmartLantern.h

#ifndef SMART_LANTERN_H
#define SMART_LANTERN_H

#include <Arduino.h>
#include <vector>
#include <Preferences.h>
#include "leds/LEDController.h"
#include "sensors/SensorController.h"
#include "leds/effects/Effect.h"
#include "leds/effects/FireEffect.h"
#include "leds/MPR121LEDHandler.h"

// Define modes
enum LanternMode {
  MODE_OFF = 0,
  MODE_AMBIENT = 1,
  MODE_GRADIENT = 2,
  MODE_ANIMATED = 3,
  MODE_PARTY = 4
};

class SmartLantern {
public:
  SmartLantern();
  ~SmartLantern();

  void begin();
  void update();

  // Mode control
  void setMode(LanternMode mode);
  LanternMode getMode() const { return currentMode; }
  void nextMode();

  // Effect control
  void nextEffect();
  unsigned int getCurrentEffect() const { return currentEffect; }

  // Power control
  void setPower(bool on);
  bool isPowered() const { return isPowerOn; }
  void togglePower();

private:
  LEDController leds;
  SensorController sensors;

  Preferences preferences;

  MPR121LEDHandler buttonFeedback;

  // Vector to store all effects for each mode
  // effects[mode][effect_index]
  std::vector<std::vector<Effect*>> effects;

  // Special effects that need direct access
  FireEffect* fireEffectPtr;       // Used for temperature override

  // State variables
  bool isPowerOn;
  bool isAutoOn;
  LanternMode currentMode;
  unsigned int currentEffect;
  int tempButtonState;
  int lightButtonState;

  // Wind-down effect variables
  bool isWindingDown;              // True when performing wind-down animation
  int windDownPosition;            // Current position in wind-down sequence
  unsigned long lastWindDownTime;  // Last time we updated wind-down animation

  // Timing variables
  unsigned long powerButtonPressTime;
  unsigned long lowLightStartTime;
  unsigned long autoOnTime;

  // Debounce timing for mode and effect buttons
  unsigned long lastModeChangeTime;
  unsigned long lastEffectChangeTime;
  static const unsigned long BUTTON_DEBOUNCE_TIME = 250; // 1/3 second in milliseconds

  // Private helper functions
  void updateBrightnessFromTOF();  // Updates LED brightness based on TOF sensor
  void processTouchInputs();
  void handleAutoLighting();
  void updateEffects();
  void initializeEffects(); // Helper method to initialize all effects
  void updateWindDown();     // Handle the wind-down animation
  void startWindDown();      // Start the wind-down sequence
};

#endif // SMART_LANTERN_H