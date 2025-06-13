// src/leds/effects/RainbowTranceEffect.h

#ifndef RAINBOW_TRANCE_EFFECT_H
#define RAINBOW_TRANCE_EFFECT_H

#include "Effect.h"
#include <vector>

// Structure to represent a synchronized trail set for inner or outer strips
struct SyncedTrail {
    int stripType;      // 1 = inner, 2 = outer
    float position;     // Current head position (float for smooth movement)
    float speed;        // Movement speed (pixels per frame)
    bool active;        // Whether this trail is active
    bool direction;     // true = upward, false = downward
    uint8_t hue;        // Color hue for this trail (0-255)
    uint8_t saturation; // Color saturation for this trail (0-255)
    uint8_t brightness; // Color brightness/vibrancy for this trail (0-255)
};

// Structure to represent a continuous ring trail
struct ContinuousRingTrail {
    float position;     // Current head position around the ring (0 to LED_STRIP_RING_COUNT)
    float speed;        // Movement speed (pixels per frame)
    int length;         // Length of the trail
    uint8_t hue;        // Fixed color hue for this trail (0=red, 85=green, 160=blue)
    bool clockwise;     // Direction (always true for all 3 trails)
};

/**
 * RainbowTranceEffect - Effect that grows random colored LEDs from center, then splits and moves outward,
 * plus synchronized trails on inner and outer strips, and 3 continuous RGB trails on ring
 *
 * Features:
 * - Core: Phase 1: Grows from 1 to 25 LEDs from center with random colors at full vibrance each cycle
 * - Core: Phase 2: Pattern duplicates and both copies move in opposite directions
 * - Inner strips: All 3 segments show the same synchronized trails shooting in random directions
 * - Outer strips: All 3 segments show the same synchronized trails shooting in random directions
 * - Multiple trails can exist on the same strip and colors blend when overlapping
 * - Trails: Breathing effect that fades from 40% to 100% brightness
 * - Ring: 3 continuous trails (red, green, blue) circling clockwise, equally spaced
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

    // Timing constants for core effect
    static const int MAX_SIZE = 12;         // Maximum LEDs on each side of center (total 25 = 12+1+12)
    static const int GROW_INTERVAL = 100;   // Milliseconds between each growth step (slower for smoother appearance)
    static const int MOVE_INTERVAL = 50;    // Milliseconds between each movement step (faster for smoother movement)

    // Trail constants - INCREASED for more trails
    static const int MAX_TRAILS = 30;       // Maximum number of synchronized trail sets (increased from 8)
    static const int TRAIL_LENGTH = 104;    // Length of each trail in LEDs
    static const int TARGET_TRAILS = 20;    // Target number of trail sets to maintain (increased from 5)

    // Trail timing - DECREASED for more frequent creation
    unsigned long lastTrailCreateTime;       // Last time we created a trail
    static const int TRAIL_CREATE_INTERVAL = 40;   // Create a new trail every 40ms (decreased from 80ms)
    static const int TRAIL_STAGGER_VARIANCE = 20;  // Add random variance to prevent waves (decreased from 40)

    // Trail management - now using synchronized trails
    std::vector<SyncedTrail> syncedTrails;  // Collection of synchronized trail sets

    // Ring trail constants - 3 continuous trails
    static const int NUM_RING_TRAILS = 3;        // Exactly 3 trails (red, green, blue)
    static const int RING_TRAIL_LENGTH = 12;     // Length of each ring trail in LEDs

    // Ring trails - fixed array of 3 continuous trails
    ContinuousRingTrail ringTrails[NUM_RING_TRAILS];

    // Ring effect constants
    static constexpr float RING_MIN_BRIGHTNESS = 0.15f; // 15% minimum brightness for dramatic breathing
    static constexpr float RING_MAX_BRIGHTNESS = 1.0f;  // 100% maximum brightness for dramatic breathing

    /**
     * Generate random color values for core effect (full vibrance)
     * Called at the start of each new core cycle
     */
    void generateRandomCoreColor();

    /**
     * Generate random color values for a new synchronized trail
     * @param trail Reference to the trail to set colors for
     */
    void generateRandomTrailColor(SyncedTrail& trail);

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
     * Initialize the 3 continuous ring trails
     * Called once during construction to set up red, green, blue trails
     */
    void initializeRingTrails();

    /**
     * Update the continuous ring trails
     * Moves the 3 RGB trails around the ring continuously
     */
    void updateRingTrails();

    /**
     * Draw all 3 continuous ring trails
     */
    void drawRingTrails();

    /**
     * Create a new synchronized trail set for inner or outer strips
     */
    void createNewSyncedTrail();

    /**
     * Update all active synchronized trails
     */
    void updateSyncedTrails();

    /**
     * Draw all active synchronized trails on all segments
     */
    void drawSyncedTrails();

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