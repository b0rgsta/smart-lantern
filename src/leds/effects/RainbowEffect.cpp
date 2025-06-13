#include "RainbowEffect.h"

RainbowEffect::RainbowEffect(LEDController &ledController) : Effect(ledController),
                                                             cycle(0),
                                                             animationSpeed(30.0f)
// 30 cycles per second for smooth rainbow movement
{
}

void RainbowEffect::reset() {
    cycle = 0;
    lastUpdateTime = millis(); // Reset timing when effect resets
}

void RainbowEffect::update() {
    // Target 120 FPS for ultra-smooth rainbow animation
    if (!shouldUpdate(8)) {
        // 8ms = 125 FPS (close to 120)
        return;
    }

    // Calculate time delta manually since shouldUpdate() already updated lastUpdateTime
    // Use a fixed 8ms delta since we're limiting to 125 FPS
    float deltaTimeMs = 8.0f; // Fixed delta time in milliseconds
    float deltaTimeSeconds = deltaTimeMs / 1000.0f; // Convert to seconds

    // Update cycle based on elapsed time and desired animation speed
    // animationSpeed is cycles per second
    cycle += animationSpeed * deltaTimeSeconds;

    // Keep cycle within reasonable bounds (0-255 range)
    if (cycle >= 256.0f) {
        cycle -= 256.0f;
    }

    // Convert float cycle to integer for hue calculations
    uint16_t baseHue = (uint16_t) (cycle * 256);

    // Core strip - gradient around the strip
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        int pixelHue = baseHue + (i * 65536 / LED_STRIP_CORE_COUNT);
        leds.getCore()[i] = CHSV(pixelHue >> 8, 255, 255);
    }

    // Inner strip - gradient across all inner strips
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        int pixelHue = baseHue + (i * 65536 / LED_STRIP_INNER_COUNT);
        leds.getInner()[i] = CHSV(pixelHue >> 8, 255, 255);
    }

    // Outer strip - gradient across all outer strips
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        int pixelHue = baseHue + (i * 65536 / LED_STRIP_OUTER_COUNT);
        leds.getOuter()[i] = CHSV(pixelHue >> 8, 255, 255);
    }

    // Ring strip - gradient around the ring
    if (!skipRing)
        for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
            int pixelHue = baseHue + (i * 65536 / LED_STRIP_RING_COUNT);
            leds.getRing()[i] = CHSV(pixelHue >> 8, 255, 255);
        }

    // Show the LEDs
    leds.showAll();
}