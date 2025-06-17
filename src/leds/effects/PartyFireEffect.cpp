// src/leds/effects/PartyFireEffect.cpp

#include "PartyFireEffect.h"

PartyFireEffect::PartyFireEffect(LEDController& ledController) :
    FireEffect(ledController),          // Call base fire effect constructor
    coreGlowIntensity(0.8f),           // Start with strong core glow
    lastCoreUpdate(0),                 // Initialize timing
    ringBreathingPhase(0.0f),          // Start breathing cycle at beginning
    ringBreathingSpeed(0.02f),         // Start with medium breathing speed
    ringIntensity(0.5f),               // Start at medium ring intensity
    lastRingUpdate(0),                 // Initialize timing
    nextSpeedChange(0)                 // Initialize speed change timing
{
    // Initialize timing variables
    lastCoreUpdate = millis();
    lastRingUpdate = millis();
    nextSpeedChange = millis() + SPEED_CHANGE_INTERVAL;

    Serial.println("PartyFireEffect created - fire with core glow and random ring breathing");
}

void PartyFireEffect::reset() {
    // Reset the base fire effect first
    FireEffect::reset();

    // Reset party-specific animations
    coreGlowIntensity = 0.8f;
    ringBreathingPhase = 0.0f;
    ringBreathingSpeed = generateRandomBreathingSpeed();
    ringIntensity = 0.5f;

    // Reset timing
    lastCoreUpdate = millis();
    lastRingUpdate = millis();
    nextSpeedChange = millis() + SPEED_CHANGE_INTERVAL;

    Serial.println("PartyFireEffect reset - all animations restarted");
}

void PartyFireEffect::update() {
    // Handle fire effect timing separately from core/ring timing
    unsigned long currentTime = millis();

    // Update fire effect at its own pace (20ms intervals like base FireEffect)
    if (currentTime - lastUpdateTime >= 20) {
        lastUpdateTime = currentTime;

        // Update the fire simulation (this is the core fire algorithm)
        updateFireBase();

        // Render the fire to inner and outer strips (but not show yet)
        renderFire();
    }

    // Update core and ring at their own independent timing
    updateCoreGlow();
    updateRingBreathing();

    // Show all the updated LEDs only once per frame
    leds.showAll();
}

void PartyFireEffect::updateCoreGlow() {
    unsigned long currentTime = millis();

    // Always update breathing phase for core breathing effect
    static float coreBreathingPhase = 0.0f;
    coreBreathingPhase += 0.01f; // Back to original breathing speed

    // Keep phase within 0 to 2*PI range
    if (coreBreathingPhase > 2.0f * PI) {
        coreBreathingPhase -= 2.0f * PI;
    }

    // Calculate breathing intensity with longer hold at peak
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

    // Apply the gradient to the core strip
    applyCoreGradient(coreGlowIntensity);
}

void PartyFireEffect::updateRingBreathing() {
    // Skip ring if button feedback is active
    if (skipRing) {
        return;
    }

    unsigned long currentTime = millis();

    // Always update the breathing phase for smooth animation
    ringBreathingPhase += ringBreathingSpeed;

    // Keep phase within 0 to 2*PI range
    if (ringBreathingPhase > 2.0f * PI) {
        ringBreathingPhase -= 2.0f * PI;
    }

    // Calculate breathing intensity using sine wave
    float sineValue = sin(ringBreathingPhase);      // -1.0 to 1.0
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Map to intensity range (30% to 100% for more dramatic breathing)
    ringIntensity = 0.3f + (normalizedSine * 0.7f);

    // Check if it's time to randomly change breathing speed (for unpredictability)
    if (currentTime >= nextSpeedChange) {
        ringBreathingSpeed = generateRandomBreathingSpeed();
        nextSpeedChange = currentTime + SPEED_CHANGE_INTERVAL + random(1000); // Add 0-1 second randomness

        Serial.print("Ring breathing speed changed to: ");
        Serial.println(ringBreathingSpeed, 4);
    }

    // Only add flicker occasionally, not every frame
    if (currentTime - lastRingUpdate >= RING_UPDATE_INTERVAL) {
        lastRingUpdate = currentTime;

        // Apply some random flicker for fire unpredictability
        float flicker = (random(100) / 100.0f) * 0.15f - 0.075f; // ±7.5% flicker
        ringIntensity += flicker;

        // Keep intensity within valid range
        if (ringIntensity < 0.0f) ringIntensity = 0.0f;
        if (ringIntensity > 1.0f) ringIntensity = 1.0f;
    }

    // Apply the glow to the ring strip
    applyRingGlow(ringIntensity);
}

