// src/SmartLantern.h

#ifndef SMART_LANTERN_H
#define SMART_LANTERN_H

#include <Arduino.h>
#include <vector>
#include <Preferences.h>
#include "leds/LEDController.h"
#include "sensors/SensorController.h"
#include "leds/effects/Effect.h"
#include "leds/effects/StartupEffect.h"  // Add this back for the startup animation
#include "leds/effects/FireEffect.h"
#include "leds/effects/PartyRippleEffect.h"
#include "leds/effects/RainbowEffect.h"
#include "leds/effects/TrailsEffect.h"
#include "leds/effects/MatrixEffect.h"
#include "leds/effects/AcceleratingTrailsEffect.h"

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

  // Vector to store all effects for each mode
  // effects[mode][effect_index]
  std::vector<std::vector<Effect*>> effects;

  // Special effects that need direct access
  StartupEffect* startupEffect;    // Used for initialization
  FireEffect* fireEffectPtr;       // Used for temperature override

  // State variables
  bool isPowerOn;
  bool isAutoOn;
  LanternMode currentMode;
  unsigned int currentEffect;
  int tempButtonState;
  int lightButtonState;

  // Timing variables
  unsigned long powerButtonPressTime;
  unsigned long lowLightStartTime;
  unsigned long autoOnTime;

  // Private helper functions
  void processTouchInputs();
  void handleAutoLighting();
  void updateEffects();
  void initializeEffects(); // Helper method to initialize all effects
};

#endif // SMART_LANTERN_H