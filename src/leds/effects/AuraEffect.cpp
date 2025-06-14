// src/leds/effects/AuraEffect.cpp

#include "AuraEffect.h"

AuraEffect::AuraEffect(LEDController& ledController,
                       bool enableCore,
                       bool enableInner,
                       bool enableOuter,
                       bool enableRing) :
    Effect(ledController),
    coreEnabled(enableCore),
    innerEnabled(enableInner),
    outerEnabled(enableOuter),
    ringEnabled(enableRing),
    lastUpdate(0)
{
    // Reserve space for ripples to avoid memory reallocations
    ripples.reserve(MAX_RIPPLES);

    Serial.println("AuraEffect created - colorful expanding ripples with fade-out");
    Serial.print("Enabled strips - Core: ");
    Serial.print(coreEnabled ? "YES" : "NO");
    Serial.print(", Inner: ");
    Serial.print(innerEnabled ? "YES" : "NO");
    Serial.print(", Outer: ");
    Serial.print(outerEnabled ? "YES" : "NO");
    Serial.print(", Ring: ");
    Serial.println(ringEnabled ? "YES" : "NO");
}

AuraEffect::~AuraEffect() {
    // Vector automatically cleans up
}

void AuraEffect::reset() {
    // Clear all active ripples
    ripples.clear();
    lastUpdate = millis();

    Serial.println("AuraEffect reset - all ripples cleared");
}

