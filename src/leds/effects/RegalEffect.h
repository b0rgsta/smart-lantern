// src/leds/effects/TechnoOrangeEffect.h

#ifndef TECHNO_ORANGE_EFFECT_H
#define TECHNO_ORANGE_EFFECT_H

#include "Effect.h"

/**
 * TechnoOrangeEffect - An animated color effect for party mode
 *
 * This effect creates dynamic animations on each strip:
 * - Inner strips: Wave animation from bottom to top, then fade out (bluish-purple color)
 * - Outer strips: Orange-to-black gradient with breathing effect (20% to 100%)
 * - Core strip: Purple wave animation synchronized with inner strips (with shimmer effect)
 * - Ring strip: Orange breathing effect that breathes opposite to outer strips
 *
 * Inner strip cycle: Fill from bottom (2 seconds) -> hold (1 second) -> fade out (3 seconds)
 * Core strip cycle: Waits for inner 50% fill -> fills with purple + shimmer -> fades with inner
 * Outer strip cycle: Breathe between 20% and 100% brightness every 5 seconds
 * Ring strip cycle: Breathe between 30% and 90% brightness opposite to outer strips
 */
class RegalEffect : public Effect {
public:
    /**
     * Constructor - creates the techno orange effect
     * @param ledController Reference to the LED controller for drawing
     */
    RegalEffect(LEDController& ledController);

    /**
     * Destructor - cleans up allocated memory
     */
    ~RegalEffect();

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
    static const uint32_t INNER_COLOR = 0x250da3;  // More vibrant blue with slight purple tint (Royal Blue)
    static const uint32_t OUTER_COLOR = 0xFF4500;  // Fiery orange (orange red)
    static const uint32_t CORE_COLOR = 0x9314FF;   // Hot pink (deep pink) - not used anymore

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

    // Shimmer effect variables for core animation
    unsigned long lastShimmerUpdate;         // When shimmer was last updated
    static constexpr unsigned long SHIMMER_UPDATE_INTERVAL = 70;  // Update shimmer every 50ms (20 FPS)

    // Array to store shimmer brightness multipliers for each core LED
    float* coreShimmerValues;                // Dynamic array for shimmer values

    // Timing constants (in milliseconds) - moved to public section for access
    static constexpr unsigned long INNER_FILL_TIME = 2000;     // 2 seconds to fill from bottom to top
    static constexpr unsigned long INNER_HOLD_TIME = 1000;     // 1 second hold at full brightness
    static constexpr unsigned long INNER_FADE_TIME = 3000;     // 3 seconds to fade out
    static constexpr unsigned long CORE_FILL_TIME = 2000;      // 2 seconds for core to fill (same as inner)
    static constexpr unsigned long OUTER_BREATHING_CYCLE = 5000; // 5 seconds for full breathing cycle

    // Core color definition - now purple instead of white
    static constexpr uint32_t CORE_PURPLE_COLOR = 0x8A2BE2;    // Purple color for core animation

    // Brightness constants for outer strips
    static constexpr float OUTER_MIN_BRIGHTNESS = 0.2f;  // 20% minimum brightness
    static constexpr float OUTER_MAX_BRIGHTNESS = 1.0f;  // 100% maximum brightness

    // Brightness constants for ring strips (breathing opposite to outer)
    static constexpr float RING_MIN_BRIGHTNESS = 0.1f;   // 10% minimum brightness (more dramatic)
    static constexpr float RING_MAX_BRIGHTNESS = 1.0f;   // 100% maximum brightness (more dramatic)

    // Ring color definition - more red than outer strips
    static constexpr uint32_t RING_COLOR = 0xFF2000;     // Red-orange color (more red than OUTER_COLOR)

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
     * Update the shimmer effect for core LEDs
     * Creates random brightness variations for a dazzling effect
     */
    void updateCoreShimmer();

    /**
     * Update the outer strip breathing effect
     * Creates smooth breathing between 20% and 100% brightness
     */
    void updateOuterAnimation();

    /**
     * Update the ring strip breathing effect
     * Creates smooth breathing opposite to outer strips using orange color
     */
    void updateRingAnimation();

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

    /**
     * Apply a solid color to an entire LED strip with brightness control
     * @param strip Pointer to the LED strip array
     * @param count Number of LEDs in the strip
     * @param color The color to apply (as 32-bit RGB value)
     * @param brightness Brightness multiplier (0.0 to 1.0)
     */
    void applyColorToStripWithBrightness(CRGB* strip, int count, uint32_t color, float brightness);
};

#endif // TECHNO_ORANGE_EFFECT_H