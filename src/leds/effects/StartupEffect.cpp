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
        delay(500);
        leds.clearAll();
        complete = true;
        return;
    }
  
    if (position < LED_STRIP_CORE_COUNT) {
        leds.getCore().setPixelColor(position, leds.getCore().Color(0, 255, 0));
        leds.getCore().show();
    }
  
    if (position < LED_STRIP_INNER_COUNT) {
        leds.getInner().setPixelColor(position, leds.getInner().Color(0, 0, 255));
        leds.getInner().show();
    }
  
    if (position < LED_STRIP_OUTER_COUNT) {
        leds.getOuter().setPixelColor(position, leds.getOuter().Color(255, 0, 0));
        leds.getOuter().show();
    }
  
    if (position < LED_STRIP_RING_COUNT) {
        leds.getRing().setPixelColor(position, leds.getRing().Color(255, 255, 255));
        leds.getRing().show();
    }
  
    position++;
}