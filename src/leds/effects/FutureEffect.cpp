// src/leds/effects/FutureEffect.cpp

#include "FutureEffect.h"

FutureEffect::FutureEffect(LEDController& ledController) :
    Effect(ledController),
    lastUpdateTime(0),
    breathingPhase(0.0f),
    unpredictableBreathingPhase(0.0f),
    unpredictableBreathingSpeed(0.01f),
    unpredictableBreathingTarget(0.45f), // Start at middle of range
    unpredictableBreathingCurrent(0.45f),
    lastBreathingChange(0),
    lastShimmerUpdate(0)
{
    // Reserve space for trails to avoid memory reallocations
    trails.reserve(MAX_TRAILS);

    // Initialize all trails as inactive
    for (int i = 0; i < MAX_TRAILS; i++) {
        FutureTrail trail;
        trail.isActive = false;
        trail.position = 0;
        trail.speed = 0;
        trail.acceleration = 0;
        trails.push_back(trail);
    }

    // Allocate memory for core shimmer values
    coreShimmerValues = new float[LED_STRIP_CORE_COUNT];

    // Allocate memory for inner shimmer values
    innerShimmerValues = new float[LED_STRIP_INNER_COUNT];

    // Allocate memory for outer shimmer values
    outerShimmerValues = new float[LED_STRIP_OUTER_COUNT];

    // Initialize all shimmer values to 1.0 (no effect initially)
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        coreShimmerValues[i] = 1.0f;
    }

    // Initialize inner shimmer values
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        innerShimmerValues[i] = 1.0f;
    }

    // Initialize outer shimmer values
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        outerShimmerValues[i] = 1.0f;
    }

    Serial.println("FutureEffect initialized - trails with shimmering core and unpredictable inner/outer breathing");
}

FutureEffect::~FutureEffect() {
    // Clean up allocated shimmer arrays
    if (coreShimmerValues) {
        delete[] coreShimmerValues;
    }
    if (innerShimmerValues) {
        delete[] innerShimmerValues;
    }
    if (outerShimmerValues) {
        delete[] outerShimmerValues;
    }
    // Vector automatically cleans up when destroyed
}

void FutureEffect::reset() {
    // Mark all trails as inactive
    for (auto& trail : trails) {
        trail.isActive = false;
    }

    // Reset breathing phases
    breathingPhase = 0.0f;
    unpredictableBreathingPhase = 0.0f;
    unpredictableBreathingCurrent = 0.45f;
    unpredictableBreathingTarget = 0.45f;
    lastBreathingChange = millis();

    // Reset shimmer values
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        coreShimmerValues[i] = 1.0f;
    }

    // Reset inner shimmer values
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        innerShimmerValues[i] = 1.0f;
    }

    // Reset outer shimmer values
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        outerShimmerValues[i] = 1.0f;
    }

    Serial.println("FutureEffect reset - all trails cleared");
}

void FutureEffect::update() {
    // Target 120 FPS for ultra-smooth trail animation
    if (!shouldUpdate(8)) {  // 8ms = 125 FPS
        return;
    }

    // Clear all strips first
    leds.clearAll();

    // Update breathing phase for core (predictable)
    breathingPhase += BREATHING_SPEED;
    if (breathingPhase > 2.0f * PI) {
        breathingPhase -= 2.0f * PI;  // Keep phase in 0 to 2*PI range
    }

    // Update unpredictable breathing parameters
    updateUnpredictableBreathing();

    // Randomly create new trails
    if (random(100) < TRAIL_CREATE_CHANCE) {
        createNewTrail();
    }

    // Update all active trails (physics and deactivation)
    updateTrails();

    // Draw all active trails
    drawTrails();

    // Apply breathing effect on top of trails
    applyBreathingEffect();

    // Show all the changes
    leds.showAll();
}

