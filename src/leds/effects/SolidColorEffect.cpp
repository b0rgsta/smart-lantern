// src/leds/effects/SolidColorEffect.cpp

#include "SolidColorEffect.h"

// Constructor for single color (all strips same color)
SolidColorEffect::SolidColorEffect(LEDController& ledController, uint32_t color) :
    Effect(ledController),
    coreColor(color),
    innerColor(color),
    outerColor(color),
    ringColor(color)
{
    // No initialization needed beyond initializer list
}

// Constructor for multi-color (different color per strip)
SolidColorEffect::SolidColorEffect(LEDController& ledController,
                                 uint32_t coreColor,
                                 uint32_t innerColor,
                                 uint32_t outerColor,
                                 uint32_t ringColor) :
    Effect(ledController),
    coreColor(coreColor),
    innerColor(innerColor),
    outerColor(outerColor),
    ringColor(ringColor)
{
    // No initialization needed beyond initializer list
}

void SolidColorEffect::reset() {
    // Nothing to reset in this effect
}

void SolidColorEffect::update() {
    // Apply colors to each strip (or turn off if COLOR_NONE)
    applyColor(leds.getCore(), LED_STRIP_CORE_COUNT, coreColor);
    applyColor(leds.getInner(), LED_STRIP_INNER_COUNT, innerColor);
    applyColor(leds.getOuter(), LED_STRIP_OUTER_COUNT, outerColor);
    applyColor(leds.getRing(), LED_STRIP_RING_COUNT, ringColor);

    // Show all changes
    leds.showAll();
}

void SolidColorEffect::setCoreColor(uint32_t color) {
    coreColor = color;
}

void SolidColorEffect::setInnerColor(uint32_t color) {
    innerColor = color;
}

void SolidColorEffect::setOuterColor(uint32_t color) {
    outerColor = color;
}

void SolidColorEffect::setRingColor(uint32_t color) {
    ringColor = color;
}

void SolidColorEffect::setAllColors(uint32_t color) {
    coreColor = color;
    innerColor = color;
    outerColor = color;
    ringColor = color;
}

String SolidColorEffect::getName() const {
    return "Solid Color Effect";
}

bool SolidColorEffect::isValidColor(uint32_t color) const {
    // Check if color is valid (not COLOR_NONE)
    return color != COLOR_NONE;
}

void SolidColorEffect::applyColor(CRGB* strip, int count, uint32_t color) {
    if (isValidColor(color)) {
        // Convert color and apply to strip
        CRGB rgbColor = leds.neoColorToCRGB(color);
        fill_solid(strip, count, rgbColor);
    } else {
        // Turn off this strip
        fill_solid(strip, count, CRGB::Black);
    }
}