void PartyFireEffect::applyCoreGradient(float intensity) {
    // Apply gradient from deep red at bottom to black at top across all core segments
    // Important: segment 1 (middle) is flipped according to mapLEDPosition

    int segmentLength = LED_STRIP_CORE_COUNT / 3;

    // Apply gradient to each of the 3 core segments
    for (int segment = 0; segment < 3; segment++) {
        for (int i = 0; i < segmentLength; i++) {
            // Calculate position as percentage from bottom (0.0) to top (1.0)
            float position = (float)i / (float)(segmentLength - 1);

            // Create gradient: fade to black over last 85%, with top 15% completely black
            float gradientIntensity;
            if (position < 0.85f) {
                // Bottom 85% fades from full brightness to black
                float fadePosition = position / 0.85f; // 0.0 to 1.0 over bottom 85%
                gradientIntensity = 1.0f - fadePosition; // Fade from 1.0 to 0.0
            } else {
                // Top 15% is completely black
                gradientIntensity = 0.0f;
            }

            // Apply overall breathing intensity
            float finalIntensity = gradientIntensity * intensity;

            // Convert deep red color to RGB components
            CRGB baseColor = leds.neoColorToCRGB(CORE_DEEP_RED);

            // Apply intensity to create the final color
            CRGB finalColor = CRGB(
                baseColor.r * finalIntensity,
                baseColor.g * finalIntensity,
                baseColor.b * finalIntensity
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

void PartyFireEffect::applyRingGlow(float intensity) {
    // Create color transition using the new MORE RED colors
    // Use breathing intensity to blend between the two red colors

    // Color 1: Primary red (0xEE1100) - brighter red
    CRGB color1 = leds.neoColorToCRGB(RING_RED_PRIMARY);

    // Color 2: Deeper red (0xCC0000) - darker red
    CRGB color2 = leds.neoColorToCRGB(RING_RED_SECONDARY);

    // Use intensity to blend between colors (0.0 = color2, 1.0 = color1)
    // This creates a color fade as the ring breathes
    float blendRatio = intensity; // Higher intensity = brighter red, lower = deeper red

    CRGB blendedColor = CRGB(
        color2.r + ((color1.r - color2.r) * blendRatio),
        color2.g + ((color1.g - color2.g) * blendRatio),
        color2.b + ((color1.b - color2.b) * blendRatio)
    );

    // Apply the overall intensity to the blended color for brightness breathing
    CRGB finalColor = CRGB(
        blendedColor.r * intensity,
        blendedColor.g * intensity,
        blendedColor.b * intensity
    );

    // Set all ring LEDs to the final color
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        leds.getRing()[i] = finalColor;
    }
}

float PartyFireEffect::generateRandomBreathingSpeed() {
    // Generate random breathing speed for unpredictable fire breathing
    // Range from slow to fast breathing (0.01 to 0.05 radians per update)
    // Slower = more dramatic, faster = more energetic

    float minSpeed = 0.008f;  // Very slow breathing (about 8 seconds per cycle)
    float maxSpeed = 0.04f;   // Fast breathing (about 1.5 seconds per cycle)

    // Generate random speed in this range
    float randomSpeed = minSpeed + ((random(100) / 100.0f) * (maxSpeed - minSpeed));

    return randomSpeed;
}