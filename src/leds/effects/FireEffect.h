#ifndef FIRE_EFFECT_H
#define FIRE_EFFECT_H

#include "Effect.h"

/**
 * FireEffect - Creates a realistic fire animation effect on LED strips
 *
 * This effect creates a realistic fire animation with:
 * - Core strip: Glowing embers between red and orange
 * - Inner strips: First layer of fire animation
 * - Outer strips: Second layer of fire animation
 * - Ring strip: Off (no LEDs lit)
 */
class FireEffect : public Effect {
public:
    /**
     * Constructor
     *
     * @param ledController Reference to the LED controller
     */
    FireEffect(LEDController& ledController);

    /**
     * Update the fire effect animation
     * Called on each animation frame
     */
    void update() override;

    /**
     * Reset the effect to its initial state
     */
    void reset() override;

private:
    // Heat values for each LED in each strip
    uint8_t* heatCore;
    uint8_t* heatInner;
    uint8_t* heatOuter;

    // Initialize heat arrays
    void initHeatArrays();

    // Generate fire effect for a specific strip
    void updateFire(uint8_t* heat, int count, Adafruit_NeoPixel& strip, uint8_t cooling, uint8_t sparking);

    // Map "heat" value to RGB color for a specific LED
    uint32_t heatToColor(uint8_t heat);
};

#endif // FIRE_EFFECT_H