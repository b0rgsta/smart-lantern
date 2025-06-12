// src/leds/effects/WaterfallEffect.h

#ifndef WATERFALL_EFFECT_H
#define WATERFALL_EFFECT_H

#include "Effect.h"
#include <vector>

/**
 * Structure to represent a single water drop falling down the strips
 * Each drop has position, speed, color properties and a splash effect
 */
struct WaterDrop {
    float position;         // Current position on the strip (float for smooth movement)
    float speed;           // How fast the drop is falling (pixels per frame)
    float acceleration;    // How much speed increases each frame (gravity effect)
    uint8_t brightness;    // Current brightness of the drop (0-255)
    uint8_t maxBrightness; // Maximum brightness this drop will reach when fully formed
    uint8_t hue;          // Color hue of the drop (0-255 for FastLED)
    uint8_t trailLength;  // How long the fading trail behind this drop is (12-90 pixels, no small dots)
    uint8_t fadeInFrames; // How many frames it takes for this drop to fade in
    uint8_t currentFrame; // Current frame since drop was created (for fade-in)
    bool isActive;        // Whether this drop is currently falling
    bool hasSplashed;     // Whether this drop has hit the bottom and splashed
    int splashFrame;      // Which frame of the splash animation we're on
    int stripType;        // Which strip this drop is on (1=inner, 2=outer)
    int subStrip;         // Which segment of the strip (0-2)
};

/**
 * WaterfallEffect - Creates a realistic waterfall animation
 *
 * Features:
 * - Water drops fall from top to bottom with realistic physics
 * - Drops accelerate as they fall (gravity effect)
 * - Blue-white water colors with varying transparency
 * - Splash effects when drops hit the bottom
 * - Multiple drops can be active simultaneously
 * - Background water on all LEDs for constant waterfall appearance
 * - Core strip stays off to focus attention on falling water
 */
class WaterfallEffect : public Effect {
public:
    /**
     * Constructor - sets up the waterfall effect
     * @param ledController Reference to the LED controller for drawing
     */
    WaterfallEffect(LEDController& ledController);

    /**
     * Destructor - cleans up any allocated memory
     */
    ~WaterfallEffect();

    /**
     * Update the waterfall animation each frame
     * Handles drop physics, creation, and splash effects
     */
    void update() override;

    /**
     * Reset the effect to initial state
     * Clears all active drops and splash effects
     */
    void reset() override;

    /**
     * Get the name of this effect for debugging/display
     * @return The effect name as a string
     */
    String getName() const override { return "Waterfall Effect"; }

private:
    // Collection of all water drops (both active and inactive)
    std::vector<WaterDrop> waterDrops;

    // Effect parameters - these control how the waterfall looks and behaves
    static const int MAX_DROPS = 25;           // More drops for denser waterfall
    static const int DROP_CREATE_CHANCE = 15;  // Higher chance per frame to create new drop (out of 100)
    static const int SPLASH_FRAMES = 12;       // Longer splash duration

    // Physics parameters for realistic water movement (slower speeds)
    static constexpr float MIN_START_SPEED = 0.02f;   // Slowest initial drop speed (much slower)
    static constexpr float MAX_START_SPEED = 0.08f;   // Fastest initial drop speed (much slower)
    static constexpr float GRAVITY = 0.005f;          // How much drops accelerate each frame (less gravity)
    static constexpr float MAX_SPEED = 0.3f;          // Terminal velocity (much slower max speed)

    /**
     * Fill all LEDs with a dim background water color
     * Creates the base waterfall appearance before adding bright drops
     */
    void fillBackgroundWater();

    /**
     * Create a new water drop at the top of a random strip
     * Finds an inactive drop slot and initializes it with random properties
     */
    void createNewDrop();

    /**
     * Update the physics for a single water drop
     * Handles position, speed, and collision detection with bottom
     * @param drop Reference to the drop to update
     */
    void updateDrop(WaterDrop& drop);

    /**
     * Draw a single water drop on its strip
     * Handles color, brightness, and positioning
     * @param drop The drop to draw
     */
    void drawDrop(const WaterDrop& drop);

    /**
     * Draw splash effect for a drop that hit the bottom
     * Creates expanding splash pattern from the impact point
     * @param drop The drop that is splashing
     */
    void drawSplash(const WaterDrop& drop);

    /**
     * Get the water color for a drop based on its properties
     * Creates blue-white water colors with transparency effects
     * @param hue The base hue of the drop
     * @param brightness The brightness/transparency level
     * @return The final color as CRGB
     */
    CRGB getWaterColor(uint8_t hue, uint8_t brightness);

    /**
     * Get the length of a strip based on its type
     * @param stripType The type of strip (1=inner, 2=outer)
     * @return Number of LEDs in that strip type
     */
    int getStripLength(int stripType);
};

#endif // WATERFALL_EFFECT_H