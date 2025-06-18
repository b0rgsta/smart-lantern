// src/leds/effects/SuspendedFireEffect.h

#ifndef SUSPENDED_FIRE_EFFECT_H
#define SUSPENDED_FIRE_EFFECT_H

#include "Effect.h"
#include <Arduino.h>

/**
 * SuspendedFireEffect - Creates a suspended/inverted fire animation on LED strips
 *
 * Features:
 * - Flames hang downward from the top (inverted fire direction)
 * - Red base at the top with red/orange coloring descending downward
 * - Darker "smoke" effect at the bottom
 * - Black gradient overlay remains in original position (outer strips fade to black at top)
 * - Core strip remains off
 * - Same realistic fire physics as FireEffect but with inverted flame direction
 */
class SuspendedFireEffect : public Effect {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller
     */
    SuspendedFireEffect(LEDController& ledController);

    /**
     * Destructor - clean up allocated memory
     */
    ~SuspendedFireEffect();

    /**
     * Update the suspended fire effect animation
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

    String getName() const override { return "Suspended Fire Effect"; }

protected:
    // Heat simulation arrays for each strip (same as FireEffect)
    unsigned char* heatCore;
    unsigned char* heatInner;
    unsigned char* heatOuter;

    // Animation timing
    unsigned long lastUpdateTime;

    // Fire intensity (0-100)
    unsigned char intensity;

    // Dynamic flame height control for each strip segment
    float innerFlameHeights[NUM_INNER_STRIPS];     // Current flame height for each inner strip (0.0 to 1.0)
    float outerFlameHeights[NUM_OUTER_STRIPS];     // Current flame height for each outer strip (0.0 to 1.0)
    float innerHeightTargets[NUM_INNER_STRIPS];    // Target height for smooth transitions
    float outerHeightTargets[NUM_OUTER_STRIPS];    // Target height for smooth transitions
    unsigned long lastHeightUpdate;                // Last time we updated height targets

    // Helper methods
    void updateSuspendedFireBase();  // Modified fire simulation for downward flames
    void updateFlameHeights();       // Update dynamic flame heights for realistic variation
    void applyFlameHeightCutoff(int segment, bool isInnerStrip); // Apply individual flame height limits
    void renderSuspendedFire();      // Modified rendering for inverted flames
    uint32_t heatToColor(unsigned char heat);  // Same color mapping as FireEffect
    int mapLEDPosition(int stripType, int position, int subStrip = 0);  // LED position mapping
};

#endif // SUSPENDED_FIRE_EFFECT_H