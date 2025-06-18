// src/leds/effects/EmeraldCityEffect.cpp

#include "EmeraldCityEffect.h"

EmeraldCityEffect::EmeraldCityEffect(LEDController& ledController) :
    Effect(ledController),
    lastSparkleUpdate(0),
    lastUpdateTime(0),
    coreWavePosition(0.0f)  // Initialize wave position
{
    // Initialize green color palette with various shades of green
    initializeGreenPalette();

    // Initialize trails for each inner strip
    for (int i = 0; i < NUM_INNER_STRIPS; i++) {
        innerTrails[i].resize(MAX_TRAILS_PER_STRIP);
        // Set all trails as inactive initially
        for (auto& trail : innerTrails[i]) {
            trail.isActive = false;
        }
    }

    // Initialize trails for each outer strip
    for (int i = 0; i < NUM_OUTER_STRIPS; i++) {
        outerTrails[i].resize(MAX_TRAILS_PER_STRIP);
        // Set all trails as inactive initially
        for (auto& trail : outerTrails[i]) {
            trail.isActive = false;
        }
    }

    // Allocate memory for sparkle arrays
    innerSparkleValues = new float[LED_STRIP_INNER_COUNT];
    outerSparkleValues = new float[LED_STRIP_OUTER_COUNT];
    ringSparkleValues = new float[LED_STRIP_RING_COUNT];

    // Allocate memory for sparkle color arrays
    innerSparkleColors = new uint8_t[LED_STRIP_INNER_COUNT];
    outerSparkleColors = new uint8_t[LED_STRIP_OUTER_COUNT];
    ringSparkleColors = new uint8_t[LED_STRIP_RING_COUNT];

    // Allocate memory for sparkle brightness arrays
    innerSparkleBrightness = new float[LED_STRIP_INNER_COUNT];
    outerSparkleBrightness = new float[LED_STRIP_OUTER_COUNT];
    ringSparkleBrightness = new float[LED_STRIP_RING_COUNT];

    // Allocate memory for sparkle speed arrays
    innerSparkleSpeed = new float[LED_STRIP_INNER_COUNT];
    outerSparkleSpeed = new float[LED_STRIP_OUTER_COUNT];
    ringSparkleSpeed = new float[LED_STRIP_RING_COUNT];

    // Initialize all sparkle values to 0.0 (no sparkle initially)
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        innerSparkleValues[i] = 0.0f;
        innerSparkleColors[i] = random(2);  // Random color (0=white, 1=light green)
        innerSparkleBrightness[i] = 0.2f + (random(80) / 100.0f);  // Random brightness 20% to 100%
        // Random speed: 50% to 200% of base speed
        innerSparkleSpeed[i] = BASE_SPARKLE_SPEED * (MIN_SPEED_MULTIPLIER + (random(150) / 100.0f));
    }
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        outerSparkleValues[i] = 0.0f;
        outerSparkleColors[i] = random(2);  // Random color
        outerSparkleBrightness[i] = 0.2f + (random(80) / 100.0f);  // Random brightness 20% to 100%
        // Random speed: 50% to 200% of base speed
        outerSparkleSpeed[i] = BASE_SPARKLE_SPEED * (MIN_SPEED_MULTIPLIER + (random(150) / 100.0f));
    }
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        ringSparkleValues[i] = 0.0f;
        // Ring sparkles: 75% green, 25% white
        ringSparkleColors[i] = (random(100) < 75) ? 1 : 0;  // 75% chance for green (1), 25% for white (0)
        ringSparkleBrightness[i] = 0.2f + (random(80) / 100.0f);  // Random brightness 20% to 100%
        // Ring sparkles are slower (twice as long): 25% to 100% of base speed
        ringSparkleSpeed[i] = BASE_SPARKLE_SPEED * RING_SPEED_MULTIPLIER * (MIN_SPEED_MULTIPLIER + (random(100) / 100.0f));
    }

    Serial.println("EmeraldCityEffect created - green trails with white sparkles");

    // Start with some trails already in motion for immediate visual effect
    initializeStartupTrails();
}

