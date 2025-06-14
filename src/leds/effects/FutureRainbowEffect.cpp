// src/leds/effects/FutureRainbowEffect.cpp

#include "FutureRainbowEffect.h"

FutureRainbowEffect::FutureRainbowEffect(LEDController& ledController) :
    Effect(ledController),
    lastUpdateTime(0),
    rainbowPhase(0.0f),
    saturationPhase(0.0f),
    effectStartTime(millis()),
    breathingPhase(0.0f),
    unpredictableBreathingPhase(0.0f),
    unpredictableBreathingSpeed(0.01f),
    unpredictableBreathingTarget(0.55f),
    unpredictableBreathingCurrent(0.55f),
    lastBreathingChange(0),
    lastShimmerUpdate(0)
{
    // Reserve space for trails to avoid memory reallocations
    trails.reserve(MAX_TRAILS);

    // Initialize all trails as inactive
    for (int i = 0; i < MAX_TRAILS; i++) {
        FutureRainbowTrail trail;
        trail.isActive = false;
        trail.position = 0;
        trail.speed = 0;
        trail.acceleration = 0;
        trail.creationTime = 0;
        trails.push_back(trail);
    }

    // Allocate memory for shimmer values
    coreShimmerValues = new float[LED_STRIP_CORE_COUNT];
    innerShimmerValues = new float[LED_STRIP_INNER_COUNT];
    outerShimmerValues = new float[LED_STRIP_OUTER_COUNT];

    // Initialize all shimmer values to 1.0 (no effect initially)
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        coreShimmerValues[i] = 1.0f;
    }
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        innerShimmerValues[i] = 1.0f;
    }
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        outerShimmerValues[i] = 1.0f;
    }

    Serial.println("FutureRainbowEffect initialized - rainbow trails with saturation cycling");
}

FutureRainbowEffect::~FutureRainbowEffect() {
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
}

void FutureRainbowEffect::reset() {
    // Mark all trails as inactive
    for (auto& trail : trails) {
        trail.isActive = false;
    }

    // Reset phases
    rainbowPhase = 0.0f;
    saturationPhase = 0.0f;
    breathingPhase = 0.0f;
    unpredictableBreathingPhase = 0.0f;
    unpredictableBreathingCurrent = 0.55f;
    unpredictableBreathingTarget = 0.55f;
    lastBreathingChange = millis();
    effectStartTime = millis(); // Reset the start time for rainbow cycle

    // Reset shimmer values
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        coreShimmerValues[i] = 1.0f;
    }
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        innerShimmerValues[i] = 1.0f;
    }
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        outerShimmerValues[i] = 1.0f;
    }

    Serial.println("FutureRainbowEffect reset - all trails cleared");
}

