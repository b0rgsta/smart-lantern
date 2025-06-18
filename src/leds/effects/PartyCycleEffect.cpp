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

    // Add rainbow notification to ring after effects update
    addRainbowRingNotification();
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

    // Debug: Print progress occasionally
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

void PartyCycleEffect::addRainbowRingNotification() {
    // Skip ring updates if button feedback is active to avoid conflicts
    if (skipRing) {
        return;
    }

    // Notification section is LEDs 11-22 (12 LEDs total) from MPR121LEDHandler
    static const int NOTIFICATION_START = 11;
    static const int NOTIFICATION_END = 22;
    static const int NOTIFICATION_COUNT = 12;

    // Representative colors for each party effect (based on SmartLantern.cpp order)
    // partyEffectsForCycling order: lust, emerald, suspendedPartyFire, codeRed, matrix,
    // technoOrange, rainbowTrance, partyFire, rainbow, future, futureRainbow, rgbPattern
    CRGB effectColors[] = {
        CRGB(255, 20, 147),    // Lust Effect - Deep pink/magenta
        CRGB(0, 255, 127),     // Emerald City - Emerald green
        CRGB(255, 69, 0),      // Suspended Party Fire - Orange red
        CRGB(220, 20, 60),     // Code Red - Crimson red
        CRGB(0, 255, 0),       // Matrix - Bright green
        CRGB(255, 140, 0),     // Techno Orange (Regal) - Dark orange
        CRGB(138, 43, 226),    // Rainbow Trance - Blue violet
        CRGB(255, 0, 0),       // Party Fire - Red
        CRGB(255, 255, 0),     // Rainbow - Yellow (center of rainbow)
        CRGB(0, 191, 255),     // Future - Deep sky blue
        CRGB(255, 0, 255),     // Future Rainbow - Magenta
        CRGB(128, 0, 128)      // RGB Pattern - Purple
    };

    int numEffects = sizeof(effectColors) / sizeof(effectColors[0]);

    // Get current time for animation
    unsigned long currentTime = millis();

    // Create a subtle breathing effect that cycles every 4 seconds
    float breathePhase = (currentTime % 4000) / 4000.0f * 2.0f * PI; // 0 to 2*PI over 4 seconds
    float breatheIntensity = (sin(breathePhase) + 1.0f) / 2.0f; // 0.0 to 1.0
    uint8_t brightness = 120 + (uint8_t)(breatheIntensity * 80); // 120 to 200 brightness range

    // Distribute effect colors across the notification LEDs
    for (int i = 0; i < NOTIFICATION_COUNT; i++) {
        int ringIndex = NOTIFICATION_START + i;

        // Map LED position to effect index
        // This spreads all effects across the 12 LEDs
        int effectIndex = (i * numEffects) / NOTIFICATION_COUNT;
        if (effectIndex >= numEffects) effectIndex = numEffects - 1;

        // Get the base color for this effect
        CRGB baseColor = effectColors[effectIndex];

        // Apply breathing brightness
        baseColor.nscale8_video(brightness);

        // Set the LED
        leds.getRing()[ringIndex] = baseColor;
    }
}