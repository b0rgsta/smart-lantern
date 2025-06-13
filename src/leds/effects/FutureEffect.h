// src/leds/effects/FutureEffect.h

#ifndef FUTURE_EFFECT_H
#define FUTURE_EFFECT_H

#include "Effect.h"
#include <vector>

/**
 * Structure to represent a single upward-moving trail
 * Each trail has position, speed, acceleration, color, and fade properties
 */
struct FutureTrail {
    float position;         // Current position on the strip (float for smooth movement)
    float speed;           // Current speed - how fast the trail moves upward (pixels per frame)
    float acceleration;    // How much the speed increases each frame (randomized)
    int stripType;         // Which strip type (1=inner, 2=outer)
    int subStrip;          // Which segment of the strip (0-2)
    bool isActive;         // Whether this trail is currently active
    int trailLength;       // Length of the trail in pixels
};

/**
 * FutureEffect - Creates upward-moving accelerating trails with breathing core effect
 *
 * Features:
 * - Trails with electric blue tip and white tail that fades to black
 * - Trails randomly appear and move upward with acceleration
 * - Core strip breathes electric blue from 0% to 100% brightness
 * - Inner strips have unpredictable blue breathing overlay at 10% to 80% brightness
 * - Outer strips have unpredictable blue breathing overlay at 10% to 80% brightness
 * - Breathing effects use random variations for unpredictable patterns
 * - Both core and inner/outer strips have shimmering effect
 */
class FutureEffect : public Effect {
public:
    /**
     * Constructor - sets up the future effect
     * @param ledController Reference to the LED controller for drawing
     */
    FutureEffect(LEDController& ledController);

    /**
     * Destructor - cleans up any allocated memory
     */
    ~FutureEffect();

    /**
     * Update the effect animation each frame
     * Handles trail creation, movement, and rendering
     */
    void update() override;

    /**
     * Reset the effect to initial state
     * Clears all active trails
     */
    void reset() override;

    /**
     * Get the name of this effect for debugging/display
     * @return The effect name as a string
     */
    String getName() const override { return "Future Effect"; }

private:
    // Collection of all trails (both active and inactive)
    std::vector<FutureTrail> trails;

    // Effect parameters
    static const int MAX_TRAILS = 20;              // Maximum number of simultaneous trails
    static const int TRAIL_CREATE_CHANCE = 8;      // Chance per frame to create new trail (out of 100)

    // Trail length parameters (doubled from original)
    static const int MIN_TRAIL_LENGTH = 30;        // Minimum trail length in pixels
    static const int MAX_TRAIL_LENGTH = 60;        // Maximum trail length in pixels

    // Speed parameters (all trails start slow and accelerate) - SPED UP BY 50%
    static constexpr float MIN_INITIAL_SPEED = 0.045f;   // Minimum initial speed (increased by 50% from 0.03f)
    static constexpr float MAX_INITIAL_SPEED = 0.15f;    // Maximum initial speed (increased by 50% from 0.10f)
    static constexpr float MIN_ACCELERATION = 0.003f;    // Minimum acceleration (increased by 50% from 0.002f)
    static constexpr float MAX_ACCELERATION = 0.009f;    // Maximum acceleration (increased by 50% from 0.006f)
    static constexpr float MAX_SPEED = 0.9f;             // Terminal velocity (increased by 50% from 0.6f)

    // Color for the electric blue trails (#03d7fc)
    static const uint32_t COLORED_TRAIL_RGB = 0x03d7fc;  // Electric blue color

    // Timing
    unsigned long lastUpdateTime;

    // Core breathing effect variables (predictable)
    float breathingPhase;               // Current phase of breathing cycle (0.0 to 2*PI)
    static constexpr float BREATHING_SPEED = 0.005f;  // Speed of breathing cycle (slowed by 4x from 0.02f)

    // Unpredictable breathing effect variables for inner/outer strips
    float unpredictableBreathingPhase;  // Current phase for unpredictable breathing
    float unpredictableBreathingSpeed;  // Current speed (changes randomly)
    float unpredictableBreathingTarget; // Target brightness we're moving towards
    float unpredictableBreathingCurrent;// Current actual brightness
    unsigned long lastBreathingChange;  // When we last changed breathing parameters
    static constexpr float MIN_BREATHING_SPEED = 0.002f;  // Minimum breathing speed
    static constexpr float MAX_BREATHING_SPEED = 0.02f;   // Maximum breathing speed
    static constexpr unsigned long BREATHING_CHANGE_INTERVAL = 2000; // Change every 2 seconds

    // Shimmer effect variables for core and inner strips
    float* coreShimmerValues;           // Array to store shimmer brightness multipliers for core
    float* innerShimmerValues;          // Array to store shimmer brightness multipliers for inner strips
    float* outerShimmerValues;          // Array to store shimmer brightness multipliers for outer strips
    unsigned long lastShimmerUpdate;    // When shimmer was last updated
    static constexpr unsigned long SHIMMER_UPDATE_INTERVAL = 100;  // Update shimmer every 100ms (slowed by 2x from 50ms)

    /**
     * Create a new trail at the bottom of a random strip
     * Initializes with random properties including speed and acceleration
     */
    void createNewTrail();

    /**
     * Update all active trails
     * Handles movement with acceleration and deactivation
     */
    void updateTrails();

    /**
     * Draw all active trails on the LED strips
     * Creates the fade effect from head to tail
     */
    void drawTrails();

    /**
     * Get the length of a strip based on its type
     * @param stripType The type of strip (1=inner, 2=outer)
     * @return Number of LEDs in that strip type
     */
    int getStripLength(int stripType);

    /**
     * Apply breathing effect to core and inner/outer strips
     * Core breathes 0-100% predictably
     * Inner/outer breathe 10-80% unpredictably
     */
    void applyBreathingEffect();

    /**
     * Update shimmer effect for core, inner, and outer LEDs
     * Creates random brightness variations for dazzling effect
     */
    void updateShimmer();

    /**
     * Update the unpredictable breathing parameters
     * Changes speed and target brightness randomly
     */
    void updateUnpredictableBreathing();
};

#endif // FUTURE_EFFECT_H