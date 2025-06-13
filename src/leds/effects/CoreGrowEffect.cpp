// src/leds/effects/CoreGrowEffect.cpp

#include "CoreGrowEffect.h"

CoreGrowEffect::CoreGrowEffect(LEDController& ledController) :
    Effect(ledController),
    currentPhase(GROWING),
    currentSize(0),
    leftPosition(0),
    rightPosition(0),
    lastUpdateTime(0),
    lastTrailCreateTime(0)
{
    // Initialize trails vector
    trails.reserve(MAX_TRAILS);

    Serial.println("CoreGrowEffect created - core grows + anti-wave staggered trails");
}

void CoreGrowEffect::reset() {
    currentPhase = GROWING;
    currentSize = 0;
    leftPosition = 0;
    rightPosition = 0;
    lastUpdateTime = millis();
    lastTrailCreateTime = millis();

    // DON'T clear trails - let them continue independently
    // trails.clear(); // <- REMOVED THIS LINE

    Serial.println("CoreGrowEffect reset to growing phase (trails continue)");
}

void CoreGrowEffect::update() {
    // Clear all strips first
    leds.clearAll();

    unsigned long currentTime = millis();

    // Update and draw trails first (so core effect can overlap)
    updateTrails();
    drawTrails();

    // Create new trails with staggered timing to prevent waves
    int activeTrails = 0;
    for (const auto& trail : trails) {
        if (trail.active) activeTrails++;
    }

    // Calculate dynamic interval with randomness to prevent synchronized waves
    int createInterval = TRAIL_CREATE_INTERVAL + random(-TRAIL_STAGGER_VARIANCE, TRAIL_STAGGER_VARIANCE);

    // Always try to maintain target trails with frequent creation
    if (activeTrails < TARGET_TRAILS && (currentTime - lastTrailCreateTime >= createInterval)) {
        createNewTrail();
        lastTrailCreateTime = currentTime;
    }

    // More aggressive creation if we're well below target
    if (activeTrails < TARGET_TRAILS * 0.7) {
        createNewTrail();
        lastTrailCreateTime = currentTime;
    }

    // Emergency creation if we're very low
    if (activeTrails < TARGET_TRAILS * 0.4) {
        createNewTrail();
        lastTrailCreateTime = currentTime - createInterval; // Reset timer to allow immediate next creation
    }

    // Core effect logic (unchanged)
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
                // Both patterns are off screen, restart ONLY the core effect (not trails)
                Serial.println("Core patterns off screen - restarting core only");

                // Reset only core-related variables, leave trails alone
                currentPhase = GROWING;
                currentSize = 0;
                leftPosition = 0;
                rightPosition = 0;
                lastUpdateTime = currentTime;

                return; // Don't call reset() which would clear trails
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

void CoreGrowEffect::createNewTrail() {
    // Don't create more trails if we're at maximum
    if (trails.size() >= MAX_TRAILS) {
        return;
    }

    CoreTrail newTrail;

    // Randomly choose inner (1) or outer (2) strips
    newTrail.stripType = random(1, 3);

    // Randomly choose which segment (0, 1, or 2)
    newTrail.subStrip = random(3);

    // Get strip length
    int stripLength = (newTrail.stripType == 1) ? INNER_LEDS_PER_STRIP : OUTER_LEDS_PER_STRIP;

    // Set direction and starting position (start trails completely off the strip)
    if (newTrail.stripType == 1) {
        // Inner strips: shoot downward
        newTrail.direction = false;
        newTrail.position = stripLength - 1 + TRAIL_LENGTH; // Start with entire trail above the strip
    } else {
        // Outer strips: shoot upward
        newTrail.direction = true;
        newTrail.position = 0 - TRAIL_LENGTH; // Start with entire trail below the strip
    }

    // Random speed with more variation to prevent synchronized movement
    float baseSpeed = 0.14f + (random(100) / 100.0f) * 0.56f; // 0.14 to 0.7 (30% slower: 0.2-1.0 * 0.7)

    // Add extra randomness to speed to prevent trails from moving in sync
    float speedVariance = (random(100) / 100.0f) * 0.1f - 0.05f; // ±0.05 variance
    newTrail.speed = baseSpeed + speedVariance;
    newTrail.active = true;

    // Add to trails vector
    trails.push_back(newTrail);
}

void CoreGrowEffect::updateTrails() {
    // Update each trail
    for (auto& trail : trails) {
        if (!trail.active) continue;

        // Move the trail
        if (trail.direction) {
            // Moving upward
            trail.position += trail.speed;
        } else {
            // Moving downward
            trail.position -= trail.speed;
        }

        // Get strip length
        int stripLength = (trail.stripType == 1) ? INNER_LEDS_PER_STRIP : OUTER_LEDS_PER_STRIP;

        // Deactivate trail only when the entire trail has moved completely off the strip
        if (trail.direction && trail.position - TRAIL_LENGTH >= stripLength) {
            // Upward trail: deactivate when the tail (last LED) is above the strip
            trail.active = false;
        } else if (!trail.direction && trail.position + TRAIL_LENGTH <= 0) {
            // Downward trail: deactivate when the tail (last LED) is below the strip
            trail.active = false;
        }
    }

    // Remove inactive trails
    trails.erase(
        std::remove_if(trails.begin(), trails.end(),
            [](const CoreTrail& t) { return !t.active; }),
        trails.end());
}

void CoreGrowEffect::drawTrails() {
    for (const auto& trail : trails) {
        if (!trail.active) continue;

        // Get strip length
        int stripLength = (trail.stripType == 1) ? INNER_LEDS_PER_STRIP : OUTER_LEDS_PER_STRIP;

        // Draw the trail
        for (int i = 0; i < TRAIL_LENGTH; i++) {
            int pixelPos;

            if (trail.direction) {
                // Upward trail: head is at position, tail extends downward
                pixelPos = (int)trail.position - i;
            } else {
                // Downward trail: head is at position, tail extends upward
                pixelPos = (int)trail.position + i;
            }

            // Skip if pixel is outside strip bounds (but don't stop drawing the trail)
            if (pixelPos < 0 || pixelPos >= stripLength) continue;

            // Calculate brightness with more obvious fade to black
            // Use exponential fade for more dramatic effect
            float brightness = 1.0f - ((float)i / TRAIL_LENGTH);
            brightness = brightness * brightness; // Square the brightness for more dramatic fade

            // Physical position calculation
            int physicalPos = leds.mapPositionToPhysical(trail.stripType, pixelPos, trail.subStrip);

            // Adjust for segment offset
            if (trail.stripType == 1) {
                // Inner strips
                physicalPos += trail.subStrip * INNER_LEDS_PER_STRIP;
            } else {
                // Outer strips
                physicalPos += trail.subStrip * OUTER_LEDS_PER_STRIP;
            }

            // Set color
            CRGB color;
            if (i == 0) {
                // Head pixel: bright white
                color = CRGB(255, 255, 255);
            } else {
                // Trail pixels: red fading to black with more dramatic fade
                uint8_t redValue = 255 * brightness;
                color = CRGB(redValue, 0, 0);
            }

            // Set the LED
            if (trail.stripType == 1) {
                // Inner strips
                if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                    leds.getInner()[physicalPos] = color;
                }
            } else {
                // Outer strips
                if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                    leds.getOuter()[physicalPos] = color;
                }
            }
        }
    }
}

float CoreGrowEffect::calculateBrightness(int offset) {
    // Create smooth fade from center (100%) to edges (15%)
    // Center = 100%, edges = 15% (more visible than 10%)
    float brightnessRange = 1.0f - 0.15f;  // 85% range to work with (100% to 15%)
    float distanceRatio = (float)offset / MAX_SIZE;  // 0.0 at center, 1.0 at max distance

    // Start at 100% and reduce to 15% as distance increases
    return 1.0f - (distanceRatio * brightnessRange);
}

void CoreGrowEffect::drawPattern(int segment, int centerPos) {
    int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;
    int segmentStart = segment * coreSegmentLength;
    int segmentEnd = segmentStart + coreSegmentLength - 1;

    // Draw center LED in bright red at 100% brightness
    if (centerPos >= segmentStart && centerPos <= segmentEnd) {
        // Use max instead of add to prevent brightness increase from overlapping
        CRGB currentColor = leds.getCore()[centerPos];
        CRGB newColor = CRGB(255, 0, 0); // Bright red
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

        // Apply brightness to red color
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