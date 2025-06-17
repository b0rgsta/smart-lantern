// src/leds/effects/CandleFlickerEffect.cpp

#include "CandleFlickerEffect.h"

// Timing constants - FASTER AND SMOOTHER
static constexpr unsigned long FLICKER_UPDATE_INTERVAL = 60; // Faster updates (every 60ms instead of 120ms)

// Global flicker parameters (affects whole lamp) - FASTER AND SMOOTHER
static constexpr float GLOBAL_MIN_INTENSITY = 0.7f;       // Minimum global brightness (70%)
static constexpr float GLOBAL_MAX_INTENSITY = 1.2f;       // Maximum global brightness (120%)
static constexpr float GLOBAL_BRIGHT_INTENSITY = 1.4f;    // Rare bright flickers (140%)
static constexpr int GLOBAL_FLICKER_CHANCE = 40;          // 40% chance to change global (more frequent)
static constexpr int GLOBAL_BRIGHT_FLICKER_CHANCE = 10;   // 10% chance for bright global flicker
static constexpr float GLOBAL_SMOOTH_FACTOR = 0.18f;      // How quickly global flicker changes (much faster)

// Zone flicker parameters (subtle variations on top of global) - FASTER AND SMOOTHER
static constexpr float ZONE_BASE_INTENSITY = 1.0f;        // Base zone intensity
static constexpr float ZONE_VARIATION_RANGE = 0.4f;       // ±20% variation range
static constexpr int ZONE_FLICKER_CHANCE = 35;            // 35% chance to change zones (more frequent)
static constexpr float ZONE_SMOOTH_FACTOR = 0.15f;        // How quickly zones change (much faster)

// Overall candle parameters - BRIGHTER BASE
static constexpr float BASE_BRIGHTNESS = 1.0f;            // Overall candle brightness (full brightness)

// Fade parameters for outer strips (matches other ambient effects)
static constexpr float FADE_START_POSITION = 0.3f;        // Start fading at 30% up the strip
static constexpr float FADE_END_POSITION = 0.9f;          // Complete fade by 90% up the strip

CandleFlickerEffect::CandleFlickerEffect(LEDController& ledController) :
    Effect(ledController),
    lastFlickerUpdate(0),
    globalFlickerIntensity(1.0f),
    mainFlameIntensity(1.0f),
    secondaryFlameIntensity(1.1f),      // Slightly brighter middle
    baseGlowIntensity(1.3f),            // Much brighter base for visibility
    globalFlickerTarget(1.0f),
    mainFlameTarget(1.0f),
    secondaryFlameTarget(1.1f),         // Slightly brighter middle target
    baseGlowTarget(1.3f)                // Much brighter base target
{
    // Initialize base candle color (warm 1800K)
    baseColor = getCandleColor();

    Serial.println("CandleFlickerEffect initialized - smooth global flicker with gentle zones");
}

void CandleFlickerEffect::reset() {
    // Reset all flame intensities to default values with brighter base
    globalFlickerIntensity = 1.0f;
    mainFlameIntensity = 1.0f;
    secondaryFlameIntensity = 1.1f;     // Slightly brighter middle
    baseGlowIntensity = 1.3f;           // Much brighter base

    // Reset targets
    globalFlickerTarget = 1.0f;
    mainFlameTarget = 1.0f;
    secondaryFlameTarget = 1.1f;        // Slightly brighter middle target
    baseGlowTarget = 1.3f;              // Much brighter base target

    // Reset timing
    lastFlickerUpdate = 0;
}

void CandleFlickerEffect::update() {
    // Standard effect timing check
    if (!shouldUpdate(25)) { // Update at ~40 FPS for smoother animation (was ~30 FPS)
        return;
    }

    // Update global and zone flicker intensities
    updateFlickerIntensities();

    // Clear all LEDs first
    leds.clearAll();

    // Apply candle flame effect to inner strips
    applyCandleFlameToInner();

    // Apply candle flame with fade to outer strips
    applyCandleFlameAndFadeToOuter();

    // Show the updated LEDs
    leds.showAll();
}

