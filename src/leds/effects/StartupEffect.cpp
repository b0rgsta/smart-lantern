// src/leds/effects/StartupEffect.cpp

#include "StartupEffect.h"

StartupEffect::StartupEffect(LEDController& ledController) :
  Effect(ledController),
  position(0),
  lastUpdate(0),
  complete(false)
{
}

void StartupEffect::reset() {
    position = 0;
    complete = false;
}

void StartupEffect::update() {
    if (complete) return;

    unsigned long currentTime = millis();
    if (currentTime - lastUpdate < 20) return;  // 20ms between updates

    lastUpdate = currentTime;

    int maxPosition = max(max(LED_STRIP_CORE_COUNT, LED_STRIP_INNER_COUNT),
                        max(LED_STRIP_OUTER_COUNT, LED_STRIP_RING_COUNT));

    if (position >= maxPosition) {
        // Animation finished
        delay(100);
        leds.clearAll();
        complete = true;
        return;
    }


    if (position < LED_STRIP_INNER_COUNT) {
        leds.getInner()[position] = CRGB(0, 0, 255); // Blue
    }

    if (position < LED_STRIP_OUTER_COUNT) {
        leds.getOuter()[position] = CRGB(255, 0, 0); // Red
    }

    if (position < LED_STRIP_RING_COUNT) {
        leds.getRing()[position] = CRGB(255, 255, 255); // White
    }

    // Show all LEDs at once rather than individual strips
    leds.showAll();

    position++;
}