EmeraldCityEffect::~EmeraldCityEffect() {
    // Clean up allocated memory
    delete[] innerSparkleValues;
    delete[] outerSparkleValues;
    delete[] ringSparkleValues;
    delete[] innerSparkleColors;
    delete[] outerSparkleColors;
    delete[] ringSparkleColors;
    delete[] innerSparkleBrightness;
    delete[] outerSparkleBrightness;
    delete[] ringSparkleBrightness;
    delete[] innerSparkleSpeed;
    delete[] outerSparkleSpeed;
    delete[] ringSparkleSpeed;
}

void EmeraldCityEffect::initializeGreenPalette() {
    // Create a palette focused on blue-green tones (even more blue-dominant)
    // FastLED uses 0-255 for hue, pushing further toward blue-green/cyan spectrum
    greenHues[0] = 110;  // Deep blue-green (cyan-emerald)
    greenHues[1] = 105;  // Blue-green (ocean teal)
    greenHues[2] = 115;  // Deeper blue-green (cyan-dominant)
    greenHues[3] = 108;  // Teal-blue (blue-leaning emerald)
    greenHues[4] = 112;  // Dark cyan-emerald (sophisticated blue-green)
    greenHues[5] = 107;  // Ocean blue-green (blue-dominant teal)
}

void EmeraldCityEffect::initializeStartupTrails() {
    // Create some initial trails at various positions for immediate visual effect
    // This ensures the effect starts with green trails already visible

    // Initialize trails for inner strips
    for (int stripIndex = 0; stripIndex < NUM_INNER_STRIPS; stripIndex++) {
        // Create 3-5 trails per strip at startup
        int numStartupTrails = 3 + random(3);  // 3 to 5 trails

        for (int trailIndex = 0; trailIndex < numStartupTrails && trailIndex < MAX_TRAILS_PER_STRIP; trailIndex++) {
            EmeraldTrail& trail = innerTrails[stripIndex][trailIndex];
            trail.isActive = true;
            trail.stripType = 1;  // Inner strip
            trail.subStrip = stripIndex;

            // Random position throughout the strip height
            int stripLength = getStripLength(1);
            trail.position = random(stripLength * 0.2f, stripLength * 0.8f);  // 20% to 80% up the strip

            // Random speed within normal range
            trail.speed = MIN_TRAIL_SPEED + (random(100) / 100.0f) * (MAX_TRAIL_SPEED - MIN_TRAIL_SPEED);
            trail.greenHue = getRandomGreenHue();
            trail.brightness = TRAIL_BRIGHTNESS + random(75);  // Add brightness variation
        }
    }

    // Initialize trails for outer strips
    for (int stripIndex = 0; stripIndex < NUM_OUTER_STRIPS; stripIndex++) {
        // Create 2-4 trails per strip at startup (slightly fewer than inner)
        int numStartupTrails = 2 + random(3);  // 2 to 4 trails

        for (int trailIndex = 0; trailIndex < numStartupTrails && trailIndex < MAX_TRAILS_PER_STRIP; trailIndex++) {
            EmeraldTrail& trail = outerTrails[stripIndex][trailIndex];
            trail.isActive = true;
            trail.stripType = 2;  // Outer strip
            trail.subStrip = stripIndex;

            // Random position throughout the strip height
            int stripLength = getStripLength(2);
            trail.position = random(stripLength * 0.2f, stripLength * 0.8f);  // 20% to 80% up the strip

            // Random speed within normal range
            trail.speed = MIN_TRAIL_SPEED + (random(100) / 100.0f) * (MAX_TRAIL_SPEED - MIN_TRAIL_SPEED);
            trail.greenHue = getRandomGreenHue();
            trail.brightness = TRAIL_BRIGHTNESS + random(75);  // Add brightness variation
        }
    }
}

void EmeraldCityEffect::reset() {
    // Mark all trails as inactive
    for (int i = 0; i < NUM_INNER_STRIPS; i++) {
        for (auto& trail : innerTrails[i]) {
            trail.isActive = false;
        }
    }

    for (int i = 0; i < NUM_OUTER_STRIPS; i++) {
        for (auto& trail : outerTrails[i]) {
            trail.isActive = false;
        }
    }

    // Reset sparkle values
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        innerSparkleValues[i] = 0.0f;
    }
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        outerSparkleValues[i] = 0.0f;
    }
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        ringSparkleValues[i] = 0.0f;
    }

    // Reset wave position
    coreWavePosition = 0.0f;

    Serial.println("EmeraldCityEffect reset");
}

