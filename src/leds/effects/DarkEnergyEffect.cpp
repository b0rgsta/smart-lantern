// src/leds/effects/DarkEnergyEffect.cpp

#include "DarkEnergyEffect.h"
#include <algorithm>

// Constructor - sets up the dark energy effect
DarkEnergyEffect::DarkEnergyEffect(LEDController& ledController)
    : Effect(ledController), ballPosition(0.5f), ballVelocity(0.0f),
      breathingPhase(0.0f), rangePhase(0.0f), energyPhase(0.0f), lastUpdateTime(0) {
    // Constructor intentionally does NOT call leds.clear() as per your requirements
    // Effect will be applied on first update() call
    // Ball starts in center with no initial velocity
}

// Update the effect - applies dark energy pattern with hovering black ball
void DarkEnergyEffect::update() {
    // Get frame time for smooth animation
    unsigned long currentTime = millis();
    if (lastUpdateTime == 0) {
        lastUpdateTime = currentTime;
    }
    float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
    lastUpdateTime = currentTime;

    // Update black ball animation
    updateBlackBall();

    // Clear all strips to start fresh
    leds.clearAll();

    // Apply base red pattern to inner and outer strips
    applyInnerPattern();
    applyOuterPattern();

    // Apply black ball effect on top of red base
    applyBlackBall();

    // Core and ring strips remain off (already cleared)

    // Show all changes to make them visible
    leds.showAll();
}

// Reset the effect to initial state
void DarkEnergyEffect::reset() {
    // Reset ball to center position with no velocity
    ballPosition = 0.5f;
    ballVelocity = 0.0f;
    breathingPhase = 0.0f;
    rangePhase = 0.0f;
    energyPhase = 0.0f;
    lastUpdateTime = 0;
}

// Apply dark energy pattern to inner strips
void DarkEnergyEffect::applyInnerPattern() {
    // Apply pattern to each of the 3 inner strips independently
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        int segmentStart = segment * INNER_LEDS_PER_STRIP;

        // Apply red with fade to each LED in this strip
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;

            // Calculate fade brightness for this position
            float fadeBrightness = calculateFadeBrightness(i, INNER_LEDS_PER_STRIP);

            // Apply red color with calculated brightness
            applyRedWithBrightness(leds.getInner()[ledIndex], fadeBrightness);
        }
    }
}

// Apply dark energy pattern to outer strips
void DarkEnergyEffect::applyOuterPattern() {
    // Apply pattern to each of the 3 outer strips independently
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        // Apply red with fade to each LED in this strip
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;

            // Calculate fade brightness for this position
            float fadeBrightness = calculateFadeBrightness(i, OUTER_LEDS_PER_STRIP);

            // Apply red color with calculated brightness
            applyRedWithBrightness(leds.getOuter()[ledIndex], fadeBrightness);
        }
    }
}

// Calculate fade brightness for position (90% black fade from both ends)
float DarkEnergyEffect::calculateFadeBrightness(int position, int stripLength) {
    // Calculate distance from center as a ratio (0.0 = center, 1.0 = edge)
    float center = (stripLength - 1) / 2.0f;
    float distanceFromCenter = abs(position - center) / center;

    // Clamp distance to valid range
    distanceFromCenter = std::min(1.0f, distanceFromCenter);

    // Apply 90% fade:
    // - At center (distance = 0.0): brightness = 1.0 (full effect)
    // - At edges (distance = 1.0): brightness = 0.1 (10% remaining = 90% fade)
    float brightness = 1.0f - (distanceFromCenter * FADE_PERCENTAGE);

    return brightness;
}

// Apply red color with specified brightness to an LED
void DarkEnergyEffect::applyRedWithBrightness(CRGB& color, float brightnessFactor) {
    // Start with base red color at 50% brightness
    CRGB baseColor = leds.neoColorToCRGB(BASE_RED_COLOR);

    // Apply base brightness (50%) and fade brightness
    float finalBrightness = BASE_BRIGHTNESS * brightnessFactor;

    // Set the LED color with calculated brightness
    color = CRGB(
        (uint8_t)(baseColor.r * finalBrightness),
        (uint8_t)(baseColor.g * finalBrightness),
        (uint8_t)(baseColor.b * finalBrightness)
    );
}

