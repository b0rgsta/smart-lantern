// src/leds/effects/SolidColorEffect.cpp

#include "SolidColorEffect.h"

SolidColorEffect::SolidColorEffect(LEDController& ledController, uint32_t color) :
    Effect(ledController),
    color(color)
{
}

void SolidColorEffect::reset() {
    // Nothing to reset
}

void SolidColorEffect::update() {
    // Convert the color to CRGB
    CRGB rgbColor = leds.neoColorToCRGB(color);

    // Apply the solid color to all strips
    fill_solid(leds.getCore(), LED_STRIP_CORE_COUNT, rgbColor);
    fill_solid(leds.getInner(), LED_STRIP_INNER_COUNT, rgbColor);
    fill_solid(leds.getOuter(), LED_STRIP_OUTER_COUNT, rgbColor);
    fill_solid(leds.getRing(), LED_STRIP_RING_COUNT, rgbColor);

    leds.showAll();
}

void SolidColorEffect::setColor(uint32_t newColor) {
    color = newColor;
}