void AuraEffect::update() {
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

void AuraEffect::createNewRipple() {
    // Don't create more ripples if we're at maximum
    if (ripples.size() >= MAX_RIPPLES) {
        return;
    }

    // Count how many strip types are enabled
    int enabledCount = 0;
    if (coreEnabled) enabledCount++;
    if (innerEnabled) enabledCount++;
    if (outerEnabled) enabledCount++;
    if (ringEnabled) enabledCount++;

    // If no strips are enabled, don't create ripples
    if (enabledCount == 0) {
        return;
    }

    Ripple newRipple;

    // Randomly choose a strip type from enabled strips only
    int choice = random(enabledCount);
    int currentChoice = 0;

    // Map the choice to an enabled strip type
    if (coreEnabled) {
        if (currentChoice == choice) {
            newRipple.stripType = 0; // Core
        }
        currentChoice++;
    }
    if (innerEnabled && currentChoice - 1 < choice) {
        if (currentChoice == choice) {
            newRipple.stripType = 1; // Inner
        }
        currentChoice++;
    }
    if (outerEnabled && currentChoice - 1 < choice) {
        if (currentChoice == choice) {
            newRipple.stripType = 2; // Outer
        }
        currentChoice++;
    }
    if (ringEnabled && currentChoice - 1 < choice) {
        newRipple.stripType = 3; // Ring
    }

    // For strips with segments, randomly choose which segment
    if (newRipple.stripType == 0) {
        // Core has 3 segments
        newRipple.subStrip = random(3);
    } else if (newRipple.stripType == 1 || newRipple.stripType == 2) {
        // Inner and outer have 3 segments
        newRipple.subStrip = random(3);
    } else {
        // Ring doesn't have segments
        newRipple.subStrip = 0;
    }

    // Get strip length
    int stripLength = getStripLength(newRipple.stripType, newRipple.subStrip);

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
    String stripNames[] = {"Core", "Inner", "Outer", "Ring"};
    Serial.print("New ripple created: ");
    Serial.print(stripNames[newRipple.stripType]);
    Serial.print(" strip");
    if (newRipple.stripType != 3) { // Not ring
        Serial.print(", segment ");
        Serial.print(newRipple.subStrip);
    }
    Serial.print(", position ");
    Serial.print(newRipple.centerPos);
    if (newRipple.centerPos < 0) {
        Serial.print(" (starting off left edge)");
    } else if (newRipple.centerPos >= stripLength) {
        Serial.print(" (starting off right edge)");
    }
    Serial.println();
}

void AuraEffect::updateRipples() {
    // Update each ripple
    for (auto& ripple : ripples) {
        if (!ripple.active) continue;

        // Expand the ripple radius
        ripple.radius += RIPPLE_SPEED;

        // Start fading when ripple reaches fade start radius
        if (ripple.radius > FADE_START_RADIUS) {
            // Calculate fade based on how far we've traveled
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

void AuraEffect::drawRipples() {
    // Draw each active ripple
    for (const auto& ripple : ripples) {
        if (!ripple.active) continue;

        // Skip if this strip type is disabled
        if (ripple.stripType == 0 && !coreEnabled) continue;
        if (ripple.stripType == 1 && !innerEnabled) continue;
        if (ripple.stripType == 2 && !outerEnabled) continue;
        if (ripple.stripType == 3 && !ringEnabled) continue;

        // Draw the ripple from center-radius to center+radius
        int startPos = ripple.centerPos - (int)MAX_RADIUS;
        int endPos = ripple.centerPos + (int)MAX_RADIUS;

        // Get strip length
        int stripLength = getStripLength(ripple.stripType, ripple.subStrip);

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

            // Adjust for segment offset based on strip type
            if (ripple.stripType == 0) {
                // Core strips
                int segmentLength = LED_STRIP_CORE_COUNT / 3;
                physicalPos += ripple.subStrip * segmentLength;

                // Add color to existing (allows ripples to blend)
                if (physicalPos >= 0 && physicalPos < LED_STRIP_CORE_COUNT) {
                    leds.getCore()[physicalPos] += ledColor;
                }
            } else if (ripple.stripType == 1) {
                // Inner strips
                physicalPos += ripple.subStrip * INNER_LEDS_PER_STRIP;

                // Add color to existing (allows ripples to blend)
                if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                    leds.getInner()[physicalPos] += ledColor;
                }
            } else if (ripple.stripType == 2) {
                // Outer strips
                physicalPos += ripple.subStrip * OUTER_LEDS_PER_STRIP;

                // Add color to existing (allows ripples to blend)
                if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                    leds.getOuter()[physicalPos] += ledColor;
                }
            } else if (ripple.stripType == 3) {
                // Ring strip (no segments)
                if (physicalPos >= 0 && physicalPos < LED_STRIP_RING_COUNT) {
                    leds.getRing()[physicalPos] += ledColor;
                }
            }
        }
    }

    // Apply brightness limiting to prevent oversaturation when ripples overlap
    // This ensures overlapping ripples create nice color blends without going pure white

    // Limit core strip brightness
    if (coreEnabled) {
        for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
            CRGB& pixel = leds.getCore()[i];
            uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
            if (maxComponent > 230) {
                float scale = 230.0f / maxComponent;
                pixel.r = pixel.r * scale;
                pixel.g = pixel.g * scale;
                pixel.b = pixel.b * scale;
            }
        }
    }

    // Limit inner strip brightness
    if (innerEnabled) {
        for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
            CRGB& pixel = leds.getInner()[i];
            uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
            if (maxComponent > 230) {
                float scale = 230.0f / maxComponent;
                pixel.r = pixel.r * scale;
                pixel.g = pixel.g * scale;
                pixel.b = pixel.b * scale;
            }
        }
    }

    // Limit outer strip brightness
    if (outerEnabled) {
        for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
            CRGB& pixel = leds.getOuter()[i];
            uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
            if (maxComponent > 230) {
                float scale = 230.0f / maxComponent;
                pixel.r = pixel.r * scale;
                pixel.g = pixel.g * scale;
                pixel.b = pixel.b * scale;
            }
        }
    }

    // Limit ring strip brightness
    if (ringEnabled && !skipRing) {
        for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
            CRGB& pixel = leds.getRing()[i];
            uint8_t maxComponent = max(max(pixel.r, pixel.g), pixel.b);
            if (maxComponent > 230) {
                float scale = 230.0f / maxComponent;
                pixel.r = pixel.r * scale;
                pixel.g = pixel.g * scale;
                pixel.b = pixel.b * scale;
            }
        }
    }
}

CRGB AuraEffect::generateRandomColor() {
    // Generate vibrant colors using HSV color space
    // Random hue for variety, full saturation for vibrancy
    uint8_t hue = random(256);      // Random hue (0-255)
    uint8_t saturation = 255;       // Full saturation for bright colors
    uint8_t value = 255;            // Full brightness

    // Convert HSV to RGB
    return CHSV(hue, saturation, value);
}

float AuraEffect::calculateRippleBrightness(float distance, float radius, float fadeOut) {
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

int AuraEffect::getStripLength(int stripType, int subStrip) {
    switch (stripType) {
        case 0:  // Core - each segment is 1/3 of total
            return LED_STRIP_CORE_COUNT / 3;
        case 1:  // Inner strips
            return INNER_LEDS_PER_STRIP;
        case 2:  // Outer strips
            return OUTER_LEDS_PER_STRIP;
        case 3:  // Ring
            return LED_STRIP_RING_COUNT;
        default:
            return 0;
    }
}