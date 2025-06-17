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

            // Random speed and other properties
            trail.speed = MIN_TRAIL_SPEED + (random(100) / 100.0f) * (MAX_TRAIL_SPEED - MIN_TRAIL_SPEED);
            trail.greenHue = getRandomGreenHue();
            trail.brightness = TRAIL_BRIGHTNESS + random(50);  // Some brightness variation
        }
    }

    // Initialize trails for outer strips
    for (int stripIndex = 0; stripIndex < NUM_OUTER_STRIPS; stripIndex++) {
        // Create 3-5 trails per strip at startup
        int numStartupTrails = 3 + random(3);  // 3 to 5 trails

        for (int trailIndex = 0; trailIndex < numStartupTrails && trailIndex < MAX_TRAILS_PER_STRIP; trailIndex++) {
            EmeraldTrail& trail = outerTrails[stripIndex][trailIndex];
            trail.isActive = true;
            trail.stripType = 2;  // Outer strip
            trail.subStrip = stripIndex;

            // Random position throughout the strip height
            int stripLength = getStripLength(2);
            trail.position = random(stripLength * 0.2f, stripLength * 0.8f);  // 20% to 80% up the strip

            // Random speed and other properties
            trail.speed = MIN_TRAIL_SPEED + (random(100) / 100.0f) * (MAX_TRAIL_SPEED - MIN_TRAIL_SPEED);
            trail.greenHue = getRandomGreenHue();
            trail.brightness = TRAIL_BRIGHTNESS + random(50);  // Some brightness variation
        }
    }

    Serial.println("Initialized startup trails for immediate visual effect");
}

void EmeraldCityEffect::reset() {
    // Reset all trails to inactive
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

    // Reset all sparkle values to 0.0 (no sparkles) and randomize colors and brightness
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        innerSparkleValues[i] = 0.0f;
        innerSparkleColors[i] = random(2);  // Randomize color again
        innerSparkleBrightness[i] = 0.2f + (random(80) / 100.0f);  // Random brightness 20% to 100%
    }
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        outerSparkleValues[i] = 0.0f;
        outerSparkleColors[i] = random(2);  // Randomize color again
        outerSparkleBrightness[i] = 0.2f + (random(80) / 100.0f);  // Random brightness 20% to 100%
    }
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        ringSparkleValues[i] = 0.0f;
        ringSparkleColors[i] = random(2);  // Randomize color again
        ringSparkleBrightness[i] = 0.2f + (random(80) / 100.0f);  // Random brightness 20% to 100%
    }

    // Reset core glow - REMOVED (core should stay off)
    // coreGlowIntensity = 0.8f;
    // coreGlowPhase = 0.0f;

    // Reset timing
    lastUpdateTime = millis();
    lastSparkleUpdate = 0;

    // Reset core wave
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

    // Update white sparkle effects
    updateSparkles();

    // Apply fade-to-black overlay to outer strips for ambient effect
    applyOuterFadeOverlay();

    // Apply blue-green wave effect to core strip
    applyCoreWaveEffect();

    // Apply moving black fade to inner strips that follows the core wave
    applyInnerWaveFade();

    // Show all LED updates
    leds.showAll();
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

    // Apply fade to all inner strip segments
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            // Calculate physical LED position for this segment
            int physicalPos = i + (segment * INNER_LEDS_PER_STRIP);

            // Map inner strip position to core position for wave following
            // Scale inner strip position to match core strip length
            float mappedCorePosition = ((float)i / (float)INNER_LEDS_PER_STRIP) * LED_STRIP_CORE_COUNT;

            // Calculate distance from this LED to the core wave center
            float distanceFromWaveCenter = abs(mappedCorePosition - coreWavePosition);

            // Calculate fade multiplier based on distance from wave
            float fadeMultiplier = 1.0f;  // Default to full brightness

            // Create a black fade zone that follows the wave
            float fadeZoneRadius = CORE_WAVE_LENGTH * 0.6f;  // Fade zone is 60% of wave length

            if (distanceFromWaveCenter < fadeZoneRadius) {
                // Inside the fade zone - create black fade effect
                float normalizedDistance = distanceFromWaveCenter / fadeZoneRadius;

                // Inverse fade - closer to wave center = more black
                fadeMultiplier = normalizedDistance;  // 0.0 (black) at center, 1.0 (full) at edge

                // Smooth the fade with quadratic curve
                fadeMultiplier = fadeMultiplier * fadeMultiplier;  // Quadratic for smoother transition
            }

            // Apply fade multiplier to the inner LED
            CRGB& pixel = leds.getInner()[physicalPos];
            pixel.r = (uint8_t)(pixel.r * fadeMultiplier);
            pixel.g = (uint8_t)(pixel.g * fadeMultiplier);
            pixel.b = (uint8_t)(pixel.b * fadeMultiplier);
        }
    }
}

