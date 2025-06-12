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
                int segmentCenter = coreSegmentLength / 2; // Normal center calculation
                leftPosition = segmentCenter;
                rightPosition = segmentCenter;

                // Don't update lastUpdateTime here - let the moving phase handle timing
                // This prevents the flicker by ensuring immediate movement

                Serial.println("Switching to moving phase - patterns will move in both directions");
            } else {
                Serial.print("Growing to size: ");
                Serial.print(currentSize);
                Serial.print(" (total LEDs: ");
                Serial.print(1 + 2 * currentSize);
                Serial.print(" / 25)");
                Serial.println();

                lastUpdateTime = currentTime; // Only update timing during normal growth
            }
        }

        if (currentSize <= MAX_SIZE) {
            // Draw growing pattern on all 3 core segments using the same method as moving phase
            int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;

            for (int segment = 0; segment < 3; segment++) {
                int segmentCenter = (segment * coreSegmentLength) + (coreSegmentLength / 2);

                // Draw the pattern using the same drawPattern method used in moving phase
                drawPattern(segment, segmentCenter);
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

        // Always draw both moving patterns
        for (int segment = 0; segment < 3; segment++) {
            int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;

            // Calculate segment positions
            int segmentStart = segment * coreSegmentLength;
            int segmentBaseCenter = segmentStart + (coreSegmentLength / 2);

            // Calculate positions for this segment's patterns
            int segmentLeftPos = segmentBaseCenter + (leftPosition - coreSegmentLength / 2);
            int segmentRightPos = segmentBaseCenter + (rightPosition - coreSegmentLength / 2);

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
        // Use max instead of add to prevent brightness increase from overlapping
        CRGB currentColor = leds.getCore()[centerPos];
        CRGB newColor = CRGB(255, 0, 0);
        leds.getCore()[centerPos] = CRGB(
            max(currentColor.r, newColor.r),
            max(currentColor.g, newColor.g),
            max(currentColor.b, newColor.b)
        );
    }

    // Draw LEDs on both sides with brightness fade
    // Use currentSize during growing phase, MAX_SIZE during moving phase
    int drawSize = (currentPhase == GROWING) ? currentSize : MAX_SIZE;

    for (int offset = 1; offset <= drawSize; offset++) {
        float brightness = calculateBrightness(offset);
        uint8_t redValue = 255 * brightness;

        // Left side LED
        int leftPos = centerPos - offset;
        if (leftPos >= segmentStart && leftPos <= segmentEnd) {
            // Use max instead of add to prevent brightness increase from overlapping
            CRGB currentColor = leds.getCore()[leftPos];
            CRGB newColor = CRGB(redValue, 0, 0);
            leds.getCore()[leftPos] = CRGB(
                max(currentColor.r, newColor.r),
                max(currentColor.g, newColor.g),
                max(currentColor.b, newColor.b)
            );
        }

        // Right side LED
        int rightPos = centerPos + offset;
        if (rightPos >= segmentStart && rightPos <= segmentEnd) {
            // Use max instead of add to prevent brightness increase from overlapping
            CRGB currentColor = leds.getCore()[rightPos];
            CRGB newColor = CRGB(redValue, 0, 0);
            leds.getCore()[rightPos] = CRGB(
                max(currentColor.r, newColor.r),
                max(currentColor.g, newColor.g),
                max(currentColor.b, newColor.b)
            );
        }
    }
}