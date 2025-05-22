// src/leds/effects/PartyRippleEffect.cpp

#include "PartyRippleEffect.h"

PartyRippleEffect::PartyRippleEffect(LEDController& ledController) :
    Effect(ledController),
    lastRippleTime(0),
    rippleInterval(2000)  // 2 seconds between new ripples
{
    // Initialize with some trail and ripple elements
    reset();
}

PartyRippleEffect::~PartyRippleEffect() {
    // Vector cleanup is automatic
}

void PartyRippleEffect::reset() {
    // Clear all trails and ripples
    outerTrails.clear();
    innerTrails.clear();
    ripples.clear();

    // Create initial elements
    for (int i = 0; i < 5; i++) {
        createOuterTrail();
        createInnerTrail();
    }

    // Create initial ripple
    createRipple();

    // Reset timing
    lastRippleTime = millis();
}

void PartyRippleEffect::update() {
    // Target 120 FPS for ultra-smooth party effects
    if (!shouldUpdate(8)) {  // 8ms = 125 FPS (close to 120)
        return;
    }

    // Clear all LEDs
    leds.clearAll();

    // Update trails and ripples
    updateOuterTrails();
    updateInnerTrails();
    updateRipples();

    // Check if it's time to create a new ripple (same 2 second interval)
    unsigned long currentTime = millis();
    if (currentTime - lastRippleTime >= rippleInterval) {
        createRipple();
        lastRippleTime = currentTime;
    }

    // Random chance to create new trails (reduced for higher frame rate)
    if (random(120) == 0) {  // ~0.8% chance per frame instead of 10%
        if (outerTrails.size() < MAX_OUTER_TRAILS) {
            createOuterTrail();
        }

        if (innerTrails.size() < MAX_INNER_TRAILS) {
            createInnerTrail();
        }
    }

    // Show all changes
    leds.showAll();
}

void PartyRippleEffect::createOuterTrail() {
    PartyTrail trail;

    // Random substrip (0-2)
    trail.subStrip = random(NUM_OUTER_STRIPS);

    // Random position
    trail.position = random(OUTER_LEDS_PER_STRIP);

    // Move in positive direction (up)
    trail.direction = true;

    // Full trail length
    trail.length = TRAIL_LENGTH;

    // Green-blue hue range (approx 85-170 in FastLED's scale, which is 0-255)
    // This is equivalent to 21845-43690 in our 16-bit scale
    trail.hue = random(21845, 43690);

    // Activate the trail
    trail.active = true;

    // Add to collection
    outerTrails.push_back(trail);
}

void PartyRippleEffect::createInnerTrail() {
    PartyTrail trail;

    // Random substrip (0-2)
    trail.subStrip = random(NUM_INNER_STRIPS);

    // Random position
    trail.position = random(INNER_LEDS_PER_STRIP);

    // Move in negative direction (down)
    trail.direction = false;

    // Full trail length
    trail.length = TRAIL_LENGTH;

    // Blue-purple hue range (approx 170-255 in FastLED's scale)
    // This is equivalent to 43690-65535 in our 16-bit scale
    trail.hue = random(43690, 65535);

    // Activate the trail
    trail.active = true;

    // Add to collection
    innerTrails.push_back(trail);
}

void PartyRippleEffect::createRipple() {
    // Limit maximum number of ripples
    if (ripples.size() >= MAX_RIPPLES) return;

    Ripple ripple;

    // Start at the center of the core strip
    ripple.position = LED_STRIP_CORE_COUNT / 2;

    // Start with size of 1
    ripple.size = 1;

    // Will grow to max size before splitting
    ripple.maxSize = RIPPLE_MAX_SIZE;

    // Not split yet
    ripple.hasSplit = false;

    // Active
    ripple.active = true;

    // Random color in the full spectrum
    ripple.hue = random(65536);

    // These will be set when the ripple splits
    ripple.leftPosition = 0;
    ripple.rightPosition = 0;

    // Width of ripple after splitting
    ripple.width = RIPPLE_WIDTH;

    // Add to ripples
    ripples.push_back(ripple);
}

void PartyRippleEffect::updateOuterTrails() {
    // Update and draw outer trails
    auto it = outerTrails.begin();
    while (it != outerTrails.end()) {
        // If not active, remove it
        if (!it->active) {
            it = outerTrails.erase(it);
            continue;
        }

        // Draw the trail
        for (int i = 0; i < it->length; i++) {
            // Calculate position based on direction
            float pos = it->position - i;

            // Skip if outside LED strip
            if (pos < 0 || pos >= OUTER_LEDS_PER_STRIP) {
                continue;
            }

            // Calculate brightness based on position in trail (fade toward end)
            float brightness = 1.0f - (float)i / it->length;

            // Map to physical position
            int physicalPos = leds.mapPositionToPhysical(2, (int)pos, it->subStrip);
            physicalPos += it->subStrip * OUTER_LEDS_PER_STRIP;

            // Set color - head pixel is colored, rest are white fading to black
            CRGB color;
            if (i == 0) {
                // Head pixel - green/blue color
                color = getTrailHeadColor(it->hue, false);
            } else {
                // Trail - white fading to black
                color = getTrailColor(it->hue, brightness, false);
            }

            // Set the pixel
            leds.getOuter()[physicalPos] = color;
        }

        // Move the trail
        if (it->direction) {
            it->position += 0.5f;  // Speed control

            // If trail is completely off the strip, deactivate
            if (it->position - it->length >= OUTER_LEDS_PER_STRIP) {
                it->active = false;
            }
        }

        // Next trail
        it++;
    }
}

