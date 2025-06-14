// src/leds/effects/AuraEffect.h

#ifndef AURA_EFFECT_H
#define AURA_EFFECT_H

#include "Effect.h"
#include <vector>

/**
 * Structure to represent a single ripple
 * Each ripple starts at a position and expands outward with a color
 */
struct Ripple {
    int stripType;      // 0 = core, 1 = inner, 2 = outer, 3 = ring
    int subStrip;       // Which segment (0, 1, or 2) - not used for ring
    int centerPos;      // Center position of the ripple
    float radius;       // Current radius of the ripple (0 to MAX_RADIUS)
    CRGB color;         // Color of this ripple
    bool active;        // Whether this ripple is still active
    float fadeOut;      // Fade-out multiplier (1.0 = full bright, 0.0 = fully faded)
};

/**
 * AuraEffect - Creates colorful ripples that expand from random positions
 *
 * Features:
 * - Random ripples appear on any enabled strip (core, inner, outer, ring)
 * - Each ripple has a random bright color
 * - Ripples expand outward from center point
 * - Multiple ripples can overlap and blend colors
 * - Smooth fade-out when ripples reach maximum size
 * - Individual strip enable/disable control
 */
class AuraEffect : public Effect {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller
     * @param enableCore Whether to show ripples on core strip (default: true)
     * @param enableInner Whether to show ripples on inner strips (default: true)
     * @param enableOuter Whether to show ripples on outer strips (default: true)
     * @param enableRing Whether to show ripples on ring strip (default: true)
     */
    AuraEffect(LEDController& ledController,
               bool enableCore = true,
               bool enableInner = true,
               bool enableOuter = true,
               bool enableRing = true);

    /**
     * Destructor
     */
    ~AuraEffect();

    /**
     * Update the ripple animation
     */
    void update() override;

    /**
     * Reset the effect to initial state
     */
    void reset() override;

    /**
     * Get the name of this effect
     */
    String getName() const override { return "Aura Effect"; }

private:
    // Collection of all active ripples
    std::vector<Ripple> ripples;

    // Strip enable flags - control which strips show ripples
    bool coreEnabled;       // Whether core strip shows ripples
    bool innerEnabled;      // Whether inner strips show ripples
    bool outerEnabled;      // Whether outer strips show ripples
    bool ringEnabled;       // Whether ring strip shows ripples

    // Effect parameters
    static const int MAX_RIPPLES = 50;              // Maximum number of simultaneous ripples
    static const int RIPPLE_CREATE_CHANCE = 12;     // Chance to create new ripple each frame (out of 100)
    static constexpr float MAX_RADIUS = 14.0f;      // Maximum radius (12 LEDs on each side = 24 total)
    static constexpr float FADE_START_RADIUS = 6.0f; // Start fading at this radius
    static constexpr float RIPPLE_SPEED = 0.2f;     // Speed of ripple expansion per frame

    // Timing
    unsigned long lastUpdate;

    /**
     * Create a new ripple at a random position on a random enabled strip
     */
    void createNewRipple();

    /**
     * Update all active ripples (expand and check if complete)
     */
    void updateRipples();

    /**
     * Draw all active ripples to the LED strips
     */
    void drawRipples();

    /**
     * Generate a random bright color for ripples
     * @return Random color as CRGB
     */
    CRGB generateRandomColor();

    /**
     * Calculate brightness for a ripple at a given distance from center
     * @param distance Distance from ripple center
     * @param radius Current radius of the ripple
     * @param fadeOut Current fade-out value of the ripple
     * @return Brightness factor (0.0 to 1.0)
     */
    float calculateRippleBrightness(float distance, float radius, float fadeOut);

    /**
     * Get the length of a strip based on its type
     * @param stripType The type of strip (0=core, 1=inner, 2=outer, 3=ring)
     * @param subStrip Which segment for multi-segment strips
     * @return Number of LEDs in that strip/segment
     */
    int getStripLength(int stripType, int subStrip = 0);
};

#endif // AURA_EFFECT_H