void FutureRainbowEffect::update() {
    // Target 120 FPS for ultra-smooth trail animation
    if (!shouldUpdate(8)) {  // 8ms = 125 FPS
        return;
    }

    // Clear all strips first
    leds.clearAll();

    // Update rainbow phase based on elapsed time (30-second cycle)
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - effectStartTime;
    rainbowPhase = (float)(elapsedTime % (unsigned long)RAINBOW_CYCLE_TIME) / RAINBOW_CYCLE_TIME;

    // Update saturation phase for outer strips (4-second cycle)
    float saturationElapsed = (float)(elapsedTime % (unsigned long)SATURATION_CYCLE_TIME);
    saturationPhase = (saturationElapsed / SATURATION_CYCLE_TIME) * 2.0f * PI;

    // Update breathing phase for core (predictable)
    breathingPhase += BREATHING_SPEED;
    if (breathingPhase > 2.0f * PI) {
        breathingPhase -= 2.0f * PI;
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

CRGB FutureRainbowEffect::getCurrentRainbowColor() {
    // Convert rainbow phase (0.0 to 1.0) to hue (0-255)
    uint8_t hue = (uint8_t)(rainbowPhase * 255);

    // Return full saturation, full brightness rainbow color
    return CHSV(hue, 255, 255);
}

uint8_t FutureRainbowEffect::getCurrentOuterSaturation() {
    // Use sine wave to smoothly cycle between 30% and 100% saturation
    float sineValue = sin(saturationPhase);           // -1.0 to 1.0
    float normalizedSine = (sineValue + 1.0f) / 2.0f; // 0.0 to 1.0

    // Map to saturation range (30% to 100%)
    // 30% = 77 in 0-255 range, 100% = 255
    return (uint8_t)(77 + (normalizedSine * 178));
}

void FutureRainbowEffect::createNewTrail() {
    // Find an inactive trail slot to use
    for (auto& trail : trails) {
        if (!trail.isActive) {
            // Initialize this trail with random properties
            trail.stripType = random(1, 3);  // 1 or 2
            trail.subStrip = random(3);
            trail.position = 0.0f;
            trail.speed = MIN_INITIAL_SPEED +
                         (random(100) / 100.0f) * (MAX_INITIAL_SPEED - MIN_INITIAL_SPEED);
            trail.acceleration = MIN_ACCELERATION +
                               (random(100) / 100.0f) * (MAX_ACCELERATION - MIN_ACCELERATION);
            trail.trailLength = random(MIN_TRAIL_LENGTH, MAX_TRAIL_LENGTH + 1);
            trail.creationTime = rainbowPhase; // Store current rainbow phase when created
            trail.isActive = true;

            return;
        }
    }
}

void FutureRainbowEffect::updateTrails() {
    // Update each active trail
    for (auto& trail : trails) {
        if (!trail.isActive) continue;

        // Apply acceleration to speed
        trail.speed += trail.acceleration;

        // Cap the maximum speed
        if (trail.speed > MAX_SPEED) {
            trail.speed = MAX_SPEED;
        }

        // Move the trail upward
        trail.position += trail.speed;

        // Get strip length to check if trail has gone off the top
        int stripLength = getStripLength(trail.stripType);

        // Deactivate trail if it has completely moved off the strip
        if (trail.position - trail.trailLength >= stripLength) {
            trail.isActive = false;
        }
    }
}

void FutureRainbowEffect::drawTrails() {
    // Draw each active trail
    for (const auto& trail : trails) {
        if (!trail.isActive) continue;

        // Get current rainbow color for this trail
        CRGB rainbowColor = getCurrentRainbowColor();

        // Get strip length for bounds checking
        int stripLength = getStripLength(trail.stripType);

        // Draw the trail with fade effect
        for (int i = 0; i < trail.trailLength; i++) {
            // Calculate position for this pixel
            int pixelPos = (int)(trail.position - i);

            // Skip if pixel is outside strip bounds
            if (pixelPos < 0 || pixelPos >= stripLength) continue;

            // Calculate brightness fade
            float fadeRatio;
            if (i == 0) {
                fadeRatio = 1.0f; // Leading LED - full brightness
            } else if (i == 1) {
                fadeRatio = 0.8f; // Second LED - 80% brightness
            } else if (i == 2) {
                fadeRatio = 0.4f; // Third LED - 40% brightness
            } else {
                // Rest of trail - linear fade from 40% to 0%
                float fadePosition = (float)(i - 3) / (trail.trailLength - 3);
                fadeRatio = 0.4f * (1.0f - fadePosition);
            }

            // Calculate color based on position in trail
            CRGB color;
            if (i == 0 || i == 1) {
                // First two LEDs are rainbow colored for vibrant tip
                if (i == 0) {
                    // First LED - full brightness rainbow with boost
                    color = CRGB(
                        min(255, (int)(rainbowColor.r * 1.2f)),
                        min(255, (int)(rainbowColor.g * 1.2f)),
                        min(255, (int)(rainbowColor.b * 1.2f))
                    );
                } else {
                    // Second LED - 80% brightness rainbow
                    color = CRGB(
                        rainbowColor.r * 0.8f,
                        rainbowColor.g * 0.8f,
                        rainbowColor.b * 0.8f
                    );
                }
            } else {
                // Rest of trail is white with rainbow tint and reduced brightness
                uint8_t brightness = 255 * fadeRatio * 0.6f; // Reduce white brightness by 40%

                // Add rainbow tint to the white trail
                // Mix 30% rainbow color with 70% white
                color = CRGB(
                    (brightness * 0.7f) + (rainbowColor.r * 0.3f * fadeRatio),
                    (brightness * 0.7f) + (rainbowColor.g * 0.3f * fadeRatio),
                    (brightness * 0.7f) + (rainbowColor.b * 0.3f * fadeRatio)
                );
            }

            // Map logical position to physical LED position
            int physicalPos = leds.mapPositionToPhysical(trail.stripType, pixelPos, trail.subStrip);

            // Adjust for segment offset
            if (trail.stripType == 1) {
                physicalPos += trail.subStrip * INNER_LEDS_PER_STRIP;
            } else {
                physicalPos += trail.subStrip * OUTER_LEDS_PER_STRIP;
            }

            // Set the LED color using additive blending
            if (trail.stripType == 1) {
                if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                    leds.getInner()[physicalPos] += color;
                }
            } else {
                if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                    leds.getOuter()[physicalPos] += color;
                }
            }
        }
    }

    // Apply brightness limiting
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        CRGB& pixel = leds.getInner()[i];
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        if (maxComponent > 160) {
            float scale = 160.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }

    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        CRGB& pixel = leds.getOuter()[i];
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        if (maxComponent > 160) {
            float scale = 160.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }
}

