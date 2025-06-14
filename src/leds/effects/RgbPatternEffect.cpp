// src/leds/effects/RgbPatternEffect.cpp

#include "RgbPatternEffect.h"

RgbPatternEffect::RgbPatternEffect(LEDController& ledController) :
    Effect(ledController),
    scrollPosition(0.0f),
    sizePhase(0.0f),
    lastUpdateTime(0)
{
    // Initialize without clearing LEDs as requested
    Serial.println("RgbPatternEffect created - RGB pattern with size transitions (2-6 LEDs) scrolling upward on core");
}

void RgbPatternEffect::reset() {
    // Reset scroll position and size phase to start
    scrollPosition = 0.0f;
    sizePhase = 0.0f;
    lastUpdateTime = millis();

    Serial.println("RgbPatternEffect reset - pattern starting from bottom");
}

void RgbPatternEffect::update() {
    // Target 60 FPS for smooth scrolling
    if (!shouldUpdate(16)) {  // 16ms = ~60 FPS
        return;
    }

    // Clear all strips first
    leds.clearAll();

    // Update scroll position for upward movement
    scrollPosition -= SCROLL_SPEED;

    // Wrap around when we've scrolled a full pattern length
    if (scrollPosition < 0) {
        scrollPosition += PATTERN_LENGTH;
    }

    // Update size phase for smooth size transitions
    sizePhase += SIZE_SPEED;
    if (sizePhase > 2.0f * PI) {
        sizePhase -= 2.0f * PI;
    }

    // Draw the pattern on all 3 core segments
    for (int segment = 0; segment < 3; segment++) {
        drawSegment(segment);
    }

    // Show the changes
    leds.showAll();
}

int RgbPatternEffect::getCurrentDotSize() {
    // Use sine wave to smoothly transition between BASE_DOT_SIZE and MAX_DOT_SIZE
    float sineValue = sin(sizePhase);  // -1.0 to 1.0
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Calculate dot size (2 to 6)
    int dotSize = BASE_DOT_SIZE + (int)(normalizedSine * (MAX_DOT_SIZE - BASE_DOT_SIZE));

    return dotSize;
}

bool RgbPatternEffect::isColorDot(float position, int dotSize, int colorIndex) {
    // Calculate where each color starts in the pattern
    float colorStart = colorIndex * PATTERN_SPACING;

    // Wrap position to pattern bounds
    while (position >= PATTERN_LENGTH) {
        position -= PATTERN_LENGTH;
    }
    while (position < 0) {
        position += PATTERN_LENGTH;
    }

    // Check if position falls within the dot for this color
    // We center the variable-sized dot within the allocated space
    float dotOffset = (MAX_DOT_SIZE - dotSize) / 2.0f;  // Center the dot
    float dotStart = colorStart + dotOffset;
    float dotEnd = dotStart + dotSize;

    // Handle wrap-around at pattern boundaries
    if (dotEnd > PATTERN_LENGTH) {
        // Dot wraps around
        return (position >= dotStart) || (position < dotEnd - PATTERN_LENGTH);
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

void RgbPatternEffect::drawSegment(int segment) {
    // Calculate the length of each core segment
    int segmentLength = LED_STRIP_CORE_COUNT / 3;

    // Get current dot size
    int currentDotSize = getCurrentDotSize();

    // Draw each LED in this segment
    for (int i = 0; i < segmentLength; i++) {
        // Calculate which position in the pattern this LED represents
        float patternPosition = i + scrollPosition;

        // Get the color for this pattern position
        CRGB color = getColorAtPosition(patternPosition, currentDotSize);

        // Map logical position to physical position using the LED controller's mapping
        // This handles the middle segment reversal automatically
        int physicalPos = leds.mapPositionToPhysical(0, i, segment); // 0 = core strip

        // Add segment offset
        physicalPos += segment * segmentLength;

        // Make sure we're within bounds
        if (physicalPos >= 0 && physicalPos < LED_STRIP_CORE_COUNT) {
            leds.getCore()[physicalPos] = color;
        }
    }
}