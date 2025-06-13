// src/leds/effects/PartyRippleEffect.cpp

#include "PartyRippleEffect.h"

PartyRippleEffect::PartyRippleEffect(LEDController& ledController) :
    Effect(ledController),
    lastUpdate(0)
{
    // Reserve space for ripples to avoid memory reallocations
    ripples.reserve(MAX_RIPPLES);

    Serial.println("PartyRippleEffect created - colorful expanding ripples with fade-out");
}

PartyRippleEffect::~PartyRippleEffect() {
    // Vector automatically cleans up
}

void PartyRippleEffect::reset() {
    // Clear all active ripples
    ripples.clear();
    lastUpdate = millis();

    Serial.println("PartyRippleEffect reset - all ripples cleared");
}

void PartyRippleEffect::update() {
    // Target 60 FPS for smooth ripple animation
    if (!shouldUpdate(16)) {  // 16ms = ~60 FPS
        return;
    }

    // Clear all strips first
    leds.clearAll();

    // Randomly create new ripples
    if (random(100) < RIPPLE_CREATE_CHANCE) {
        createNewRipple();
    }

    // Update all existing ripples
    updateRipples();

    // Draw all ripples
    drawRipples();

    // Show the changes
    leds.showAll();
}

void PartyRippleEffect::createNewRipple() {
    // Don't create more ripples if we're at maximum
    if (ripples.size() >= MAX_RIPPLES) {
        return;
    }

    Ripple newRipple;

    // Randomly choose inner (1) or outer (2) strips
    newRipple.stripType = random(1, 3);

    // Randomly choose which segment (0, 1, or 2)
    newRipple.subStrip = random(3);

    // Get strip length
    int stripLength = (newRipple.stripType == 1) ? INNER_LEDS_PER_STRIP : OUTER_LEDS_PER_STRIP;

    // Allow ripple center to be off the edges of the strip
    // This creates ripples that enter from the sides
    // Range: -MAX_RADIUS to (stripLength + MAX_RADIUS)
    newRipple.centerPos = random(-MAX_RADIUS, stripLength + MAX_RADIUS);

    // Start with radius 0 (will expand outward)
    newRipple.radius = 0.0f;

    // Start with full brightness (no fade)
    newRipple.fadeOut = 1.0f;

    // Generate a random bright color
    newRipple.color = generateRandomColor();

    // Activate the ripple
    newRipple.active = true;

    // Add to ripples vector
    ripples.push_back(newRipple);

    // Debug output
    Serial.print("New ripple created: ");
    Serial.print(newRipple.stripType == 1 ? "Inner" : "Outer");
    Serial.print(" strip, segment ");
    Serial.print(newRipple.subStrip);
    Serial.print(", position ");
    Serial.print(newRipple.centerPos);
    if (newRipple.centerPos < 0) {
        Serial.print(" (starting off left edge)");
    } else if (newRipple.centerPos >= stripLength) {
        Serial.print(" (starting off right edge)");
    }
    Serial.println();
}

void PartyRippleEffect::updateRipples() {
    // Update each ripple
    for (auto& ripple : ripples) {
        if (!ripple.active) continue;

        // Expand the ripple radius
        ripple.radius += RIPPLE_SPEED;

        // Start fading when ripple reaches fade start radius
        if (ripple.radius > FADE_START_RADIUS) {
            // Calculate fade based on how far we've traveled
            // We'll fade from radius 6 to radius 28 (very extended)
            float fadeDistance = ripple.radius - FADE_START_RADIUS;
            float maxFadeDistance = 22.0f; // Fade over 22 radius units (6 to 28)

            float fadeProgress = fadeDistance / maxFadeDistance;
            fadeProgress = min(1.0f, fadeProgress); // Clamp to 1.0

            // Very gentle fade curve - stay bright for most of the journey
            if (fadeProgress < 0.5f) {
                // First half: barely fade at all (100% to 90%)
                ripple.fadeOut = 1.0f - (fadeProgress * 0.2f); // 1.0 to 0.9
            } else if (fadeProgress < 0.8f) {
                // Next 30%: gentle fade (90% to 60%)
                float midProgress = (fadeProgress - 0.5f) / 0.3f;
                ripple.fadeOut = 0.9f - (midProgress * 0.3f); // 0.9 to 0.6
            } else {
                // Final 20%: faster fade to completion (60% to 0%)
                float finalProgress = (fadeProgress - 0.8f) / 0.2f;
                ripple.fadeOut = 0.6f - (finalProgress * 0.6f); // 0.6 to 0.0
            }
        }

        // Only deactivate when ripple is WAY past its visible range
        if (ripple.radius > 28.0f || ripple.fadeOut <= 0.01f) {
            ripple.active = false;
        }
    }

    // Remove inactive ripples
    ripples.erase(
        std::remove_if(ripples.begin(), ripples.end(),
            [](const Ripple& r) { return !r.active; }),
        ripples.end());
}

