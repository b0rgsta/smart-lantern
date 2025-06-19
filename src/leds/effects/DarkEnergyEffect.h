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
 * - Core strip: Remains off
 * - Ring strip: Remains off
 * - Static effect - no animation, maintains constant appearance
 *
 * This effect creates a mysterious dark energy appearance with red glow
 * fading to black at the extremities, suggesting contained energy.
 */
class DarkEnergyEffect : public Effect {
public:
    /**
     * Constructor - initializes the dark energy effect
     * @param ledController Reference to the LED controller for managing LEDs
     */
    DarkEnergyEffect(LEDController& ledController);

    /**
     * Update the effect - applies the dark energy pattern
     * Since this is a static effect, it only sets colors once when needed
     */
    void update() override;

    /**
     * Reset the effect to initial state
     * Reapplies the dark energy pattern
     */
    void reset() override;

    /**
     * Get the name of this effect for debugging/display
     * @return The effect name as a string
     */
    String getName() const override { return "Dark Energy Effect"; }

private:
    // Flag to track if we need to update the LEDs (for static effect optimization)
    bool needsUpdate;

    // Effect color constants
    static constexpr uint32_t BASE_RED_COLOR = 0xFF0000;    // Pure red color
    static constexpr float BASE_BRIGHTNESS = 0.5f;         // 50% brightness
    static constexpr float FADE_PERCENTAGE = 0.9f;         // 90% fade to black

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
};

#endif // DARK_ENERGY_EFFECT_H