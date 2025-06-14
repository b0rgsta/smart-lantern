// src/leds/effects/RgbPatternEffect.cpp

#include "RgbPatternEffect.h"

RgbPatternEffect::RgbPatternEffect(LEDController& ledController) :
    Effect(ledController),
    scrollPosition(0.0f),
    ringScrollPosition(0.0f),
    sizePhase(0.0f),
    lastUpdateTime(0),
    outerBreathingPhase(0.0f)
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

    // Update scroll position for upward movement (core and inner)
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

    // Draw effects on each strip type

    // Core: upward moving RGB dots
    for (int segment = 0; segment < 3; segment++) {
        drawCoreSegment(segment);
    }

    // Inner: same pattern as core (synchronized with UV mapping)
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        drawInnerSegment(segment);
    }

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
    // Use sine wave to smoothly transition between BASE_DOT_SIZE and MAX_DOT_SIZE
    float sineValue = sin(sizePhase);  // -1.0 to 1.0
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Calculate dot size
    int dotSize = BASE_DOT_SIZE + (int)(normalizedSine * (MAX_DOT_SIZE - BASE_DOT_SIZE));

    return dotSize;
}

bool RgbPatternEffect::isColorDot(float position, int dotSize, int colorIndex) {
    // Calculate where each color starts in the pattern
    float colorStart = colorIndex * PATTERN_SPACING;

    // Ensure position is within pattern bounds
    position = fmod(position, PATTERN_LENGTH);
    if (position < 0) position += PATTERN_LENGTH;

    // Check if position falls within the dot for this color
    float dotOffset = (MAX_DOT_SIZE - dotSize) / 2.0f;  // Center the dot
    float dotStart = colorStart + dotOffset;
    float dotEnd = dotStart + dotSize;

    // Handle wrap-around at pattern boundaries
    if (dotEnd > PATTERN_LENGTH) {
        // Dot wraps around
        return (position >= dotStart) || (position < (dotEnd - PATTERN_LENGTH));
    }

    return (position >= dotStart && position < dotEnd);
}

CRGB RgbPatternEffect::getColorAtPosition(float position, int dotSize) {
    // Check each color to see if this position is part of a dot
    if (isColorDot(position, dotSize, 0)) {
        return CRGB::Red;
    }
    if (isColorDot(position, dotSize, 1)) {
        return CRGB::Green;
    }
    if (isColorDot(position, dotSize, 2)) {
        return CRGB::Blue;
    }

    // Otherwise, this position is off (black)
    return CRGB::Black;
}

float RgbPatternEffect::uvToPatternPosition(float uvPosition) {
    // Map UV coordinate (0.0 to 1.0) to pattern position
    // Account for scrolling
    float patternPosition = (uvPosition * PATTERN_LENGTH) + scrollPosition;

    // Wrap around pattern length
    while (patternPosition >= PATTERN_LENGTH) {
        patternPosition -= PATTERN_LENGTH;
    }
    while (patternPosition < 0) {
        patternPosition += PATTERN_LENGTH;
    }

    return patternPosition;
}

CRGB RgbPatternEffect::getColorAtUV(float uvPosition, int dotSize) {
    float patternPosition = uvToPatternPosition(uvPosition);
    return getColorAtPosition(patternPosition, dotSize);
}

CRGB RgbPatternEffect::getIndexedColor(int colorIndex) {
    switch (colorIndex) {
        case 0: return CRGB::Red;
        case 1: return CRGB::Green;
        case 2: return CRGB::Blue;
        default: return CRGB::Black;
    }
}

void RgbPatternEffect::drawCoreSegment(int segment) {
    // Calculate the length of each core segment
    int segmentLength = LED_STRIP_CORE_COUNT / 3;

    // Get current dot size
    int currentDotSize = getCurrentDotSize();

    // Draw each LED in this segment
    for (int i = 0; i < segmentLength; i++) {
        // Calculate UV position (0.0 to 1.0) for this LED
        float uvPosition = (float)i / (segmentLength - 1);

        // Get the color for this UV position
        CRGB color = getColorAtUV(uvPosition, currentDotSize);

        // Map logical position to physical position
        int physicalPos = leds.mapPositionToPhysical(0, i, segment); // 0 = core strip
        physicalPos += segment * segmentLength;

        // Make sure we're within bounds
        if (physicalPos >= 0 && physicalPos < LED_STRIP_CORE_COUNT) {
            leds.getCore()[physicalPos] = color;
        }
    }
}

void RgbPatternEffect::drawInnerSegment(int segment) {
    // Get current dot size
    int currentDotSize = getCurrentDotSize();

    // Draw each LED in this inner segment using UV mapping
    for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
        // Calculate UV position (0.0 to 1.0) for this LED
        float uvPosition = (float)i / (INNER_LEDS_PER_STRIP - 1);

        // Get the color for this UV position (same as core)
        CRGB color = getColorAtUV(uvPosition, currentDotSize);

        // Calculate physical LED position
        int physicalPos = segment * INNER_LEDS_PER_STRIP + i;

        // Make sure we're within bounds
        if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
            leds.getInner()[physicalPos] = color;
        }
    }
}

void RgbPatternEffect::drawRing() {
    // Get current dot size
    int currentDotSize = getCurrentDotSize();

    // Draw each LED in the ring
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        // Map ring position to pattern position with continuous scrolling
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