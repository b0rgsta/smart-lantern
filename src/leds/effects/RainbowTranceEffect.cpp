// src/leds/effects/RainbowTranceEffect.cpp

#include "RainbowTranceEffect.h"

RainbowTranceEffect::RainbowTranceEffect(LEDController& ledController) :
    Effect(ledController),
    currentPhase(GROWING),
    currentSize(0),
    leftPosition(0),
    rightPosition(0),
    lastUpdateTime(0),
    lastTrailCreateTime(0),
    lastRingTrailCreateTime(0),
    breathingPhase(0.0f),
    breathingSpeed(0.02f),      // Slow breathing cycle
    minBrightness(0.4f),        // 40% minimum brightness
    maxBrightness(1.0f),        // 100% maximum brightness
    colorCycleStartTime(0)      // Initialize color cycle timing
{
    // Initialize trails vectors
    trails.reserve(MAX_TRAILS);
    ringTrails.reserve(MAX_RING_TRAILS);

    // Generate initial random core colors (full vibrance)
    generateRandomCoreColor();

    // Initialize color cycle timing
    colorCycleStartTime = millis();

    Serial.println("RainbowTranceEffect created - random colored core grows + random colored trails + color cycling ring");
}

void RainbowTranceEffect::reset() {
    currentPhase = GROWING;
    currentSize = 0;
    leftPosition = 0;
    rightPosition = 0;
    lastUpdateTime = millis();
    lastTrailCreateTime = millis();
    lastRingTrailCreateTime = millis();

    // Generate new random colors for core effect
    generateRandomCoreColor();

    // Reset color cycle timing
    colorCycleStartTime = millis();

    // DON'T clear trails - let them continue independently
    // trails.clear(); // <- REMOVED THIS LINE
    // ringTrails.clear(); // <- Also don't clear ring trails

    Serial.println("RainbowTranceEffect reset to growing phase with new random colors (trails continue)");
}

void RainbowTranceEffect::generateRandomCoreColor() {
    // Cycle through red, green, blue in sequence for core effect
    static int coreColorIndex = 0; // Static variable to remember which color we're on

    switch (coreColorIndex) {
        case 0:
            // Red
            coreHue = 0;    // Red hue in FastLED (0 = red)
            Serial.println("Core color: RED");
            break;
        case 1:
            // Green
            coreHue = 85;   // Green hue in FastLED (85 = green)
            Serial.println("Core color: GREEN");
            break;
        case 2:
            // Blue - using a more vibrant blue
            coreHue = 160;  // More vibrant blue hue in FastLED (160 = full blue)
            Serial.println("Core color: BLUE");
            break;
    }

    // Move to next color for next cycle
    coreColorIndex = (coreColorIndex + 1) % 3;

    // Always use full saturation and brightness for maximum vibrance
    coreSaturation = 255;  // Full saturation = most vibrant colors
    coreBrightness = 255;  // Full brightness = maximum intensity
}

void RainbowTranceEffect::generateRandomTrailColor(RainbowTrail& trail) {
    // Randomly choose red, green, or blue for each trail
    int colorChoice = random(3); // 0, 1, or 2

    switch (colorChoice) {
        case 0:
            // Red
            trail.hue = 0;    // Red hue in FastLED
            break;
        case 1:
            // Green
            trail.hue = 85;   // Green hue in FastLED
            break;
        case 2:
            // Blue - using a more vibrant blue
            trail.hue = 160;  // More vibrant blue hue in FastLED (160 = full blue)
            break;
    }

    // Always use full saturation and brightness for maximum vibrance
    trail.saturation = 255;  // Full saturation = most vibrant colors
    trail.brightness = 255;  // Full brightness = maximum intensity

    // Debug output for trail colors (commented out to reduce spam)
    // String colorNames[] = {"RED", "GREEN", "BLUE"};
    // Serial.print("New trail color: ");
    // Serial.println(colorNames[colorChoice]);
}