void EmeraldCityEffect::applyCoreWaveEffect() {
    // Apply a large blue-green wave that moves across the entire core strip
    // Wave fades in and out with center being brightest

    // Update wave position
    coreWavePosition += CORE_WAVE_SPEED;

    // Reset wave position when it moves completely off the strip
    if (coreWavePosition > LED_STRIP_CORE_COUNT + CORE_WAVE_LENGTH) {
        coreWavePosition = -CORE_WAVE_LENGTH;  // Start from before the beginning
    }

    // Apply the wave to each LED in the core strip
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        // Calculate distance from this LED to the wave center
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
                // Fade out more aggressively near edges
                float edgeDistance = (normalizedDistance - 0.7f) / 0.3f;  // 0.0 to 1.0
                edgeFade = 1.0f - (edgeDistance * edgeDistance);  // Quadratic fade
            }

            waveIntensity *= edgeFade;  // Apply edge fade
            waveIntensity *= CORE_WAVE_BRIGHTNESS;
        }

        // Apply the wave color to the LED
        if (waveIntensity > 0.01f) {  // Small threshold to avoid very dim pixels
            CHSV waveHSV(CORE_WAVE_HUE, 255, (uint8_t)(255 * waveIntensity));
            CRGB waveColor;
            hsv2rgb_rainbow(waveHSV, waveColor);

            // Set the core LED
            leds.getCore()[i] = waveColor;
        } else {
            // Turn off LED when not in wave
            leds.getCore()[i] = CRGB::Black;
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
                innerSparkleColors[i] = random(2);  // Choose new random color
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

    // Update sparkles for outer strips (same smooth sine logic)
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        // If not currently sparkling, random chance to start (50% less for inner/outer)
        if (outerSparkleValues[i] <= 0.0f) {
            if (random(1000) < (INNER_OUTER_SPARKLE_CHANCE * 1000)) {
                outerSparkleValues[i] = 0.01f;  // Start the fade cycle
                outerSparkleColors[i] = random(2);  // Choose new random color
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

    // Update sparkles for ring strip (if not skipped) - same smooth sine logic
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
                        // White sparkle (more vibrant)
                        uint8_t brightValue = (uint8_t)(255 * intensity);
                        sparkleColor = CRGB(brightValue, brightValue, brightValue);
                    } else {
                        // Light green sparkle (much more vibrant - increased saturation and brightness)
                        uint8_t brightValue = (uint8_t)(255 * intensity * 1.3f);  // 30% brighter
                        if (brightValue > 255) brightValue = 255;  // Cap at maximum
                        sparkleColor = CRGB(brightValue * 0.2f, brightValue, brightValue * 0.4f);  // More vibrant green ratio
                    }

                    // Set ring color (since ring doesn't have base trails, just sparkles)
                    leds.getRing()[i] = sparkleColor;
                }
            }
        }
    }
}

uint8_t EmeraldCityEffect::getRandomGreenHue() {
    // Return a random green hue from our palette
    int index = random(NUM_GREEN_COLORS);
    return greenHues[index];
}

void EmeraldCityEffect::applyOuterFadeOverlay() {
    // Apply fade-to-black overlay to ALL outer strip segments for ambient lighting effect
    // This creates a gradient from full brightness at bottom to 90% black at top

    // Apply fade to all 3 outer strip segments
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            // Calculate physical LED position for this segment
            int physicalPos = i + (segment * OUTER_LEDS_PER_STRIP);

            // Calculate position along the strip (0.0 = bottom, 1.0 = top)
            float position = (float)i / (float)OUTER_LEDS_PER_STRIP;

            // Calculate fade multiplier based on position
            float fadeMultiplier = 1.0f;  // Default to full brightness

            if (position >= FADE_START_POSITION) {
                if (position >= FADE_END_POSITION) {
                    // Fade to 90% black (10% brightness remaining)
                    fadeMultiplier = 0.1f;
                } else {
                    // Gradual fade from FADE_START_POSITION to FADE_END_POSITION
                    float fadeProgress = (position - FADE_START_POSITION) / (FADE_END_POSITION - FADE_START_POSITION);
                    fadeMultiplier = 1.0f - (fadeProgress * 0.9f);  // 1.0 to 0.1 (90% fade)
                }
            }

            // Apply fade multiplier to the outer LED
            CRGB& pixel = leds.getOuter()[physicalPos];
            pixel.r = (uint8_t)(pixel.r * fadeMultiplier);
            pixel.g = (uint8_t)(pixel.g * fadeMultiplier);
            pixel.b = (uint8_t)(pixel.b * fadeMultiplier);
        }
    }
}

int EmeraldCityEffect::getStripLength(int stripType) {
    // Return the length of strips based on type
    switch (stripType) {
        case 1:  // Inner
            return INNER_LEDS_PER_STRIP;
        case 2:  // Outer
            return OUTER_LEDS_PER_STRIP;
        default:
            return 0;
    }
}