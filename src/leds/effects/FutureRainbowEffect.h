// src/leds/effects/FutureRainbowEffect.h

#ifndef FUTURE_RAINBOW_EFFECT_H
#define FUTURE_RAINBOW_EFFECT_H

#include "Effect.h"
#include <vector>

/**
 * Structure to represent a single upward-moving trail
 * Same as FutureEffect but with rainbow colors
 */
struct FutureRainbowTrail {
    float position;         // Current position on the strip (float for smooth movement)
    float speed;           // Current speed - how fast the trail moves upward (pixels per frame)
    float acceleration;    // How much the speed increases each frame (randomized)
    int stripType;         // Which strip type (1=inner, 2=outer)
    int subStrip;          // Which segment of the strip (0-2)
    bool isActive;         // Whether this trail is currently active
    int trailLength;       // Length of the trail in pixels
    float creationTime;    // When this trail was created (for color calculation)
};

/**
 * FutureRainbowEffect - Creates upward-moving accelerating trails with rainbow colors
 *
 * Features:
 * - Same as FutureEffect but cycles through rainbow colors over 30 seconds
 * - Trails have rainbow-colored tips that match the current cycle position
 * - Core strip breathes current rainbow color from 0% to 100% brightness
 * - Inner strips have unpredictable rainbow breathing overlay at 10% to 80% brightness
 * - Outer strips have unpredictable rainbow breathing with saturation cycling (100% to 30% over 4 seconds)
 * - All breathing effects use the same rainbow cycle timing
 */
class FutureRainbowEffect : public Effect {
public:
    /**
     * Constructor - sets up the future rainbow effect
     * @param ledController Reference to the LED controller for drawing
     */
    FutureRainbowEffect(LEDController& ledController);

    /**
     * Destructor - cleans up any allocated memory
     */
    ~FutureRainbowEffect();

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
    String getName() const override { return "Future Rainbow Effect"; }

private:
    // Collection of all trails (both active and inactive)
    std::vector<FutureRainbowTrail> trails;

    // Effect parameters (same as FutureEffect)
    static const int MAX_TRAILS = 20;              // Maximum number of simultaneous trails
    static const int TRAIL_CREATE_CHANCE = 8;      // Chance per frame to create new trail (out of 100)

    // Trail length parameters
    static const int MIN_TRAIL_LENGTH = 30;        // Minimum trail length in pixels
    static const int MAX_TRAIL_LENGTH = 60;        // Maximum trail length in pixels

    // Speed parameters (same as FutureEffect)
    static constexpr float MIN_INITIAL_SPEED = 0.045f;   // Minimum initial speed
    static constexpr float MAX_INITIAL_SPEED = 0.15f;    // Maximum initial speed
    static constexpr float MIN_ACCELERATION = 0.003f;    // Minimum acceleration
    static constexpr float MAX_ACCELERATION = 0.009f;    // Maximum acceleration
    static constexpr float MAX_SPEED = 0.9f;             // Terminal velocity

    // Rainbow cycle parameters
    float rainbowPhase;                              // Current position in rainbow cycle (0.0 to 1.0)
    static constexpr float RAINBOW_CYCLE_TIME = 30000.0f; // 30 seconds in milliseconds
    unsigned long effectStartTime;                   // When the effect started (for rainbow timing)

    // Outer strip saturation cycling parameters
    float saturationPhase;                           // Current position in saturation cycle (0.0 to 2*PI)
    static constexpr float SATURATION_CYCLE_TIME = 4000.0f; // 4 seconds in milliseconds

    // Timing
    unsigned long lastUpdateTime;

    // Core breathing effect variables (predictable)
    float breathingPhase;                            // Current phase of breathing cycle (0.0 to 2*PI)
    static constexpr float BREATHING_SPEED = 0.005f; // Speed of breathing cycle

    // Unpredictable breathing effect variables for inner/outer strips
    float unpredictableBreathingPhase;  // Current phase for unpredictable breathing
    float unpredictableBreathingSpeed;  // Current speed (changes randomly)
    float unpredictableBreathingTarget; // Target brightness we're moving towards
    float unpredictableBreathingCurrent;// Current actual brightness
    unsigned long lastBreathingChange;  // When we last changed breathing parameters
    static constexpr float MIN_BREATHING_SPEED = 0.002f;  // Minimum breathing speed
    static constexpr float MAX_BREATHING_SPEED = 0.02f;   // Maximum breathing speed
    static constexpr unsigned long BREATHING_CHANGE_INTERVAL = 2000; // Change every 2 seconds

    // Shimmer effect variables for core and inner/outer strips
    float* coreShimmerValues;           // Array to store shimmer brightness multipliers for core
    float* innerShimmerValues;          // Array to store shimmer brightness multipliers for inner strips
    float* outerShimmerValues;          // Array to store shimmer brightness multipliers for outer strips
    unsigned long lastShimmerUpdate;    // When shimmer was last updated
    static constexpr unsigned long SHIMMER_UPDATE_INTERVAL = 100;  // Update shimmer every 100ms

    /**
     * Get current rainbow color based on the 30-second cycle
     * @return Current rainbow color as CRGB
     */
    CRGB getCurrentRainbowColor();

    /**
     * Get current saturation value for outer strips (100% to 30% over 4 seconds)
     * @return Saturation value (0-255 range)
     */
    uint8_t getCurrentOuterSaturation();

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
     * Creates the fade effect from head to tail with rainbow colors
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
     * Uses current rainbow color with different behaviors per strip type
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

#endif // FUTURE_RAINBOW_EFFECT_H