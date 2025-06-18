// src/leds/effects/SuspendedPartyFireEffect.cpp

#include "SuspendedPartyFireEffect.h"

SuspendedPartyFireEffect::SuspendedPartyFireEffect(LEDController& ledController) :
    SuspendedFireEffect(ledController),    // Call base suspended fire effect constructor
    coreGlowIntensity(0.8f),              // Start with strong core glow
    lastCoreUpdate(0),                    // Initialize timing
    ringBreathingPhase(0.0f),             // Start breathing cycle at beginning
    ringIntensity(0.05f),                 // Start at minimum intensity
    lastRingUpdate(0),                    // Initialize timing
    nextSpeedChange(0)                    // Initialize speed change timing
{
    // Initialize timing variables - NO SPEED CHANGE TIMING
    lastCoreUpdate = millis();

    Serial.println("SuspendedPartyFireEffect created - suspended fire with FLIPPED core glow and simple ring breathing");
}

void SuspendedPartyFireEffect::reset() {
    // Reset the base suspended fire effect first
    SuspendedFireEffect::reset();

    // Reset party-specific animations - NO SPEED CHANGES
    coreGlowIntensity = 0.8f;
    ringBreathingPhase = 0.0f;
    ringIntensity = 0.05f;

    // Reset timing - no speed change timing
    lastCoreUpdate = millis();

    Serial.println("SuspendedPartyFireEffect reset - core and ring restarted");
}

void SuspendedPartyFireEffect::update() {
    // Handle suspended fire effect timing separately from core/ring timing
    unsigned long currentTime = millis();

    // Update suspended fire effect at its own pace (20ms intervals like base SuspendedFireEffect)
    if (currentTime - lastUpdateTime >= 20) {
        lastUpdateTime = currentTime;

        // Update dynamic flame heights for natural variation
        updateFlameHeights();

        // Update the suspended fire simulation
        updateSuspendedFireBase();

        // Render the suspended fire to inner and outer strips (but not show yet)
        renderSuspendedFire();
    }

    // Update core and ring at their own independent timing
    updateCoreGlow();
    updateRingBreathing();

    // Show all the updated LEDs only once per frame
    leds.showAll();
}

void SuspendedPartyFireEffect::updateCoreGlow() {
    unsigned long currentTime = millis();

    // Always update breathing phase for core breathing effect
    static float coreBreathingPhase = 0.0f;
    coreBreathingPhase += 0.005f; // Half speed: was 0.01f, now 0.005f

    // Keep phase within 0 to 2*PI range
    if (coreBreathingPhase > 2.0f * PI) {
        coreBreathingPhase -= 2.0f * PI;
    }

    // Calculate breathing intensity with longer hold at peak (same as PartyFireEffect)
    float sineValue = sin(coreBreathingPhase);      // -1.0 to 1.0
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Create a breathing pattern that holds longer at peak brightness
    float breathingCurve;
    if (normalizedSine > 0.7f) {
        // Hold at peak for longer (when sine is in top 30% of cycle)
        breathingCurve = 1.0f;
    } else {
        // Scale the remaining 70% of the cycle to 0.0-1.0 range
        breathingCurve = normalizedSine / 0.7f;
    }

    // Map back to original intensity range (20% to 100%) but with longer peak hold
    float breathingIntensity = 0.2f + (breathingCurve * 0.8f);

    // Check if it's time to update core glow variations
    if (currentTime - lastCoreUpdate >= CORE_UPDATE_INTERVAL) {
        lastCoreUpdate = currentTime;

        // Minimal random variation to keep breathing obvious
        float intensityVariation = (random(100) / 100.0f) * 0.03f - 0.015f; // ±1.5% variation
        coreGlowIntensity = breathingIntensity + intensityVariation;

        // Keep intensity within valid range
        if (coreGlowIntensity < 0.0f) coreGlowIntensity = 0.0f;
        if (coreGlowIntensity > 1.0f) coreGlowIntensity = 1.0f;
    }

    // Apply the FLIPPED gradient to the core strip
    applyCoreGradientFlipped(coreGlowIntensity);
}