uint8_t RainbowTranceEffect::calculateRingCycleHue() {
    // Calculate how much time has passed since the color cycle started
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - colorCycleStartTime;

    // Calculate progress through the 10-second cycle (0.0 to 1.0)
    float cycleProgress = (float)(elapsedTime % COLOR_CYCLE_DURATION) / COLOR_CYCLE_DURATION;

    // Convert progress to hue value (0-255 covers full color wheel)
    uint8_t cycleHue = (uint8_t)(cycleProgress * 255);

    return cycleHue;
}

void RainbowTranceEffect::update() {
    // Clear all strips first
    leds.clearAll();

    // Update breathing phase for trails (synchronized)
    breathingPhase += breathingSpeed;
    if (breathingPhase > 2.0f * PI) {
        breathingPhase -= 2.0f * PI;  // Keep phase in 0 to 2*PI range
    }

    unsigned long currentTime = millis();

    // Update and draw trails first (so core effect can overlap)
    updateTrails();
    drawTrails();

    // Update ring trail effects with color cycling
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

    // Core effect logic with random colors
    if (currentPhase == GROWING) {
        // GROWING PHASE: Grow from 1 to 17 LEDs with current random color
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

                Serial.println("Switching to moving phase - random colored patterns will move in both directions");
            } else {
                Serial.print("Growing to size: ");
                Serial.print(currentSize);
                Serial.print(" (total LEDs: ");
                Serial.print(1 + 2 * currentSize);
                Serial.print(" / 25) with hue: ");
                Serial.println(coreHue);

                lastUpdateTime = currentTime; // Only update timing during normal growth
            }
        }

        if (currentSize <= MAX_SIZE) {
            // Draw growing pattern on all 3 core segments using current random colors
            int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;

            for (int segment = 0; segment < 3; segment++) {
                int segmentCenter = (segment * coreSegmentLength) + (coreSegmentLength / 2);

                // Draw the pattern using the current random colors
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

            // Check if both patterns are completely off the strip
            int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;
            if (leftPosition + MAX_SIZE < 0 && rightPosition - MAX_SIZE >= coreSegmentLength) {
                // Both patterns are off screen, restart ONLY the core effect with new random colors
                Serial.println("Core patterns off screen - restarting core with new random colors");

                // Generate new random colors for the next cycle
                generateRandomCoreColor();

                // Reset only core-related variables, leave trails alone
                currentPhase = GROWING;
                currentSize = 0;
                leftPosition = 0;
                rightPosition = 0;
                lastUpdateTime = currentTime;

                return; // Don't call reset() which would clear trails
            }
        }

        // Always draw both moving patterns using current random colors
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

float RainbowTranceEffect::calculateBreathingBrightness() {
    // Use sine wave to create smooth breathing effect
    // sin() returns -1 to 1, we want to map this to minBrightness to maxBrightness
    float sineValue = sin(breathingPhase);  // -1.0 to 1.0

    // Convert from -1,1 range to 0,1 range
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Map to our desired brightness range (40% to 100%)
    return minBrightness + (normalizedSine * (maxBrightness - minBrightness));
}

float RainbowTranceEffect::calculateRingBreathingBrightness() {
    // Use the same breathing phase as trails but with different brightness range
    // This keeps the ring synchronized with the trail breathing
    float sineValue = sin(breathingPhase);  // -1.0 to 1.0

    // Convert from -1,1 range to 0,1 range
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Map to ring-specific brightness range (15% to 100%)
    return RING_MIN_BRIGHTNESS + (normalizedSine * (RING_MAX_BRIGHTNESS - RING_MIN_BRIGHTNESS));
}

void RainbowTranceEffect::updateRingTrails() {
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

        // Check if trail should start fading or be deactivated
        if (!trail.isFading && (currentTime - trail.creationTime >= trail.lifespan)) {
            // Start fade-out phase
            trail.isFading = true;
            trail.fadeStartTime = currentTime;
        } else if (trail.isFading && (currentTime - trail.fadeStartTime >= RainbowRingTrail::FADE_DURATION)) {
            // Fade-out complete, deactivate trail
            trail.active = false;
        }
    }

    // Remove inactive ring trails
    ringTrails.erase(
        std::remove_if(ringTrails.begin(), ringTrails.end(),
            [](const RainbowRingTrail& t) { return !t.active; }),
        ringTrails.end());

    // Draw all active ring trails with color cycling
    drawRingTrails();
}

