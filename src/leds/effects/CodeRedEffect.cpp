// src/leds/effects/CoreGrowEffect.cpp

#include "CodeRedEffect.h"

CodeRedEffect::CodeRedEffect(LEDController& ledController) :
    Effect(ledController),
    currentPhase(GROWING),
    currentSize(0),
    leftPosition(0),
    rightPosition(0),
    lastUpdateTime(0),
    lastTrailCreateTime(0),
    lastRingTrailCreateTime(0),  // Initialize ring trail timing
    breathingPhase(0.0f),
    breathingSpeed(0.02f),      // Slow breathing cycle - adjust this for faster/slower breathing
    minBrightness(0.4f),        // 40% minimum brightness
    maxBrightness(1.0f)         // 100% maximum brightness
{
    // Initialize trails vectors
    trails.reserve(MAX_TRAILS);
    ringTrails.reserve(MAX_RING_TRAILS);

    Serial.println("CoreGrowEffect created - core grows + breathing trails + breathing ring trails");
}

void CodeRedEffect::reset() {
    currentPhase = GROWING;
    currentSize = 0;
    leftPosition = 0;
    rightPosition = 0;
    lastUpdateTime = millis();
    lastTrailCreateTime = millis();
    lastRingTrailCreateTime = millis();  // Reset ring trail timing

    // DON'T clear trails - let them continue independently
    // trails.clear(); // <- REMOVED THIS LINE
    // ringTrails.clear(); // <- Also don't clear ring trails

    Serial.println("CoreGrowEffect reset to growing phase (trails continue)");
}

void CodeRedEffect::update() {
    // Clear all strips first
    leds.clearAll();

    // Update breathing phase for trails AND ring (synchronized)
    breathingPhase += breathingSpeed;
    if (breathingPhase > 2.0f * PI) {
        breathingPhase -= 2.0f * PI;  // Keep phase in 0 to 2*PI range
    }

    unsigned long currentTime = millis();

    // Update and draw trails first (so core effect can overlap)
    updateTrails();
    drawTrails();

    // Update ring trail effects (replaces single breathing ring)
    updateRingTrails();

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

float CodeRedEffect::calculateBreathingBrightness() {
    // Use sine wave to create smooth breathing effect
    // sin() returns -1 to 1, we want to map this to minBrightness to maxBrightness
    float sineValue = sin(breathingPhase);  // -1.0 to 1.0

    // Convert from -1,1 range to 0,1 range
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Map to our desired brightness range (40% to 100%)
    return minBrightness + (normalizedSine * (maxBrightness - minBrightness));
}

float CodeRedEffect::calculateRingBreathingBrightness() {
    // Use the same breathing phase as trails but with different brightness range
    // This keeps the ring synchronized with the trail breathing
    float sineValue = sin(breathingPhase);  // -1.0 to 1.0

    // Convert from -1,1 range to 0,1 range
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Map to ring-specific brightness range (30% to 80%)
    return RING_MIN_BRIGHTNESS + (normalizedSine * (RING_MAX_BRIGHTNESS - RING_MIN_BRIGHTNESS));
}

void CodeRedEffect::updateRingTrails() {
    // Skip ring if button feedback is active (effect base class handles this)
    if (skipRing) {
        return;
    }

    unsigned long currentTime = millis();

    // Count active ring trails
    int activeRingTrails = 0;
    for (const auto& trail : ringTrails) {
        if (trail.active) activeRingTrails++;
    }

    // Calculate dynamic interval with randomness to prevent synchronized waves
    int createInterval = RING_TRAIL_CREATE_INTERVAL + random(-RING_TRAIL_STAGGER_VARIANCE, RING_TRAIL_STAGGER_VARIANCE);

    // Create new ring trails as needed
    if (activeRingTrails < TARGET_RING_TRAILS && (currentTime - lastRingTrailCreateTime >= createInterval)) {
        createNewRingTrail();
        lastRingTrailCreateTime = currentTime;
    }

    // Update existing ring trails
    for (auto& trail : ringTrails) {
        if (!trail.active) continue;

        // Move the trail around the ring
        if (trail.clockwise) {
            trail.position += trail.speed;
            // Wrap around when we reach the end
            if (trail.position >= LED_STRIP_RING_COUNT) {
                trail.position -= LED_STRIP_RING_COUNT;
            }
        } else {
            trail.position -= trail.speed;
            // Wrap around when we go below 0
            if (trail.position < 0) {
                trail.position += LED_STRIP_RING_COUNT;
            }
        }

        // Check if trail has exceeded its lifespan
        if (currentTime - trail.creationTime >= trail.lifespan) {
            trail.active = false;
        }
    }

    // Remove inactive ring trails
    ringTrails.erase(
        std::remove_if(ringTrails.begin(), ringTrails.end(),
            [](const RingTrail& t) { return !t.active; }),
        ringTrails.end());

    // Draw all active ring trails
    drawRingTrails();
}

void CodeRedEffect::createNewRingTrail() {
    // Don't create more ring trails if we're at maximum
    if (ringTrails.size() >= MAX_RING_TRAILS) {
        return;
    }

    RingTrail newTrail;

    // Random starting position around the ring
    newTrail.position = random(LED_STRIP_RING_COUNT);

    // Random direction (clockwise or counter-clockwise)
    newTrail.clockwise = random(2) == 1;

    // Random speed (slower than linear trails for smooth circular motion)
    newTrail.speed = 0.08f + (random(100) / 100.0f) * 0.12f; // 0.08 to 0.20 speed range

    // Set trail length
    newTrail.length = RING_TRAIL_LENGTH;

    // Set creation time and random lifespan (8-15 seconds for nice variety)
    newTrail.creationTime = millis();
    newTrail.lifespan = 8000 + random(7000); // 8000ms to 15000ms (8-15 seconds)

    // Activate the trail
    newTrail.active = true;

    // Add to ring trails vector
    ringTrails.push_back(newTrail);
}

void CodeRedEffect::drawRingTrails() {
    // Clear the ring first
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        leds.getRing()[i] = CRGB::Black;
    }

    // Calculate the current breathing brightness multiplier for all ring trails
    float breathingMultiplier = calculateRingBreathingBrightness();

    for (const auto& trail : ringTrails) {
        if (!trail.active) continue;

        // Draw the trail
        for (int i = 0; i < trail.length; i++) {
            // Calculate position for this part of the trail
            int pixelPos;
            if (trail.clockwise) {
                // For clockwise motion, trail extends behind the head
                pixelPos = (int)trail.position - i;
            } else {
                // For counter-clockwise motion, trail extends behind the head
                pixelPos = (int)trail.position + i;
            }

            // Handle wrapping around the ring
            while (pixelPos < 0) {
                pixelPos += LED_STRIP_RING_COUNT;
            }
            while (pixelPos >= LED_STRIP_RING_COUNT) {
                pixelPos -= LED_STRIP_RING_COUNT;
            }

            // Calculate brightness with fade and breathing effect
            float brightness = 1.0f - ((float)i / trail.length);
            brightness = brightness * brightness; // Square for more dramatic fade
            brightness *= breathingMultiplier; // Apply breathing effect

            // Pure red color for all pixels in ring trails (no white tips)
            uint8_t redValue = 255 * brightness;
            CRGB color = CRGB(redValue, 0, 0);

            // Add the color to the existing pixel (in case trails overlap)
            leds.getRing()[pixelPos] += color;
        }
    }
}