void SuspendedPartyFireEffect::updateRingBreathing() {
    // Skip ring if button feedback is active
    if (skipRing) {
        return;
    }

    // Fixed slow breathing speed - no random changes, no bursts
    static float fixedBreathingSpeed = 0.01f;

    // Always update the breathing phase for smooth animation
    ringBreathingPhase += fixedBreathingSpeed;

    // Keep phase within 0 to 2*PI range
    if (ringBreathingPhase > 2.0f * PI) {
        ringBreathingPhase -= 2.0f * PI;
    }

    // Calculate breathing intensity using sine wave
    float sineValue = sin(ringBreathingPhase);      // -1.0 to 1.0
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Map to intensity range (5% to 20% for very dim base)
    ringIntensity = 0.05f + (normalizedSine * 0.15f);

    // Apply the glow to the ring strip - NO OTHER MODIFICATIONS
    applyRingGlow(ringIntensity);
}

void SuspendedPartyFireEffect::applyCoreGradientFlipped(float intensity) {
    // Apply FLIPPED gradient from deep red at BOTTOM to black at TOP across all core segments
    // This is opposite from PartyFireEffect which goes from red at bottom to black at top
    // Important: segment 1 (middle) is flipped according to mapLEDPosition

    int segmentLength = LED_STRIP_CORE_COUNT / 3;

    // Apply gradient to each of the 3 core segments
    for (int segment = 0; segment < 3; segment++) {
        for (int i = 0; i < segmentLength; i++) {
            // Calculate position as percentage from BOTTOM (0.0) to TOP (1.0)
            float position = (float)i / (float)(segmentLength - 1);

            // Red starts closer to the edge - reduce gap from 15% to 8%
            float gradientIntensity;

            if (position > 0.92f) {
                // Top 8%: completely black (smaller gap before red starts)
                gradientIntensity = 0.0f;
            } else {
                // From bottom to 92%: fade from black at bottom to full red at 92%
                float fadePosition = position / 0.92f; // 0.0 to 1.0 over bottom 92%
                gradientIntensity = fadePosition; // Fade from 0.0 to 1.0 as we go up
            }

            // Apply overall breathing intensity
            float finalIntensity = gradientIntensity * intensity;

            // Convert deep red color to RGB components and add orange tint
            CRGB baseColor = leds.neoColorToCRGB(CORE_DEEP_RED);

            // Create red-orange color (more red, less orange)
            CRGB redOrangeColor = CRGB(
                220,                    // Higher red (more red than before)
                baseColor.g + 20,       // Less green for more red tint (reduced from +30)
                baseColor.b             // Keep minimal blue
            );

            // Apply intensity to create the final red-orange color
            CRGB finalColor = CRGB(
                redOrangeColor.r * finalIntensity,
                redOrangeColor.g * finalIntensity,
                redOrangeColor.b * finalIntensity
            );

            // Get physical LED position (handles segment flipping automatically)
            int physicalPos = mapLEDPosition(0, i, segment); // 0 = core strip type

            // Calculate the actual LED index in the strip array
            int actualLEDIndex = (segment * segmentLength) + physicalPos;

            if (actualLEDIndex >= 0 && actualLEDIndex < LED_STRIP_CORE_COUNT) {
                leds.getCore()[actualLEDIndex] = finalColor;
            }
        }
    }
}

void SuspendedPartyFireEffect::applyRingGlow(float intensity) {
    // Apply simple solid breathing glow to ring strip (no complex blending)

    // Use the primary red color and just scale by intensity
    CRGB primaryColor = leds.neoColorToCRGB(RING_RED_PRIMARY);
    CRGB finalColor = CRGB(
        primaryColor.r * intensity,
        primaryColor.g * intensity,
        primaryColor.b * intensity
    );

    // Fill entire ring with the simple breathing color
    fill_solid(leds.getRing(), LED_STRIP_RING_COUNT, finalColor);
}

float SuspendedPartyFireEffect::generateRandomBreathingSpeed() {
    // Generate random breathing speed between 0.005 and 0.035 (same as PartyFireEffect)
    // This creates variety in the breathing pattern for natural fire effects
    return 0.005f + (random(300) / 10000.0f); // 0.005 to 0.035
}