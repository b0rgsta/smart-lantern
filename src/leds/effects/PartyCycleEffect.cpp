// src/leds/effects/PartyCycleEffect.cpp

#include "PartyCycleEffect.h"
#include "FastLED.h"

PartyCycleEffect::PartyCycleEffect(LEDController& ledController, const std::vector<Effect*>& partyEffects) :
    Effect(ledController),
    partyEffects(partyEffects),
    currentEffectIndex(0),
    nextEffectIndex(1),
    inTransition(false),
    effectStartTime(0),
    transitionStartTime(0)
{
    effectStartTime = millis();

    // Calculate next effect index
    if (partyEffects.size() > 1) {
        nextEffectIndex = 1;
    } else {
        nextEffectIndex = 0;
    }

    Serial.println("PartyCycleEffect created with " + String(this->partyEffects.size()) + " effects");
}

PartyCycleEffect::~PartyCycleEffect() {
    Serial.println("PartyCycleEffect destroyed");
}

void PartyCycleEffect::reset() {
    currentEffectIndex = 0;
    nextEffectIndex = (partyEffects.size() > 1) ? 1 : 0;
    inTransition = false;
    effectStartTime = millis();
    Serial.println("PartyCycleEffect reset");
}

void PartyCycleEffect::update() {
    if (partyEffects.empty()) {
        Serial.println("WARNING: PartyCycleEffect has no party effects to cycle through");
        return;
    }

    unsigned long currentTime = millis();

    if (inTransition) {
        updateTransition();
    } else {
        // Run the current effect normally
        partyEffects[currentEffectIndex]->update();

        // Check if it's time to start a transition
        if (currentTime - effectStartTime >= EFFECT_DURATION) {
            startTransition();
        }
    }
}

void PartyCycleEffect::startTransition() {
    inTransition = true;
    transitionStartTime = millis();

    // Calculate next effect index
    nextEffectIndex = (currentEffectIndex + 1) % partyEffects.size();

    // Reset the next effect so it starts fresh
    partyEffects[nextEffectIndex]->reset();

    Serial.println("PartyCycleEffect: Starting 8-second transition from '" +
                   partyEffects[currentEffectIndex]->getName() + "' to '" +
                   partyEffects[nextEffectIndex]->getName() + "'");
}

