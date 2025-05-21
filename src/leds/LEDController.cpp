// src/leds/LEDController.cpp
#include "LEDController.h"

LEDController::LEDController() : brightness(77) // 30% default brightness
{
}

void LEDController::begin() {
    // Configure each LED strip with FastLED
    FastLED.addLeds<WS2812B, LED_STRIP_CORE_PIN, RGB>(ledsCore, LED_STRIP_CORE_COUNT);
    FastLED.addLeds<WS2812B, LED_STRIP_INNER_PIN, RGB>(ledsInner, LED_STRIP_INNER_COUNT);
    FastLED.addLeds<WS2812B, LED_STRIP_OUTER_PIN, RGB>(ledsOuter, LED_STRIP_OUTER_COUNT);
    FastLED.addLeds<WS2812B, LED_STRIP_RING_PIN, RGB>(ledsRing, LED_STRIP_RING_COUNT);

    // Set default brightness
    FastLED.setBrightness(brightness);

    // Clear all LEDs initially
    clearAll();

    Serial.println("LED strips initialized with FastLED");
}

void LEDController::clearAll() {
    // Clear all LED arrays
    fill_solid(ledsCore, LED_STRIP_CORE_COUNT, CRGB::Black);
    fill_solid(ledsInner, LED_STRIP_INNER_COUNT, CRGB::Black);
    fill_solid(ledsOuter, LED_STRIP_OUTER_COUNT, CRGB::Black);
    fill_solid(ledsRing, LED_STRIP_RING_COUNT, CRGB::Black);
}

void LEDController::showAll() {
    // FastLED optimization: update all strips in one call
    FastLED.show();
}

void LEDController::setBrightness(uint8_t newBrightness) {
    brightness = newBrightness;
    FastLED.setBrightness(brightness);
}

uint32_t LEDController::colorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
    // Use FastLED's HSV to RGB conversion
    CHSV hsv(hue >> 8, sat, val); // FastLED uses 0-255 for hue
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);

    return CRGBToNeoColor(rgb);
}

uint32_t LEDController::color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

CRGB LEDController::neoColorToCRGB(uint32_t color) {
    return CRGB(
        (color >> 16) & 0xFF,
        (color >> 8) & 0xFF,
        color & 0xFF
    );
}

uint32_t LEDController::CRGBToNeoColor(CRGB color) {
    return ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | color.b;
}

int LEDController::mapPositionToPhysical(int stripId, int logicalPos, int subStrip) {
    int physicalPos = logicalPos;

    switch (stripId) {
        case 0: // Core strip
            // For core, we need to flip the B-C sections
            if (logicalPos > LED_STRIP_CORE_COUNT / 3) {  // If in sections B or C
                // Flip the position for B-C sections
                physicalPos = LED_STRIP_CORE_COUNT - 1 - logicalPos;
            }
            break;

        case 1: // Inner strips
            // No flipping for inner strips - they're in-line
            physicalPos = logicalPos % INNER_LEDS_PER_STRIP;
            break;

        case 2: // Outer strips
            // No flipping for outer strips - they're in-line
            physicalPos = logicalPos % OUTER_LEDS_PER_STRIP;
            break;

        case 3: // Ring strip
            // No additional mapping needed
            break;
    }

    return physicalPos;
}