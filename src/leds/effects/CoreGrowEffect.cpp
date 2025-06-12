// src/leds/effects/CoreGrowEffect.cpp

#include "CoreGrowEffect.h"

CoreGrowEffect::CoreGrowEffect(LEDController& ledController) :
    Effect(ledController),
    currentPhase(GROWING),
    currentSize(0),
    leftPosition(0),
    rightPosition(0),
    lastUpdateTime(0)
{
    Serial.println("CoreGrowEffect created - grows to 25 LEDs then moves in both directions (30% faster)");
}

void CoreGrowEffect::reset() {
    currentPhase = GROWING;
    currentSize = 0;
    leftPosition = 0;
    rightPosition = 0;
    lastUpdateTime = millis();
    Serial.println("CoreGrowEffect reset to growing phase");
}

void CoreGrowEffect::update() {
    // Clear all strips first
    leds.clearAll();

    unsigned long currentTime = millis();

    if (currentPhase == GROWING) {
        // GROWING PHASE: Grow from 1 to 17 LEDs
        if (currentTime - lastUpdateTime >= GROW_INTERVAL) {
            currentSize++;

            // When we reach full size, switch to moving phase
            if (currentSize > MAX_SIZE) {
                currentPhase = MOVING;

                // Set initial positions for moving phase (both start at center)
                int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;
                int segmentCenter = (coreSegmentLength / 2) - 5; // Same offset as first segment (was -8, now -5)
                leftPosition = segmentCenter;
                rightPosition = segmentCenter;

                Serial.println("Switching to moving phase - patterns will move in both directions");
            } else {
                Serial.print("Growing to size: ");
                Serial.print(currentSize);
                Serial.print(" (total LEDs: ");
                Serial.print(1 + 2 * currentSize);
                Serial.print(" / 25)");
                Serial.println();
            }

            lastUpdateTime = currentTime;
        }

        if (currentSize <= MAX_SIZE) {
            // Draw growing pattern on all 3 core segments
            int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;

            for (int segment = 0; segment < 3; segment++) {
                int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;
                int segmentCenter;

                // Calculate center position for each segment
                if (segment == 0) {
                    // First segment: move center down by 5 LEDs (was 8, moved up 3)
                    segmentCenter = (segment * coreSegmentLength) + (coreSegmentLength / 2) - 5;
                } else {
                    // Other segments: use normal center calculation
                    segmentCenter = (segment * coreSegmentLength) + (coreSegmentLength / 2);
                }

                Serial.print("Segment ");
                Serial.print(segment);
                Serial.print(" center at: ");
                Serial.println(segmentCenter);

                // Center LED at 100% brightness
                leds.getCore()[segmentCenter] = CRGB(255, 0, 0);

                // LEDs on both sides with brightness fade
                for (int offset = 1; offset <= currentSize; offset++) {
                    float brightness = calculateBrightness(offset);
                    uint8_t redValue = 255 * brightness;

                    // Left side LED
                    int leftPos = segmentCenter - offset;
                    if (leftPos >= segment * coreSegmentLength) {
                        leds.getCore()[leftPos] = CRGB(redValue, 0, 0);
                    }

                    // Right side LED
                    int rightPos = segmentCenter + offset;
                    if (rightPos < (segment + 1) * coreSegmentLength) {
                        leds.getCore()[rightPos] = CRGB(redValue, 0, 0);
                    }
                }
            }
        }

    } else if (currentPhase == MOVING) {
        // MOVING PHASE: Two full patterns moving in opposite directions
        if (currentTime - lastUpdateTime >= MOVE_INTERVAL) {
            // Move both patterns
            leftPosition--;   // Move left pattern toward start of segment
            rightPosition++;  // Move right pattern toward end of segment

            lastUpdateTime = currentTime;

            Serial.print("Moving: left=");
            Serial.print(leftPosition);
            Serial.print(" right=");
            Serial.println(rightPosition);

            // Check if both patterns are completely off the strip
            int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;
            if (leftPosition + MAX_SIZE < 0 && rightPosition - MAX_SIZE >= coreSegmentLength) {
                // Both patterns are off screen, restart the effect
                Serial.println("Both patterns off screen - restarting");
                reset();
                return;
            }
        }

        // Draw both moving patterns on all 3 core segments
        for (int segment = 0; segment < 3; segment++) {
            int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;

            // Calculate segment positions
            int segmentStart = segment * coreSegmentLength;
            int segmentBaseCenter;

            if (segment == 0) {
                // First segment: use offset center
                segmentBaseCenter = segmentStart + (coreSegmentLength / 2) - 5;
            } else {
                // Other segments: use normal center
                segmentBaseCenter = segmentStart + (coreSegmentLength / 2);
            }

            // Calculate positions for this segment's patterns
            int segmentLeftPos = segmentBaseCenter + (leftPosition - ((coreSegmentLength / 2) - 5));
            int segmentRightPos = segmentBaseCenter + (rightPosition - ((coreSegmentLength / 2) - 5));

            // Draw left-moving pattern
            drawPattern(segment, segmentLeftPos);

            // Draw right-moving pattern
            drawPattern(segment, segmentRightPos);
        }
    }

    // Show the LEDs
    leds.showAll();
}

float CoreGrowEffect::calculateBrightness(int offset) {
    // Create smooth fade from center (100%) to edges (close to 0%)
    return 1.0f - ((float)offset / (MAX_SIZE + 1));
}

void CoreGrowEffect::drawPattern(int segment, int centerPos) {
    int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;
    int segmentStart = segment * coreSegmentLength;
    int segmentEnd = segmentStart + coreSegmentLength - 1;

    // Draw center LED
    if (centerPos >= segmentStart && centerPos <= segmentEnd) {
        leds.getCore()[centerPos] = CRGB(255, 0, 0);
    }

    // Draw LEDs on both sides with brightness fade
    for (int offset = 1; offset <= MAX_SIZE; offset++) {
        float brightness = calculateBrightness(offset);
        uint8_t redValue = 255 * brightness;

        // Left side LED
        int leftPos = centerPos - offset;
        if (leftPos >= segmentStart && leftPos <= segmentEnd) {
            // Add to existing color in case patterns overlap
            CRGB currentColor = leds.getCore()[leftPos];
            currentColor.r = min(255, currentColor.r + redValue);
            leds.getCore()[leftPos] = currentColor;
        }

        // Right side LED
        int rightPos = centerPos + offset;
        if (rightPos >= segmentStart && rightPos <= segmentEnd) {
            // Add to existing color in case patterns overlap
            CRGB currentColor = leds.getCore()[rightPos];
            currentColor.r = min(255, currentColor.r + redValue);
            leds.getCore()[rightPos] = currentColor;
        }
    }
}