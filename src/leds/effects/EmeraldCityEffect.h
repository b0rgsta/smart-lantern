// src/leds/effects/EmeraldCityEffect.h

#ifndef EMERALD_CITY_EFFECT_H
#define EMERALD_CITY_EFFECT_H

#include "Effect.h"
#include <vector>

/**
 * EmeraldCityEffect - Creates green trails with white sparkles effect
 *
 * This effect replicates the nexus floor lamp's Emerald City effect for the lantern.
 *
 * Features:
 * - Green trails (various shades) moving upward on inner and outer strips
 * - NO white trails - all trails are solid green colors
 * - White sparkles on inner, outer, and ring strips (core strip doesn't sparkle)
 * - Core strip stays completely off (no lighting)
 * - Multiple shades of green for visual depth
 * - Random sparkle timing to create magical emerald appearance
 */

// Structure to represent a falling green trail
struct EmeraldTrail {
    float position;      // Current position on the strip (0 = bottom, stripLength = top)
    float speed;         // Movement speed in pixels per frame
    uint8_t greenHue;    // Green hue variation (different shades of green)
    uint8_t brightness;  // Current brightness for this trail head
    bool isActive;       // Whether this trail is currently active
    int stripType;       // Which strip type this trail belongs to (1=inner, 2=outer)
    int subStrip;        // Which specific strip within the type
};

class EmeraldCityEffect : public Effect {
public:
    /**
     * Constructor - initializes the effect with sparkle arrays
     * @param ledController Reference to the LED controller for managing LEDs
     */
    EmeraldCityEffect(LEDController& ledController);

    /**
     * Destructor - cleans up allocated memory for sparkle arrays
     */
    ~EmeraldCityEffect();

    /**
     * Update the effect each frame
     * Handles trail movement, creation, and sparkle effects
     */
    void update() override;

    /**
     * Reset the effect to initial state
     * Clears all active trails and resets sparkle values
     */
    void reset() override;

    /**
     * Get the name of this effect for debugging/display
     * @return The effect name as a string
     */
    String getName() const override { return "Emerald City Effect"; }

private:
    // Collection of all green trails (both active and inactive)
    std::vector<EmeraldTrail> innerTrails[NUM_INNER_STRIPS];  // Trails for each inner strip
    std::vector<EmeraldTrail> outerTrails[NUM_OUTER_STRIPS];  // Trails for each outer strip

    // Effect parameters for green trails
    static const int MAX_TRAILS_PER_STRIP = 12;        // Maximum trails per strip (half the amount: 25 -> 12)
    static const int TRAIL_CREATE_CHANCE = 37;         // Chance per frame to create new trail (half the frequency: 75 -> 37)
    static const int TRAIL_LENGTH = 25;                 // Length of each green trail (half length: 50 -> 25)
    static const int TRAIL_BRIGHTNESS = 220;           // Base brightness of the trails (brighter)

    // Speed parameters for trail movement (upward motion)
    static constexpr float MIN_TRAIL_SPEED = 0.08f;    // Minimum trail speed
    static constexpr float MAX_TRAIL_SPEED = 0.25f;    // Maximum trail speed

    // Green color palette - various shades of green for trails
    static const int NUM_GREEN_COLORS = 6;
    uint8_t greenHues[NUM_GREEN_COLORS];               // Array of green hue values

    // White sparkle effect variables for inner, outer, and ring strips
    float* innerSparkleValues;          // Array to store sparkle intensity for each inner LED (0.0 to 1.0)
    float* outerSparkleValues;          // Array to store sparkle intensity for each outer LED (0.0 to 1.0)
    float* ringSparkleValues;           // Array to store sparkle intensity for each ring LED (0.0 to 1.0)

    // Sparkle color tracking (0 = white, 1 = light green)
    uint8_t* innerSparkleColors;        // Array to store sparkle color type for each inner LED
    uint8_t* outerSparkleColors;        // Array to store sparkle color type for each outer LED
    uint8_t* ringSparkleColors;         // Array to store sparkle color type for each ring LED

    // Sparkle brightness tracking (random brightness levels from 20% to 100%)
    float* innerSparkleBrightness;      // Array to store random max brightness for each inner LED
    float* outerSparkleBrightness;      // Array to store random max brightness for each outer LED
    float* ringSparkleBrightness;       // Array to store random max brightness for each ring LED

    // Sparkle speed tracking (random fade speeds for each sparkle)
    float* innerSparkleSpeed;           // Array to store random fade speed for each inner LED
    float* outerSparkleSpeed;           // Array to store random fade speed for each outer LED
    float* ringSparkleSpeed;            // Array to store random fade speed for each ring LED (slower)