void FutureRainbowEffect::applyBreathingEffect() {
    // Update shimmer effect for all strips
    updateShimmer();

    // Get current rainbow color
    CRGB rainbowColor = getCurrentRainbowColor();

    // Calculate core breathing intensity using sine wave (predictable)
    float sineValue = sin(breathingPhase);
    float normalizedSine = (sineValue + 1.0f) / 2.0f; // 0.0 to 1.0

    // Apply breathing with shimmer to core strip (0% to 100%)
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        float shimmerMultiplier = coreShimmerValues[i];
        float finalIntensity = normalizedSine * shimmerMultiplier;
        finalIntensity = min(1.0f, finalIntensity);

        CRGB coreColor = CRGB(
            rainbowColor.r * finalIntensity,
            rainbowColor.g * finalIntensity,
            rainbowColor.b * finalIntensity
        );

        leds.getCore()[i] = coreColor;
    }

    // Apply unpredictable breathing overlay to inner strips (25% to 90%)
    float innerOuterIntensity = unpredictableBreathingCurrent;

    // Add breathing overlay with shimmer to all inner strip LEDs
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        float shimmerMultiplier = innerShimmerValues[i];
        float finalIntensity = innerOuterIntensity * shimmerMultiplier * 1.2f; // Boost by 20%
        finalIntensity = min(0.9f, finalIntensity);

        CRGB innerOverlay = CRGB(
            rainbowColor.r * finalIntensity,
            rainbowColor.g * finalIntensity,
            rainbowColor.b * finalIntensity
        );

        // Blend with existing trail color
        CRGB& pixel = leds.getInner()[i];
        float rainbowWeight = 0.7f;  // 70% rainbow overlay
        float trailWeight = 0.3f;    // 30% original trail

        pixel.r = (pixel.r * trailWeight) + (innerOverlay.r * rainbowWeight);
        pixel.g = (pixel.g * trailWeight) + (innerOverlay.g * rainbowWeight);
        pixel.b = (pixel.b * trailWeight) + (innerOverlay.b * rainbowWeight);

        // Apply brightness limiting
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        if (maxComponent > 240) {
            float scale = 240.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }

    // Add breathing overlay to outer strips WITH SATURATION CYCLING
    uint8_t currentSaturation = getCurrentOuterSaturation();
    CHSV rainbowHSV = rgb2hsv_approximate(rainbowColor);
    rainbowHSV.s = currentSaturation; // Apply the cycling saturation

    // Convert back to RGB with adjusted saturation
    CRGB outerRainbowColor;
    hsv2rgb_rainbow(rainbowHSV, outerRainbowColor);

    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        float shimmerMultiplier = outerShimmerValues[i];
        float finalIntensity = innerOuterIntensity * shimmerMultiplier * 1.2f; // Boost by 20%
        finalIntensity = min(0.9f, finalIntensity);

        CRGB outerOverlay = CRGB(
            outerRainbowColor.r * finalIntensity,
            outerRainbowColor.g * finalIntensity,
            outerRainbowColor.b * finalIntensity
        );

        // Blend with existing trail color
        CRGB& pixel = leds.getOuter()[i];
        float rainbowWeight = 0.7f;  // 70% rainbow overlay
        float trailWeight = 0.3f;    // 30% original trail

        pixel.r = (pixel.r * trailWeight) + (outerOverlay.r * rainbowWeight);
        pixel.g = (pixel.g * trailWeight) + (outerOverlay.g * rainbowWeight);
        pixel.b = (pixel.b * trailWeight) + (outerOverlay.b * rainbowWeight);

        // Apply brightness limiting
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        if (maxComponent > 240) {
            float scale = 240.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }
}

