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
    lastShimmerUpdate(0),
    lastSparkleUpdate(0),  // Initialize sparkle timing
    whiteWavePosition(-WHITE_WAVE_LENGTH) // Start the wave off-screen
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

    // Allocate memory for ring sparkle values
    ringSparkleValues = new float[LED_STRIP_RING_COUNT];

    // Initialize all sparkle values to 0.0 (no sparkle initially)
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        ringSparkleValues[i] = 0.0f;
    }

    Serial.println("FutureRainbowEffect initialized - rainbow trails with saturation cycling and sparkly ring");
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
    if (ringSparkleValues) {
        delete[] ringSparkleValues;
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
    whiteWavePosition = -WHITE_WAVE_LENGTH; // Reset wave position

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

    // Reset ring sparkle values
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        ringSparkleValues[i] = 0.0f;
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

    // Apply white wave overlay on top of breathing effect
    applyWhiteWaveOverlay();

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
                // Rest of trail fades to white
                float whiteMix = (float)(i - 2) / (trail.trailLength - 2);
                uint8_t white = 255 * fadeRatio;
                color = CRGB(
                    rainbowColor.r * (1.0f - whiteMix) * fadeRatio + white * whiteMix,
                    rainbowColor.g * (1.0f - whiteMix) * fadeRatio + white * whiteMix,
                    rainbowColor.b * (1.0f - whiteMix) * fadeRatio + white * whiteMix
                );
            }

            // Apply the color to the correct strip
            if (trail.stripType == 1) { // Inner strips
                int globalIndex = trail.subStrip * INNER_LEDS_PER_STRIP + pixelPos;
                if (globalIndex < LED_STRIP_INNER_COUNT) {
                    leds.getInner()[globalIndex] = color;
                }
            } else if (trail.stripType == 2) { // Outer strips
                int globalIndex = trail.subStrip * OUTER_LEDS_PER_STRIP + pixelPos;
                if (globalIndex < LED_STRIP_OUTER_COUNT) {
                    leds.getOuter()[globalIndex] = color;
                }
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

void FutureRainbowEffect::applyBreathingEffect() {
    // Update shimmer effect for all strips
    updateShimmer();

    // Calculate base hue for the gradient (0-255 range)
    uint8_t baseHue = (uint8_t)(rainbowPhase * 255);

    // Calculate core breathing intensity using sine wave (predictable)
    float sineValue = sin(breathingPhase);
    float normalizedSine = (sineValue + 1.0f) / 2.0f; // 0.0 to 1.0

    // Apply breathing with shimmer to core strip (0% to 100%) with gradient
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        // Calculate position ratio (0.0 at bottom, 1.0 at top)
        float positionRatio = (float)i / (LED_STRIP_CORE_COUNT - 1);

        // Calculate hue offset for this position (REVERSED - top to bottom)
        uint8_t hueOffset = (uint8_t)((1.0f - positionRatio) * 51); // 20% of 255, reversed
        uint8_t pixelHue = baseHue + hueOffset;

        // Create color for this pixel
        CRGB rainbowColor = CHSV(pixelHue, 255, 255);

        // Apply shimmer and breathing
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

    // Add breathing overlay with shimmer and gradient to all inner strip LEDs
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        // Calculate which segment and position within segment
        int segment = i / INNER_LEDS_PER_STRIP;
        int positionInSegment = i % INNER_LEDS_PER_STRIP;

        // Calculate position ratio (0.0 at bottom, 1.0 at top)
        float positionRatio = (float)positionInSegment / (INNER_LEDS_PER_STRIP - 1);

        // Calculate hue for this position (REVERSED)
        uint8_t hueOffset = (uint8_t)((1.0f - positionRatio) * 51); // 20% of 255, reversed
        uint8_t pixelHue = baseHue + hueOffset;

        // Create rainbow color for this pixel
        CRGB innerRainbowColor = CHSV(pixelHue, 255, 255);

        // Apply shimmer and breathing intensity
        float shimmerMultiplier = innerShimmerValues[i];
        float finalIntensity = innerOuterIntensity * shimmerMultiplier * 1.2f; // Boost by 20%
        finalIntensity = min(0.9f, finalIntensity);

        CRGB innerOverlay = CRGB(
            innerRainbowColor.r * finalIntensity,
            innerRainbowColor.g * finalIntensity,
            innerRainbowColor.b * finalIntensity
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

    // Add unpredictable breathing overlay to outer strips with saturation cycling
    uint8_t outerSaturation = getCurrentOuterSaturation();
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        // Calculate which segment and position within segment
        int segment = i / OUTER_LEDS_PER_STRIP;
        int positionInSegment = i % OUTER_LEDS_PER_STRIP;

        // Calculate position ratio (0.0 at bottom, 1.0 at top)
        float positionRatio = (float)positionInSegment / (OUTER_LEDS_PER_STRIP - 1);

        // Calculate hue for this position (REVERSED)
        uint8_t hueOffset = (uint8_t)((1.0f - positionRatio) * 51); // 20% of 255, reversed
        uint8_t pixelHue = baseHue + hueOffset;

        // Create rainbow color with cycling saturation
        CRGB outerRainbowColor = CHSV(pixelHue, outerSaturation, 255);

        // Apply shimmer and breathing intensity
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

    // Add sparkly breathing effect to ring strip
    updateRingSparkles();

    // Calculate core breathing intensity (10% to 100% for more dramatic effect)
    float ringBreathingIntensity = 0.1f + (normalizedSine * 0.9f); // 10% to 100%

    // Apply sparkles with random gradient colors to ring
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        // Generate a random position in the gradient range for this LED
        float randomPosition = (float)random(100) / 100.0f; // 0.0 to 1.0

        // Calculate hue for this random position (matching the gradient pattern)
        uint8_t hueOffset = (uint8_t)((1.0f - randomPosition) * 51); // 20% of 255, reversed
        uint8_t pixelHue = baseHue + hueOffset;

        // Create rainbow color for this pixel
        CRGB rainbowColor = CHSV(pixelHue, 255, 255);

        // Apply sparkle multiplier AND breathing intensity
        float sparkleMultiplier = ringSparkleValues[i];

        // Make sparkles affected by breathing - they sparkle within the breathing range
        // When breathing is low, even sparkles are dim.
        // When breathing is high, sparkles are bright
        float finalIntensity = ringBreathingIntensity * (0.3f + sparkleMultiplier * 0.7f);
        // This means: minimum 30% of breathing intensity, up to 100% when sparkling

        // Apply the color with sparkle and breathing
        CRGB ringColor = CRGB(
            rainbowColor.r * finalIntensity,
            rainbowColor.g * finalIntensity,
            rainbowColor.b * finalIntensity
        );

        leds.getRing()[i] = ringColor;
    }
}

void FutureRainbowEffect::applyWhiteWaveOverlay() {
    // Update white wave position
    whiteWavePosition += WHITE_WAVE_SPEED;

    // Reset wave when it completely passes off the end of the strip
    if (whiteWavePosition >= LED_STRIP_CORE_COUNT + WHITE_WAVE_LENGTH) {
        whiteWavePosition = -WHITE_WAVE_LENGTH; // Start from before the beginning
    }

    // Apply white wave overlay to core strip
    CRGB* coreStrip = leds.getCore();

    // Calculate the wave start and end positions
    int waveStart = (int)whiteWavePosition;
    int waveEnd = waveStart + WHITE_WAVE_LENGTH;

    // Apply white overlay to LEDs within the wave range
    for (int i = 0; i < WHITE_WAVE_LENGTH; i++) {
        int ledIndex = waveStart + i;

        // Skip if LED is outside the strip bounds
        if (ledIndex < 0 || ledIndex >= LED_STRIP_CORE_COUNT) {
            continue;
        }

        // Calculate wave progress from 0.0 to 1.0 across the wave length
        float waveProgress = (float)i / (WHITE_WAVE_LENGTH - 1);
        float intensity;

        // Create smooth bell curve for natural fading
        if (waveProgress <= 0.5f) {
            // First half - fade in
            float normalizedProgress = waveProgress * 2.0f; // 0.0 to 1.0
            intensity = normalizedProgress;
        } else {
            // Second half - fade out
            float normalizedProgress = (1.0f - waveProgress) * 2.0f; // 1.0 to 0.0
            intensity = normalizedProgress;
        }

        // Apply smooth sine curve for natural bell shape
        intensity = sin(intensity * PI * 0.5f);

        // Get current LED color
        CRGB currentColor = coreStrip[ledIndex];

        // Blend between current color and white for visible but natural effect
        CRGB whiteColor = CRGB::White;
        float blendAmount = intensity * 0.6f; // 60% white blend at peak

        // Create a blend that brightens while adding some white
        coreStrip[ledIndex] = CRGB(
            currentColor.r + (whiteColor.r - currentColor.r) * blendAmount,
            currentColor.g + (whiteColor.g - currentColor.g) * blendAmount,
            currentColor.b + (whiteColor.b - currentColor.b) * blendAmount
        );
    }
}

void FutureRainbowEffect::updateShimmer() {
    unsigned long currentTime = millis();

    // Only update shimmer at specified intervals
    if (currentTime - lastShimmerUpdate < 50) { // SHIMMER_UPDATE_INTERVAL
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

void FutureRainbowEffect::updateRingSparkles() {
    unsigned long currentTime = millis();

    // Only update sparkles at specified intervals
    if (currentTime - lastSparkleUpdate < 50) { // SPARKLE_UPDATE_INTERVAL
        return;
    }

    lastSparkleUpdate = currentTime;

    // Update each LED's sparkle state
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        // Random chance to start a new sparkle (reduced chance)
        if (ringSparkleValues[i] < 0.1f && random(1000) < (SPARKLE_CHANCE * 1000)) {
            // Start a new sparkle at full sparkle value (not full brightness anymore)
            ringSparkleValues[i] = 1.0f;
        } else {
            // Decay existing sparkle
            ringSparkleValues[i] *= SPARKLE_DECAY;

            // Ensure minimum threshold (consider fully faded below 0.01)
            if (ringSparkleValues[i] < 0.01f) {
                ringSparkleValues[i] = 0.0f;
            }
        }
    }
}

void FutureRainbowEffect::updateUnpredictableBreathing() {
    unsigned long currentTime = millis();

    // Randomly change breathing parameters every few seconds
    if (currentTime - lastBreathingChange > 3000) { // BREATHING_CHANGE_INTERVAL
        lastBreathingChange = currentTime;

        // Randomly change breathing speed
        unpredictableBreathingSpeed = 0.005f +
            (random(100) / 100.0f) * (0.02f - 0.005f); // MIN_BREATHING_SPEED to MAX_BREATHING_SPEED

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

    // Clamp to valid range (25% to 90%)
    unpredictableBreathingCurrent = max(0.25f, min(0.9f, unpredictableBreathingCurrent));
}