void EmeraldCityEffect::update() {
    // Target smooth frame rate (~60 FPS)
    if (!shouldUpdate(16)) {  // 16ms = ~62 FPS
        return;
    }

    // Clear all strips before drawing
    leds.clearAll();

    // Update and draw green trails on inner and outer strips
    updateInnerTrails();
    updateOuterTrails();

    // Apply glowing green overlay to ring strip (before sparkles)
    applyRingGreenOverlay();

    // Update white sparkle effects (sparkles go on top of the green glow)
    updateSparkles();

    // Apply fade-to-black overlay to outer strips for ambient effect
    applyOuterFadeOverlay();

    // Apply blue wave effect to core strip
    applyCoreWaveEffect();

    // Apply moving black fade to inner strips that follows the core wave
    applyInnerWaveFade();

    // Show all LED updates
    leds.showAll();
}

void EmeraldCityEffect::applyRingGreenOverlay() {
    // Skip ring updates if button feedback is active to avoid conflicts
    if (skipRing) {
        return;
    }

    // Apply a soft, glowing green overlay to the entire ring strip
    // This creates a base green glow underneath the white/green sparkles

    // Use a time-based gentle breathing effect for the glow
    unsigned long currentTime = millis();
    float breathingPhase = (currentTime * 0.0008f);  // Slow breathing cycle (0.8ms per increment)

    // Create a gentle breathing pattern using sine wave (0.3 to 0.8 intensity)
    float breathingIntensity = 0.3f + 0.5f * (0.5f + 0.5f * sin(breathingPhase));

    // Use a pleasant emerald green hue from our palette
    uint8_t greenHue = 110;  // Deep blue-green (cyan-emerald) from our palette
    uint8_t saturation = 200;  // Rich but not oversaturated
    uint8_t brightness = (uint8_t)(255 * breathingIntensity);

    // Convert HSV to RGB for the base glow color
    CHSV glowHSV(greenHue, saturation, brightness);
    CRGB glowColor;
    hsv2rgb_rainbow(glowHSV, glowColor);

    // Apply the glow to all ring LEDs
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        // Set the base green glow color
        // Sparkles will be added on top of this in updateSparkles()
        leds.getRing()[i] = glowColor;
    }
}

void EmeraldCityEffect::updateInnerTrails() {
    // Update trails for each inner strip
    for (int i = 0; i < NUM_INNER_STRIPS; i++) {
        updateStripTrails(1, i);  // stripType 1 = inner
    }
}

void EmeraldCityEffect::applyInnerWaveFade() {
    // Apply a moving black fade to inner strips that follows the core wave position
    // Creates a shadow effect that moves with the core wave
    // The fade intensity cycles through: dark → darker → lighter → light → dark

    // Calculate normalized wave position (0.0 to 1.0)
    float normalizedWavePos = coreWavePosition / LED_STRIP_CORE_COUNT;
    if (normalizedWavePos > 1.0f) normalizedWavePos -= 1.0f;  // Wrap around

    // For each inner strip
    for (int strip = 0; strip < NUM_INNER_STRIPS; strip++) {
        int stripStartIndex = strip * INNER_LEDS_PER_STRIP;
        int stripLength = INNER_LEDS_PER_STRIP;

        // Apply fade effect based on wave position
        for (int i = 0; i < stripLength; i++) {
            // Calculate normalized position within this strip (0.0 = bottom, 1.0 = top)
            float normalizedPos = (float)i / stripLength;

            // Calculate distance from wave position
            float waveDistance = abs(normalizedPos - normalizedWavePos);

            // Create fade intensity based on distance from wave
            // Closer to wave = more fade (darker), farther = less fade (lighter)
            float fadeIntensity;
            if (waveDistance < 0.2f) {
                // Close to wave - strong fade (dark shadow)
                fadeIntensity = 0.8f - (waveDistance / 0.2f) * 0.4f;  // 0.8 to 0.4
            } else if (waveDistance < 0.5f) {
                // Medium distance - medium fade
                fadeIntensity = 0.4f - ((waveDistance - 0.2f) / 0.3f) * 0.2f;  // 0.4 to 0.2
            } else {
                // Far from wave - light fade
                fadeIntensity = 0.2f - ((waveDistance - 0.5f) / 0.5f) * 0.1f;  // 0.2 to 0.1
            }

            // Apply the fade (darken the existing color)
            int ledIndex = stripStartIndex + i;
            leds.getInner()[ledIndex].nscale8_video((uint8_t)(255 * (1.0f - fadeIntensity)));
        }
    }
}