void PartyCycleEffect::updateTransition() {
    unsigned long currentTime = millis();
    unsigned long transitionElapsed = currentTime - transitionStartTime;

    // Check if transition is complete
    if (transitionElapsed >= TRANSITION_DURATION) {
        // Transition complete - switch to next effect
        currentEffectIndex = nextEffectIndex;
        inTransition = false;
        effectStartTime = currentTime;

        Serial.println("PartyCycleEffect: Transition complete, now showing '" +
                       partyEffects[currentEffectIndex]->getName() + "'");
        return;
    }

    // Calculate fade progress (0.0 = full old effect, 1.0 = full new effect)
    float fadeProgress = (float)transitionElapsed / (float)TRANSITION_DURATION;

    // Apply smooth S-curve for gradual fade
    float smoothProgress = fadeProgress * fadeProgress * (3.0f - 2.0f * fadeProgress);

    // Update both effects but don't let them show LEDs yet
    // Store current LED state first
    CRGB tempCore[LED_STRIP_CORE_COUNT];
    CRGB tempInner[LED_STRIP_INNER_COUNT];
    CRGB tempOuter[LED_STRIP_OUTER_COUNT];
    CRGB tempRing[LED_STRIP_RING_COUNT];

    // Update old effect and capture
    partyEffects[currentEffectIndex]->update();
    memcpy(tempCore, leds.getCore(), LED_STRIP_CORE_COUNT * sizeof(CRGB));
    memcpy(tempInner, leds.getInner(), LED_STRIP_INNER_COUNT * sizeof(CRGB));
    memcpy(tempOuter, leds.getOuter(), LED_STRIP_OUTER_COUNT * sizeof(CRGB));
    memcpy(tempRing, leds.getRing(), LED_STRIP_RING_COUNT * sizeof(CRGB));

    // Store old effect state
    memcpy(oldEffectLEDs.core, tempCore, LED_STRIP_CORE_COUNT * sizeof(CRGB));
    memcpy(oldEffectLEDs.inner, tempInner, LED_STRIP_INNER_COUNT * sizeof(CRGB));
    memcpy(oldEffectLEDs.outer, tempOuter, LED_STRIP_OUTER_COUNT * sizeof(CRGB));
    memcpy(oldEffectLEDs.ring, tempRing, LED_STRIP_RING_COUNT * sizeof(CRGB));

    // Update new effect and capture
    partyEffects[nextEffectIndex]->update();
    memcpy(newEffectLEDs.core, leds.getCore(), LED_STRIP_CORE_COUNT * sizeof(CRGB));
    memcpy(newEffectLEDs.inner, leds.getInner(), LED_STRIP_INNER_COUNT * sizeof(CRGB));
    memcpy(newEffectLEDs.outer, leds.getOuter(), LED_STRIP_OUTER_COUNT * sizeof(CRGB));
    memcpy(newEffectLEDs.ring, leds.getRing(), LED_STRIP_RING_COUNT * sizeof(CRGB));

    // Now manually blend the two captured states
    uint8_t newWeight = (uint8_t)(smoothProgress * 255);
    uint8_t oldWeight = 255 - newWeight;

    // Blend and write directly to LED strips
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        leds.getCore()[i] = CRGB(
            ((oldEffectLEDs.core[i].r * oldWeight) + (newEffectLEDs.core[i].r * newWeight)) >> 8,
            ((oldEffectLEDs.core[i].g * oldWeight) + (newEffectLEDs.core[i].g * newWeight)) >> 8,
            ((oldEffectLEDs.core[i].b * oldWeight) + (newEffectLEDs.core[i].b * newWeight)) >> 8
        );
    }

    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        leds.getInner()[i] = CRGB(
            ((oldEffectLEDs.inner[i].r * oldWeight) + (newEffectLEDs.inner[i].r * newWeight)) >> 8,
            ((oldEffectLEDs.inner[i].g * oldWeight) + (newEffectLEDs.inner[i].g * newWeight)) >> 8,
            ((oldEffectLEDs.inner[i].b * oldWeight) + (newEffectLEDs.inner[i].b * newWeight)) >> 8
        );
    }

    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        leds.getOuter()[i] = CRGB(
            ((oldEffectLEDs.outer[i].r * oldWeight) + (newEffectLEDs.outer[i].r * newWeight)) >> 8,
            ((oldEffectLEDs.outer[i].g * oldWeight) + (newEffectLEDs.outer[i].g * newWeight)) >> 8,
            ((oldEffectLEDs.outer[i].b * oldWeight) + (newEffectLEDs.outer[i].b * newWeight)) >> 8
        );
    }

    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        leds.getRing()[i] = CRGB(
            ((oldEffectLEDs.ring[i].r * oldWeight) + (newEffectLEDs.ring[i].r * newWeight)) >> 8,
            ((oldEffectLEDs.ring[i].g * oldWeight) + (newEffectLEDs.ring[i].g * newWeight)) >> 8,
            ((oldEffectLEDs.ring[i].b * oldWeight) + (newEffectLEDs.ring[i].b * newWeight)) >> 8
        );
    }

    // Finally show the blended result - only we control when LEDs update
    leds.showAll();

    // Debug
    static unsigned long lastProgressPrint = 0;
    if (millis() - lastProgressPrint > 1000) {
        Serial.println("Smooth transition: " + String(smoothProgress * 100.0f, 1) + "%");
        lastProgressPrint = millis();
    }
}

