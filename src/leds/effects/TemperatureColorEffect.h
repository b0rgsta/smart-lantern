// src/leds/effects/TemperatureColorEffect.h

#ifndef TEMPERATURE_COLOR_EFFECT_H
#define TEMPERATURE_COLOR_EFFECT_H

#include "Effect.h"

/**
 * TemperatureColorEffect - Creates a solid color based on color temperature (Kelvin)
 *
 * Features:
 * - Converts color temperature in Kelvin to RGB color
 * - Allows enabling/disabling individual LED strips (core, inner, outer, ring)
 * - Applies fade to black gradient on outer strips (from bottom to top)
 * - Static effect - no animation, just sets the color once
 *
 * Common color temperatures:
 * - 1700K: Match flame
 * - 2700K: Warm white (incandescent bulb)
 * - 3000K: Soft white
 * - 4000K: Cool white (fluorescent)
 * - 5000K: Daylight
 * - 6500K: Cool daylight
 */
class TemperatureColorEffect : public Effect {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller
     * @param temperatureK Color temperature in Kelvin (1000-10000K typical range)
     * @param enableCore Whether to show color on core strip (default: true)
     * @param enableInner Whether to show color on inner strips (default: true)
     * @param enableOuter Whether to show color on outer strips with fade (default: true)
     * @param enableRing Whether to show color on ring strip (default: true)
     */
    TemperatureColorEffect(LEDController& ledController,
                          uint16_t temperatureK = 3000,  // Default warm white
                          bool enableCore = true,
                          bool enableInner = true,
                          bool enableOuter = true,
                          bool enableRing = true);

    /**
     * Update the effect - applies the color temperature to enabled strips
     * Since this is a static effect, it only needs to set colors once
     */
    void update() override;

    /**
     * Reset the effect - for static effect, just reapplies the color
     */
    void reset() override;

    /**
     * Get the name of this effect
     * @return Effect name for display/debugging
     */
    String getName() const override;

    /**
     * Set a new color temperature
     * @param temperatureK New temperature in Kelvin
     */
    void setTemperature(uint16_t temperatureK);

    /**
     * Get current color temperature
     * @return Current temperature in Kelvin
     */
    uint16_t getTemperature() const { return temperature; }

    /**
     * Enable or disable individual strips
     */
    void setCoreEnabled(bool enabled) { coreEnabled = enabled; needsUpdate = true; }
    void setInnerEnabled(bool enabled) { innerEnabled = enabled; needsUpdate = true; }
    void setOuterEnabled(bool enabled) { outerEnabled = enabled; needsUpdate = true; }
    void setRingEnabled(bool enabled) { ringEnabled = enabled; needsUpdate = true; }

private:
    // Current color temperature in Kelvin
    uint16_t temperature;

    // Calculated RGB color from temperature
    CRGB calculatedColor;

    // Strip enable flags - control which strips show the color
    bool coreEnabled;       // Whether core strip shows color
    bool innerEnabled;      // Whether inner strips show color
    bool outerEnabled;      // Whether outer strips show color (with fade)
    bool ringEnabled;       // Whether ring strip shows color

    // Flag to track if we need to update the LEDs
    bool needsUpdate;

    // Fade parameters for outer strips
    static constexpr float FADE_START_POSITION = 0.9f;  // Start fading at 30% up the strip
    static constexpr float MIN_BRIGHTNESS = 0.0f;       // Fade to complete black at top

    /**
     * Convert color temperature in Kelvin to RGB color
     * Uses algorithm based on Tanner Helland's work
     * @param kelvin Temperature in Kelvin (1000-40000K)
     * @return RGB color as CRGB
     */
    CRGB kelvinToRGB(uint16_t kelvin);

    /**
     * Apply solid color to a strip (no fade)
     * @param strip Pointer to the LED strip array
     * @param count Number of LEDs in the strip
     * @param color The color to apply
     */
    void applySolidColor(CRGB* strip, int count, CRGB color);

    /**
     * Apply color with fade to black gradient to outer strips
     * Fades from full brightness at bottom to black at top
     * @param strip Pointer to the LED strip array
     * @param count Total number of LEDs in the strip
     * @param color The base color to apply
     */
    void applyFadeToOuter(CRGB* strip, int count, CRGB color);
};

#endif // TEMPERATURE_COLOR_EFFECT_H