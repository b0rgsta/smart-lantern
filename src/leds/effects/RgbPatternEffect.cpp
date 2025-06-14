// src/leds/effects/RgbPatternEffect.cpp

#include "RgbPatternEffect.h"

RgbPatternEffect::RgbPatternEffect(LEDController& ledController) :
    Effect(ledController),
    scrollPosition(0.0f),
    ringScrollPosition(0.0f),
    sizePhase(0.0f),
    lastUpdateTime(0),
    outerBreathingPhase(0.0f),
    innerBreathingPhase(0.0f)  // NEW: Initialize inner breathing phase
{
    // Initialize without clearing LEDs as requested
    Serial.println("RgbPatternEffect created - Synchronized RGB patterns on all strips");
}

void RgbPatternEffect::reset() {
    // Reset all animation states
    scrollPosition = 0.0f;
    ringScrollPosition = 0.0f;
    sizePhase = 0.0f;
    outerBreathingPhase = 0.0f;
    innerBreathingPhase = 0.0f;  // NEW: Reset inner breathing phase
    lastUpdateTime = millis();

    Serial.println("RgbPatternEffect reset - all patterns restarting");
}

void RgbPatternEffect::update() {
    // Target 60 FPS for smooth animation
    if (!shouldUpdate(16)) {  // 16ms = ~60 FPS
        return;
    }

    // Clear all strips first
    leds.clearAll();

    // Update scroll position for upward movement (core only now)
    scrollPosition -= SCROLL_SPEED;
    if (scrollPosition < 0) {
        scrollPosition += PATTERN_LENGTH;
    }

    // Update ring scroll position for continuous rotation
    ringScrollPosition += RING_SCROLL_SPEED;
    if (ringScrollPosition >= PATTERN_LENGTH) {
        ringScrollPosition -= PATTERN_LENGTH;
    }

    // Update size phase for smooth size transitions
    sizePhase += SIZE_SPEED;
    if (sizePhase > 2.0f * PI) {
        sizePhase -= 2.0f * PI;
    }

    // Update outer breathing phase (synchronized with size phase)
    outerBreathingPhase = sizePhase;  // Keep in sync with dot size

    // Update inner breathing phase
    innerBreathingPhase += INNER_BREATHING_SPEED;
    if (innerBreathingPhase > 6.0f * PI) {  // 3 complete cycles for R-G-B
        innerBreathingPhase -= 6.0f * PI;
    }

    // Draw effects on each strip type

    // Core: upward moving RGB dots
    for (int segment = 0; segment < 3; segment++) {
        drawCoreSegment(segment);
    }

    // Inner: NEW breathing RGB cycle
    updateInnerBreathing();

    // Ring: rotating RGB pattern
    if (!skipRing) {
        drawRing();
    }

    // Outer: breathing RGB waves
    updateOuterWaves();

    // Show all changes
    leds.showAll();
}

int RgbPatternEffect::getCurrentDotSize() {
    // Use sine wave to smoothly transition between sizes
    float sineValue = sin(sizePhase);
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Interpolate between BASE_DOT_SIZE and MAX_DOT_SIZE
    int size = BASE_DOT_SIZE + (int)(normalizedSine * (MAX_DOT_SIZE - BASE_DOT_SIZE));
    return size;
}

CRGB RgbPatternEffect::getColorAtPosition(float position, int dotSize) {
    // Normalize position to pattern length
    float normalizedPos = fmod(position, PATTERN_LENGTH);

    // Determine which color section we're in
    int colorSection = (int)(normalizedPos / PATTERN_SPACING);
    float posInSection = fmod(normalizedPos, PATTERN_SPACING);

    // Check if we're within the dot size
    if (posInSection < dotSize) {
        // Return the appropriate color
        switch (colorSection) {
            case 0: return CRGB(255, 0, 0);    // Red
            case 1: return CRGB(0, 255, 0);    // Green
            case 2: return CRGB(0, 0, 255);    // Blue
            default: return CRGB::Black;
        }
    }

    return CRGB::Black;  // Gap between dots
}

float RgbPatternEffect::uvToPatternPosition(float uvPosition) {
    // Map UV (0.0 to 1.0) to pattern position
    // UV 0.0 = bottom of strip, UV 1.0 = top of strip
    return uvPosition * CORE_LEDS_PER_SEGMENT;
}

CRGB RgbPatternEffect::getColorAtUV(float uvPosition, int dotSize) {
    float patternPos = uvToPatternPosition(uvPosition);
    return getColorAtPosition(patternPos + scrollPosition, dotSize);
}