void EmeraldCityEffect::updateOuterTrails() {
    // Update trails for each outer strip
    for (int i = 0; i < NUM_OUTER_STRIPS; i++) {
        updateStripTrails(2, i);  // stripType 2 = outer
    }
}

void EmeraldCityEffect::updateStripTrails(int stripType, int subStrip) {
    std::vector<EmeraldTrail>* trails;
    int stripLength = getStripLength(stripType);

    // Get the appropriate trails vector
    if (stripType == 1) {  // Inner
        trails = &innerTrails[subStrip];
    } else {  // Outer
        trails = &outerTrails[subStrip];
    }

    // Random chance to create a new trail
    if (random(100) < TRAIL_CREATE_CHANCE) {
        createTrail(stripType, subStrip);
    }

    // Update all active trails
    for (auto& trail : *trails) {
        if (trail.isActive) {
            // Move the trail upward
            trail.position += trail.speed;

            // Deactivate if trail has moved completely off the top
            if (trail.position > stripLength + TRAIL_LENGTH) {
                trail.isActive = false;
                continue;
            }

            // Render this trail
            renderTrail(trail, stripType, subStrip, stripLength);
        }
    }
}

void EmeraldCityEffect::createTrail(int stripType, int subStrip) {
    std::vector<EmeraldTrail>* trails;

    // Get the appropriate trails vector
    if (stripType == 1) {  // Inner
        trails = &innerTrails[subStrip];
    } else {  // Outer
        trails = &outerTrails[subStrip];
    }

    // Find an inactive trail to reuse
    for (auto& trail : *trails) {
        if (!trail.isActive) {
            // Initialize the new trail
            trail.isActive = true;
            trail.position = -TRAIL_LENGTH;  // Start below the strip
            trail.speed = MIN_TRAIL_SPEED + (random(100) / 100.0f) * (MAX_TRAIL_SPEED - MIN_TRAIL_SPEED);
            trail.greenHue = getRandomGreenHue();
            trail.brightness = TRAIL_BRIGHTNESS + random(75);  // Add some brightness variation
            trail.stripType = stripType;
            trail.subStrip = subStrip;
            break;
        }
    }
}

void EmeraldCityEffect::renderTrail(EmeraldTrail& trail, int stripType, int subStrip, int stripLength) {
    // Draw the trail with fading effect from head to tail AND BLENDING for overlaps
    for (int i = 0; i < TRAIL_LENGTH; i++) {
        int trailPos = (int)(trail.position - i);

        // Only draw if position is within strip bounds
        if (trailPos >= 0 && trailPos < stripLength) {
            // Map logical position to physical LED position
            int physicalPos = leds.mapPositionToPhysical(stripType, trailPos, subStrip);

            // Adjust for strip offsets
            if (stripType == 1) {  // Inner
                physicalPos += subStrip * INNER_LEDS_PER_STRIP;
            } else if (stripType == 2) {  // Outer
                physicalPos += subStrip * OUTER_LEDS_PER_STRIP;
            }

            // Calculate trail brightness (decreasing from head to tail)
            uint8_t trailBrightness = trail.brightness * (TRAIL_LENGTH - i) / TRAIL_LENGTH;

            // Create green color using HSV
            CHSV hsvColor(trail.greenHue, 255, trailBrightness);  // Full saturation for vibrant green
            CRGB greenColor;
            hsv2rgb_rainbow(hsvColor, greenColor);

            // BLEND with existing color instead of replacing (additive blending for overlaps)
            if (stripType == 1) {  // Inner
                leds.getInner()[physicalPos] += greenColor;  // Additive blending
            } else if (stripType == 2) {  // Outer
                leds.getOuter()[physicalPos] += greenColor;  // Additive blending
            }
        }
    }
}

