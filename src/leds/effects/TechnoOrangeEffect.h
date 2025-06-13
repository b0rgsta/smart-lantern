// src/leds/effects/TechnoOrangeEffect.h

#ifndef TECHNO_ORANGE_EFFECT_H
#define TECHNO_ORANGE_EFFECT_H

#include "Effect.h"

/**
 * TechnoOrangeEffect - An animated color effect for party mode
 *
 * This effect creates dynamic animations on each strip:
 * - Inner strips: Wave animation from bottom to top, then fade out
 * - Outer strips: Orange-to-black gradient with breathing effect (20% to 100%)
 * - Core strip: Static hot pink
 * - Ring strip: Stays off (for button feedback)
 *
 * Inner strip cycle: Fill from bottom (2 seconds) -> hold (1 second) -> fade out (3 seconds)
 * Outer strip cycle: Breathe between 20% and 100% brightness every 5 seconds
 */
class TechnoOrangeEffect : public Effect {
public:
    /**
     * Constructor - creates the techno orange effect
     * @param ledController Reference to the LED controller for drawing
     */
    TechnoOrangeEffect(LEDController& ledController);

    /**
     * Update the effect - applies the colors to all strips
     * Called every frame but colors don't change
     */
    void update() override;

    /**
     * Reset the effect to initial state
     * For this effect, there's nothing to reset since it's static
     */
    void reset() override;

    /**
     * Get the name of this effect for debugging/display
     * @return The effect name as a string
     */
    String getName() const override { return "Techno Orange Effect"; }

private:
    // Color definitions for each strip
    static const uint32_t INNER_COLOR = 0x4B0082;  // Deep purple/blue (indigo)
    static const uint32_t OUTER_COLOR = 0xFF4500;  // Fiery orange (orange red)
    static const uint32_t CORE_COLOR = 0xFF1493;   // Hot pink (deep pink)

    // Animation states for inner strips
    enum InnerAnimationState {
        FILLING_UP = 0,     // LEDs lighting up from bottom to top
        HOLDING = 1,        // All LEDs lit, waiting before fade
        FADING_OUT = 2      // All LEDs fading out together
    };

    // Animation states for core strips
    enum CoreAnimationState {
        CORE_WAITING = 0,   // Waiting for inner strips to reach top
        CORE_FILLING = 1,   // Core LEDs lighting up from bottom to top
        CORE_FADING = 2     // Core LEDs fading out with inner strips
    };

    // Animation timing and state variables
    InnerAnimationState innerState;          // Current state of inner strip animation
    CoreAnimationState coreState;           // Current state of core strip animation
    unsigned long innerAnimationStartTime;   // When current inner animation phase started
    unsigned long coreAnimationStartTime;   // When current core animation phase started
    int innerFillPosition;                   // Current position of the inner fill wave
    int coreFillPosition;                    // Current position of the core fill wave

    // Outer strip breathing variables
    unsigned long outerBreathingStartTime;   // When outer breathing cycle started

    // Timing constants (in milliseconds)
    static const unsigned long INNER_FILL_TIME = 2000;     // 2 seconds to fill from bottom to top
    static const unsigned long INNER_HOLD_TIME = 1000;     // 1 second hold at full brightness
    static const unsigned long INNER_FADE_TIME = 3000;     // 3 seconds to fade out
    static const unsigned long CORE_FILL_TIME = 2000;      // 2 seconds for core to fill (same as inner)
    static const unsigned long OUTER_BREATHING_CYCLE = 5000; // 5 seconds for full breathing cycle

    // Core color definition
    static const uint32_t CORE_WHITE_COLOR = 0xFFFFFF;     // Pure white for core animation

    // Brightness constants for outer strips
    static constexpr float OUTER_MIN_BRIGHTNESS = 0.2f;  // 20% minimum brightness
    static constexpr float OUTER_MAX_BRIGHTNESS = 1.0f;  // 100% maximum brightness

    /**
     * Update the inner strip wave animation
     * Handles filling up, holding, and fading out phases
     */
    void updateInnerAnimation();

    /**
     * Update the core strip wave animation
     * Handles waiting, filling up, and fading out phases (synchronized with inner strips)
     */
    void updateCoreAnimation();

    /**
     * Update the outer strip breathing effect
     * Creates smooth breathing between 20% and 100% brightness
     */
    void updateOuterAnimation();

    /**
     * Apply a gradient from a color to black on a strip
     * @param strip Pointer to the LED strip array
     * @param count Number of LEDs in the strip
     * @param baseColor The starting color (bottom of strip)
     * @param brightness Overall brightness multiplier (0.0 to 1.0)
     */
    void applyGradientToStrip(CRGB* strip, int count, uint32_t baseColor, float brightness);

    /**
     * Apply a solid color to an entire LED strip
     * @param strip Pointer to the LED strip array
     * @param count Number of LEDs in the strip
     * @param color The color to apply (as 32-bit RGB value)
     */
    void applyColorToStrip(CRGB* strip, int count, uint32_t color);
};

#endif // TECHNO_ORANGE_EFFECT_H