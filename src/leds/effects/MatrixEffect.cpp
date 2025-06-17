// src/leds/effects/MatrixEffect.cpp

#include "MatrixEffect.h"

MatrixEffect::MatrixEffect(LEDController &ledController) : Effect(ledController),
                                                           hueCounter(0),
                                                           baseHue(0),
                                                           lastUpdate(0),
                                                           lastHueUpdate(0),
                                                           lastRingTrailCreateTime(0) {
    // Initialize drops for each strip
    // Core strips - 3 segments
    for (int segment = 0; segment < 3; segment++) {
        coreDrops[segment].resize(MAX_DROPS_PER_STRIP);
    }

    ringDrops.resize(MAX_DROPS_PER_STRIP);

    // Initialize ring trails vector
    ringTrails.reserve(MAX_RING_TRAILS);

    for (int i = 0; i < NUM_INNER_STRIPS; i++) {
        innerDrops[i].resize(MAX_DROPS_PER_STRIP);
    }

    for (int i = 0; i < NUM_OUTER_STRIPS; i++) {
        outerDrops[i].resize(MAX_DROPS_PER_STRIP);
    }

    // Initialize color palette
    updateColorPalette();
}

MatrixEffect::~MatrixEffect() {
    // No dynamic memory to clean up
}

void MatrixEffect::reset() {
    // Reset all drops to inactive
    // Update core drops reset to handle 3 segments
    for (int segment = 0; segment < 3; segment++) {
        for (auto& drop : coreDrops[segment]) {
            drop.isActive = false;
        }
    }

    for (auto& drop : ringDrops) {
        drop.isActive = false;
    }

    // Reset ring trails
    ringTrails.clear();
    lastRingTrailCreateTime = 0;

    for (int i = 0; i < NUM_INNER_STRIPS; i++) {
        for (auto& drop : innerDrops[i]) {
            drop.isActive = false;
        }
    }

    for (int i = 0; i < NUM_OUTER_STRIPS; i++) {
        for (auto& drop : outerDrops[i]) {
            drop.isActive = false;
        }
    }

    // Reset timing and hue counter
    lastUpdate = millis();
    lastHueUpdate = millis();
    hueCounter = 0;
    baseHue = 0;

    // Clear all LEDs
    leds.clearAll();
    leds.showAll();
}

void MatrixEffect::updateColorPalette() {
    // Update color palette with colors based on current base hue
    // Using green-matrix theme with varying shades
    for (int i = 0; i < NUM_COLORS; i++) {
        // Create palette mostly in green range but with some variation
        uint16_t hue = baseHue + i * (65536 / NUM_COLORS);

        // Tint toward green (hue around 21845 or 85 in 0-255 scale)
        hue = (hue & 0x3FFF) + 21845; // Keep in green range (1/3 of the way around the color wheel)

        // Convert to FastLED's CRGB using HSV
        CHSV hsv(hue >> 8, 200, 255); // FastLED uses 0-255 for hue
        CRGB rgb;
        hsv2rgb_rainbow(hsv, rgb);

        // Store the color in palette
        colorPalette[i] = rgb;
    }
}

void MatrixEffect::update() {
    // Target 120 FPS for ultra-smooth matrix drops
    if (!shouldUpdate(8)) {  // 8ms = 125 FPS (close to 120)
        return;
    }

    // Clear all strips before drawing
    leds.clearAll();

    // Update each strip type
    // Core - now process each segment separately
    for (int segment = 0; segment < 3; segment++) {
        updateStrip(0, segment);
    }

    for (int i = 0; i < NUM_INNER_STRIPS; i++) {
        updateStrip(1, i); // Inner strips
    }

    for (int i = 0; i < NUM_OUTER_STRIPS; i++) {
        updateStrip(2, i); // Outer strips
    }

    if (!skipRing)
        updateRingTrails(); // Use new continuous trail system

    // Update hue counter for precise 0.025 rotation speed (4x slower than 0.1)
    hueCounter += HUE_ROTATION_SPEED;  // Add 1 each frame

    // Convert counter to hue (divide by 40 to get 0.025 effective speed)
    uint8_t currentBaseHue = (hueCounter / 40) % 255;

    // Update color palette periodically
    static int paletteUpdateCounter = 0;
    static uint8_t lastBaseHue = 255; // Force first update
    paletteUpdateCounter++;

    if (paletteUpdateCounter >= 5 || currentBaseHue != lastBaseHue) {
        // Update the global baseHue for palette generation
        baseHue = currentBaseHue;
        updateColorPalette();
        paletteUpdateCounter = 0;
        lastBaseHue = currentBaseHue;
    }

    // Show all updates
    leds.showAll();
}

