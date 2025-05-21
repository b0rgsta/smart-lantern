// src/leds/effects/AcceleratingTrailsEffect.cpp

#include "AcceleratingTrailsEffect.h"

AcceleratingTrailsEffect::AcceleratingTrailsEffect(LEDController& ledController,
                                                  int minTrails,
                                                  int maxTrails,
                                                  int trailLength) :
    Effect(ledController),
    minTrails(minTrails),
    maxTrails(maxTrails),
    trailLength(trailLength),
    minVelocity(0.3f),    // Start with a slow upward movement
    maxVelocity(0.7f),    // Variation in starting speed
    minAccel(0.05f),      // Minimum acceleration
    maxAccel(0.15f)       // Maximum acceleration
{
    // Initialize with some trails
    reset();
}

AcceleratingTrailsEffect::~AcceleratingTrailsEffect() {
    // Vector will clean up automatically
}

void AcceleratingTrailsEffect::reset() {
    // Clear all trails
    trails.clear();

    // Create initial trails
    for (int i = 0; i < minTrails; i++) {
        createNewTrail();
    }
}

void AcceleratingTrailsEffect::createNewTrail() {
    // Check if we're already at maximum
    if (trails.size() >= maxTrails) {
        return;
    }

    AccelTrail newTrail;

    // Randomly select inner or outer strip (1 or 2)
    newTrail.stripId = random(1, 3); // 1 = inner, 2 = outer

    // Randomly select which substrip (0, 1, or 2)
    newTrail.subStrip = random(NUM_INNER_STRIPS); // Same for both inner and outer

    // Get strip length based on type
    int stripLength = (newTrail.stripId == 1) ?
                      INNER_LEDS_PER_STRIP :
                      OUTER_LEDS_PER_STRIP;

    // Start at the bottom of the strip
    newTrail.position = 0;

    // Set trail properties
    newTrail.length = trailLength;
    newTrail.hue = random(65536); // Random color (full spectrum)
    newTrail.active = true;

    // Random starting velocity and acceleration
    newTrail.velocity = random(minVelocity * 100, maxVelocity * 100) / 100.0f;
    newTrail.acceleration = random(minAccel * 100, maxAccel * 100) / 100.0f;

    // Add to trails vector
    trails.push_back(newTrail);
}

void AcceleratingTrailsEffect::ensureMinimumTrails() {
    // Create new trails if we're below minimum
    int activeCount = 0;
    for (const auto& trail : trails) {
        if (trail.active) activeCount++;
    }

    // Create new trails if needed
    int trailsToCreate = minTrails - activeCount;
    for (int i = 0; i < trailsToCreate; i++) {
        createNewTrail();
    }
}

void AcceleratingTrailsEffect::update() {
    // Clear all LEDs
    leds.clearAll();

    // Randomly create new trails (~15% chance each update)
    if (random(100) < 15 && trails.size() < maxTrails) {
        createNewTrail();
    }

    // Update each trail
    for (auto& trail : trails) {
        if (!trail.active) continue;

        // Get strip length based on type
        int stripLength = (trail.stripId == 1) ?
                          INNER_LEDS_PER_STRIP :
                          OUTER_LEDS_PER_STRIP;

        // Update position with acceleration
        trail.velocity += trail.acceleration;
        trail.position += trail.velocity;

        // Deactivate if the entire trail is off the top
        if (trail.position - trail.length >= stripLength) {
            trail.active = false;
            continue;
        }

        // Draw the trail with colored head and white fading tail
        for (int i = 0; i < trail.length; i++) {
            // Calculate position of this pixel in the trail
            int pixelPos = trail.position - i;

            // Skip if pixel is off the bottom or top
            if (pixelPos < 0 || pixelPos >= stripLength) continue;

            // Calculate brightness based on position in trail (fade toward end)
            float brightness = 1.0f - (float)i / trail.length;
            int val = 255 * brightness;

            // Physical position calculation
            int physicalPos = leds.mapPositionToPhysical(trail.stripId, pixelPos, trail.subStrip);

            // For inner/outer strips with multiple sections, adjust the physical position
            if (trail.stripId == 1) { // Inner
                physicalPos += trail.subStrip * INNER_LEDS_PER_STRIP;
            } else { // Outer
                physicalPos += trail.subStrip * OUTER_LEDS_PER_STRIP;
            }

            // First LED is colored, rest fade from white to black
            CRGB color;
            if (i == 0) {
                // Lead pixel in full color - convert from HSV
                CHSV hsv(trail.hue >> 8, 255, val); // FastLED uses 0-255 for hue
                hsv2rgb_rainbow(hsv, color);
            } else {
                // Rest of trail is white fading to black (reduced saturation)
                // Gradually change saturation from 0 (white) to very low
                // as we go from head to tail
                uint8_t sat = min(255 * i / trail.length, 50); // Max 50 to keep mostly white

                // Convert from HSV
                CHSV hsv(trail.hue >> 8, sat, val);
                hsv2rgb_rainbow(hsv, color);
            }

            // Set the pixel
            if (trail.stripId == 1) { // Inner
                leds.getInner()[physicalPos] = color;
            } else { // Outer
                leds.getOuter()[physicalPos] = color;
            }
        }
    }

    // Remove inactive trails
    trails.erase(
        std::remove_if(trails.begin(), trails.end(),
            [](const AccelTrail& t) { return !t.active; }),
        trails.end());

    // Ensure we have minimum number of trails
    ensureMinimumTrails();

    // Show all strips
    leds.showAll();
}