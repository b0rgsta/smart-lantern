#ifndef FIRE_EFFECT_H
#define FIRE_EFFECT_H

#include "Effect.h"
#include <Arduino.h>
#include <vector>

/**
 * RealisticFireEffect - Creates a realistic fire animation on LED strips
 *
 * Features:
 * - Dynamic fire base at the bottom with red/orange/yellow coloring
 * - Rising flames with proper physics simulation
 * - Ember particles that float upward
 * - Smoke effect at the top for a complete look
 * - Variable intensity based on a "heat source" simulation
 */
class FireEffect : public Effect {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller
     */
    FireEffect(LEDController& ledController);

    /**
     * Destructor - clean up allocated memory
     */
    ~FireEffect();

    /**
     * Update the fire effect animation
     * Called on each animation frame
     */
    void update() override;

    /**
     * Reset the effect to initial state
     */
    void reset() override;

    /**
     * Set the fire intensity (0-100)
     * @param intensity Fire intensity percentage
     */
    void setIntensity(unsigned char intensity);

private:
    // Heat simulation arrays for each strip
    unsigned char* heatCore;
    unsigned char* heatInner;
    unsigned char* heatOuter;

    // Ember particles simulation
    struct Ember {
        float x;        // Horizontal position (LED index)
        float y;        // Vertical position (LED strip position)
        float vx;       // Horizontal velocity
        float vy;       // Vertical velocity
        unsigned char heat;   // Ember heat/brightness
        uint16_t hue;   // Ember color hue
    };

    std::vector<Ember> embers;
    unsigned long lastEmberTime;

    // Smoke simulation at the top
    struct SmokeParticle {
        float x;         // Position
        float y;         // Height (0 = top of fire, positive = above)
        float opacity;   // How visible the smoke is (0-255)
        float velocity;  // How fast it's rising
    };

    std::vector<SmokeParticle> smoke;

    // Fire parameters
    unsigned char intensity;         // Overall fire intensity (0-100)
    unsigned char windDirection;     // Direction fire is being blown (0-255, where 128 is straight up)
    unsigned char windStrength;      // How strong the wind effect is (0-100)
    unsigned char baseFlickerRate;   // How quickly the base of the fire flickers (0-100)

    // Animation timing
    unsigned long lastUpdateTime;
    float animationSpeed;

    // Helper methods
    void initHeatArrays();
    void updateFireBase();
    void updateEmbers();
    void updateSmoke();
    void renderFire();

    // Convert heat value to color, with more realistic fire colors
    uint32_t heatToColor(unsigned char heat, bool isEmber = false);

    // Helper to map an LED position based on strip and physical arrangement
    int mapLEDPosition(int stripType, int position, int subStrip = 0);

    // Create a new ember particle
    void createEmber();

    // Helper method to blend colors
    uint32_t blendColors(uint32_t color1, uint32_t color2, unsigned char blend);
};

#endif // FIRE_EFFECT_H