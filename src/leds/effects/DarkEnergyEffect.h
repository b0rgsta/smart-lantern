// src/leds/effects/DarkEnergyEffect.h

#ifndef DARK_ENERGY_EFFECT_H
#define DARK_ENERGY_EFFECT_H

#include "Effect.h"

/**
 * DarkEnergyEffect - Creates a dark energy visual effect
 *
 * Features:
 * - Inner and outer strips: Red color at 50% brightness
 * - 90% black fade from both ends toward the center
 * - Hovering black ball that moves up and down with smooth acceleration/deceleration
 * - Black ball grows and shrinks with breathing effect
 * - Ball covers 70% of strip length when centered
 * - Ball travel range animates between 30% and 70% of strip length
 * - Core strip: Remains off
 * - Ring strip: Remains off
 * - Dynamic effect with smooth animations
 *
 * This effect creates a mysterious dark energy appearance with red glow
 * fading to black at the extremities, with a hovering dark sphere that
 * suggests contained and controlled dark energy with expanding/contracting boundaries.
 */
class DarkEnergyEffect : public Effect {
public:
    /**
     * Constructor - initializes the dark energy effect
     * @param ledController Reference to the LED controller for managing LEDs
     */
    DarkEnergyEffect(LEDController& ledController);

    /**
     * Update the effect - applies the dark energy pattern with hovering black ball
     * Handles ball movement, breathing, range animation, and red base pattern
     */
    void update() override;

    /**
     * Reset the effect to initial state
     * Resets ball position, breathing phase, and range phase
     */
    void reset() override;

    /**
     * Get the name of this effect for debugging/display
     * @return The effect name as a string
     */
    String getName() const override { return "Dark Energy Effect"; }

private:
    // Effect color constants
    static constexpr uint32_t BASE_RED_COLOR = 0xFF0000;    // Pure red color
    static constexpr float BASE_BRIGHTNESS = 0.5f;         // 50% brightness
    static constexpr float FADE_PERCENTAGE = 0.9f;         // 90% fade to black

    // Black ball animation constants
    static constexpr float BALL_COVERAGE = 0.7f;           // Ball covers 70% of strip length
    static constexpr float BALL_MOVE_SPEED = 0.025f;       // Speed of up/down movement
    static constexpr float BALL_BREATHING_SPEED = 0.015f;  // Speed of breathing effect
    static constexpr float BALL_MIN_SIZE = 0.6f;           // Minimum size (60% of base size)
    static constexpr float BALL_MAX_SIZE = 1.4f;           // Maximum size (140% of base size)
    static constexpr float BALL_GAP_PERCENTAGE = 0.2f;     // 20% gap at each end of strip
    static constexpr float BALL_RANGE_SPEED = 0.008f;      // Speed of range expansion/contraction
    static constexpr float BALL_MIN_RANGE = 0.3f;          // Minimum travel range (30% of strip)
    static constexpr float BALL_MAX_RANGE = 0.7f;          // Maximum travel range (70% of strip)

    // Energy pulse constants
    static constexpr float ENERGY_PULSE_SPEED = 0.012f;    // Speed of energy pulsing
    static constexpr float ENERGY_MIN_INTENSITY = 0.7f;    // Minimum pulse intensity (70% of base)
    static constexpr float ENERGY_MAX_INTENSITY = 1.3f;    // Maximum pulse intensity (130% of base)

    // Animation state variables
    float ballPosition;         // Current vertical position (0.0 = bottom, 1.0 = top)
    float ballVelocity;         // Current movement velocity
    float breathingPhase;       // Current breathing animation phase (0.0 to 2*PI)
    float rangePhase;           // Current range animation phase (0.0 to 2*PI)
    float energyPhase;          // Current energy pulse phase (0.0 to 2*PI)
    unsigned long lastUpdateTime;  // Time tracking for frame-rate independence

    /**
     * Apply the dark energy pattern to inner strips
     * Each strip gets red at 50% brightness with 90% black fade from both ends
     */
    void applyInnerPattern();

    /**
     * Apply the dark energy pattern to outer strips
     * Each strip gets red at 50% brightness with 90% black fade from both ends
     */
    void applyOuterPattern();

    /**
     * Calculate the brightness multiplier for a given position in a strip
     * Creates 90% black fade from both ends toward center
     * @param position Current LED position in the strip (0 to stripLength-1)
     * @param stripLength Total length of the strip
     * @return Brightness multiplier (0.0 = black, 1.0 = full effect brightness)
     */
    float calculateFadeBrightness(int position, int stripLength);

    /**
     * Apply red color with calculated brightness to a specific LED
     * @param color Reference to the LED color to modify
     * @param brightnessFactor Brightness multiplier to apply (0.0 to 1.0)
     */
    void applyRedWithBrightness(CRGB& color, float brightnessFactor);

    /**
     * Update the black ball animation
     * Handles position, velocity, breathing, and range calculations
     */
    void updateBlackBall();

    /**
     * Apply the black ball effect to both inner and outer strips
     * Creates black sphere that covers the red base effect
     */
    void applyBlackBall();

    /**
     * Calculate current ball size based on breathing effect
     * @return Size multiplier (BALL_MIN_SIZE to BALL_MAX_SIZE)
     */
    float calculateBallSize();

    /**
     * Calculate current travel range based on range animation
     * @return Current travel range (BALL_MIN_RANGE to BALL_MAX_RANGE)
     */
    float calculateTravelRange();

    /**
     * Calculate current energy pulse intensity
     * @return Energy intensity multiplier (ENERGY_MIN_INTENSITY to ENERGY_MAX_INTENSITY)
     */
    float calculateEnergyIntensity();
};

#endif // DARK_ENERGY_EFFECT_H