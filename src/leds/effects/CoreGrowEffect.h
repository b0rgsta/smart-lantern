// src/leds/effects/CoreGrowEffect.h

#ifndef CORE_GROW_EFFECT_H
#define CORE_GROW_EFFECT_H

#include "Effect.h"
#include <vector>

// Structure to represent a core effect trail
struct CoreTrail {
    int stripType;      // 1 = inner, 2 = outer
    int subStrip;       // Which segment (0, 1, or 2)
    float position;     // Current head position (float for smooth movement)
    float speed;        // Movement speed (pixels per frame)
    bool active;        // Whether this trail is active
    bool direction;     // true = upward, false = downward
};

/**
 * CoreGrowEffect - Effect that grows red LEDs from center, then splits and moves outward,
 * plus trails on inner and outer strips with breathing brightness
 *
 * Features:
 * - Core: Phase 1: Grows from 1 to 25 LEDs from center with brightness fade
 * - Core: Phase 2: Pattern duplicates and both copies move in opposite directions
 * - Outer strips: Random red trails (15 LEDs) shooting upward, white leading LED
 * - Inner strips: Random red trails (15 LEDs) shooting downward, white leading LED
 * - Trails: Breathing effect that fades from 40% to 100% brightness
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

    // Breathing effect variables for trails
    float breathingPhase;               // Current phase of breathing cycle (0.0 to 2*PI)
    float breathingSpeed;               // Speed of breathing cycle
    float minBrightness;                // Minimum brightness (40%)
    float maxBrightness;                // Maximum brightness (100%)

    // Timing constants for core effect
    static const int MAX_SIZE = 12;         // Maximum LEDs on each side of center (total 25 = 12+1+12)
    static const int GROW_INTERVAL = 100;   // Milliseconds between each growth step (slower for smoother appearance)
    static const int MOVE_INTERVAL = 50;    // Milliseconds between each movement step (faster for smoother movement)

    // Trail constants
    static const int MAX_TRAILS = 24;       // Maximum number of trails at once (doubled from 12)
    static const int TRAIL_LENGTH = 52;     // Length of each trail in LEDs (doubled from 26)
    static const int TARGET_TRAILS = 16;    // Target number of trails to maintain (doubled from 8)

    // Trail timing
    unsigned long lastTrailCreateTime;       // Last time we created a trail
    static const int TRAIL_CREATE_INTERVAL = 80;  // Create a new trail every 80ms (very frequent)
    static const int TRAIL_STAGGER_VARIANCE = 40; // Add random variance to prevent waves

    // Trail management
    std::vector<CoreTrail> trails;          // Collection of all trails

    /**
     * Calculate the current breathing brightness multiplier
     * @return Brightness multiplier between minBrightness and maxBrightness
     */
    float calculateBreathingBrightness();

    /**
     * Create a new trail on a random strip
     */
    void createNewTrail();

    /**
     * Update all active trails
     */
    void updateTrails();

    /**
     * Draw all active trails (with breathing brightness applied)
     */
    void drawTrails();

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