void FutureRainbowEffect::updateUnpredictableBreathing() {
    unsigned long currentTime = millis();

    // Randomly change breathing parameters every few seconds
    if (currentTime - lastBreathingChange > BREATHING_CHANGE_INTERVAL) {
        lastBreathingChange = currentTime;

        // Randomly change breathing speed
        unpredictableBreathingSpeed = MIN_BREATHING_SPEED +
            (random(100) / 100.0f) * (MAX_BREATHING_SPEED - MIN_BREATHING_SPEED);

        // Randomly set a new target brightness (25% to 90%)
        unpredictableBreathingTarget = 0.25f + (random(66) / 100.0f);

        // Occasionally add a "glitch"
        if (random(100) < 20) {
            unpredictableBreathingCurrent = 0.25f + (random(66) / 100.0f);
        }
    }

    // Update breathing phase with current speed
    unpredictableBreathingPhase += unpredictableBreathingSpeed;
    if (unpredictableBreathingPhase > 2.0f * PI) {
        unpredictableBreathingPhase -= 2.0f * PI;
    }

    // Calculate base sine wave
    float sineValue = sin(unpredictableBreathingPhase);
    float normalizedSine = (sineValue + 1.0f) / 2.0f;

    // Mix sine wave with target for unpredictable movement
    float targetInfluence = 0.3f;
    float sineInfluence = 0.7f;

    float desiredBrightness = (unpredictableBreathingTarget * targetInfluence) +
                              ((0.25f + normalizedSine * 0.65f) * sineInfluence);

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

    // Clamp to valid range
    unpredictableBreathingCurrent = max(0.25f, min(0.9f, unpredictableBreathingCurrent));
}

void FutureRainbowEffect::updateShimmer() {
    unsigned long currentTime = millis();

    // Only update shimmer at specified intervals
    if (currentTime - lastShimmerUpdate < SHIMMER_UPDATE_INTERVAL) {
        return;
    }

    lastShimmerUpdate = currentTime;

    // Update shimmer values for each core LED
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        if (random(100) < 50) {
            coreShimmerValues[i] = 0.4f + (random(120) / 100.0f); // 0.4 to 1.6
            if (random(100) < 10) {
                coreShimmerValues[i] = 1.8f + (random(40) / 100.0f); // 1.8 to 2.2 for bright flashes
            }
        } else {
            if (coreShimmerValues[i] < 1.0f) {
                coreShimmerValues[i] += 0.1f;
                if (coreShimmerValues[i] > 1.0f) coreShimmerValues[i] = 1.0f;
            } else if (coreShimmerValues[i] > 1.0f) {
                coreShimmerValues[i] -= 0.1f;
                if (coreShimmerValues[i] < 1.0f) coreShimmerValues[i] = 1.0f;
            }
        }
    }

    // Update shimmer values for inner and outer LEDs (same logic)
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        if (random(100) < 50) {
            innerShimmerValues[i] = 0.4f + (random(120) / 100.0f);
            if (random(100) < 10) {
                innerShimmerValues[i] = 1.8f + (random(40) / 100.0f);
            }
        } else {
            if (innerShimmerValues[i] < 1.0f) {
                innerShimmerValues[i] += 0.1f;
                if (innerShimmerValues[i] > 1.0f) innerShimmerValues[i] = 1.0f;
            } else if (innerShimmerValues[i] > 1.0f) {
                innerShimmerValues[i] -= 0.1f;
                if (innerShimmerValues[i] < 1.0f) innerShimmerValues[i] = 1.0f;
            }
        }
    }

    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        if (random(100) < 50) {
            outerShimmerValues[i] = 0.4f + (random(120) / 100.0f);
            if (random(100) < 10) {
                outerShimmerValues[i] = 1.8f + (random(40) / 100.0f);
            }
        } else {
            if (outerShimmerValues[i] < 1.0f) {
                outerShimmerValues[i] += 0.1f;
                if (outerShimmerValues[i] > 1.0f) outerShimmerValues[i] = 1.0f;
            } else if (outerShimmerValues[i] > 1.0f) {
                outerShimmerValues[i] -= 0.1f;
                if (outerShimmerValues[i] < 1.0f) outerShimmerValues[i] = 1.0f;
            }
        }
    }
}

int FutureRainbowEffect::getStripLength(int stripType) {
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