#include "FireEffect.h"

FireEffect::FireEffect(LEDController& ledController) :
  Effect(ledController),
  temperature(20.0f)  // Default to room temperature
{
}

void FireEffect::setTemperature(float temp) {
    temperature = temp;
}

void FireEffect::update() {
    // Determine fire colors based on temperature
    uint32_t fireColor;

    if (temperature <= TEMP_THRESHOLD_BLUE) {
        // Cold - blue fire
        fireColor = leds.getCore().Color(0, 0, 255);
    } else if (temperature <= TEMP_THRESHOLD_RED_BLUE) {
        // Cool - purple/blue fire
        fireColor = leds.getCore().Color(128, 0, 255);
    } else {
        // Warm - red/orange fire
        fireColor = leds.getCore().Color(255, 50, 0);
    }

    // Apply the fire color to all strips
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        leds.getCore().setPixelColor(i, fireColor);
    }

    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        leds.getInner().setPixelColor(i, fireColor);
    }

    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        leds.getOuter().setPixelColor(i, fireColor);
    }

    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        leds.getRing().setPixelColor(i, fireColor);
    }

    // Show the LEDs
    leds.showAll();
}