// src/leds/effects/LustEffect.cpp

#include "LustEffect.h"
#include <math.h>

LustEffect::LustEffect(LEDController& ledController)
    : Effect(ledController),
      cycleStartTime(0),
      breathingPhase(0.0f),
      isFirstHalf(true),
      gradientOffset(0.0f),
      colorSetStartTime(0)
{
    // Constructor initializes timing variables
    // Note: Not calling leds.clear() as requested to avoid wrecking code
}

void LustEffect::update() {
    unsigned long currentTime = millis();

    // Initialize cycle start times on first run
    if (cycleStartTime == 0) {
        cycleStartTime = currentTime;
        colorSetStartTime = currentTime;
    }

    // Calculate elapsed time in current cycle
    unsigned long elapsedTime = currentTime - cycleStartTime;

    // Handle cycle completion and restart
    if (elapsedTime >= CYCLE_DURATION) {
        cycleStartTime = currentTime;
        elapsedTime = 0;
    }

    // Determine which half of the cycle we're in
    isFirstHalf = (elapsedTime < HALF_CYCLE);

    // Calculate breathing intensity using smooth sine wave
    float intensity = calculateBreathingIntensity();

    // Calculate current color set blend ratio
    float colorSetRatio = calculateColorSetBlendRatio();

    // Get current colors based on animation
    uint32_t hotColor, coolColor;
    getCurrentColorSet(colorSetRatio, hotColor, coolColor);

    // Update gradient animation offset
    gradientOffset += GRADIENT_SPEED;

    // Update each strip with breathing effect
    updateCoreBreathing(intensity, hotColor, coolColor);
    updateInnerBreathing(intensity, hotColor, coolColor);
    updateOuterBreathing(intensity, hotColor, coolColor);
    updateRingBreathing(intensity, hotColor, coolColor);

    // Display the updated colors
    leds.showAll();
}

void LustEffect::reset() {
    // Reset animation to beginning of cycle
    cycleStartTime = millis();
    breathingPhase = 0.0f;
    isFirstHalf = true;
    gradientOffset = 0.0f;
    colorSetStartTime = millis();
}

float LustEffect::calculateBreathingIntensity() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - cycleStartTime;

    // Calculate position within the 4-second cycle (0.0 to 1.0)
    float cyclePosition = (float)elapsedTime / (float)CYCLE_DURATION;

    // Use sine wave for smooth breathing effect
    // Sin goes from 0 to 1 to 0 over full cycle
    float sineValue = sin(cyclePosition * 2.0f * PI);

    // Convert sine wave (-1 to 1) to breathing intensity (0 to 1)
    // Absolute value ensures we always breathe "outward"
    return abs(sineValue);
}

float LustEffect::calculateColorSetBlendRatio() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - colorSetStartTime;

    // Calculate position within the 8-second color set cycle (0.0 to 1.0)
    float cyclePosition = (float)(elapsedTime % COLOR_SET_CYCLE) / (float)COLOR_SET_CYCLE;

    // Use sine wave for smooth transition between color sets
    float sineValue = sin(cyclePosition * 2.0f * PI);

    // Convert sine wave (-1 to 1) to blend ratio (0 to 1)
    return (sineValue + 1.0f) * 0.5f;
}

void LustEffect::getCurrentColorSet(float colorSetRatio, uint32_t& hotColor, uint32_t& coolColor) {
    // Blend between the two color sets based on the ratio
    CRGB hotColorRGB = blendColors(HOT_PINK_RED_SET1, HOT_PINK_RED_SET2, colorSetRatio);
    CRGB coolColorRGB = blendColors(DEEP_PURPLE_BLUE_SET1, DEEP_PURPLE_BLUE_SET2, colorSetRatio);

    // Convert back to uint32_t
    hotColor = ((uint32_t)hotColorRGB.r << 16) | ((uint32_t)hotColorRGB.g << 8) | (uint32_t)hotColorRGB.b;
    coolColor = ((uint32_t)coolColorRGB.r << 16) | ((uint32_t)coolColorRGB.g << 8) | (uint32_t)coolColorRGB.b;
}

