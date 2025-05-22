// src/leds/effects/MatrixEffect.cpp

#include "MatrixEffect.h"

MatrixEffect::MatrixEffect(LEDController &ledController) : Effect(ledController),
                                                           baseHue(0),
                                                           lastUpdate(0),
                                                           lastHueUpdate(0) {
    // Initialize drops for each strip
    coreDrops.resize(MAX_DROPS_PER_STRIP);
    ringDrops.resize(MAX_DROPS_PER_STRIP);

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
    for (auto &drop: coreDrops) {
        drop.isActive = false;
    }

    for (auto &drop: ringDrops) {
        drop.isActive = false;
    }

    for (int i = 0; i < NUM_INNER_STRIPS; i++) {
        for (auto &drop: innerDrops[i]) {
            drop.isActive = false;
        }
    }

    for (int i = 0; i < NUM_OUTER_STRIPS; i++) {
        for (auto &drop: outerDrops[i]) {
            drop.isActive = false;
        }
    }

    // Reset timing
    lastUpdate = millis();
    lastHueUpdate = millis();

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
    if (!shouldUpdate(8)) {
        // 8ms = 125 FPS (close to 120)
        return;
    }

    // Clear all strips before drawing
    leds.clearAll();

    // Update each strip type
    updateStrip(0); // Core

    for (int i = 0; i < NUM_INNER_STRIPS; i++) {
        updateStrip(1, i); // Inner strips
    }

    for (int i = 0; i < NUM_OUTER_STRIPS; i++) {
        updateStrip(2, i); // Outer strips
    }

    if (!skipRing)
        updateStrip(3); // Ring

    // Update hue for color cycling (slower due to higher frame rate)
    static int hueUpdateCounter = 0;
    hueUpdateCounter++;
    if (hueUpdateCounter >= 10) {
        // Update hue every 10 frames instead of every frame
        baseHue += HUE_ROTATION_SPEED;
        updateColorPalette();
        hueUpdateCounter = 0;
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
            drops = &coreDrops;
            stripLength = LED_STRIP_CORE_COUNT;
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
            drop.hue = random(NUM_COLORS); // Random color from palette
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
            drops = &coreDrops;
            stripLength = LED_STRIP_CORE_COUNT;
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

        // For inner/outer strips, adjust for segment offset
        if (stripType == 1) {
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
            // Colored drop - get from palette
            color = colorPalette[drop.hue];

            // Apply brightness adjustment
            color.nscale8_video(drop.brightness);
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

    // Draw the trailing fade
    for (uint8_t i = 1; i <= TRAIL_LENGTH; i++) {
        int trailPos = headPos + i;
        if (trailPos >= 0 && trailPos < stripLength) {
            // Map to physical position
            int physicalPos = leds.mapPositionToPhysical(stripType, trailPos, subStrip);

            // Adjust for segment offset for inner/outer strips
            if (stripType == 1) {
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
