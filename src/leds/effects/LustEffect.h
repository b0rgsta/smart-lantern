// src/leds/effects/LustEffect.h

#ifndef LUST_EFFECT_H
#define LUST_EFFECT_H

#include "Effect.h"

/**
 * LustEffect - A passionate breathing effect that swaps colors between strips
 *
 * This effect creates a breathing animation with contrasting hot and cool colors:
 * - Core strip: Starts with hot pink/red, breathes to blue
 * - Inner strips: Start with blue, breathe to hot pink/red
 * - Outer strips: Start with hot pink/red, breathe to blue
 * - Ring strip: Starts with hot pink/red, breathes to blue
 *
 * The breathing cycle creates a "lust" effect where hot and cool colors
 * flip between strips, creating visual tension and passion.
 *
 * Animation cycle: 4 seconds total
 * - 2 seconds breathing from start colors to flipped colors
 * - 2 seconds breathing back to original colors
 */
class LustEffect : public Effect {
public:
    /**
     * Constructor - creates the lust effect
     * @param ledController Reference to the LED controller for drawing
     */
    LustEffect(LEDController& ledController);

    /**
     * Update the effect - animates the breathing color transition
     * Called every frame to create smooth breathing animation
     */
    void update() override;

    /**
     * Reset the effect to initial state
     * Restarts the breathing cycle from the beginning
     */
    void reset() override;

    /**
     * Get the name of this effect for debugging/display
     * @return The effect name as a string
     */
    String getName() const override { return "Lust Effect"; }

private:
    // Color definitions - two color sets that will animate between each other
    static constexpr uint32_t HOT_PINK_RED_SET1 = 0xFF4569;     // Hot pink with orange undertones (original)
    static constexpr uint32_t DEEP_PURPLE_BLUE_SET1 = 0x4A00B0; // More purple-blue (increased purple component)
    static constexpr uint32_t HOT_PINK_RED_SET2 = 0xFF0000;     // Pure red
    static constexpr uint32_t DEEP_PURPLE_BLUE_SET2 = 0x6600FF; // Purple-blue instead of pure blue

    // Current blended colors (halfway between the two sets)
    static constexpr uint32_t HOT_PINK_RED = 0xFF2234;          // Halfway blend
    static constexpr uint32_t DEEP_PURPLE_BLUE = 0x550058;     // Updated halfway blend with more purple

    // Animation timing constants
    static constexpr unsigned long CYCLE_DURATION = 4000;  // Total cycle time (4 seconds)
    static constexpr unsigned long HALF_CYCLE = CYCLE_DURATION / 2;  // Half cycle (2 seconds)
    static constexpr unsigned long COLOR_SET_CYCLE = 16000; // Color set transition cycle (16 seconds - doubled)

    // Gradient animation constants
    static constexpr float GRADIENT_SPEED = 0.1152f;        // Speed of gradient wave movement (20% faster)
    static constexpr float WAVE_LENGTH = 50.0f;             // Length of one complete gradient wave (longer for smoother)

    // Breathing animation variables
    unsigned long cycleStartTime;           // When current breathing cycle started
    float breathingPhase;                   // Current phase (0.0 to 1.0)
    bool isFirstHalf;                       // True for first half of cycle, false for second half
    float gradientOffset;                   // Current gradient animation offset
    unsigned long colorSetStartTime;       // When current color set cycle started

    /**
     * Calculate the current breathing intensity (0.0 to 1.0)
     * Uses smooth sine wave for natural breathing feel
     * @return Breathing intensity from 0.0 (start colors) to 1.0 (flipped colors)
     */
    float calculateBreathingIntensity();

    /**
     * Blend between two colors based on intensity
     * @param color1 Starting color (32-bit RGB)
     * @param color2 Target color (32-bit RGB)
     * @param intensity Blend amount (0.0 = color1, 1.0 = color2)
     * @return Blended color as CRGB
     */
    CRGB blendColors(uint32_t color1, uint32_t color2, float intensity);

    /**
     * Apply moving gradient to core strip
     * Core has gradient that moves upward
     * @param intensity Current breathing intensity (0.0 to 1.0)
     * @param hotColor Current hot color to use
     * @param coolColor Current cool color to use
     */
    void updateCoreBreathing(float intensity, uint32_t hotColor, uint32_t coolColor);

    /**
     * Apply moving gradient to inner strips
     * Inner has opposing gradient (offset by half pattern length)
     * @param intensity Current breathing intensity (0.0 to 1.0)
     * @param hotColor Current hot color to use
     * @param coolColor Current cool color to use
     */
    void updateInnerBreathing(float intensity, uint32_t hotColor, uint32_t coolColor);

    /**
     * Apply moving gradient to outer strips
     * Outer has same gradient pattern as core
     * @param intensity Current breathing intensity (0.0 to 1.0)
     * @param hotColor Current hot color to use
     * @param coolColor Current cool color to use
     */
    void updateOuterBreathing(float intensity, uint32_t hotColor, uint32_t coolColor);

    /**
     * Apply moving gradient to ring strip
     * Ring has same gradient pattern as core and outer
     * @param intensity Current breathing intensity (0.0 to 1.0)
     * @param hotColor Current hot color to use
     * @param coolColor Current cool color to use
     */
    void updateRingBreathing(float intensity, uint32_t hotColor, uint32_t coolColor);

    /**
     * Calculate current color set blend ratio (0.0 to 1.0)
     * 0.0 = first color set, 1.0 = second color set
     * @return Color set blend ratio
     */
    float calculateColorSetBlendRatio();

    /**
     * Get the current blended colors based on color set animation
     * @param colorSetRatio Blend ratio between color sets (0.0 to 1.0)
     * @param hotColor Output parameter for the current hot color
     * @param coolColor Output parameter for the current cool color
     */
    void getCurrentColorSet(float colorSetRatio, uint32_t& hotColor, uint32_t& coolColor);

    /**
     * Generate gradient wave color for a specific position and offset
     * Creates a smooth wave that transitions between the current color set
     * @param position LED position in the strip
     * @param offset Animation offset for wave movement
     * @param intensity Breathing intensity for color blending
     * @param reversed True to reverse the wave direction (for inner strips)
     * @param hotColor Current hot color to use
     * @param coolColor Current cool color to use
     * @return Color for this position
     */
    CRGB getGradientWaveColor(int position, float offset, float intensity, bool reversed, uint32_t hotColor, uint32_t coolColor);
};

#endif // LUST_EFFECT_H