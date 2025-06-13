#include "RainbowEffect.h"

RainbowEffect::RainbowEffect(LEDController &ledController,
                           bool enableCore,
                           bool enableInner,
                           bool enableOuter,
                           bool enableRing) :
    Effect(ledController),
    cycle(0),
    animationSpeed(30.0f), // 30 cycles per second for smooth rainbow movement
    breathingPhase(0.0f),
    breathingSpeed(0.004f), // Speed to complete 5 second breathing cycle (2*PI / 5000ms * 8ms)
    coreEnabled(enableCore),
    innerEnabled(enableInner),
    outerEnabled(enableOuter),
    ringEnabled(enableRing)
{
    // Constructor - no LED clearing as per instructions
}

void RainbowEffect::reset() {
    cycle = 0;
    breathingPhase = 0.0f;              // Reset breathing to start position
    lastUpdateTime = millis();          // Reset timing when effect resets
}

void RainbowEffect::update() {
    // Target 120 FPS for ultra-smooth rainbow animation
    if (!shouldUpdate(8)) {
        // 8ms = 125 FPS (close to 120)
        return;
    }

    // Clear all LEDs first - this ensures disabled strips stay off
    leds.clearAll();

    // Calculate time delta manually since shouldUpdate() already updated lastUpdateTime
    // Use a fixed 8ms delta since we're limiting to 125 FPS
    float deltaTimeMs = 8.0f; // Fixed delta time in milliseconds
    float deltaTimeSeconds = deltaTimeMs / 1000.0f; // Convert to seconds

    // Update rainbow cycle based on elapsed time and desired animation speed
    // animationSpeed is cycles per second
    cycle += animationSpeed * deltaTimeSeconds;

    // Keep cycle within reasonable bounds (0-255 range)
    if (cycle >= 256.0f) {
        cycle -= 256.0f;
    }

    // Update breathing phase for core strip (5 second full cycle)
    breathingPhase += breathingSpeed;
    if (breathingPhase > 2.0f * PI) {
        breathingPhase -= 2.0f * PI;  // Keep phase in 0 to 2*PI range
    }

    // Calculate core breathing brightness (0 to 1.0)
    // Use sine wave to create smooth breathing effect
    float sineValue = sin(breathingPhase);      // -1.0 to 1.0
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0
    // Core brightness fades from 0% to 100% and back over 5 seconds
    float coreBrightness = normalizedSine;

    // Convert float cycle to integer for hue calculations
    uint16_t baseHue = (uint16_t) (cycle * 256);

    // Core strip - gradient around the strip with breathing brightness and 2x speed
    // Only update if core is enabled
    if (coreEnabled) {
        for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
            // Make core colors move twice as fast by multiplying baseHue by 2
            int pixelHue = (baseHue * 2) + (i * 65536 / LED_STRIP_CORE_COUNT);

            // Create the rainbow color at full brightness first
            CHSV hsvColor(pixelHue >> 8, 255, 255);
            CRGB rgbColor;
            hsv2rgb_rainbow(hsvColor, rgbColor);

            // Apply breathing brightness to the RGB color
            rgbColor.nscale8_video((uint8_t)(coreBrightness * 255));

            leds.getCore()[i] = rgbColor;
        }
    }

    // Inner strip - normal rainbow gradient (no breathing)
    // Only update if inner is enabled
    if (innerEnabled) {
        for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
            int pixelHue = baseHue + (i * 65536 / LED_STRIP_INNER_COUNT);
            leds.getInner()[i] = CHSV(pixelHue >> 8, 255, 255);
        }
    }

    // Outer strip - normal rainbow gradient (no breathing)
    // Only update if outer is enabled
    if (outerEnabled) {
        for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
            int pixelHue = baseHue + (i * 65536 / LED_STRIP_OUTER_COUNT);
            leds.getOuter()[i] = CHSV(pixelHue >> 8, 255, 255);
        }
    }

    // Ring strip - normal rainbow gradient (no breathing, unless skipped for button feedback)
    // Only update if ring is enabled AND not skipped for button feedback
    if (ringEnabled && !skipRing) {
        for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
            int pixelHue = baseHue + (i * 65536 / LED_STRIP_RING_COUNT);
            leds.getRing()[i] = CHSV(pixelHue >> 8, 255, 255);
        }
    }

    // Show the LEDs
    leds.showAll();
}