void PartyCycleEffect::storeLEDState() {
    // This method is called before we start manipulating LED states
    // We don't need to store anything here as we'll capture after each effect update
}

void PartyCycleEffect::captureLEDState(LEDSnapshot& snapshot) {
    // Capture current LED state for all strips - optimized
    memcpy(snapshot.core, leds.getCore(), LED_STRIP_CORE_COUNT * sizeof(CRGB));
    memcpy(snapshot.inner, leds.getInner(), LED_STRIP_INNER_COUNT * sizeof(CRGB));
    memcpy(snapshot.outer, leds.getOuter(), LED_STRIP_OUTER_COUNT * sizeof(CRGB));
    memcpy(snapshot.ring, leds.getRing(), LED_STRIP_RING_COUNT * sizeof(CRGB));
}

void PartyCycleEffect::blendEffectsOptimized(float fadeProgress) {
    // Apply more aggressive smoothing curve to make fade even more gradual
    // Using cubic smoothstep for very smooth transitions
    float smoothProgress = fadeProgress * fadeProgress * fadeProgress * (fadeProgress * (fadeProgress * 6.0f - 15.0f) + 10.0f);

    // Pre-calculate integer fade ratio for faster math
    uint8_t newRatio = (uint8_t)(smoothProgress * 255);
    uint8_t oldRatio = 255 - newRatio;

    // Debug: Print progress occasionally
    static unsigned long lastProgressPrint = 0;
    if (millis() - lastProgressPrint > 2000) { // Every 2 seconds for less spam
        Serial.println("Transition progress: " + String(fadeProgress * 100.0f, 1) + "% (smooth: " + String(smoothProgress * 100.0f, 1) + "%)");
        lastProgressPrint = millis();
    }

    // Blend core LEDs - only if both effects use core
    if (LED_STRIP_CORE_COUNT > 0) {
        for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
            leds.getCore()[i] = CRGB(
                ((oldEffectLEDs.core[i].r * oldRatio) + (newEffectLEDs.core[i].r * newRatio)) >> 8,
                ((oldEffectLEDs.core[i].g * oldRatio) + (newEffectLEDs.core[i].g * newRatio)) >> 8,
                ((oldEffectLEDs.core[i].b * oldRatio) + (newEffectLEDs.core[i].b * newRatio)) >> 8
            );
        }
    }

    // Blend inner LEDs
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        leds.getInner()[i] = CRGB(
            ((oldEffectLEDs.inner[i].r * oldRatio) + (newEffectLEDs.inner[i].r * newRatio)) >> 8,
            ((oldEffectLEDs.inner[i].g * oldRatio) + (newEffectLEDs.inner[i].g * newRatio)) >> 8,
            ((oldEffectLEDs.inner[i].b * oldRatio) + (newEffectLEDs.inner[i].b * newRatio)) >> 8
        );
    }

    // Blend outer LEDs
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        leds.getOuter()[i] = CRGB(
            ((oldEffectLEDs.outer[i].r * oldRatio) + (newEffectLEDs.outer[i].r * newRatio)) >> 8,
            ((oldEffectLEDs.outer[i].g * oldRatio) + (newEffectLEDs.outer[i].g * newRatio)) >> 8,
            ((oldEffectLEDs.outer[i].b * oldRatio) + (newEffectLEDs.outer[i].b * newRatio)) >> 8
        );
    }

    // Blend ring LEDs - only if both effects use ring
    if (LED_STRIP_RING_COUNT > 0) {
        for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
            leds.getRing()[i] = CRGB(
                ((oldEffectLEDs.ring[i].r * oldRatio) + (newEffectLEDs.ring[i].r * newRatio)) >> 8,
                ((oldEffectLEDs.ring[i].g * oldRatio) + (newEffectLEDs.ring[i].g * newRatio)) >> 8,
                ((oldEffectLEDs.ring[i].b * oldRatio) + (newEffectLEDs.ring[i].b * newRatio)) >> 8
            );
        }
    }
}