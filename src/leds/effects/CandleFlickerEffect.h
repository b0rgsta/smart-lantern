// src/leds/effects/CandleFlickerEffect.h

#ifndef CANDLE_FLICKER_EFFECT_H
#define CANDLE_FLICKER_EFFECT_H

#include "Effect.h"

/**
 * CandleFlickerEffect - Creates a warm candle-like flickering effect
 *
 * Features:
 * - Warm candle color temperature (1800K) with realistic flickering
 * - Global flicker affecting entire lamp for unified candle breathing
 * - Subtle zone variations on top of global flicker for flame movement
 * - Outer strips fade to black from bottom to top (like other ambient effects)
 * - Inner strips show full flickering candle light
 * - Core and ring strips disabled for ambient lighting focus
 *
 * This effect simulates a realistic candle with smooth global breathing
 * and gentle zone variations instead of harsh strobing effects.
 */
class CandleFlickerEffect : public Effect {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller
     */
    CandleFlickerEffect(LEDController& ledController);

    /**
     * Update the flickering candle effect
     * Updates global and zone flicker intensities and applies candle color with fade
     */
    void update() override;

    /**
     * Reset the effect - resets all flicker intensities
     */
    void reset() override;

    /**
     * Get the name of this effect
     * @return Effect name for display/debugging
     */
    String getName() const override { return "Candle Flicker"; }

private:
    // Base candle color (warm 1800K temperature)
    CRGB baseColor;

    // Global flicker intensity (affects entire lamp for unified candle breathing)
    float globalFlickerIntensity;
    float globalFlickerTarget;

    // Zone intensities (subtle variations on top of global flicker)
    float mainFlameIntensity;        // Top flame zone (subtle variation)
    float secondaryFlameIntensity;   // Middle flame zone (more subtle)
    float baseGlowIntensity;         // Bottom glow zone (most stable)

    // Zone targets (for smooth transitions)
    float mainFlameTarget;
    float secondaryFlameTarget;
    float baseGlowTarget;

    // Timing control
    unsigned long lastFlickerUpdate;

    /**
     * Update global and zone flicker intensities
     * Creates smooth global candle breathing with subtle zone variations
     * - Global flicker affects entire lamp for unified candle effect
     * - Zone variations add subtle flame movement on top of global flicker
     */
    void updateFlickerIntensities();

    /**
     * Apply candle flame zones to inner strips
     * Creates realistic flame gradient with different zones
     */
    void applyCandleFlameToInner();

    /**
     * Apply candle flame zones with fade to outer strips
     * Combines flame zones with fade to black gradient (bottom to top)
     */
    void applyCandleFlameAndFadeToOuter();

    /**
     * Convert color temperature to RGB (1800K candle color)
     * @return Warm candle color as CRGB
     */
    CRGB getCandleColor();
};

#endif // CANDLE_FLICKER_EFFECT_H