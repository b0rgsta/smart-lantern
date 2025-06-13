// src/leds/effects/RainbowTranceEffect.h

#ifndef RAINBOW_TRANCE_EFFECT_H
#define RAINBOW_TRANCE_EFFECT_H

#include "Effect.h"
#include <vector>

// Structure to represent a core effect trail with random colors
struct RainbowTrail {
    int stripType;      // 1 = inner, 2 = outer
    int subStrip;       // Which segment (0, 1, or 2)
    float position;     // Current head position (float for smooth movement)
    float speed;        // Movement speed (pixels per frame)
    bool active;        // Whether this trail is active
    bool direction;     // true = upward, false = downward
    uint8_t hue;        // Color hue for this trail (0-255)
    uint8_t saturation; // Color saturation for this trail (0-255)
    uint8_t brightness; // Color brightness/vibrancy for this trail (0-255)
};

// Structure to represent a ring trail (circular movement) with RGB colors
struct RainbowRingTrail {
    float position;     // Current head position around the ring (0 to LED_STRIP_RING_COUNT)
    float speed;        // Movement speed (pixels per frame)
    int length;         // Length of the trail
    bool active;        // Whether this trail is active
    bool clockwise;     // true = clockwise, false = counter-clockwise
    unsigned long creationTime;  // When this trail was created (for lifespan tracking)
    unsigned long lifespan;      // How long this trail should live (in milliseconds)
    uint8_t hue;        // Color hue for this ring trail (0=red, 85=green, 170=blue)
    bool isFading;      // Whether this trail is in fade-out phase
    unsigned long fadeStartTime; // When the fade-out started
    static const unsigned long FADE_DURATION = 2000; // 2 seconds fade-out time
};

/**
 * RainbowTranceEffect - Effect that grows random colored LEDs from center, then splits and moves outward,
 * plus trails on inner and outer strips with random colors and vibrancy, and rainbow cycling ring trails
 *
 * Features:
 * - Core: Phase 1: Grows from 1 to 25 LEDs from center with random colors at full vibrance each cycle
 * - Core: Phase 2: Pattern duplicates and both copies move in opposite directions
 * - Outer strips: Random colored trails (52 LEDs) shooting upward, white leading LED, random vibrancy
 * - Inner strips: Random colored trails (52 LEDs) shooting downward, white leading LED, random vibrancy
 * - Trails: Breathing effect that fades from 40% to 100% brightness
 * - Ring: Breathing red trails that move in circles and cycle through color wheel over 10 seconds
 * - All breathing elements use the same timing for synchronized effect
 */
class RainbowTranceEffect : public Effect {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller
     */
    RainbowTranceEffect(LEDController& ledController);

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
    String getName() const override { return "Rainbow Trance Effect"; }

private:
    // Animation phases for core effect
    enum Phase {
        GROWING = 0,    // Growing from 1 to 17 LEDs
        MOVING = 1      // Two patterns moving in opposite directions
    };

    Phase currentPhase;                 // Current animation phase
    int currentSize;                    // Current size during growing phase (0 to 8)
    int leftPosition;                   // Center position of left-moving pattern
    int rightPosition;                  // Center position of right-moving pattern
    unsigned long lastUpdateTime;       // Last time we updated the animation

    // Core effect random colors - these change each cycle
    uint8_t coreHue;                    // Current hue for core effect (0-255)
    uint8_t coreSaturation;             // Current saturation for core effect (always 255 for full vibrance)
    uint8_t coreBrightness;             // Current brightness for core effect (always 255 for full vibrance)

    // Breathing effect variables for trails (synchronized)
    float breathingPhase;               // Current phase of breathing cycle (0.0 to 2*PI)
    float breathingSpeed;               // Speed of breathing cycle
    float minBrightness;                // Minimum brightness (40%)
    float maxBrightness;                // Maximum brightness (100%)