void MatrixEffect::createDrop(int stripType, int subStrip) {
    std::vector<Drop> *drops;
    int stripLength;

    // Get the appropriate drops array based on strip type
    switch (stripType) {
        case 0: // Core
            drops = &coreDrops[subStrip];  // Use segment-specific drops
            stripLength = LED_STRIP_CORE_COUNT / 3;  // Each segment is 1/3 of total
            break;
        case 1: // Inner
            drops = &innerDrops[subStrip];
            stripLength = INNER_LEDS_PER_STRIP;
            break;
        case 2: // Outer
            drops = &outerDrops[subStrip];
            stripLength = OUTER_LEDS_PER_STRIP;
            break;
        case 3: // Ring
            drops = &ringDrops;
            stripLength = LED_STRIP_RING_COUNT;
            break;
        default:
            return; // Invalid strip type
    }

    // Find an inactive drop slot
    for (auto &drop: *drops) {
        if (!drop.isActive) {
            // Initialize a new drop
            drop.position = stripLength - 1; // Start at the top
            drop.speed = MIN_SPEED + ((float) random(100) / 100.0f) * (MAX_SPEED - MIN_SPEED);

            // Assign hue within 20% of color wheel around current rotating base hue
            // 20% of 255 = 51, so random range of ±25 around base hue
            int hueVariation = random(51) - 25; // Random from -25 to +25
            drop.hue = (baseHue + hueVariation) & 0xFF; // Keep within 0-255 range with wraparound

            drop.brightness = 255;
            drop.isActive = true;
            drop.isWhite = (random(100) < WHITE_FLASH_CHANCE); // Small chance to start as white
            return;
        }
    }
}

void MatrixEffect::updateStrip(int stripType, int subStrip) {
    std::vector<Drop> *drops;
    int stripLength;

    // Get the appropriate drops array and strip length
    switch (stripType) {
        case 0: // Core
            drops = &coreDrops[subStrip];  // Use segment-specific drops
            stripLength = LED_STRIP_CORE_COUNT / 3;  // Each segment is 1/3 of total
            break;
        case 1: // Inner
            drops = &innerDrops[subStrip];
            stripLength = INNER_LEDS_PER_STRIP;
            break;
        case 2: // Outer
            drops = &outerDrops[subStrip];
            stripLength = OUTER_LEDS_PER_STRIP;
            break;
        case 3: // Ring
            drops = &ringDrops;
            stripLength = LED_STRIP_RING_COUNT;
            break;
        default:
            return; // Invalid strip type
    }

    // Random chance to create a new drop
    if (random(20) == 0) {
        createDrop(stripType, subStrip);
    }

    // Update and render all active drops
    for (auto &drop: *drops) {
        if (drop.isActive) {
            // Update position
            drop.position -= drop.speed;

            // Random chance to flicker
            if (random(100) < FLICKER_CHANCE) {
                drop.brightness = 255 - random(FLICKER_INTENSITY);
            } else {
                // Gradually restore brightness
                if (drop.brightness < 255) {
                    drop.brightness = min(255, drop.brightness + 20);
                }
            }

            // Random chance to flash white
            if (!drop.isWhite && random(100) < WHITE_FLASH_CHANCE) {
                drop.isWhite = true;
            } else if (drop.isWhite && random(100) < 50) {
                drop.isWhite = false;
            }

            // Deactivate if offscreen
            if (drop.position < -TRAIL_LENGTH) {
                drop.isActive = false;
                continue;
            }

            // Render this drop and its trail
            renderDrop(drop, stripType, subStrip, stripLength);
        }
    }
}