    // Sparkle timing and parameters
    unsigned long lastSparkleUpdate;    // When sparkles were last updated
    static constexpr unsigned long SPARKLE_UPDATE_INTERVAL = 10;   // Update sparkles every 10ms (ultra smooth)
    static constexpr float INNER_OUTER_SPARKLE_CHANCE = 0.0033f;   // 0.33% chance for inner/outer (50% less: 0.67% -> 0.33%)
    static constexpr float RING_SPARKLE_CHANCE = 0.0067f;         // 0.67% chance for ring (unchanged)
    static constexpr float BASE_SPARKLE_SPEED = 0.1f;             // Base fade speed for inner/outer sparkles
    static constexpr float MIN_SPEED_MULTIPLIER = 0.5f;           // Minimum speed multiplier (50% of base)
    static constexpr float MAX_SPEED_MULTIPLIER = 2.0f;           // Maximum speed multiplier (200% of base)
    static constexpr float RING_SPEED_MULTIPLIER = 0.5f;          // Ring sparkles are 50% slower (twice as long)
    static constexpr float MAX_SPARKLE_BRIGHTNESS = 0.6f;         // Maximum sparkle brightness

    // Timing variables
    unsigned long lastUpdateTime;       // For frame rate control

    // Fade parameters for outer strips (fade to black from bottom to top)
    static constexpr float FADE_START_POSITION = 0.3f;        // Start fading at 30% up the strip
    static constexpr float FADE_END_POSITION = 0.9f;          // Complete fade by 90% up the strip

    // Core wave effect parameters
    float coreWavePosition;                                    // Current position of the wave (0.0 to LED_STRIP_CORE_COUNT)
    static constexpr float CORE_WAVE_SPEED = 0.52f;          // Speed of the wave movement (30% faster: 0.4 -> 0.52)
    static constexpr int CORE_WAVE_LENGTH = 50;               // Length of the wave in pixels (shorter for less pause: 80 -> 50)
    static constexpr float CORE_WAVE_BRIGHTNESS = 0.9f;       // Maximum brightness of the wave (more vibrant: 0.7 -> 0.9)
    static constexpr uint8_t CORE_WAVE_HUE = 105;            // Vibrant blue-green hue (more vibrant: 110 -> 105)

    /**
     * Initialize the green color palette
     * Sets up various shades of green for trail variety
     */
    void initializeGreenPalette();

    /**
     * Initialize some trails at startup for immediate visual effect
     * Creates trails at various positions so effect starts with green trails visible
     */
    void initializeStartupTrails();

    /**
     * Create a new green trail on a specific strip
     * @param stripType Strip type (1=inner, 2=outer)
     * @param subStrip Which specific strip within the type
     */
    void createTrail(int stripType, int subStrip);

    /**
     * Update all green trails on inner strips
     * Handles movement, creation, and deactivation
     */
    void updateInnerTrails();

    /**
     * Update all green trails on outer strips
     * Handles movement, creation, and deactivation
     */
    void updateOuterTrails();

    /**
     * Update trails for a specific strip type and substrip
     * @param stripType Strip type (1=inner, 2=outer)
     * @param subStrip Which specific strip within the type
     */
    void updateStripTrails(int stripType, int subStrip);

    /**
     * Render a green trail on the LED strip
     * @param trail The trail to render
     * @param stripType Strip type (1=inner, 2=outer)
     * @param subStrip Which specific strip within the type
     * @param stripLength Length of the strip being drawn on
     */
    void renderTrail(EmeraldTrail& trail, int stripType, int subStrip, int stripLength);

    /**
     * Update white sparkle effects for inner, outer, and ring strips
     * Creates random white sparkles that fade over time (much slower than before)
     */
    void updateSparkles();

    /**
     * Apply fade-to-black overlay to outer strips
     * Creates ambient lighting effect where outer strips fade from full brightness to black
     */
    void applyOuterFadeOverlay();

    /**
     * Apply blue-green wave effect to core strip
     * Creates a large wave that moves across the entire core strip
     */
    void applyCoreWaveEffect();

    /**
     * Apply moving black fade to inner strips that follows the core wave
     * Creates a shadow effect that moves with the core wave
     */
    void applyInnerWaveFade();


    /**
     * Get a random green hue from the palette
     * @return A green hue value for trail coloring
     */
    uint8_t getRandomGreenHue();

    /**
     * Get the length of a strip based on its type
     * @param stripType The type of strip (1=inner, 2=outer)
     * @return Number of LEDs in that strip type
     */
    int getStripLength(int stripType);
    /**
     * Apply glowing green overlay to ring strip
     * Creates a soft green base glow underneath the sparkles
     */
    void applyRingGreenOverlay();
};

#endif // EMERALD_CITY_EFFECT_H