void FutureEffect::updateUnpredictableBreathing() {
    unsigned long currentTime = millis();

    // Randomly change breathing parameters every few seconds
    if (currentTime - lastBreathingChange > BREATHING_CHANGE_INTERVAL) {
        lastBreathingChange = currentTime;

        // Randomly change breathing speed
        unpredictableBreathingSpeed = MIN_BREATHING_SPEED +
            (random(100) / 100.0f) * (MAX_BREATHING_SPEED - MIN_BREATHING_SPEED);

        // Randomly set a new target brightness (10% to 80%)
        unpredictableBreathingTarget = 0.1f + (random(71) / 100.0f); // 0.1 to 0.8

        // Occasionally add a "glitch" - sudden jump to random brightness
        if (random(100) < 20) { // 20% chance of glitch
            unpredictableBreathingCurrent = 0.1f + (random(71) / 100.0f);
        }
    }

    // Update breathing phase with current speed
    unpredictableBreathingPhase += unpredictableBreathingSpeed;
    if (unpredictableBreathingPhase > 2.0f * PI) {
        unpredictableBreathingPhase -= 2.0f * PI;
    }

    // Calculate base sine wave
    float sineValue = sin(unpredictableBreathingPhase);
    float normalizedSine = (sineValue + 1.0f) / 2.0f; // 0.0 to 1.0

    // Mix sine wave with target for more unpredictable movement
    float targetInfluence = 0.3f; // How much the target affects the brightness
    float sineInfluence = 0.7f;   // How much the sine wave affects the brightness

    float desiredBrightness = (unpredictableBreathingTarget * targetInfluence) +
                              ((0.1f + normalizedSine * 0.7f) * sineInfluence);

    // Smooth transition to desired brightness
    float transitionSpeed = 0.05f;
    if (unpredictableBreathingCurrent < desiredBrightness) {
        unpredictableBreathingCurrent += transitionSpeed;
        if (unpredictableBreathingCurrent > desiredBrightness) {
            unpredictableBreathingCurrent = desiredBrightness;
        }
    } else if (unpredictableBreathingCurrent > desiredBrightness) {
        unpredictableBreathingCurrent -= transitionSpeed;
        if (unpredictableBreathingCurrent < desiredBrightness) {
            unpredictableBreathingCurrent = desiredBrightness;
        }
    }

    // Clamp to valid range (10% to 80%)
    unpredictableBreathingCurrent = max(0.1f, min(0.8f, unpredictableBreathingCurrent));
}

void FutureEffect::createNewTrail() {
    // Find an inactive trail slot to use
    for (auto& trail : trails) {
        if (!trail.isActive) {
            // Initialize this trail with random properties

            // Randomly choose inner (1) or outer (2) strips
            trail.stripType = random(1, 3);  // 1 or 2

            // Randomly choose which segment (0, 1, or 2)
            trail.subStrip = random(3);

            // Start at the bottom of the strip
            trail.position = 0.0f;

            // Random initial speed (all trails start relatively slow)
            trail.speed = MIN_INITIAL_SPEED +
                         (random(100) / 100.0f) * (MAX_INITIAL_SPEED - MIN_INITIAL_SPEED);

            // Random acceleration (determines how quickly it speeds up)
            trail.acceleration = MIN_ACCELERATION +
                               (random(100) / 100.0f) * (MAX_ACCELERATION - MIN_ACCELERATION);

            // Random trail length
            trail.trailLength = random(MIN_TRAIL_LENGTH, MAX_TRAIL_LENGTH + 1);

            // Activate the trail
            trail.isActive = true;

            // Only create one trail per function call
            return;
        }
    }
}

void FutureEffect::updateTrails() {
    // Update each active trail
    for (auto& trail : trails) {
        if (!trail.isActive) continue;

        // Apply acceleration to speed (physics!)
        trail.speed += trail.acceleration;

        // Cap the maximum speed to prevent trails from becoming too fast
        if (trail.speed > MAX_SPEED) {
            trail.speed = MAX_SPEED;
        }

        // Move the trail upward by its current speed
        trail.position += trail.speed;

        // Get strip length to check if trail has gone off the top
        int stripLength = getStripLength(trail.stripType);

        // Deactivate trail if it has completely moved off the strip
        // (when the tail of the trail is above the strip)
        if (trail.position - trail.trailLength >= stripLength) {
            trail.isActive = false;
        }
    }
}