void MatrixEffect::renderDrop(Drop &drop, int stripType, int subStrip, int stripLength) {
    // Draw the head of the drop
    int headPos = (int) drop.position;
    if (headPos >= 0 && headPos < stripLength) {
        // Map the logical position to physical LED
        int physicalPos = leds.mapPositionToPhysical(stripType, headPos, subStrip);

        // For core strips, adjust for segment offset
        if (stripType == 0) {
            // Core
            int segmentLength = LED_STRIP_CORE_COUNT / 3;
            physicalPos += subStrip * segmentLength;
        } else if (stripType == 1) {
            // Inner
            physicalPos += subStrip * INNER_LEDS_PER_STRIP;
        } else if (stripType == 2) {
            // Outer
            physicalPos += subStrip * OUTER_LEDS_PER_STRIP;
        }

        // Set the color
        CRGB color;
        if (drop.isWhite) {
            // White flash
            uint8_t whiteBright = max(drop.brightness, WHITE_FLASH_MIN);
            color = CRGB(whiteBright, whiteBright, whiteBright);
        } else {
            // Colored drop - use HSV with the drop's assigned hue
            CHSV hsvColor(drop.hue, 255, drop.brightness); // Full saturation
            hsv2rgb_rainbow(hsvColor, color);
        }

        // Set pixel based on strip type
        switch (stripType) {
            case 0: // Core
                leds.getCore()[physicalPos] = color;
                break;
            case 1: // Inner
                leds.getInner()[physicalPos] = color;
                break;
            case 2: // Outer
                leds.getOuter()[physicalPos] = color;
                break;
            case 3: // Ring
                leds.getRing()[physicalPos] = color;
                break;
        }
    }

    // Draw the trailing fade with white colors
    for (uint8_t i = 1; i <= TRAIL_LENGTH; i++) {
        int trailPos = headPos + i;

        if (trailPos >= 0 && trailPos < stripLength) {
            // Map to physical position
            int physicalPos = leds.mapPositionToPhysical(stripType, trailPos, subStrip);

            // Adjust for segment offset
            if (stripType == 0) {
                // Core
                int segmentLength = LED_STRIP_CORE_COUNT / 3;
                physicalPos += subStrip * segmentLength;
            } else if (stripType == 1) {
                // Inner
                physicalPos += subStrip * INNER_LEDS_PER_STRIP;
            } else if (stripType == 2) {
                // Outer
                physicalPos += subStrip * OUTER_LEDS_PER_STRIP;
            }

            // Calculate trail brightness (decreasing quadratically)
            uint8_t trailBright = ((TRAIL_LENGTH - i) * (TRAIL_LENGTH - i) * TRAIL_BRIGHTNESS) /
                                  (TRAIL_LENGTH * TRAIL_LENGTH);

            // Set trail pixel (white with decreasing brightness)
            CRGB trailColor(trailBright, trailBright, trailBright);

            // Set pixel based on strip type
            switch (stripType) {
                case 0: // Core
                    leds.getCore()[physicalPos] = trailColor;
                    break;
                case 1: // Inner
                    leds.getInner()[physicalPos] = trailColor;
                    break;
                case 2: // Outer
                    leds.getOuter()[physicalPos] = trailColor;
                    break;
                case 3: // Ring
                    leds.getRing()[physicalPos] = trailColor;
                    break;
            }
        }
    }
}

void MatrixEffect::updateRingTrails() {
    unsigned long currentTime = millis();

    // Create new ring trails periodically
    if (currentTime - lastRingTrailCreateTime >= 800) { // Create new trail every 800ms
        createNewRingTrail();
        lastRingTrailCreateTime = currentTime;
    }

    // Update existing ring trails
    for (auto& trail : ringTrails) {
        if (!trail.active) continue;

        // Move the trail around the ring
        trail.position += trail.speed;

        // Wrap around when we reach the end
        if (trail.position >= LED_STRIP_RING_COUNT) {
            trail.position -= LED_STRIP_RING_COUNT;
        }

        // Deactivate trail after it has completely faded out
        unsigned long trailAge = currentTime - trail.creationTime;
        if (trailAge >= RING_TRAIL_FADEIN + RING_TRAIL_LIFESPAN + RING_TRAIL_FADEOUT) {
            trail.active = false;
        }
    }

    // Remove inactive trails
    ringTrails.erase(
        std::remove_if(ringTrails.begin(), ringTrails.end(),
            [](const MatrixRingTrail& t) { return !t.active; }),
        ringTrails.end());

    // Draw all active ring trails
    drawRingTrails();
}