void EmeraldCityEffect::updateSparkles() {
    unsigned long currentTime = millis();

    // Only update sparkles at specified intervals for smooth animation
    if (currentTime - lastSparkleUpdate < SPARKLE_UPDATE_INTERVAL) {
        return;
    }

    lastSparkleUpdate = currentTime;

    // Update sparkles for inner strips
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        // If not currently sparkling, random chance to start (50% less for inner/outer)
        if (innerSparkleValues[i] <= 0.0f) {
            if (random(1000) < (INNER_OUTER_SPARKLE_CHANCE * 1000)) {
                innerSparkleValues[i] = 0.01f;  // Start the fade cycle
                innerSparkleColors[i] = random(2);  // Random color: 0=white, 1=light green
                innerSparkleBrightness[i] = 0.2f + (random(80) / 100.0f);  // New random brightness 20% to 100%
                // New random speed for this sparkle: 50% to 200% of base speed
                innerSparkleSpeed[i] = BASE_SPARKLE_SPEED * (MIN_SPEED_MULTIPLIER + (random(150) / 100.0f));
            }
        }
        // If currently sparkling, use individual speed for fade
        else {
            innerSparkleValues[i] += innerSparkleSpeed[i];  // Use individual sparkle's speed

            // Use sine wave for smooth fade in and out with more gradual curves
            float phase = innerSparkleValues[i];
            float sineValue;

            // Create a more gradual fade by using a modified sine curve
            if (phase <= PI) {
                // Use a smoother curve: sin^2 for more gradual fade in/out
                float baseSine = sin(phase);
                sineValue = baseSine * baseSine;  // Squaring makes the fade more gradual
            } else {
                // Sparkle cycle complete, turn off
                innerSparkleValues[i] = 0.0f;
                sineValue = 0.0f;
            }

            // Apply sparkle color if active
            if (sineValue > 0.0f) {
                CRGB sparkleColor;
                // Use the individual sparkle's random brightness level
                float intensity = sineValue * innerSparkleBrightness[i];

                if (innerSparkleColors[i] == 0) {
                    // White sparkle
                    uint8_t brightValue = (uint8_t)(255 * intensity);
                    sparkleColor = CRGB(brightValue, brightValue, brightValue);
                } else {
                    // Light green sparkle (pale green)
                    uint8_t brightValue = (uint8_t)(255 * intensity);
                    sparkleColor = CRGB(brightValue * 0.3f, brightValue, brightValue * 0.5f);
                }

                // Blend with existing color (additive)
                leds.getInner()[i] += sparkleColor;
            }
        }
    }

    // Update sparkles for outer strips - same smooth sine logic
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        // If not currently sparkling, random chance to start (50% less for inner/outer)
        if (outerSparkleValues[i] <= 0.0f) {
            if (random(1000) < (INNER_OUTER_SPARKLE_CHANCE * 1000)) {
                outerSparkleValues[i] = 0.01f;  // Start the fade cycle
                outerSparkleColors[i] = random(2);  // Random color: 0=white, 1=light green
                outerSparkleBrightness[i] = 0.2f + (random(80) / 100.0f);  // New random brightness 20% to 100%
                // New random speed for this sparkle: 50% to 200% of base speed
                outerSparkleSpeed[i] = BASE_SPARKLE_SPEED * (MIN_SPEED_MULTIPLIER + (random(150) / 100.0f));
            }
        }
        // If currently sparkling, use individual speed for fade
        else {
            outerSparkleValues[i] += outerSparkleSpeed[i];  // Use individual sparkle's speed

            // Use sine wave for smooth fade in and out with more gradual curves
            float phase = outerSparkleValues[i];
            float sineValue;

            // Create a more gradual fade by using a modified sine curve
            if (phase <= PI) {
                // Use a smoother curve: sin^2 for more gradual fade in/out
                float baseSine = sin(phase);
                sineValue = baseSine * baseSine;  // Squaring makes the fade more gradual
            } else {
                // Sparkle cycle complete, turn off
                outerSparkleValues[i] = 0.0f;
                sineValue = 0.0f;
            }

            // Apply sparkle color if active
            if (sineValue > 0.0f) {
                CRGB sparkleColor;
                // Use the individual sparkle's random brightness level
                float intensity = sineValue * outerSparkleBrightness[i];

                if (outerSparkleColors[i] == 0) {
                    // White sparkle
                    uint8_t brightValue = (uint8_t)(255 * intensity);
                    sparkleColor = CRGB(brightValue, brightValue, brightValue);
                } else {
                    // Light green sparkle (pale green)
                    uint8_t brightValue = (uint8_t)(255 * intensity);
                    sparkleColor = CRGB(brightValue * 0.3f, brightValue, brightValue * 0.5f);
                }

                // Blend with existing color (additive)
                leds.getOuter()[i] += sparkleColor;
            }
        }
    }

    // Update sparkles for ring strip - same smooth sine logic
    // Skip ring updates if button feedback is active to avoid conflicts
    if (!skipRing) {
        for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
            // If not currently sparkling, random chance to start (unchanged for ring)
            if (ringSparkleValues[i] <= 0.0f) {
                if (random(1000) < (RING_SPARKLE_CHANCE * 1000)) {
                    ringSparkleValues[i] = 0.01f;  // Start the fade cycle
                    // Ring sparkles: 75% green, 25% white
                    ringSparkleColors[i] = (random(100) < 75) ? 1 : 0;  // 75% chance for green (1), 25% for white (0)
                    ringSparkleBrightness[i] = 0.2f + (random(80) / 100.0f);  // New random brightness 20% to 100%
                    // New random speed for ring sparkle: slower (25% to 50% of base speed)
                    ringSparkleSpeed[i] = BASE_SPARKLE_SPEED * RING_SPEED_MULTIPLIER * (MIN_SPEED_MULTIPLIER + (random(100) / 100.0f));
                }
            }
            // If currently sparkling, use individual speed for fade (slower for ring)
            else {
                ringSparkleValues[i] += ringSparkleSpeed[i];  // Use individual sparkle's slower speed

                // Use sine wave for smooth fade in and out with more gradual curves
                float phase = ringSparkleValues[i];
                float sineValue;

                // Create a more gradual fade by using a modified sine curve
                if (phase <= PI) {
                    // Use a smoother curve: sin^2 for more gradual fade in/out
                    float baseSine = sin(phase);
                    sineValue = baseSine * baseSine;  // Squaring makes the fade more gradual
                } else {
                    // Sparkle cycle complete, turn off
                    ringSparkleValues[i] = 0.0f;
                    sineValue = 0.0f;
                }

                // Apply sparkle color if active
                if (sineValue > 0.0f) {
                    CRGB sparkleColor;
                    // Use the individual sparkle's random brightness level
                    float intensity = sineValue * ringSparkleBrightness[i];

                    if (ringSparkleColors[i] == 0) {
                        // White sparkle
                        uint8_t brightValue = (uint8_t)(255 * intensity);
                        sparkleColor = CRGB(brightValue, brightValue, brightValue);
                    } else {
                        // Light green sparkle (pale green)
                        uint8_t brightValue = (uint8_t)(255 * intensity);
                        sparkleColor = CRGB(brightValue * 0.3f, brightValue, brightValue * 0.5f);
                    }

                    // Blend with existing color (additive)
                    leds.getRing()[i] += sparkleColor;
                }
            }
        }
    } // End of skipRing check for ring sparkles
}