void FutureEffect::drawTrails() {
    // Draw each active trail
    for (const auto& trail : trails) {
        if (!trail.isActive) continue;

        // Get strip length for bounds checking
        int stripLength = getStripLength(trail.stripType);

        // Draw the trail with fade effect
        for (int i = 0; i < trail.trailLength; i++) {
            // Calculate position for this pixel of the trail
            // Head is at trail.position, tail extends downward
            int pixelPos = (int)(trail.position - i);

            // Skip if pixel is outside strip bounds
            if (pixelPos < 0 || pixelPos >= stripLength) continue;

            // Calculate brightness fade with linear fade
            float fadeRatio;
            if (i == 0) {
                // Leading LED - always full brightness (100%)
                fadeRatio = 1.0f;
            } else if (i == 1) {
                // Second LED - 80% brightness (for blue tip)
                fadeRatio = 0.8f;
            } else if (i == 2) {
                // Third LED - 60% brightness (start of white trail)
                fadeRatio = 0.6f;
            } else {
                // Rest of trail - linear fade from 60% to 0%
                // We have (trailLength - 3) positions to fade over
                float fadePosition = (float)(i - 3) / (trail.trailLength - 3);
                // Linear interpolation from 60% to 0%
                fadeRatio = 0.6f * (1.0f - fadePosition);
            }

            // Calculate color based on position in trail
            CRGB color;
            if (i == 0 || i == 1) {
                // First TWO LEDs are electric blue for more vibrant tip
                uint8_t r = (COLORED_TRAIL_RGB >> 16) & 0xFF;
                uint8_t g = (COLORED_TRAIL_RGB >> 8) & 0xFF;
                uint8_t b = COLORED_TRAIL_RGB & 0xFF;

                if (i == 0) {
                    // First LED - full brightness with boost for extra vibrancy
                    color = CRGB(
                        min(255, (int)(r * 1.2f)),  // Boost blue components slightly
                        min(255, (int)(g * 1.2f)),
                        min(255, (int)(b * 1.2f))
                    );
                } else {
                    // Second LED - 80% brightness blue (instead of white)
                    color = CRGB(r * 0.8f, g * 0.8f, b * 0.8f);
                }
            } else {
                // Rest of trail is white with fade applied
                uint8_t brightness = 255 * fadeRatio;
                color = CRGB(brightness, brightness, brightness);
            }

            // Map logical position to physical LED position
            int physicalPos = leds.mapPositionToPhysical(trail.stripType, pixelPos, trail.subStrip);

            // Adjust for segment offset
            if (trail.stripType == 1) {
                // Inner strips
                physicalPos += trail.subStrip * INNER_LEDS_PER_STRIP;
            } else {
                // Outer strips
                physicalPos += trail.subStrip * OUTER_LEDS_PER_STRIP;
            }

            // Set the LED color using additive blending for overlapping trails
            if (trail.stripType == 1) {
                // Inner strip
                if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                    // Add color to existing color for blending
                    leds.getInner()[physicalPos] += color;
                }
            } else {
                // Outer strip
                if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                    // Add color to existing color for blending
                    leds.getOuter()[physicalPos] += color;
                }
            }
        }
    }

    // Apply brightness limiting to prevent oversaturation when trails overlap
    // This ensures nice color mixing without becoming pure white

    // Limit inner strip brightness
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        CRGB& pixel = leds.getInner()[i];
        // Find the maximum color component
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        // If any component is oversaturated, scale all components down proportionally
        if (maxComponent > 240) {  // Leave some headroom
            float scale = 240.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }

    // Limit outer strip brightness
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        CRGB& pixel = leds.getOuter()[i];
        // Find the maximum color component
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        // If any component is oversaturated, scale all components down proportionally
        if (maxComponent > 240) {  // Leave some headroom
            float scale = 240.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }
}

int FutureEffect::getStripLength(int stripType) {
    // Return the number of LEDs in each strip type
    switch (stripType) {
        case 1:  // Inner strips
            return INNER_LEDS_PER_STRIP;
        case 2:  // Outer strips
            return OUTER_LEDS_PER_STRIP;
        default:
            return 0;
    }
}

void FutureEffect::applyBreathingEffect() {
    // Update shimmer effect for all strips
    updateShimmer();

    // Calculate core breathing intensity using sine wave (predictable)
    float sineValue = sin(breathingPhase);      // -1.0 to 1.0
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Extract electric blue color components
    uint8_t blueR = (COLORED_TRAIL_RGB >> 16) & 0xFF;
    uint8_t blueG = (COLORED_TRAIL_RGB >> 8) & 0xFF;
    uint8_t blueB = COLORED_TRAIL_RGB & 0xFF;

    // Apply breathing with shimmer to core strip (0% to 100%)
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        // Apply shimmer multiplier to create dazzling effect
        float shimmerMultiplier = coreShimmerValues[i];
        float finalIntensity = normalizedSine * shimmerMultiplier;

        // Ensure we don't exceed maximum brightness
        finalIntensity = min(1.0f, finalIntensity);

        CRGB coreColor = CRGB(
            blueR * finalIntensity,
            blueG * finalIntensity,
            blueB * finalIntensity
        );

        leds.getCore()[i] = coreColor;
    }

    // Apply unpredictable breathing overlay to inner strips (10% to 80%)
    float innerOuterIntensity = unpredictableBreathingCurrent;

    // Add breathing overlay with shimmer to all inner strip LEDs
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        // Apply shimmer multiplier to inner strips too
        float shimmerMultiplier = innerShimmerValues[i];
        float finalIntensity = innerOuterIntensity * shimmerMultiplier;

        // Ensure we don't exceed 80% maximum
        finalIntensity = min(0.8f, finalIntensity);

        CRGB innerOverlay = CRGB(
            blueR * finalIntensity,
            blueG * finalIntensity,
            blueB * finalIntensity
        );

        // Add the breathing color to existing color
        leds.getInner()[i] += innerOverlay;

        // Apply same brightness limiting as trails to prevent oversaturation
        CRGB& pixel = leds.getInner()[i];
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        if (maxComponent > 240) {
            float scale = 240.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }

    // Add breathing overlay with shimmer to all outer strip LEDs
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        // Apply shimmer multiplier to outer strips
        float shimmerMultiplier = outerShimmerValues[i];
        float finalIntensity = innerOuterIntensity * shimmerMultiplier;

        // Ensure we don't exceed 80% maximum
        finalIntensity = min(0.8f, finalIntensity);

        CRGB outerOverlay = CRGB(
            blueR * finalIntensity,
            blueG * finalIntensity,
            blueB * finalIntensity
        );

        // Add the breathing color to existing color
        leds.getOuter()[i] += outerOverlay;

        // Apply same brightness limiting as trails to prevent oversaturation
        CRGB& pixel = leds.getOuter()[i];
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        if (maxComponent > 240) {
            float scale = 240.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }
}

