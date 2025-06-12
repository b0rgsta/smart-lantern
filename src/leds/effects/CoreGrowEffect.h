// src/leds/effects/CoreGrowEffect.h

#ifndef CORE_GROW_EFFECT_H
#define CORE_GROW_EFFECT_H

#include "Effect.h"

/**
 * CoreGrowEffect - Effect that grows red LEDs from center, then splits and moves outward
 *
 * Features:
 * - Phase 1: Grows from 1 to 25 LEDs from center with brightness fade
 * - Phase 2: Pattern duplicates and both copies move in opposite directions
 * - Continues until both patterns are completely off the strip
 * - All other strips remain off
 */
class CoreGrowEffect : public Effect {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller
     */
    CoreGrowEffect(LEDController& ledController);

    /**
     * Update the animation
     */
    void update() override;

    /**
     * Reset the effect to initial state
     */
    void reset() override;

    /**
     * Get the name of this effect
     */
    String getName() const override { return "Core Grow Effect"; }

private:
    // Animation phases
    enum Phase {
        GROWING = 0,    // Growing from 1 to 17 LEDs
        MOVING = 1      // Two patterns moving in opposite directions
    };

    Phase currentPhase;                 // Current animation phase
    int currentSize;                    // Current size during growing phase (0 to 8)
    int leftPosition;                   // Center position of left-moving pattern
    int rightPosition;                  // Center position of right-moving pattern
    unsigned long lastUpdateTime;       // Last time we updated the animation

    // Timing constants
    static const int MAX_SIZE = 12;         // Maximum LEDs on each side of center (total 25 = 12+1+12)
    static const int GROW_INTERVAL = 140;   // Milliseconds between each growth step (30% faster: 200 * 0.7 = 140)
    static const int MOVE_INTERVAL = 70;    // Milliseconds between each movement step (30% faster: 100 * 0.7 = 70)

    /**
     * Calculate brightness based on distance from center
     * @param offset Distance from center (0 = center, higher = further out)
     * @return Brightness value from 0.0 to 1.0
     */
    float calculateBrightness(int offset);

    /**
     * Draw the full 17-LED pattern at a specific center position
     * @param segment Which core segment (0, 1, or 2)
     * @param centerPos Center position for the pattern
     */
    void drawPattern(int segment, int centerPos);
};

#endif // CORE_GROW_EFFECT_H