void EmeraldCityEffect::applyOuterFadeOverlay() {
    // Apply fade-to-black overlay to outer strips
    // Creates ambient lighting effect where outer strips fade from full brightness to black

    for (int strip = 0; strip < NUM_OUTER_STRIPS; strip++) {
        int stripStartIndex = strip * OUTER_LEDS_PER_STRIP;
        int stripLength = OUTER_LEDS_PER_STRIP;

        for (int i = 0; i < stripLength; i++) {
            // Calculate normalized position within strip (0.0 = bottom, 1.0 = top)
            float normalizedPos = (float)i / stripLength;

            // Calculate fade intensity based on position
            float fadeIntensity = 1.0f;  // Default: no fade (full brightness)

            if (normalizedPos >= FADE_START_POSITION) {
                // Start fading from FADE_START_POSITION to FADE_END_POSITION
                if (normalizedPos >= FADE_END_POSITION) {
                    // Complete fade to black
                    fadeIntensity = 0.0f;
                } else {
                    // Gradual fade
                    float fadeProgress = (normalizedPos - FADE_START_POSITION) / (FADE_END_POSITION - FADE_START_POSITION);
                    fadeIntensity = 1.0f - fadeProgress;  // Fade from 1.0 to 0.0
                }
            }

            // Apply the fade to the existing LED color
            int ledIndex = stripStartIndex + i;
            leds.getOuter()[ledIndex].nscale8_video((uint8_t)(255 * fadeIntensity));
        }
    }
}

