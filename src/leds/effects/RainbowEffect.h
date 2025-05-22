#ifndef RAINBOW_EFFECT_H
#define RAINBOW_EFFECT_H

#include "Effect.h"

/**
 * Rainbow Effect - Creates a smooth rainbow gradient that moves around the LED strips
 *
 * This effect creates a rainbow pattern that continuously shifts colors.
 * The animation speed is frame rate independent.
 */
class RainbowEffect : public Effect {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller
     */
    RainbowEffect(LEDController& ledController);

    /**
     * Update the rainbow animation
     * Uses frame rate independent timing for consistent speed
     */
    void update() override;

    /**
     * Reset the effect to starting position
     */
    void reset() override;

    /**
     * Get the name of this effect
     * @return Effect name for display/debugging
     */
    String getName() const override { return "Rainbow Effect"; }

private:
    float cycle;            // Current position in rainbow cycle (0-255.99)
    float animationSpeed;   // Animation speed in cycles per second
};

#endif // RAINBOW_EFFECT_H