void RainbowTranceEffect::createNewRingTrail() {
    // Don't create more ring trails if we're at maximum
    if (ringTrails.size() >= MAX_RING_TRAILS) {
        return;
    }

    RainbowRingTrail newTrail;

    // Random starting position around the ring
    newTrail.position = random(LED_STRIP_RING_COUNT);

    // Random direction (clockwise or counter-clockwise)
    newTrail.clockwise = random(2) == 1;

    // Random speed (slower than linear trails for smooth circular motion)
    newTrail.speed = 0.08f + (random(100) / 100.0f) * 0.12f; // 0.08 to 0.20 speed range

    // Set trail length
    newTrail.length = RING_TRAIL_LENGTH;

    // Randomly choose red, green, or blue for each ring trail
    int colorChoice = random(3); // 0, 1, or 2

    switch (colorChoice) {
        case 0:
            // Red
            newTrail.hue = 0;    // Red hue in FastLED
            break;
        case 1:
            // Green
            newTrail.hue = 85;   // Green hue in FastLED
            break;
        case 2:
            // Blue - using a more vibrant blue
            newTrail.hue = 160;  // More vibrant blue hue in FastLED (160 = full blue)
            break;
    }

    // Set creation time and random lifespan (8-15 seconds for nice variety)
    newTrail.creationTime = millis();
    newTrail.lifespan = 8000 + random(7000); // 8000ms to 15000ms (8-15 seconds)

    // Initialize fade-out variables
    newTrail.isFading = false;
    newTrail.fadeStartTime = 0;

    // Activate the trail
    newTrail.active = true;

    // Add to ring trails vector
    ringTrails.push_back(newTrail);
}

void RainbowTranceEffect::drawRingTrails() {
    // Clear the ring first
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        leds.getRing()[i] = CRGB::Black;
    }

    // Calculate the current breathing brightness multiplier for all ring trails
    float breathingMultiplier = calculateRingBreathingBrightness();

    for (const auto& trail : ringTrails) {
        if (!trail.active) continue;

        // Convert this trail's hue to RGB at full saturation and brightness first
        CHSV baseHSV(trail.hue, 255, 255);
        CRGB baseRGB;
        hsv2rgb_rainbow(baseHSV, baseRGB);

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

            // Apply fade-out if trail is fading
            if (trail.isFading) {
                unsigned long currentTime = millis();
                float fadeProgress = (float)(currentTime - trail.fadeStartTime) / RainbowRingTrail::FADE_DURATION;
                fadeProgress = min(1.0f, fadeProgress); // Clamp to 1.0
                float fadeMultiplier = 1.0f - fadeProgress; // 1.0 to 0.0 over fade duration
                brightness *= fadeMultiplier;
            }

            // Apply brightness directly to RGB components to avoid HSV blending issues
            CRGB color = CRGB(
                baseRGB.r * brightness,
                baseRGB.g * brightness,
                baseRGB.b * brightness
            );

            // Add the color to the existing pixel (in case trails overlap)
            leds.getRing()[pixelPos] += color;
        }
    }
}

void RainbowTranceEffect::createNewTrail() {
    // Don't create more trails if we're at maximum
    if (trails.size() >= MAX_TRAILS) {
        return;
    }

    RainbowTrail newTrail;

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

    // Generate random colors for this trail
    generateRandomTrailColor(newTrail);

    // Add to trails vector
    trails.push_back(newTrail);
}

void RainbowTranceEffect::updateTrails() {
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
            [](const RainbowTrail& t) { return !t.active; }),
        trails.end());
}

