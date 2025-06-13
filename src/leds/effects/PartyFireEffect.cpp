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
    coreBreathingPhase += 0.01f; // Slower breathing for more noticeable effect (about 6-7 second cycle)

    // Keep phase within 0 to 2*PI range
    if (coreBreathingPhase > 2.0f * PI) {
        coreBreathingPhase -= 2.0f * PI;
    }

    // Calculate breathing intensity using sine wave
    float sineValue = sin(coreBreathingPhase);      // -1.0 to 1.0
    float normalizedSine = (sineValue + 1.0f) / 2.0f;  // 0.0 to 1.0

    // Map to much wider intensity range (20% to 100% for very obvious breathing)
    float breathingIntensity = 0.2f + (normalizedSine * 0.8f);

    // Check if it's time to update core glow variations
    if (currentTime - lastCoreUpdate >= CORE_UPDATE_INTERVAL) {
        lastCoreUpdate = currentTime;

        // Reduce random variation so breathing is more obvious
        float intensityVariation = (random(100) / 100.0f) * 0.05f - 0.025f; // ±2.5% variation (reduced)
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

    // Core strip has 3 segments - apply gradient to each one
    int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;

    for (int segment = 0; segment < 3; segment++) {
        for (int i = 0; i < coreSegmentLength; i++) {
            // Calculate position in segment (0.0 at bottom, 1.0 at top)
            float positionRatio = (float)i / (coreSegmentLength - 1);

            // Create extremely dramatic gradient fade
            // Use quartic fade (x^4) for very sharp drop-off to black
            float fadeAmount = 1.0f - (positionRatio * positionRatio * positionRatio * positionRatio);

            // Force even more black at the top - anything above 50% position gets heavily reduced
            if (positionRatio > 0.5f) {
                float topFade = (positionRatio - 0.5f) / 0.5f; // 0.0 to 1.0 in top 50%
                fadeAmount *= (1.0f - topFade * 0.95f); // Reduce by up to 95% in top half
            }

            // Force complete black in top 20%
            if (positionRatio > 0.8f) {
                fadeAmount = 0.0f; // Force pure black at very top
            }

            // Apply overall intensity with breathing effect
            fadeAmount *= intensity;

            // Convert deep red color to RGB and apply fade
            CRGB baseColor = leds.neoColorToCRGB(CORE_DEEP_RED);
            CRGB fadedColor = CRGB(
                baseColor.r * fadeAmount,
                baseColor.g * fadeAmount,
                baseColor.b * fadeAmount
            );

            // Calculate physical LED position using the LED controller's mapping
            int physicalPos = leds.mapPositionToPhysical(0, i, segment); // 0 = core strip
            physicalPos += segment * coreSegmentLength; // Add segment offset

            // Set the LED color
            if (physicalPos >= 0 && physicalPos < LED_STRIP_CORE_COUNT) {
                leds.getCore()[physicalPos] = fadedColor;
            }
        }
    }
}

void PartyFireEffect::applyRingGlow(float intensity) {
    // Create color transition from red-orange to deeper red
    // Use breathing intensity to blend between two colors

    // Color 1: Current red-orange (0xFF4500)
    CRGB color1 = leds.neoColorToCRGB(RING_RED_ORANGE);

    // Color 2: Deeper/more red color (0xDD2200 - darker and more red)
    CRGB color2 = leds.neoColorToCRGB(0xDD2200);

    // Use intensity to blend between colors (0.0 = color2, 1.0 = color1)
    // This creates a color fade as the ring breathes
    float blendRatio = intensity; // Higher intensity = more orange, lower = more red

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