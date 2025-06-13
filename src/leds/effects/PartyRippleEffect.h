// src/leds/effects/PartyRippleEffect.h

#ifndef PARTY_RIPPLE_EFFECT_H
#define PARTY_RIPPLE_EFFECT_H

#include "Effect.h"
#include <vector>

/**
 * Structure to represent a single ripple
 * Each ripple starts at a position and expands outward with a color
 */
struct Ripple {
    int stripType;      // 1 = inner, 2 = outer
    int subStrip;       // Which segment (0, 1, or 2)
    int centerPos;      // Center position of the ripple
    float radius;       // Current radius of the ripple (0 to MAX_RADIUS)
    CRGB color;         // Color of this ripple
    bool active;        // Whether this ripple is still active
    float fadeOut;      // Fade-out multiplier (1.0 = full bright, 0.0 = fully faded)
};

/**
 * PartyRippleEffect - Creates colorful ripples that expand from random positions
 *
 * Features:
 * - Random ripples appear on inner and outer strips
 * - Each ripple has a random bright color
 * - Ripples expand outward from center point
 * - Ripples are 24 LEDs in diameter (12 on each side)
 * - Multiple ripples can overlap and blend colors
 * - Ripples fill from center and fade to edges
 * - Ripples can start from off-screen positions
 * - Smooth fade-out when ripples reach maximum size
 */
class PartyRippleEffect : public Effect {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller
     */
    PartyRippleEffect(LEDController& ledController);

    /**
     * Destructor
     */
    ~PartyRippleEffect();

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
    String getName() const override { return "Party Ripple Effect"; }

private:
    // Collection of all active ripples
    std::vector<Ripple> ripples;

    // Effect parameters
    static const int MAX_RIPPLES = 50;        // Maximum number of simultaneous ripples
    static const int RIPPLE_CREATE_CHANCE = 12; // Chance to create new ripple each frame (out of 100)
    static constexpr float MAX_RADIUS = 14.0f;  // Maximum radius (12 LEDs on each side = 24 total)
    static constexpr float FADE_START_RADIUS = 6.0f; // Start fading at this radius
    static constexpr float RIPPLE_SPEED = 0.2f; // Speed of ripple expansion per frame
    static constexpr float FADE_SPEED = 0.01f;  // Speed of fade-out per frame

    // Timing
    unsigned long lastUpdate;

    /**
     * Create a new ripple at a random position
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
};

#endif // PARTY_RIPPLE_EFFECT_H