void PartyRippleEffect::updateInnerTrails() {
    // Update and draw inner trails
    auto it = innerTrails.begin();
    while (it != innerTrails.end()) {
        // If not active, remove it
        if (!it->active) {
            it = innerTrails.erase(it);
            continue;
        }

        // Draw the trail
        for (int i = 0; i < it->length; i++) {
            // Calculate position based on direction
            float pos = it->position + i;

            // Skip if outside LED strip
            if (pos < 0 || pos >= INNER_LEDS_PER_STRIP) {
                continue;
            }

            // Calculate brightness based on position in trail (fade toward end)
            float brightness = 1.0f - (float)i / it->length;

            // Map to physical position
            int physicalPos = leds.mapPositionToPhysical(1, (int)pos, it->subStrip);
            physicalPos += it->subStrip * INNER_LEDS_PER_STRIP;

            // Set color - head pixel is colored, rest are white fading to black
            CRGB color;
            if (i == 0) {
                // Head pixel - blue/purple color
                color = getTrailHeadColor(it->hue, true);
            } else {
                // Trail - white fading to black
                color = getTrailColor(it->hue, brightness, true);
            }

            // Set the pixel
            leds.getInner()[physicalPos] = color;
        }

        // Move the trail
        if (!it->direction) {
            it->position -= 0.5f;  // Speed control

            // If trail is completely off the strip, deactivate
            if (it->position + it->length <= 0) {
                it->active = false;
            }
        }

        // Next trail
        it++;
    }
}

void PartyRippleEffect::updateRipples() {
    // Update and draw ripples
    auto it = ripples.begin();
    while (it != ripples.end()) {
        // If not active, remove it
        if (!it->active) {
            it = ripples.erase(it);
            continue;
        }

        if (!it->hasSplit) {
            // Growing ripple at center
            int center = it->position;
            int size = it->size;

            // Draw the ripple centered at position
            for (int i = -size; i <= size; i++) {
                int pos = center + i;

                // Skip if outside LED strip
                if (pos < 0 || pos >= LED_STRIP_CORE_COUNT) {
                    continue;
                }

                // Calculate brightness based on distance from center
                float distFactor = 1.0f - abs((float)i / size);

                // Set the pixel
                leds.getCore()[pos] = getRippleColor(it->hue, distFactor);
            }

            // Grow the ripple
            it->size++;

            // Check if ready to split
            if (it->size > it->maxSize) {
                it->hasSplit = true;
                it->leftPosition = center - size;
                it->rightPosition = center + size;
            }
        } else {
            // Split ripple moving outward

            // Draw left ripple
            for (int i = 0; i < it->width; i++) {
                int pos = it->leftPosition - i;

                // Skip if outside LED strip
                if (pos < 0 || pos >= LED_STRIP_CORE_COUNT) {
                    continue;
                }

                // Calculate brightness based on position in ripple
                float brightness = 1.0f - (float)i / it->width;

                // Set the pixel
                leds.getCore()[pos] = getRippleColor(it->hue, brightness);
            }

            // Draw right ripple
            for (int i = 0; i < it->width; i++) {
                int pos = it->rightPosition + i;

                // Skip if outside LED strip
                if (pos < 0 || pos >= LED_STRIP_CORE_COUNT) {
                    continue;
                }

                // Calculate brightness based on position in ripple
                float brightness = 1.0f - (float)i / it->width;

                // Set the pixel
                leds.getCore()[pos] = getRippleColor(it->hue, brightness);
            }

            // Move the ripples outward
            it->leftPosition--;
            it->rightPosition++;

            // Deactivate if both sides are off the strip
            if (it->leftPosition + it->width < 0 && it->rightPosition - it->width >= LED_STRIP_CORE_COUNT) {
                it->active = false;
            }
        }

        // Next ripple
        it++;
    }
}

CRGB PartyRippleEffect::getTrailHeadColor(uint16_t hue, bool isInner) {
    // Convert from 16-bit hue to FastLED's 8-bit hue
    uint8_t h = hue >> 8;

    // Use high saturation and value for vivid colors
    return CHSV(h, 255, 255);
}

CRGB PartyRippleEffect::getTrailColor(uint16_t hue, float brightness, bool isInner) {
    // For trail colors, we want white fading to black
    // But with a slight tint of the head color

    // Convert brightness to 0-255 range
    uint8_t bright = 255 * brightness;

    // For most of the trail, use white fading to black
    if (brightness > 0.7) {
        // Pure white for start of trail
        return CRGB(bright, bright, bright);
    } else {
        // Fade to black with slight color tint
        uint8_t h = hue >> 8;
        return CHSV(h, 128 * (1.0 - brightness), bright);
    }
}

CRGB PartyRippleEffect::getRippleColor(uint16_t hue, float brightness) {
    // Convert from 16-bit hue to FastLED's 8-bit hue
    uint8_t h = hue >> 8;

    // Convert brightness to 0-255 range
    uint8_t bright = 255 * brightness;

    // Full saturation with varying brightness
    return CHSV(h, 255, bright);
}