void CodeRedEffect::createNewTrail() {
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

    // Randomly choose direction for both inner and outer strips
    newTrail.direction = random(2) == 1; // true = upward, false = downward

    // Set starting position based on direction (start trails completely off the strip)
    if (newTrail.direction) {
        // Moving upward: start with entire trail below the strip
        newTrail.position = 0 - TRAIL_LENGTH;
    } else {
        // Moving downward: start with entire trail above the strip
        newTrail.position = stripLength - 1 + TRAIL_LENGTH;
    }

    // Random speed with less variation to keep trails slower and more consistent
    float baseSpeed = 0.14f + (random(100) / 100.0f) * 0.16f; // 0.14 to 0.30 (much smaller range)

    // Add minimal randomness to speed to prevent trails from moving in sync
    float speedVariance = (random(100) / 100.0f) * 0.03f - 0.015f; // ±0.015 variance (much smaller)
    newTrail.speed = baseSpeed + speedVariance;
    newTrail.active = true;

    // Add to trails vector
    trails.push_back(newTrail);
}

void CodeRedEffect::updateTrails() {
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

void CodeRedEffect::drawTrails() {
    // Calculate the current breathing brightness multiplier for all trails
    float breathingMultiplier = calculateBreathingBrightness();

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

            // Calculate brightness: tip at 100%, then fade from 70% to 0%
            float brightness;
            if (i == 0) {
                // First LED (tip): 100% brightness
                brightness = 1.0f;
            } else {
                // Remaining LEDs: fade from 70% to 0%
                float fadePosition = (float)(i - 1) / (TRAIL_LENGTH - 1);
                brightness = 0.7f * (1.0f - fadePosition);
            }

            // Apply breathing effect to the brightness
            brightness *= breathingMultiplier;

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

            // Pure red color for all LEDs in the trail
            uint8_t redValue = 255 * brightness;
            CRGB color = CRGB(redValue, 0, 0);

            // Apply fade-to-black mask for outer strips only
            if (trail.stripType == 2) {
                // Calculate position ratio (0.0 at bottom, 1.0 at top)
                float positionRatio = (float)pixelPos / (OUTER_LEDS_PER_STRIP - 1);

                // Apply fade starting at 30% up the strip
                if (positionRatio > 0.3f) {
                    // Calculate fade amount (0.0 at 30%, 1.0 at top)
                    float fadeProgress = (positionRatio - 0.3f) / 0.7f; // 0.7 = 1.0 - 0.3

                    // Apply exponential curve for smoother fade
                    fadeProgress = fadeProgress * fadeProgress; // Square for exponential fade

                    // Calculate fade multiplier (1.0 at 30%, 0.0 at top)
                    float fadeMask = 1.0f - fadeProgress;

                    // Apply fade mask to the color
                    color.r = color.r * fadeMask;
                    color.g = color.g * fadeMask;
                    color.b = color.b * fadeMask;
                }
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

float CodeRedEffect::calculateBrightness(int offset) {
    // Create smooth fade from center (100%) to edges (15%)
    // Center = 100%, edges = 15% (more visible than 10%)
    float brightnessRange = 1.0f - 0.15f;  // 85% range to work with (100% to 15%)
    float distanceRatio = (float)offset / MAX_SIZE;  // 0.0 at center, 1.0 at max distance

    // Start at 100% and reduce to 15% as distance increases
    return 1.0f - (distanceRatio * brightnessRange);
}

void CodeRedEffect::drawPattern(int segment, int centerPos) {
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