void RainbowTranceEffect::drawTrails() {
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

            // Calculate brightness with more obvious fade to black
            // Use exponential fade for more dramatic effect
            float brightness = 1.0f - ((float)i / TRAIL_LENGTH);
            brightness = brightness * brightness; // Square the brightness for more dramatic fade

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

            // Set color with pure RGB at head, fading to black (no white)
            CRGB color;

            // Calculate brightness fade from head (100%) to tail (0%)
            float trailBrightness = 1.0f - ((float)i / TRAIL_LENGTH);
            trailBrightness = trailBrightness * trailBrightness; // Square for more dramatic fade

            // Apply breathing effect to the brightness
            trailBrightness *= breathingMultiplier;

            // Use the trail's RGB color throughout the entire trail, just varying brightness
            CHSV hsvColor(trail.hue, trail.saturation, trail.brightness * trailBrightness);
            hsv2rgb_rainbow(hsvColor, color);

            // Set the LED with color blending when trails overlap
            if (trail.stripType == 1) {
                // Inner strips
                if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                    // Add colors together for blending when trails overlap
                    leds.getInner()[physicalPos] += color;
                }
            } else {
                // Outer strips
                if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                    // Add colors together for blending when trails overlap
                    leds.getOuter()[physicalPos] += color;
                }
            }
        }
    }
}

float RainbowTranceEffect::calculateBrightness(int offset) {
    // Create smooth fade from center (100%) to edges (15%)
    // Center = 100%, edges = 15% (more visible than 10%)
    float brightnessRange = 1.0f - 0.15f;  // 85% range to work with (100% to 15%)
    float distanceRatio = (float)offset / MAX_SIZE;  // 0.0 at center, 1.0 at max distance

    // Start at 100% and reduce to 15% as distance increases
    return 1.0f - (distanceRatio * brightnessRange);
}

void RainbowTranceEffect::drawPattern(int segment, int centerPos) {
    int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;
    int segmentStart = segment * coreSegmentLength;
    int segmentEnd = segmentStart + coreSegmentLength - 1;

    // Convert core HSV color to RGB for drawing
    CHSV coreHSV(coreHue, coreSaturation, coreBrightness);
    CRGB coreRGB;
    hsv2rgb_rainbow(coreHSV, coreRGB);

    // Draw center LED in current random color at 100% brightness
    if (centerPos >= segmentStart && centerPos <= segmentEnd) {
        // Use max instead of add to prevent brightness increase from overlapping
        CRGB currentColor = leds.getCore()[centerPos];
        leds.getCore()[centerPos] = CRGB(
            max(currentColor.r, coreRGB.r),
            max(currentColor.g, coreRGB.g),
            max(currentColor.b, coreRGB.b)
        );
    }

    // Draw LEDs on both sides with brightness fade using current random color
    // Use currentSize during growing phase, MAX_SIZE during moving phase
    int drawSize = (currentPhase == GROWING) ? currentSize : MAX_SIZE;

    for (int offset = 1; offset <= drawSize; offset++) {
        float brightness = calculateBrightness(offset);

        // Apply brightness to current random color
        CRGB fadedColor = CRGB(
            coreRGB.r * brightness,
            coreRGB.g * brightness,
            coreRGB.b * brightness
        );

        // Left side LED
        int leftPos = centerPos - offset;
        if (leftPos >= segmentStart && leftPos <= segmentEnd) {
            // Use max instead of add to prevent brightness increase from overlapping
            CRGB currentColor = leds.getCore()[leftPos];
            leds.getCore()[leftPos] = CRGB(
                max(currentColor.r, fadedColor.r),
                max(currentColor.g, fadedColor.g),
                max(currentColor.b, fadedColor.b)
            );
        }

        // Right side LED
        int rightPos = centerPos + offset;
        if (rightPos >= segmentStart && rightPos <= segmentEnd) {
            // Use max instead of add to prevent brightness increase from overlapping
            CRGB currentColor = leds.getCore()[rightPos];
            leds.getCore()[rightPos] = CRGB(
                max(currentColor.r, fadedColor.r),
                max(currentColor.g, fadedColor.g),
                max(currentColor.b, fadedColor.b)
            );
        }
    }
}