void CandleFlickerEffect::updateFlickerIntensities() {
    unsigned long currentTime = millis();

    // Update less frequently for smoother transitions
    if (currentTime - lastFlickerUpdate < FLICKER_UPDATE_INTERVAL) {
        return;
    }

    lastFlickerUpdate = currentTime;

    // === GLOBAL FLICKER (affects entire lamp) ===
    // This creates the overall candle breathing/flickering that affects everything
    if (random(100) < GLOBAL_FLICKER_CHANCE) {
        // Set new target for global flicker (smaller range for subtle effect)
        globalFlickerTarget = GLOBAL_MIN_INTENSITY + (random(100) / 100.0f) * (GLOBAL_MAX_INTENSITY - GLOBAL_MIN_INTENSITY);

        // Very rare stronger flickers for the whole lamp
        if (random(100) < GLOBAL_BRIGHT_FLICKER_CHANCE) {
            globalFlickerTarget = GLOBAL_BRIGHT_INTENSITY + (random(10) / 100.0f); // Small random variation
        }
    }

    // Smoothly move global flicker toward target
    float globalDifference = globalFlickerTarget - globalFlickerIntensity;
    globalFlickerIntensity += globalDifference * GLOBAL_SMOOTH_FACTOR;

    // === ZONE FLICKERS (subtle variations on top of global flicker) ===
    // These are much gentler and less frequent

    // Main flame zone (top) - more noticeable variation
    if (random(100) < ZONE_FLICKER_CHANCE) {
        mainFlameTarget = ZONE_BASE_INTENSITY + (random(60) / 100.0f) * ZONE_VARIATION_RANGE; // 0.8 to 1.2
    }
    float mainDifference = mainFlameTarget - mainFlameIntensity;
    mainFlameIntensity += mainDifference * ZONE_SMOOTH_FACTOR;

    // Secondary flame zone (middle) - noticeable but less than main
    if (random(100) < ZONE_FLICKER_CHANCE / 2) {
        secondaryFlameTarget = ZONE_BASE_INTENSITY * 1.1f + (random(50) / 100.0f) * ZONE_VARIATION_RANGE * 0.7f; // 0.97 to 1.38
    }
    float secondaryDifference = secondaryFlameTarget - secondaryFlameIntensity;
    secondaryFlameIntensity += secondaryDifference * ZONE_SMOOTH_FACTOR;

    // Base glow zone (bottom) - brightest with small variations
    if (random(100) < ZONE_FLICKER_CHANCE / 3) {
        baseGlowTarget = ZONE_BASE_INTENSITY * 1.3f + (random(40) / 100.0f) * ZONE_VARIATION_RANGE * 0.5f; // 1.2 to 1.56
    }
    float baseDifference = baseGlowTarget - baseGlowIntensity;
    baseGlowIntensity += baseDifference * ZONE_SMOOTH_FACTOR * 0.8f; // Slightly slower but still smooth
}

void CandleFlickerEffect::applyCandleFlameToInner() {
    // Apply flame zones to each inner strip segment
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        int segmentStart = segment * INNER_LEDS_PER_STRIP;

        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;

            // Calculate position ratio (0.0 at bottom, 1.0 at top)
            float positionRatio = (float)i / (INNER_LEDS_PER_STRIP - 1);

            // Determine which flame zone this LED belongs to and get zone intensity
            float zoneIntensity;

            if (positionRatio <= 0.3f) {
                // Bottom 30% - base glow zone (brightest and most stable)
                zoneIntensity = baseGlowIntensity;
            } else if (positionRatio <= 0.7f) {
                // Middle 40% - secondary flame zone
                // Smooth blend between base glow and secondary flame
                float blendFactor = (positionRatio - 0.3f) / 0.4f; // 0.0 to 1.0
                zoneIntensity = baseGlowIntensity * (1.0f - blendFactor) +
                               secondaryFlameIntensity * blendFactor;
            } else {
                // Top 30% - main flame zone
                // Smooth blend between secondary and main flame
                float blendFactor = (positionRatio - 0.7f) / 0.3f; // 0.0 to 1.0
                zoneIntensity = secondaryFlameIntensity * (1.0f - blendFactor) +
                               mainFlameIntensity * blendFactor;
            }

            // Apply GLOBAL flicker on top of zone intensity
            float finalIntensity = BASE_BRIGHTNESS * globalFlickerIntensity * zoneIntensity;

            // Apply intensity to base candle color
            CRGB flickeredColor = CRGB(
                (uint8_t)min(255, (int)(baseColor.r * finalIntensity)),
                (uint8_t)min(255, (int)(baseColor.g * finalIntensity)),
                (uint8_t)min(255, (int)(baseColor.b * finalIntensity))
            );

            leds.getInner()[ledIndex] = flickeredColor;
        }
    }
}

