#ifndef SMART_LANTERN_H
#define SMART_LANTERN_H

#include <Arduino.h>
#include "leds/LEDController.h"
#include "sensors/SensorController.h"
#include "leds/effects/Effect.h"
#include "leds/effects/StartupEffect.h"
#include "leds/effects/TrailsEffect.h"
#include "leds/effects/RainbowEffect.h"
#include "leds/effects/FireEffect.h"
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
  int getCurrentEffect() const { return currentEffect; }

  // Power control
  void setPower(bool on);
  bool isPowered() const { return isPowerOn; }
  void togglePower();

private:
  LEDController leds;
  SensorController sensors;

  // Effects
  StartupEffect* startupEffect;
  TrailsEffect* trailsEffect;
  RainbowEffect* rainbowEffect;
  FireEffect* fireEffect;
  MatrixEffect* matrixEffect;
  AcceleratingTrailsEffect* acceleratingTrailsEffect;

  Effect* currentEffectPtr;  // Points to the currently active effect

  // State variables
  bool isPowerOn;
  bool isAutoOn;
  LanternMode currentMode;
  int currentEffect;
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
};

#endif // SMART_LANTERN_H