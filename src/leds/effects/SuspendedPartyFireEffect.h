// src/leds/effects/SuspendedPartyFireEffect.h

#ifndef SUSPENDED_PARTY_FIRE_EFFECT_H
#define SUSPENDED_PARTY_FIRE_EFFECT_H

#include "SuspendedFireEffect.h"
#include <cstdint>

/**
 * SuspendedPartyFireEffect - Combines suspended fire visuals with party fire core animation
 *
 * Features:
 * - Inner/Outer strips: Same suspended/inverted fire animation as SuspendedFireEffect
 *   (flames hang downward from the top, red base at top, darker smoke at bottom)
 * - Core strip: Deep red glow that fades to black at the BOTTOM (flipped from PartyFireEffect)
 *   This creates upward heat glow effect that complements the downward flames
 * - Ring strip: Enhanced random breathing red glow with smooth transitions and random peaks
 *
 * The core animation direction is flipped to create upward glow while the fire hangs down,
 * creating a unique visual effect where heat appears to rise while flames descend.
 */
class SuspendedPartyFireEffect : public SuspendedFireEffect {
public:
    /**
     * Constructor - creates the suspended party fire effect
     * @param ledController Reference to the LED controller for drawing
     */
    SuspendedPartyFireEffect(LEDController& ledController);

    /**
     * Update the effect - calls base suspended fire update then adds core and ring effects
     * Called every frame to animate the fire
     */
    void update() override;

    /**
     * Reset the effect to initial state
     * Resets both base suspended fire effect and party-specific animations
     */
    void reset() override;

    /**
     * Get the name of this effect for debugging/display
     * @return The effect name as a string
     */
    String getName() const override { return "Suspended Party Fire Effect"; }

private:
    // Core glow animation variables
    float coreGlowIntensity;        // Current intensity of core glow (0.0 to 1.0)
    unsigned long lastCoreUpdate;   // Last time core glow was updated

    // Ring breathing animation variables
    float ringBreathingPhase;       // Current phase of breathing cycle (0.0 to 2*PI)
    float ringIntensity;            // Current breathing intensity (0.0 to 1.0)
    unsigned long lastRingUpdate;   // Last time ring breathing was updated
    unsigned long nextSpeedChange;  // When to change breathing speed next

    // Enhanced ring breathing animation variables for random and smooth effects
    float currentBreathingSpeed;    // Current breathing speed (changes randomly)
    float peakIntensity;           // Current peak breathing intensity (randomized)
    unsigned long lastPeakChange;  // Last time peak intensity was changed

    // Core color definitions
    static constexpr uint32_t CORE_DEEP_RED = 0x8B0000;    // Deep red color for core base

    // Ring color definitions
    static constexpr uint32_t RING_RED_PRIMARY = 0xEE1100;   // Primary red color
    static constexpr uint32_t RING_RED_SECONDARY = 0xCC0000; // Deeper red for transitions

    // Animation timing constants
    static const unsigned long CORE_UPDATE_INTERVAL = 50;      // Update core every 50ms (20 FPS)
    static const unsigned long RING_UPDATE_INTERVAL = 30;      // Update ring every 30ms (33 FPS)
    static const unsigned long SPEED_CHANGE_INTERVAL = 2000;   // Change breathing speed every 2 seconds

    /**
     * Update the core glow effect with FLIPPED direction
     * Creates a deep red glow that fades to black at the BOTTOM (opposite of PartyFireEffect)
     */
    void updateCoreGlow();

    /**
     * Update the ring breathing effect with enhanced randomness and smoothness
     * Creates random breathing red glow with varying speeds and peak intensities
     */
    void updateRingBreathing();

    /**
     * Apply FLIPPED gradient fade from deep red at BOTTOM to black at TOP on core strip
     * This is the opposite direction from PartyFireEffect's applyCoreGradient
     * @param intensity Overall intensity multiplier (0.0 to 1.0)
     */
    void applyCoreGradientFlipped(float intensity);

    /**
     * Apply breathing glow to ring strip (simple implementation)
     * @param intensity Current breathing intensity (0.0 to 1.0)
     */
    void applyRingGlow(float intensity);

    /**
     * Generate random breathing speed for unpredictable fire breathing (unused but kept for compatibility)
     * @return New breathing speed value
     */
    float generateRandomBreathingSpeed();
};

#endif // SUSPENDED_PARTY_FIRE_EFFECT_H