void RgbPatternEffect::drawCoreSegment(int segment) {
    // Get current dot size
    int currentDotSize = getCurrentDotSize();

    // Draw pattern on this segment
    CRGB* segmentStart = leds.getCore() + (segment * CORE_LEDS_PER_SEGMENT);

    for (int i = 0; i < CORE_LEDS_PER_SEGMENT; i++) {
        // Calculate UV position (0.0 at bottom, 1.0 at top)
        float uvPos = (float)i / CORE_LEDS_PER_SEGMENT;

        // Get color at this UV position
        CRGB color = getColorAtUV(uvPos, currentDotSize);

        // Set the LED color
        segmentStart[i] = color;
    }
}

// NEW: Completely new implementation for inner strip breathing
void RgbPatternEffect::updateInnerBreathing() {
    // Determine which color phase we're in (0-2*PI for each color)
    int colorPhase = (int)(innerBreathingPhase / (2.0f * PI));  // 0, 1, or 2
    float phaseInColor = fmod(innerBreathingPhase, 2.0f * PI);  // 0 to 2*PI

    // Calculate breathing intensity using sine wave
    float breathingIntensity = (sin(phaseInColor - PI/2) + 1.0f) / 2.0f;  // 0 to 1

    // Scale between min and max brightness
    float brightness = INNER_MIN_BRIGHTNESS +
                      (breathingIntensity * (INNER_MAX_BRIGHTNESS - INNER_MIN_BRIGHTNESS));

    // Determine which color to show based on phase
    CRGB currentColor;
    switch (colorPhase % 3) {  // Use modulo to wrap around
        case 0:
            currentColor = CRGB(255, 0, 0);  // Red
            break;
        case 1:
            currentColor = CRGB(0, 255, 0);  // Green
            break;
        case 2:
            currentColor = CRGB(0, 0, 255);  // Blue
            break;
    }

    // Apply brightness to the color
    currentColor.nscale8(255 * brightness);

    // Apply the same color to all inner strip LEDs
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        leds.getInner()[i] = currentColor;
    }
}

// This method is no longer used for inner strips
void RgbPatternEffect::drawInnerSegment(int segment) {
    // REMOVED: Old synchronized pattern code
    // Inner strips now use breathing RGB cycle instead
}

void RgbPatternEffect::drawRing() {
    // Get current dot size
    int currentDotSize = getCurrentDotSize();

    // Draw rotating pattern on ring
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        // Map ring position to pattern space
        float ringPosition = (float)i / LED_STRIP_RING_COUNT * PATTERN_LENGTH;
        float patternPosition = fmod(ringPosition + ringScrollPosition, PATTERN_LENGTH);

        // Get the color for this pattern position
        CRGB color = getColorAtPosition(patternPosition, currentDotSize);

        // Set the LED color
        leds.getRing()[i] = color;
    }
}

void RgbPatternEffect::updateOuterWaves() {
    // Calculate current brightness from breathing phase (synchronized with dot size)
    float sineValue = sin(outerBreathingPhase);
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Brightness ranges from 20% to 100%
    float brightness = 0.2f + (normalizedSine * 0.8f);

    // Draw breathing waves on each outer strip segment
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        // Each segment shows a different color phase
        // This creates a wave effect up the lantern
        int colorOffset = segment;  // 0, 1, or 2

        for (int led = 0; led < OUTER_LEDS_PER_STRIP; led++) {
            // Calculate which color this LED should be
            // Create gradient within each segment
            float gradientPos = (float)led / OUTER_LEDS_PER_STRIP;

            // Blend between current color and next color
            int currentColor = (colorOffset + (int)(outerBreathingPhase / (2.0f * PI / 3))) % 3;
            int nextColor = (currentColor + 1) % 3;

            // Calculate blend factor based on phase within color transition
            float phaseInColor = fmod(outerBreathingPhase, 2.0f * PI / 3) / (2.0f * PI / 3);

            // Get the two colors to blend
            CRGB color1 = getIndexedColor(currentColor);
            CRGB color2 = getIndexedColor(nextColor);

            // Blend colors
            CRGB blendedColor = color1.lerp8(color2, phaseInColor * 255);

            // Apply brightness and gradient fade
            float finalBrightness = brightness * (1.0f - gradientPos * 0.7f); // Fade to 30% at top
            blendedColor.nscale8(255 * finalBrightness);

            // Set LED color
            int physicalPos = segment * OUTER_LEDS_PER_STRIP + led;
            leds.getOuter()[physicalPos] = blendedColor;
        }
    }
}

CRGB RgbPatternEffect::getIndexedColor(int colorIndex) {
    switch (colorIndex % 3) {
        case 0: return CRGB(255, 0, 0);    // Red
        case 1: return CRGB(0, 255, 0);    // Green
        case 2: return CRGB(0, 0, 255);    // Blue
        default: return CRGB::Black;
    }
}