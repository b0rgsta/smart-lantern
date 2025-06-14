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

// Structure to represent a ring trail (circular movement)
struct RingTrail {
    float position;     // Current head position around the ring (0 to LED_STRIP_RING_COUNT)
    float speed;        // Movement speed (pixels per frame)
    int length;         // Length of the trail
    bool active;        // Whether this trail is active
    bool clockwise;     // true = clockwise, false = counter-clockwise
    unsigned long creationTime;  // When this trail was created (for lifespan tracking)
    unsigned long lifespan;      // How long this trail should live (in milliseconds)
};

/**
 * CoreGrowEffect - Effect that grows red LEDs from center, then splits and moves outward,
 * plus trails on inner and outer strips with breathing brightness, and breathing ring trails
 *
 * Features:
 * - Core: Phase 1: Grows from 1 to 25 LEDs from center with brightness fade
 * - Core: Phase 2: Pattern duplicates and both copies move in opposite directions
 * - Outer strips: Random red trails (15 LEDs) shooting upward, white leading LED
 * - Inner strips: Random red trails (15 LEDs) shooting downward, white leading LED
 * - Trails: Breathing effect that fades from 40% to 100% brightness
 * - Ring: Breathing red trails that move in circles around the ring
 * - All breathing elements use the same timing for synchronized effect
 */
class CodeRedEffect : public Effect {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller
     */
    CodeRedEffect(LEDController& ledController);

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

    // Breathing effect variables for trails AND ring (synchronized)
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
    std::vector<RingTrail> ringTrails;      // Collection of ring trails

    // Ring trail constants
    static const int MAX_RING_TRAILS = 6;        // Maximum number of ring trails at once
    static const int RING_TRAIL_LENGTH = 12;     // Length of each ring trail in LEDs
    static const int TARGET_RING_TRAILS = 4;     // Target number of ring trails to maintain

    // Ring trail timing
    unsigned long lastRingTrailCreateTime;       // Last time we created a ring trail
    static const int RING_TRAIL_CREATE_INTERVAL = 150;  // Create a new ring trail every 150ms
    static const int RING_TRAIL_STAGGER_VARIANCE = 50;  // Add random variance to prevent waves

    // Ring effect constants
    static constexpr uint32_t RING_COLOR = 0xFF0000;    // Red color for ring (matches trails)
    static constexpr float RING_MIN_BRIGHTNESS = 0.15f; // 15% minimum brightness for more dramatic breathing
    static constexpr float RING_MAX_BRIGHTNESS = 1.0f;  // 100% maximum brightness for more dramatic breathing

    /**
     * Calculate the current breathing brightness multiplier
     * Used by both trails and ring for synchronized breathing
     * @return Brightness multiplier between minBrightness and maxBrightness
     */
    float calculateBreathingBrightness();

    /**
     * Calculate the ring breathing brightness multiplier
     * Uses the same breathing phase but different min/max values
     * @return Brightness multiplier between RING_MIN_BRIGHTNESS and RING_MAX_BRIGHTNESS
     */
    float calculateRingBreathingBrightness();

    /**
     * Update the ring trail effects
     * Manages creation and movement of circular trails around the ring
     */
    void updateRingTrails();

    /**
     * Create a new ring trail at a random position
     */
    void createNewRingTrail();

    /**
     * Draw all active ring trails with breathing brightness
     */
    void drawRingTrails();

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