// src/leds/effects/FireEffect.h

#ifndef FIRE_EFFECT_H
#define FIRE_EFFECT_H

#include "Effect.h"
#include <Arduino.h>

/**
 * FireEffect - Creates a realistic fire animation on LED strips
 *
 * Features:
 * - Red base at the bottom with red/orange coloring
 * - Rising flames with proper physics simulation
 * - Darker "smoke" effect at the top
 * - Core strip remains off
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

    String getName() const override { return "Fire Effect"; }

private:
    // Heat simulation arrays for each strip
    unsigned char* heatCore;
    unsigned char* heatInner;
    unsigned char* heatOuter;

    // Animation timing
    unsigned long lastUpdateTime;

    // Fire intensity (0-100)
    unsigned char intensity;

    // Helper methods
    void updateFireBase();
    void renderFire();
    uint32_t heatToColor(unsigned char heat);
    int mapLEDPosition(int stripType, int position, int subStrip = 0);
};

#endif // FIRE_EFFECT_H