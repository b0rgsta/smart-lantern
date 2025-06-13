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
    breathingPhase(0.0f),
    breathingSpeed(0.02f),      // Slow breathing cycle
    minBrightness(0.4f),        // 40% minimum brightness
    maxBrightness(1.0f)         // 100% maximum brightness
{
    // Initialize synchronized trails vector with larger capacity
    syncedTrails.reserve(MAX_TRAILS);

    // Generate initial random core colors (full vibrance)
    generateRandomCoreColor();

    // Initialize the 3 continuous ring trails
    initializeRingTrails();

    Serial.println("RainbowTranceEffect created - random colored core + synchronized inner/outer trails + 3 RGB ring trails");
}

void RainbowTranceEffect::reset() {
    currentPhase = GROWING;
    currentSize = 0;
    leftPosition = 0;
    rightPosition = 0;
    lastUpdateTime = millis();
    lastTrailCreateTime = millis();

    // Generate new random colors for core effect
    generateRandomCoreColor();

    // DON'T clear trails - let them continue independently
    // syncedTrails.clear(); // <- REMOVED THIS LINE

    // Ring trails continue without reset - they're continuous

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

void RainbowTranceEffect::generateRandomTrailColor(SyncedTrail& trail) {
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
}

void RainbowTranceEffect::initializeRingTrails() {
    // Initialize 3 continuous trails with fixed colors and positions
    for (int i = 0; i < NUM_RING_TRAILS; i++) {
        // Equally space the trails around the ring
        ringTrails[i].position = (float)i * LED_STRIP_RING_COUNT / NUM_RING_TRAILS;

        // All trails move at the same speed, clockwise
        ringTrails[i].speed = 0.15f;  // Moderate speed for continuous movement
        ringTrails[i].clockwise = true;

        // Set trail length
        ringTrails[i].length = RING_TRAIL_LENGTH;

        // Assign fixed colors: Red, Green, Blue
        switch (i) {
            case 0:
                ringTrails[i].hue = 0;    // Red
                break;
            case 1:
                ringTrails[i].hue = 85;   // Green
                break;
            case 2:
                ringTrails[i].hue = 160;  // Blue
                break;
        }
    }

    Serial.println("Initialized 3 continuous RGB ring trails");
}

float RainbowTranceEffect::calculateBreathingBrightness() {
    // Use sine wave to create smooth breathing effect
    float sineValue = sin(breathingPhase);  // -1.0 to 1.0

    // Convert from -1,1 range to 0,1 range
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Map to our desired brightness range (40% to 100%)
    return minBrightness + (normalizedSine * (maxBrightness - minBrightness));
}

float RainbowTranceEffect::calculateRingBreathingBrightness() {
    // Use the same breathing phase as trails but with different brightness range
    float sineValue = sin(breathingPhase);  // -1.0 to 1.0

    // Convert from -1,1 range to 0,1 range
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Map to ring-specific brightness range (15% to 100%)
    return RING_MIN_BRIGHTNESS + (normalizedSine * (RING_MAX_BRIGHTNESS - RING_MIN_BRIGHTNESS));
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

    // Update and draw synchronized trails first (so core effect can overlap)
    updateSyncedTrails();
    drawSyncedTrails();

    // Update continuous ring trails
    updateRingTrails();

    // Create new synchronized trails with more aggressive creation
    int activeTrails = 0;
    for (const auto& trail : syncedTrails) {
        if (trail.active) activeTrails++;
    }

    // Calculate dynamic interval with randomness to prevent synchronized waves
    int createInterval = TRAIL_CREATE_INTERVAL + random(-TRAIL_STAGGER_VARIANCE, TRAIL_STAGGER_VARIANCE);

    // Always try to maintain target trails with frequent creation
    if (activeTrails < TARGET_TRAILS && (currentTime - lastTrailCreateTime >= createInterval)) {
        createNewSyncedTrail();
        lastTrailCreateTime = currentTime;
    }

    // More aggressive creation if we're well below target
    if (activeTrails < TARGET_TRAILS * 0.7) {
        // Create multiple trails rapidly when low
        for (int i = 0; i < 2; i++) {
            createNewSyncedTrail();
        }
        lastTrailCreateTime = currentTime;
    }

    // Emergency creation if we're very low
    if (activeTrails < TARGET_TRAILS * 0.4) {
        // Create even more trails when very low
        for (int i = 0; i < 3; i++) {
            createNewSyncedTrail();
        }
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
                int segmentCenter = coreSegmentLength / 2;
                leftPosition = segmentCenter;
                rightPosition = segmentCenter;

                Serial.println("Switching to moving phase - random colored patterns will move in both directions");
            } else {
                lastUpdateTime = currentTime;
            }
        }

        if (currentSize <= MAX_SIZE) {
            // Draw growing pattern on all 3 core segments
            int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;

            for (int segment = 0; segment < 3; segment++) {
                int segmentCenter = (segment * coreSegmentLength) + (coreSegmentLength / 2);
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

void RainbowTranceEffect::updateRingTrails() {
    // Skip ring if button feedback is active
    if (skipRing) {
        return;
    }

    // Update positions of all 3 continuous trails
    for (int i = 0; i < NUM_RING_TRAILS; i++) {
        // Move the trail around the ring
        ringTrails[i].position += ringTrails[i].speed;

        // Wrap around when we reach the end
        if (ringTrails[i].position >= LED_STRIP_RING_COUNT) {
            ringTrails[i].position -= LED_STRIP_RING_COUNT;
        }
    }

    // Draw all 3 continuous ring trails
    drawRingTrails();
}

void RainbowTranceEffect::drawRingTrails() {
    // Clear the ring first
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        leds.getRing()[i] = CRGB::Black;
    }

    // Calculate the current breathing brightness multiplier for all ring trails
    float breathingMultiplier = calculateRingBreathingBrightness();

    // Draw each of the 3 continuous trails
    for (int t = 0; t < NUM_RING_TRAILS; t++) {
        const auto& trail = ringTrails[t];

        // Convert trail's fixed hue to RGB at full saturation and brightness
        CHSV baseHSV(trail.hue, 255, 255);
        CRGB baseRGB;
        hsv2rgb_rainbow(baseHSV, baseRGB);

        // Draw the trail
        for (int i = 0; i < trail.length; i++) {
            // Calculate position for this part of the trail
            int pixelPos = (int)trail.position - i;

            // Handle wrapping around the ring
            while (pixelPos < 0) {
                pixelPos += LED_STRIP_RING_COUNT;
            }

            // Calculate brightness with fade
            float brightness = 1.0f - ((float)i / trail.length);
            brightness = brightness * brightness; // Square for more dramatic fade
            brightness *= breathingMultiplier; // Apply breathing effect

            // Apply brightness directly to RGB components
            CRGB color = CRGB(
                baseRGB.r * brightness,
                baseRGB.g * brightness,
                baseRGB.b * brightness
            );

            // Add the color to the existing pixel (allows overlapping trails to blend)
            leds.getRing()[pixelPos] += color;
        }
    }
}

void RainbowTranceEffect::createNewSyncedTrail() {
    // Don't create more trails if we're at maximum
    if (syncedTrails.size() >= MAX_TRAILS) {
        return;
    }

    SyncedTrail newTrail;

    // Randomly choose inner (1) or outer (2) strips
    newTrail.stripType = random(1, 3);

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

    // More varied speeds for interesting interactions when trails overlap
    float baseSpeed = 0.10f + (random(100) / 100.0f) * 0.25f; // Wider speed range: 0.10 to 0.35
    float speedVariance = (random(100) / 100.0f) * 0.05f - 0.025f; // ±0.025 variance
    newTrail.speed = baseSpeed + speedVariance;
    newTrail.active = true;

    // Generate random colors for this trail
    generateRandomTrailColor(newTrail);

    // Add to trails vector
    syncedTrails.push_back(newTrail);
}

void RainbowTranceEffect::updateSyncedTrails() {
    // Update each synchronized trail
    for (auto& trail : syncedTrails) {
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
    syncedTrails.erase(
        std::remove_if(syncedTrails.begin(), syncedTrails.end(),
            [](const SyncedTrail& t) { return !t.active; }),
        syncedTrails.end());
}

void RainbowTranceEffect::drawSyncedTrails() {
    // Calculate the current breathing brightness multiplier for all trails
    float breathingMultiplier = calculateBreathingBrightness();

    // First pass: draw all trails (they will blend automatically with += operator)
    for (const auto& trail : syncedTrails) {
        if (!trail.active) continue;

        // Get strip length
        int stripLength = (trail.stripType == 1) ? INNER_LEDS_PER_STRIP : OUTER_LEDS_PER_STRIP;

        // Draw the trail on ALL segments of this strip type
        int numSegments = (trail.stripType == 1) ? NUM_INNER_STRIPS : NUM_OUTER_STRIPS;

        for (int segment = 0; segment < numSegments; segment++) {
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

                // Skip if pixel is outside strip bounds
                if (pixelPos < 0 || pixelPos >= stripLength) continue;

                // Calculate brightness with fade
                float brightness = 1.0f - ((float)i / TRAIL_LENGTH);
                brightness = brightness * brightness; // Square for dramatic fade

                // Apply breathing effect to the brightness
                brightness *= breathingMultiplier;

                // Reduce overall brightness slightly to prevent oversaturation when trails overlap
                brightness *= 0.7f; // Reduce to 70% to allow better color mixing

                // Physical position calculation
                int physicalPos = leds.mapPositionToPhysical(trail.stripType, pixelPos, segment);

                // Adjust for segment offset
                if (trail.stripType == 1) {
                    // Inner strips
                    physicalPos += segment * INNER_LEDS_PER_STRIP;
                } else {
                    // Outer strips
                    physicalPos += segment * OUTER_LEDS_PER_STRIP;
                }

                // Use the trail's RGB color throughout the entire trail
                CHSV hsvColor(trail.hue, trail.saturation, trail.brightness * brightness);
                CRGB color;
                hsv2rgb_rainbow(hsvColor, color);

                // Add the color to blend with existing colors when trails overlap
                if (trail.stripType == 1) {
                    // Inner strips
                    if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                        // Use additive blending for color mixing
                        leds.getInner()[physicalPos] += color;
                    }
                } else {
                    // Outer strips
                    if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                        // Use additive blending for color mixing
                        leds.getOuter()[physicalPos] += color;
                    }
                }
            }
        }
    }

    // Second pass: Apply brightness limiting to prevent oversaturation
    // This ensures overlapping trails create nice color blends without becoming pure white
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        CRGB& pixel = leds.getInner()[i];
        // Limit maximum brightness while preserving color ratios
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        if (maxComponent > 200) {
            float scale = 200.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }

    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        CRGB& pixel = leds.getOuter()[i];
        // Limit maximum brightness while preserving color ratios
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        if (maxComponent > 200) {
            float scale = 200.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }
}

float RainbowTranceEffect::calculateBrightness(int offset) {
    // Create smooth fade from center (100%) to edges (15%)
    float brightnessRange = 1.0f - 0.15f;
    float distanceRatio = (float)offset / MAX_SIZE;

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
        CRGB currentColor = leds.getCore()[centerPos];
        leds.getCore()[centerPos] = CRGB(
            max(currentColor.r, coreRGB.r),
            max(currentColor.g, coreRGB.g),
            max(currentColor.b, coreRGB.b)
        );
    }

    // Draw LEDs on both sides with brightness fade
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
            CRGB currentColor = leds.getCore()[rightPos];
            leds.getCore()[rightPos] = CRGB(
                max(currentColor.r, fadedColor.r),
                max(currentColor.g, fadedColor.g),
                max(currentColor.b, fadedColor.b)
            );
        }
    }
}