    // Ring color cycling variables (10 second cycle)
    unsigned long colorCycleStartTime;  // When the current color cycle started
    static const unsigned long COLOR_CYCLE_DURATION = 10000; // 10 seconds in milliseconds

    // Timing constants for core effect
    static const int MAX_SIZE = 12;         // Maximum LEDs on each side of center (total 25 = 12+1+12)
    static const int GROW_INTERVAL = 100;   // Milliseconds between each growth step (slower for smoother appearance)
    static const int MOVE_INTERVAL = 50;    // Milliseconds between each movement step (faster for smoother movement)

    // Trail constants
    static const int MAX_TRAILS = 24;       // Maximum number of trails at once
    static const int TRAIL_LENGTH = 104;    // Length of each trail in LEDs (doubled from 52)
    static const int TARGET_TRAILS = 16;    // Target number of trails to maintain

    // Trail timing
    unsigned long lastTrailCreateTime;       // Last time we created a trail
    static const int TRAIL_CREATE_INTERVAL = 80;  // Create a new trail every 80ms
    static const int TRAIL_STAGGER_VARIANCE = 40; // Add random variance to prevent waves

    // Trail management
    std::vector<RainbowTrail> trails;       // Collection of all trails with random colors
    std::vector<RainbowRingTrail> ringTrails; // Collection of ring trails

    // Ring trail constants
    static const int MAX_RING_TRAILS = 6;        // Maximum number of ring trails at once
    static const int RING_TRAIL_LENGTH = 12;     // Length of each ring trail in LEDs
    static const int TARGET_RING_TRAILS = 4;     // Target number of ring trails to maintain

    // Ring trail timing
    unsigned long lastRingTrailCreateTime;       // Last time we created a ring trail
    static const int RING_TRAIL_CREATE_INTERVAL = 150;  // Create a new ring trail every 150ms
    static const int RING_TRAIL_STAGGER_VARIANCE = 50;  // Add random variance to prevent waves

    // Ring effect constants for color cycling
    static constexpr float RING_MIN_BRIGHTNESS = 0.15f; // 15% minimum brightness for dramatic breathing
    static constexpr float RING_MAX_BRIGHTNESS = 1.0f;  // 100% maximum brightness for dramatic breathing

    /**
     * Generate random color values for core effect (full vibrance)
     * Called at the start of each new core cycle
     */
    void generateRandomCoreColor();

    /**
     * Generate random color values for a new trail
     * @param trail Reference to the trail to set colors for
     */
    void generateRandomTrailColor(RainbowTrail& trail);

    /**
     * Calculate the current color hue for ring effect based on 10-second cycle
     * @return Hue value (0-255) that cycles through full color wheel over 10 seconds
     */
    uint8_t calculateRingCycleHue();

    /**
     * Calculate the current breathing brightness multiplier
     * Used by trails for synchronized breathing
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
     * Update the ring trail effects with color cycling
     * Manages creation and movement of circular trails around the ring
     */
    void updateRingTrails();

    /**
     * Create a new ring trail at a random position
     */
    void createNewRingTrail();

    /**
     * Draw all active ring trails with breathing brightness and color cycling
     */
    void drawRingTrails();

    /**
     * Create a new trail on a random strip with random colors
     */
    void createNewTrail();

    /**
     * Update all active trails
     */
    void updateTrails();

    /**
     * Draw all active trails with random colors and breathing brightness
     */
    void drawTrails();

    /**
     * Calculate brightness based on distance from center
     * @param offset Distance from center (0 = center, higher = further out)
     * @return Brightness value from 0.0 to 1.0
     */
    float calculateBrightness(int offset);

    /**
     * Draw the full 17-LED pattern at a specific center position using current core colors
     * @param segment Which core segment (0, 1, or 2)
     * @param centerPos Center position for the pattern
     */
    void drawPattern(int segment, int centerPos);
};

#endif // RAINBOW_TRANCE_EFFECT_H