void CandleFlickerEffect::applyCandleFlameAndFadeToOuter() {
    // Apply flame zones with fade to each outer strip segment
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;

            // Calculate position ratio (0.0 at bottom, 1.0 at top)
            float positionRatio = (float)i / (OUTER_LEDS_PER_STRIP - 1);

            // Determine flame zone intensity (same as inner strips)
            float zoneIntensity;

            if (positionRatio <= 0.3f) {
                // Bottom 30% - base glow zone
                zoneIntensity = baseGlowIntensity;
            } else if (positionRatio <= 0.7f) {
                // Middle 40% - secondary flame zone
                float blendFactor = (positionRatio - 0.3f) / 0.4f;
                zoneIntensity = baseGlowIntensity * (1.0f - blendFactor) +
                               secondaryFlameIntensity * blendFactor;
            } else {
                // Top 30% - main flame zone
                float blendFactor = (positionRatio - 0.7f) / 0.3f;
                zoneIntensity = secondaryFlameIntensity * (1.0f - blendFactor) +
                               mainFlameIntensity * blendFactor;
            }

            // Calculate fade factor (fade to black in upper portion)
            float fadeBrightness = 1.0f;

            if (positionRatio >= FADE_START_POSITION) {
                // Calculate fade progress within the fade zone
                float fadeProgress = (positionRatio - FADE_START_POSITION) / (FADE_END_POSITION - FADE_START_POSITION);
                fadeProgress = min(1.0f, fadeProgress);

                // Apply smooth fade for gentle black transition
                fadeProgress = fadeProgress * fadeProgress; // Square for smooth fade
                fadeBrightness = 1.0f - fadeProgress;
            }

            // Force complete black at the very top
            if (positionRatio >= FADE_END_POSITION) {
                fadeBrightness = 0.0f;
            }

            // Combine GLOBAL flicker, zone intensity, and fade effects
            float finalBrightness = BASE_BRIGHTNESS * globalFlickerIntensity * zoneIntensity * fadeBrightness;

            // Apply combined brightness to base candle color
            CRGB finalColor = CRGB(
                (uint8_t)min(255, (int)(baseColor.r * finalBrightness)),
                (uint8_t)min(255, (int)(baseColor.g * finalBrightness)),
                (uint8_t)min(255, (int)(baseColor.b * finalBrightness))
            );

            leds.getOuter()[ledIndex] = finalColor;
        }
    }
}

CRGB CandleFlickerEffect::getCandleColor() {
    // 1800K candle temperature color calculation
    // Based on blackbody radiation curve for warm candle light

    // Candle light is very warm with dominant red/orange
    float temp = 1800.0f / 100.0f; // Temperature in hundreds of Kelvin

    float red, green, blue;

    // For very warm temperatures (candle), red is always maximum
    red = 255;

    // Calculate green component for 1800K
    green = temp;
    green = 99.4708025861f * log(green) - 161.1195681661f;
    green = constrain(green, 0, 255);

    // Blue is very minimal for warm candle light
    blue = 0; // At 1800K, blue component is essentially zero

    return CRGB((uint8_t)red, (uint8_t)green, (uint8_t)blue);
}