// src/leds/effects/PartyFireEffect.h

#ifndef PARTY_FIRE_EFFECT_H
#define PARTY_FIRE_EFFECT_H

#include "FireEffect.h"

/**
 * PartyFireEffect - Enhanced fire effect for party mode
 *
 * This effect builds on the base FireEffect but adds special animations:
 * - Inner/Outer strips: Same realistic fire animation as the base effect
 * - Core strip: Deep red glow that fades to black at the top (like inner heat glow)
 * - Ring strip: Random breathing red/orange glow (unpredictable fire breathing)
 *
 * The core creates the illusion of inner heat radiating from the fire base,
 * while the ring adds atmospheric fire breathing effects.
 */
class PartyFireEffect : public FireEffect {
public:
    /**
     * Constructor - creates the party fire effect
     * @param ledController Reference to the LED controller for drawing
     */
    PartyFireEffect(LEDController& ledController);

    /**
     * Update the effect - calls base fire update then adds core and ring effects
     * Called every frame to animate the fire
     */
    void update() override;

    /**
     * Reset the effect to initial state
     * Resets both base fire effect and party-specific animations
     */
    void reset() override;

    /**
     * Get the name of this effect for debugging/display
     * @return The effect name as a string
     */
    String getName() const override { return "Party Fire Effect"; }

private:
    // Core glow animation variables
    float coreGlowIntensity;        // Current intensity of core glow (0.0 to 1.0)
    unsigned long lastCoreUpdate;   // Last time core glow was updated

    // Ring breathing animation variables
    float ringBreathingPhase;       // Current phase of breathing cycle (0.0 to 2*PI)
    float ringBreathingSpeed;       // Speed of breathing (changes randomly for unpredictability)
    float ringIntensity;            // Current breathing intensity (0.0 to 1.0)
    unsigned long lastRingUpdate;   // Last time ring breathing was updated
    unsigned long nextSpeedChange;  // When to change breathing speed next (for randomness)

    // Core color definitions
    static constexpr uint32_t CORE_DEEP_RED = 0x8B0000;    // Deep red color for core base

    // Ring color definitions
    static constexpr uint32_t RING_RED_ORANGE = 0xFF4500;  // Red-orange color for ring breathing

    // Animation timing constants
    static const unsigned long CORE_UPDATE_INTERVAL = 50;      // Update core every 50ms (20 FPS)
    static const unsigned long RING_UPDATE_INTERVAL = 30;      // Update ring every 30ms (33 FPS)
    static const unsigned long SPEED_CHANGE_INTERVAL = 2000;   // Change breathing speed every 2 seconds

    /**
     * Update the core glow effect
     * Creates a deep red glow that fades to black at the top
     */
    void updateCoreGlow();

    /**
     * Update the ring breathing effect
     * Creates random breathing red/orange glow
     */
    void updateRingBreathing();

    /**
     * Apply gradient fade from deep red at bottom to black at top on core strip
     * @param intensity Overall intensity multiplier (0.0 to 1.0)
     */
    void applyCoreGradient(float intensity);

    /**
     * Apply breathing glow to ring strip
     * @param intensity Current breathing intensity (0.0 to 1.0)
     */
    void applyRingGlow(float intensity);

    /**
     * Generate random breathing speed for unpredictable fire breathing
     * @return New breathing speed value
     */
    float generateRandomBreathingSpeed();
};

#endif // PARTY_FIRE_EFFECT_H