// Update black ball animation physics
void DarkEnergyEffect::updateBlackBall() {
    // Update breathing phase for size animation
    breathingPhase += BALL_BREATHING_SPEED;
    if (breathingPhase > 2.0f * PI) {
        breathingPhase -= 2.0f * PI;
    }

    // Update range phase for travel range animation
    rangePhase += BALL_RANGE_SPEED;
    if (rangePhase > 2.0f * PI) {
        rangePhase -= 2.0f * PI;
    }

    // Update energy phase for red base pulsing
    energyPhase += ENERGY_PULSE_SPEED;
    if (energyPhase > 2.0f * PI) {
        energyPhase -= 2.0f * PI;
    }

    // Simple smooth sine wave motion - like a pendulum
    static float movementPhase = 0.0f;
    movementPhase += BALL_MOVE_SPEED;
    if (movementPhase > 2.0f * PI) {
        movementPhase -= 2.0f * PI;
    }

    // Convert sine wave to smooth 0-1 range
    float rawPosition = (sin(movementPhase) + 1.0f) / 2.0f; // 0.0 to 1.0

    // Get current travel range (animates between 30% and 70%)
    float currentTravelRange = calculateTravelRange();

    // Calculate gaps to center the travel range
    float currentGap = (1.0f - currentTravelRange) / 2.0f;

    // Map position to dynamic range with animated gaps
    ballPosition = currentGap + (rawPosition * currentTravelRange);
}

// Calculate current ball size based on breathing effect
float DarkEnergyEffect::calculateBallSize() {
    // Use sine wave for breathing effect
    float breathingFactor = (sin(breathingPhase) + 1.0f) / 2.0f; // 0.0 to 1.0

    // Map to size range (60% to 140% of base size)
    return BALL_MIN_SIZE + (breathingFactor * (BALL_MAX_SIZE - BALL_MIN_SIZE));
}

// Calculate current travel range based on range animation
float DarkEnergyEffect::calculateTravelRange() {
    // Use sine wave for smooth range expansion/contraction
    float rangeFactor = (sin(rangePhase) + 1.0f) / 2.0f; // 0.0 to 1.0

    // Map to range between 30% and 70% of strip length
    return BALL_MIN_RANGE + (rangeFactor * (BALL_MAX_RANGE - BALL_MIN_RANGE));
}

// Calculate current energy pulse intensity
float DarkEnergyEffect::calculateEnergyIntensity() {
    // Use sine wave for smooth energy pulsing
    float energyFactor = (sin(energyPhase) + 1.0f) / 2.0f; // 0.0 to 1.0

    // Map to intensity range (70% to 130% of base brightness)
    return ENERGY_MIN_INTENSITY + (energyFactor * (ENERGY_MAX_INTENSITY - ENERGY_MIN_INTENSITY));
}

// Apply black ball effect to both inner and outer strips
void DarkEnergyEffect::applyBlackBall() {
    float currentBallSize = calculateBallSize();

    // Apply to inner strips
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        int segmentStart = segment * INNER_LEDS_PER_STRIP;
        float stripLength = INNER_LEDS_PER_STRIP;

        // Calculate ball center position on this strip
        float ballCenter = ballPosition * stripLength;

        // Calculate ball radius - fixed to be truly 70% of strip length
        // When BALL_COVERAGE = 0.7, the ball diameter should be 70% of strip length
        float ballRadius = (BALL_COVERAGE * stripLength * currentBallSize) / 2.0f;

        // Apply black ball to each LED in this strip
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;
            float distanceFromCenter = abs((float)i - ballCenter);

            // If LED is within ball radius, apply black effect
            if (distanceFromCenter <= ballRadius) {
                // Create smooth edge with falloff for realistic sphere appearance
                float edgeFalloff = 1.0f - (distanceFromCenter / ballRadius);
                edgeFalloff = edgeFalloff * edgeFalloff; // Square for smoother edge

                // Apply black by reducing existing color brightness
                CRGB currentColor = leds.getInner()[ledIndex];
                leds.getInner()[ledIndex] = CRGB(
                    (uint8_t)(currentColor.r * (1.0f - edgeFalloff)),
                    (uint8_t)(currentColor.g * (1.0f - edgeFalloff)),
                    (uint8_t)(currentColor.b * (1.0f - edgeFalloff))
                );
            }
        }
    }

    // Apply to outer strips
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;
        float stripLength = OUTER_LEDS_PER_STRIP;

        // Calculate ball center position on this strip
        float ballCenter = ballPosition * stripLength;

        // Calculate ball radius - fixed to be truly 70% of strip length
        float ballRadius = (BALL_COVERAGE * stripLength * currentBallSize) / 2.0f;

        // Apply black ball to each LED in this strip
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;
            float distanceFromCenter = abs((float)i - ballCenter);

            // If LED is within ball radius, apply black effect
            if (distanceFromCenter <= ballRadius) {
                // Create smooth edge with falloff for realistic sphere appearance
                float edgeFalloff = 1.0f - (distanceFromCenter / ballRadius);
                edgeFalloff = edgeFalloff * edgeFalloff; // Square for smoother edge

                // Apply black by reducing existing color brightness
                CRGB currentColor = leds.getOuter()[ledIndex];
                leds.getOuter()[ledIndex] = CRGB(
                    (uint8_t)(currentColor.r * (1.0f - edgeFalloff)),
                    (uint8_t)(currentColor.g * (1.0f - edgeFalloff)),
                    (uint8_t)(currentColor.b * (1.0f - edgeFalloff))
                );
            }
        }
    }
}