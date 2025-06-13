#ifndef RAINBOW_EFFECT_H
#define RAINBOW_EFFECT_H

#include "Effect.h"

/**
 * Rainbow Effect - Creates a smooth rainbow gradient that moves around the LED strips
 *
 * This effect creates a rainbow pattern that continuously shifts colors.
 * The animation speed is frame rate independent.
 * The core strip breathes in and out over 5 seconds (fading from full brightness to off and back).
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

    // Core breathing effect variables
    float breathingPhase;   // Current phase of breathing cycle (0.0 to 2*PI)
    float breathingSpeed;   // Speed of breathing cycle (to complete 5 second cycle)
};

#endif // RAINBOW_EFFECT_H