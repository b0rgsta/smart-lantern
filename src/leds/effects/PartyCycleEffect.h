// src/leds/effects/PartyCycleEffect.h

#ifndef PARTY_CYCLE_EFFECT_H
#define PARTY_CYCLE_EFFECT_H

#include "Effect.h"
#include <vector>
#include "FastLED.h"

class PartyCycleEffect : public Effect {
public:
    PartyCycleEffect(LEDController& ledController, const std::vector<Effect*>& partyEffects);
    ~PartyCycleEffect();

    void update() override;
    void reset() override;
    String getName() const override { return "Party Cycle Effect"; }

private:
    // LED state storage for transitions
    struct LEDSnapshot {
        CRGB core[LED_STRIP_CORE_COUNT];
        CRGB inner[LED_STRIP_INNER_COUNT];
        CRGB outer[LED_STRIP_OUTER_COUNT];
        CRGB ring[LED_STRIP_RING_COUNT];
    };

    std::vector<Effect*> partyEffects;  // Copy of effects to cycle through
    int currentEffectIndex;             // Current effect being shown
    int nextEffectIndex;                // Next effect for transitions
    bool inTransition;                  // True when transitioning between effects

    unsigned long effectStartTime;     // When current effect started
    unsigned long transitionStartTime; // When current transition started

    // LED state snapshots for blending
    LEDSnapshot oldEffectLEDs;          // LEDs from the outgoing effect
    LEDSnapshot newEffectLEDs;          // LEDs from the incoming effect

    static const unsigned long EFFECT_DURATION = 15000;    // 15 seconds per effect
    static const unsigned long TRANSITION_DURATION = 8000;  // 8 seconds transition

    /**
     * Start transitioning to the next effect
     */
    void startTransition();

    /**
     * Update the transition between two effects
     */
    void updateTransition();

    /**
     * Store current LED state before manipulation
     */
    void storeLEDState();

    /**
     * Capture current LED state into a snapshot
     * @param snapshot The snapshot to store the current LED state in
     */
    void captureLEDState(LEDSnapshot& snapshot);

    /**
     * Blend two effect states together (optimized version)
     * @param fadeProgress Progress of fade (0.0 = full old, 1.0 = full new)
     */
    void blendEffectsOptimized(float fadeProgress);
};

#endif // PARTY_CYCLE_EFFECT_H