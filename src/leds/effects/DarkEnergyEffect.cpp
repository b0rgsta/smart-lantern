// src/leds/effects/DarkEnergyEffect.cpp

#include "DarkEnergyEffect.h"
#include <algorithm>

// Constructor - sets up the dark energy effect
DarkEnergyEffect::DarkEnergyEffect(LEDController& ledController)
    : Effect(ledController), needsUpdate(true) {
    // Constructor intentionally does NOT call leds.clear() as per your requirements
    // Effect will be applied on first update() call
}

// Update the effect - applies dark energy pattern when needed
void DarkEnergyEffect::update() {
    // Only update if needed (static effect optimization)
    if (!needsUpdate) {
        return;
    }

    // Clear all strips to start fresh
    leds.clearAll();

    // Apply dark energy pattern to inner and outer strips
    applyInnerPattern();
    applyOuterPattern();

    // Core and ring strips remain off (already cleared)

    // Show all changes to make them visible
    leds.showAll();

    // Mark as updated
    needsUpdate = false;
}

// Reset the effect - triggers a reapplication of the pattern
void DarkEnergyEffect::reset() {
    needsUpdate = true;
}

// Apply dark energy pattern to inner strips
void DarkEnergyEffect::applyInnerPattern() {
    // Apply pattern to each of the 3 inner strips independently
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        int segmentStart = segment * INNER_LEDS_PER_STRIP;

        // Apply red with fade to each LED in this strip
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;

            // Calculate fade brightness for this position
            float fadeBrightness = calculateFadeBrightness(i, INNER_LEDS_PER_STRIP);

            // Apply red color with calculated brightness
            applyRedWithBrightness(leds.getInner()[ledIndex], fadeBrightness);
        }
    }
}

// Apply dark energy pattern to outer strips
void DarkEnergyEffect::applyOuterPattern() {
    // Apply pattern to each of the 3 outer strips independently
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        // Apply red with fade to each LED in this strip
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;

            // Calculate fade brightness for this position
            float fadeBrightness = calculateFadeBrightness(i, OUTER_LEDS_PER_STRIP);

            // Apply red color with calculated brightness
            applyRedWithBrightness(leds.getOuter()[ledIndex], fadeBrightness);
        }
    }
}

// Calculate fade brightness for position (90% black fade from both ends)
float DarkEnergyEffect::calculateFadeBrightness(int position, int stripLength) {
    // Calculate distance from center as a ratio (0.0 = center, 1.0 = edge)
    float center = (stripLength - 1) / 2.0f;
    float distanceFromCenter = abs(position - center) / center;

    // Clamp distance to valid range
    distanceFromCenter = std::min(1.0f, distanceFromCenter);

    // Apply 90% fade:
    // - At center (distance = 0.0): brightness = 1.0 (full effect)
    // - At edges (distance = 1.0): brightness = 0.1 (10% remaining = 90% fade)
    float brightness = 1.0f - (distanceFromCenter * FADE_PERCENTAGE);

    return brightness;
}

// Apply red color with specified brightness to an LED
void DarkEnergyEffect::applyRedWithBrightness(CRGB& color, float brightnessFactor) {
    // Start with base red color at 50% brightness
    CRGB baseColor = leds.neoColorToCRGB(BASE_RED_COLOR);

    // Apply base brightness (50%) and fade brightness
    float finalBrightness = BASE_BRIGHTNESS * brightnessFactor;

    // Set the LED color with calculated brightness
    color = CRGB(
        (uint8_t)(baseColor.r * finalBrightness),
        (uint8_t)(baseColor.g * finalBrightness),
        (uint8_t)(baseColor.b * finalBrightness)
    );
}