void EmeraldCityEffect::applyCoreWaveEffect() {
    // Apply a large blue wave that moves across each core segment uniformly
    // Each of the 3 core segments displays the same wave pattern

    // Update wave position
    coreWavePosition += CORE_WAVE_SPEED;

    // Calculate segment length (core strip divided into 3 equal segments)
    int segmentLength = LED_STRIP_CORE_COUNT / 3;

    // Reset wave position for shorter gaps between waves
    // Instead of waiting for full wave + length, reset sooner for more frequent waves
    if (coreWavePosition > segmentLength + (CORE_WAVE_LENGTH * 0.3f)) {
        coreWavePosition = -CORE_WAVE_LENGTH;  // Start from before the beginning
    }

    // Apply the same wave pattern to all 3 core segments
    for (int segment = 0; segment < 3; segment++) {
        int segmentStartIndex = segment * segmentLength;

        // Apply the wave to each LED in this segment
        for (int i = 0; i < segmentLength; i++) {
            // Calculate distance from this LED to the wave center (within this segment)
            float distanceFromWaveCenter = abs((float)i - coreWavePosition);

            // Calculate wave intensity with fade in/out (center is brightest)
            float waveIntensity = 0.0f;
            if (distanceFromWaveCenter < CORE_WAVE_LENGTH / 2) {
                // Use cosine wave for smooth bell curve (center brightest, edges fade out)
                float normalizedDistance = distanceFromWaveCenter / (CORE_WAVE_LENGTH / 2.0f);
                waveIntensity = cos(normalizedDistance * PI / 2.0f);  // Cosine gives smooth fade

                // Apply additional fade-in/fade-out at wave edges for smoother appearance
                float edgeFade = 1.0f;
                if (normalizedDistance > 0.7f) {
                    edgeFade = 1.0f - ((normalizedDistance - 0.7f) / 0.3f);
                }
                waveIntensity *= edgeFade;

                // Apply maximum brightness setting
                waveIntensity *= CORE_WAVE_BRIGHTNESS;

                // Ensure we don't exceed maximum
                waveIntensity = constrain(waveIntensity, 0.0f, 1.0f);
            }

            // Apply the wave color to this LED if intensity > 0
            if (waveIntensity > 0.0f) {
                // Calculate actual LED index in the core array
                int ledIndex = segmentStartIndex + i;

                // Map logical position to physical position for this segment
                int physicalIndex = leds.mapPositionToPhysical(0, i, segment);
                if (segment == 0 || segment == 2) {
                    // Segments A and C use direct mapping
                    physicalIndex = segmentStartIndex + physicalIndex;
                } else {
                    // Segment B (middle) is flipped, so use the mapped position
                    physicalIndex = segmentStartIndex + physicalIndex;
                }

                // Create blue-green wave color
                uint8_t red = (uint8_t)(30 * waveIntensity);    // Slight red tint
                uint8_t green = (uint8_t)(180 * waveIntensity); // Strong green
                uint8_t blue = (uint8_t)(255 * waveIntensity);  // Full blue

                // Set the LED color (overwrites any existing color for this effect)
                leds.getCore()[physicalIndex] = CRGB(red, green, blue);
            }
        }
    }
}

uint8_t EmeraldCityEffect::getRandomGreenHue() {
    // Return a random green hue from the palette
    return greenHues[random(6)];  // 6 green hues in the palette
}

int EmeraldCityEffect::getStripLength(int stripType) {
    // Return the length of the specified strip type
    switch (stripType) {
        case 1:  // Inner
            return INNER_LEDS_PER_STRIP;
        case 2:  // Outer
            return OUTER_LEDS_PER_STRIP;
        default:
            return 0;
    }
}