void MatrixEffect::createNewRingTrail() {
    // Don't create more trails if we're at maximum
    if (ringTrails.size() >= MAX_RING_TRAILS) {
        return;
    }

    MatrixRingTrail newTrail;

    // Random starting position around the ring
    newTrail.position = random(LED_STRIP_RING_COUNT);

    // Speed between 0.1 and 0.3 pixels per frame for smooth movement
    newTrail.speed = 0.1f + (random(100) / 100.0f) * 0.2f;

    // Set trail length
    newTrail.trailLength = RING_TRAIL_LENGTH;

    // Assign hue within 20% of color wheel around current rotating base hue
    int hueVariation = random(51) - 25; // Random from -25 to +25
    newTrail.hue = (baseHue + hueVariation) & 0xFF;

    // Set creation time
    newTrail.creationTime = millis();

    // Activate the trail
    newTrail.active = true;

    // Add to ring trails vector
    ringTrails.push_back(newTrail);
}

void MatrixEffect::drawRingTrails() {
    unsigned long currentTime = millis();

    // Draw all active ring trails
    for (const auto& trail : ringTrails) {
        if (!trail.active) continue;

        // Calculate trail age and fade multiplier
        unsigned long trailAge = currentTime - trail.creationTime;
        float trailFadeMultiplier = 1.0f;

        // Apply gradual fade-in during first RING_TRAIL_FADEIN period
        if (trailAge < RING_TRAIL_FADEIN) {
            trailFadeMultiplier = (float)trailAge / RING_TRAIL_FADEIN;
        }
        // Apply gradual fade-out after RING_TRAIL_FADEIN + RING_TRAIL_LIFESPAN
        else if (trailAge > RING_TRAIL_FADEIN + RING_TRAIL_LIFESPAN) {
            unsigned long fadeAge = trailAge - RING_TRAIL_FADEIN - RING_TRAIL_LIFESPAN;
            trailFadeMultiplier = 1.0f - ((float)fadeAge / RING_TRAIL_FADEOUT);
            trailFadeMultiplier = max(0.0f, trailFadeMultiplier); // Ensure non-negative
        }
        // Full brightness during middle period
        // (trailFadeMultiplier remains 1.0f)

        // Draw the trail with fading effect
        for (int i = 0; i < trail.trailLength; i++) {
            // Calculate position for this segment of the trail
            float segmentPos = trail.position - i;

            // Handle wraparound
            while (segmentPos < 0) segmentPos += LED_STRIP_RING_COUNT;
            while (segmentPos >= LED_STRIP_RING_COUNT) segmentPos -= LED_STRIP_RING_COUNT;

            int ledIndex = (int)segmentPos;

            // Calculate brightness fade (head is brightest, tail fades out)
            float fadeRatio = 1.0f - ((float)i / trail.trailLength);
            fadeRatio = fadeRatio * fadeRatio; // Square for exponential fade

            // Apply overall trail fade multiplier
            fadeRatio *= trailFadeMultiplier;

            CRGB color;
            if (i == 0) {
                // Head of trail - use the colored hue
                CHSV hsvColor(trail.hue, 255, 255 * fadeRatio);
                hsv2rgb_rainbow(hsvColor, color);
            } else {
                // Trail segments - white with decreasing brightness
                uint8_t whiteBright = 255 * fadeRatio * TRAIL_BRIGHTNESS / 255; // Scale by TRAIL_BRIGHTNESS
                color = CRGB(whiteBright, whiteBright, whiteBright);
            }

            // Blend with existing color (for overlapping trails)
            leds.getRing()[ledIndex] += color;
        }
    }
}