void PartyRippleEffect::drawRipples() {
    // Draw each active ripple
    for (const auto& ripple : ripples) {
        if (!ripple.active) continue;

        // Draw the ripple from center-radius to center+radius
        int startPos = ripple.centerPos - (int)MAX_RADIUS;
        int endPos = ripple.centerPos + (int)MAX_RADIUS;

        // Get strip length
        int stripLength = (ripple.stripType == 1) ? INNER_LEDS_PER_STRIP : OUTER_LEDS_PER_STRIP;

        // Draw each LED in the potential ripple area
        for (int pos = startPos; pos <= endPos; pos++) {
            // Skip if outside strip bounds
            if (pos < 0 || pos >= stripLength) continue;

            // Calculate distance from ripple center
            float distance = abs(pos - ripple.centerPos);

            // Calculate brightness based on distance, current radius, and fade-out
            float brightness = calculateRippleBrightness(distance, ripple.radius, ripple.fadeOut);

            // Skip if this LED is not part of the current ripple
            if (brightness <= 0.0f) continue;

            // Apply brightness to ripple color
            CRGB ledColor = CRGB(
                ripple.color.r * brightness,
                ripple.color.g * brightness,
                ripple.color.b * brightness
            );

            // Get physical LED position
            int physicalPos = leds.mapPositionToPhysical(ripple.stripType, pos, ripple.subStrip);

            // Adjust for segment offset
            if (ripple.stripType == 1) {
                // Inner strips
                physicalPos += ripple.subStrip * INNER_LEDS_PER_STRIP;

                // Add color to existing (allows ripples to blend)
                if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                    leds.getInner()[physicalPos] += ledColor;
                }
            } else {
                // Outer strips
                physicalPos += ripple.subStrip * OUTER_LEDS_PER_STRIP;

                // Add color to existing (allows ripples to blend)
                if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                    leds.getOuter()[physicalPos] += ledColor;
                }
            }
        }
    }

    // Apply brightness limiting to prevent oversaturation when ripples overlap
    // This ensures overlapping ripples create nice color blends without going pure white
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        CRGB& pixel = leds.getInner()[i];
        // Limit maximum brightness while preserving color ratios
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        if (maxComponent > 230) {  // Threshold for better color mixing
            float scale = 230.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }

    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        CRGB& pixel = leds.getOuter()[i];
        // Limit maximum brightness while preserving color ratios
        uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
        if (maxComponent > 230) {  // Threshold for better color mixing
            float scale = 230.0f / maxComponent;
            pixel.r = pixel.r * scale;
            pixel.g = pixel.g * scale;
            pixel.b = pixel.b * scale;
        }
    }
}

CRGB PartyRippleEffect::generateRandomColor() {
    // Generate vibrant colors using HSV color space
    // Random hue for variety, full saturation for vibrancy
    uint8_t hue = random(256);      // Random hue (0-255)
    uint8_t saturation = 255;       // Full saturation for bright colors
    uint8_t value = 255;            // Full brightness

    // Convert HSV to RGB
    return CHSV(hue, saturation, value);
}

float PartyRippleEffect::calculateRippleBrightness(float distance, float radius, float fadeOut) {
    // If this LED is outside the current ripple radius, it's off
    if (distance > radius) {
        return 0.0f;
    }

    // LED is within the ripple - calculate brightness
    float brightness;

    // Maintain ripple shape even as it expands beyond MAX_RADIUS
    float effectiveRadius = min(radius, MAX_RADIUS * 1.2f); // Soft cap at 16.8

    // Calculate base brightness with distance from center
    brightness = 1.0f - (distance / effectiveRadius);

    // Apply very gentle curve to maintain visibility
    // Using power of 1.2 instead of 2 for much brighter ripples
    brightness = pow(brightness, 1.2f);

    // Ensure minimum brightness for visible parts of the ripple
    // This keeps the ripple visible even at edges
    if (brightness > 0.0f) {
        brightness = max(brightness, 0.15f); // Minimum 15% brightness
    }

    // Apply the fade-out multiplier
    brightness *= fadeOut;

    return brightness;
}