CRGB LustEffect::blendColors(uint32_t color1, uint32_t color2, float intensity) {
    // Extract RGB components from 32-bit colors
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;

    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;

    // Clamp intensity to valid range
    intensity = max(0.0f, min(1.0f, intensity));

    // Linear interpolation between colors
    uint8_t r = r1 + (uint8_t)((r2 - r1) * intensity);
    uint8_t g = g1 + (uint8_t)((g2 - g1) * intensity);
    uint8_t b = b1 + (uint8_t)((b2 - b1) * intensity);

    return CRGB(r, g, b);
}

void LustEffect::updateCoreBreathing(float intensity, uint32_t hotColor, uint32_t coolColor) {
    // Core has moving gradient wave
    CRGB* coreStrip = leds.getCore();
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        coreStrip[i] = getGradientWaveColor(i, gradientOffset, intensity, false, hotColor, coolColor);
    }
}

void LustEffect::updateInnerBreathing(float intensity, uint32_t hotColor, uint32_t coolColor) {
    // Inner has opposing gradient wave, but each of the 3 strips shows the same pattern
    // Add 15% offset to create phase difference from core/outer strips
    float offsetGradient = gradientOffset + (WAVE_LENGTH * 0.15f);
    CRGB* innerStrip = leds.getInner();

    // Apply the same gradient pattern to each of the 3 inner strips
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        int segmentStart = segment * INNER_LEDS_PER_STRIP;

        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;
            // Use local position 'i' with offset gradient (but still reversed direction)
            innerStrip[ledIndex] = getGradientWaveColor(i, offsetGradient, intensity, true, hotColor, coolColor);
        }
    }
}

void LustEffect::updateOuterBreathing(float intensity, uint32_t hotColor, uint32_t coolColor) {
    // Outer has same gradient wave as core, but each of the 3 strips shows the same pattern
    // Plus fade to black overlay from bottom to top
    CRGB* outerStrip = leds.getOuter();

    // Apply the same gradient pattern to each of the 3 outer strips
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;

            // Get the base gradient color
            CRGB baseColor = getGradientWaveColor(i, gradientOffset, intensity, false, hotColor, coolColor);

            // Calculate fade factor (0.0 at top, 1.0 at bottom)
            float fadePosition = (float)i / (float)(OUTER_LEDS_PER_STRIP - 1);
            float fadeFactor = 1.0f - fadePosition; // 1.0 at bottom (i=0), 0.0 at top (i=max)

            // Apply fade to black by scaling the color components
            baseColor.r = (uint8_t)(baseColor.r * fadeFactor);
            baseColor.g = (uint8_t)(baseColor.g * fadeFactor);
            baseColor.b = (uint8_t)(baseColor.b * fadeFactor);

            // Use local position 'i' instead of global position 'ledIndex'
            // so all 3 strips show the same pattern with fade overlay
            outerStrip[ledIndex] = baseColor;
        }
    }
}

void LustEffect::updateRingBreathing(float intensity, uint32_t hotColor, uint32_t coolColor) {
    // Skip ring if button feedback is active
    if (skipRing) {
        return;
    }

    // Ring has same gradient wave as core and outer
    CRGB* ringStrip = leds.getRing();
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        ringStrip[i] = getGradientWaveColor(i, gradientOffset, intensity, false, hotColor, coolColor);
    }
}

CRGB LustEffect::getGradientWaveColor(int position, float offset, float intensity, bool reversed, uint32_t hotColor, uint32_t coolColor) {
    // Calculate wave position with animation offset
    float wavePosition = (float)position + (reversed ? -offset : offset);

    // Use sine wave to create smooth gradient transition
    float sineInput = wavePosition * 2.0f * PI / WAVE_LENGTH;
    float waveValue = sin(sineInput);

    // Convert sine wave (-1 to 1) to blend ratio (0 to 1)
    float blendRatio = (waveValue + 1.0f) * 0.5f;

    // No breathing modulation - just pure gradient wave
    // Removed: float breathingModulation = sin(intensity * PI) * 0.2f;
    // Removed: float finalBlendRatio = blendRatio + breathingModulation;

    // Use pure blend ratio for smooth gradient
    float finalBlendRatio = blendRatio;

    // Clamp to valid range
    finalBlendRatio = max(0.0f, min(1.0f, finalBlendRatio));

    // Blend between the current hot and cool colors
    return blendColors(hotColor, coolColor, finalBlendRatio);
}