void FutureEffect::updateShimmer() {
    unsigned long currentTime = millis();

    // Only update shimmer at specified intervals
    if (currentTime - lastShimmerUpdate < SHIMMER_UPDATE_INTERVAL) {
        return;
    }

    lastShimmerUpdate = currentTime;

    // Update shimmer values for each core LED
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        // Higher chance for each LED to shimmer (50% instead of 30%)
        if (random(100) < 50) {  // 50% chance per frame for each LED to change
            // Create more dramatic shimmer effect with values between 0.4 and 1.6
            // This creates a ±60% brightness variation (much more noticeable)
            coreShimmerValues[i] = 0.4f + (random(120) / 100.0f);  // 0.4 to 1.6

            // Occasionally create super bright flashes (10% chance)
            if (random(100) < 10) {
                coreShimmerValues[i] = 1.8f + (random(40) / 100.0f);  // 1.8 to 2.2 for bright flashes
            }
        } else {
            // Faster return to normal brightness for more active shimmering
            if (coreShimmerValues[i] < 1.0f) {
                coreShimmerValues[i] += 0.1f;  // Faster fade up
                if (coreShimmerValues[i] > 1.0f) coreShimmerValues[i] = 1.0f;
            } else if (coreShimmerValues[i] > 1.0f) {
                coreShimmerValues[i] -= 0.1f;  // Faster fade down
                if (coreShimmerValues[i] < 1.0f) coreShimmerValues[i] = 1.0f;
            }
        }
    }

    // Update shimmer values for each inner LED
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        // Same shimmer behavior as core but for inner strips
        if (random(100) < 50) {  // 50% chance per frame for each LED to change
            // Create shimmer effect with values between 0.4 and 1.6
            innerShimmerValues[i] = 0.4f + (random(120) / 100.0f);  // 0.4 to 1.6

            // Occasionally create super bright flashes (10% chance)
            if (random(100) < 10) {
                innerShimmerValues[i] = 1.8f + (random(40) / 100.0f);  // 1.8 to 2.2 for bright flashes
            }
        } else {
            // Return to normal brightness
            if (innerShimmerValues[i] < 1.0f) {
                innerShimmerValues[i] += 0.1f;  // Fade up
                if (innerShimmerValues[i] > 1.0f) innerShimmerValues[i] = 1.0f;
            } else if (innerShimmerValues[i] > 1.0f) {
                innerShimmerValues[i] -= 0.1f;  // Fade down
                if (innerShimmerValues[i] < 1.0f) innerShimmerValues[i] = 1.0f;
            }
        }
    }

    // Update shimmer values for each outer LED
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        // Same shimmer behavior as core but for outer strips
        if (random(100) < 50) {  // 50% chance per frame for each LED to change
            // Create shimmer effect with values between 0.4 and 1.6
            outerShimmerValues[i] = 0.4f + (random(120) / 100.0f);  // 0.4 to 1.6

            // Occasionally create super bright flashes (10% chance)
            if (random(100) < 10) {
                outerShimmerValues[i] = 1.8f + (random(40) / 100.0f);  // 1.8 to 2.2 for bright flashes
            }
        } else {
            // Return to normal brightness
            if (outerShimmerValues[i] < 1.0f) {
                outerShimmerValues[i] += 0.1f;  // Fade up
                if (outerShimmerValues[i] > 1.0f) outerShimmerValues[i] = 1.0f;
            } else if (outerShimmerValues[i] > 1.0f) {
                outerShimmerValues[i] -= 0.1f;  // Fade down
                if (outerShimmerValues[i] < 1.0f) outerShimmerValues[i] = 1.0f;
            }
        }
    }
}