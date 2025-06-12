// src/leds/effects/WaterfallEffect.cpp

#include "WaterfallEffect.h"

WaterfallEffect::WaterfallEffect(LEDController& ledController) : Effect(ledController) {
    // Reserve space for all our water drops to avoid memory reallocations
    waterDrops.reserve(MAX_DROPS);

    // Initialize all drops as inactive
    for (int i = 0; i < MAX_DROPS; i++) {
        WaterDrop drop;
        drop.isActive = false;
        drop.hasSplashed = false;
        drop.splashFrame = 0;
        waterDrops.push_back(drop);
    }

    Serial.println("WaterfallEffect initialized with " + String(MAX_DROPS) + " drop slots");
}

WaterfallEffect::~WaterfallEffect() {
    // Vector automatically cleans up when destroyed
    // No manual cleanup needed
}

void WaterfallEffect::reset() {
    // Mark all drops as inactive to clear the effect
    for (auto& drop : waterDrops) {
        drop.isActive = false;
        drop.hasSplashed = false;
        drop.splashFrame = 0;
    }

    Serial.println("WaterfallEffect reset - all drops cleared");
}

void WaterfallEffect::update() {
    // Target 120 FPS for ultra-smooth waterfall animation
    if (!shouldUpdate(8)) {  // 8ms = 125 FPS (close to 120)
        return;
    }

    // Clear all LED strips before drawing new frame
    leds.clearAll();

    // Randomly create new water drops
    if (random(100) < DROP_CREATE_CHANCE) {
        createNewDrop();
    }

    // Update and draw all active drops
    for (auto& drop : waterDrops) {
        if (drop.isActive) {
            // Update the drop's physics (position, speed, etc.)
            updateDrop(drop);

            // Draw the drop (or its splash) on the LEDs
            if (drop.hasSplashed) {
                drawSplash(drop);
            } else {
                drawDrop(drop);
            }
        }
    }

    // Show all the changes on the LED strips
    leds.showAll();
}

void WaterfallEffect::createNewDrop() {
    // Find an inactive drop slot to use
    for (auto& drop : waterDrops) {
        if (!drop.isActive) {
            // Initialize this drop with random properties

            // Choose which strip type (inner or outer only)
            drop.stripType = random(1, 3);  // 1 or 2 (inner or outer)

            // Choose which segment of that strip type (0, 1, or 2)
            drop.subStrip = random(3);

            // Start at the very top of the strip
            drop.position = getStripLength(drop.stripType) - 1;

            // Random starting speed (drops don't all fall at same rate)
            drop.speed = MIN_START_SPEED +
                        (random(100) / 100.0f) * (MAX_START_SPEED - MIN_START_SPEED);

            // All drops experience the same gravity
            drop.acceleration = GRAVITY;

            // Water drops are blue-ish with some variation
            drop.hue = 140 + random(40);  // Blue range (140-180 in FastLED hue)

            // Random brightness for variety (but not too dim)
            drop.brightness = 150 + random(105);  // 150-255 range

            // Mark as active and not splashed yet
            drop.isActive = true;
            drop.hasSplashed = false;
            drop.splashFrame = 0;

            // Only create one drop per function call
            return;
        }
    }

    // If we get here, all drop slots are full (which is fine)
}

void WaterfallEffect::updateDrop(WaterDrop& drop) {
    // If drop is splashing, just update splash animation
    if (drop.hasSplashed) {
        drop.splashFrame++;

        // Remove drop when splash animation is complete
        if (drop.splashFrame >= SPLASH_FRAMES) {
            drop.isActive = false;
        }
        return;
    }

    // Update drop physics (gravity makes it go faster)
    drop.speed += drop.acceleration;

    // Cap the maximum speed (terminal velocity)
    if (drop.speed > MAX_SPEED) {
        drop.speed = MAX_SPEED;
    }

    // Move the drop down by its current speed
    drop.position -= drop.speed;

    // Check if drop has hit the bottom of the strip
    if (drop.position <= 0) {
        // Drop has hit bottom - start splash effect
        drop.hasSplashed = true;
        drop.splashFrame = 0;
        drop.position = 0;  // Keep at bottom for splash
    }
}

void WaterfallEffect::drawDrop(const WaterDrop& drop) {
    // Get the strip length to ensure we're drawing within bounds
    int stripLength = getStripLength(drop.stripType);

    // Calculate which LED this drop should light up
    int ledPosition = (int)drop.position;

    // Make sure position is valid
    if (ledPosition < 0 || ledPosition >= stripLength) {
        return;
    }

    // Map the logical position to physical LED position
    int physicalPos = leds.mapPositionToPhysical(drop.stripType, ledPosition, drop.subStrip);

    // Adjust for the segment offset (each segment has multiple strips)
    if (drop.stripType == 1) {  // Inner strips
        physicalPos += drop.subStrip * INNER_LEDS_PER_STRIP;
    } else if (drop.stripType == 2) {  // Outer strips
        physicalPos += drop.subStrip * OUTER_LEDS_PER_STRIP;
    }

    // Get the water color for this drop
    CRGB waterColor = getWaterColor(drop.hue, drop.brightness);

    // Set the LED color based on strip type
    if (drop.stripType == 1) {  // Inner
        if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
            leds.getInner()[physicalPos] = waterColor;
        }
    } else if (drop.stripType == 2) {  // Outer
        if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
            leds.getOuter()[physicalPos] = waterColor;
        }
    }
}

void WaterfallEffect::drawSplash(const WaterDrop& drop) {
    // Create a splash effect that expands from the impact point

    // Calculate splash size based on animation frame
    int splashSize = drop.splashFrame + 1;  // Grows each frame

    // Calculate splash brightness (fades as it grows)
    float fadeRatio = 1.0f - ((float)drop.splashFrame / SPLASH_FRAMES);
    uint8_t splashBrightness = drop.brightness * fadeRatio;

    // Get base water color
    CRGB splashColor = getWaterColor(drop.hue, splashBrightness);

    // Draw splash pixels extending from impact point
    for (int i = 0; i < splashSize; i++) {
        int ledPosition = i;  // Start from bottom (position 0) and go up

        // Make sure we don't go beyond strip length
        if (ledPosition >= getStripLength(drop.stripType)) {
            break;
        }

        // Calculate brightness for this splash pixel (dimmer at edges)
        float pixelFade = 1.0f - ((float)i / splashSize);
        CRGB pixelColor = splashColor;
        pixelColor.nscale8_video(255 * pixelFade);

        // Map to physical position
        int physicalPos = leds.mapPositionToPhysical(drop.stripType, ledPosition, drop.subStrip);

        // Adjust for segment offset
        if (drop.stripType == 1) {  // Inner strips
            physicalPos += drop.subStrip * INNER_LEDS_PER_STRIP;
        } else if (drop.stripType == 2) {  // Outer strips
            physicalPos += drop.subStrip * OUTER_LEDS_PER_STRIP;
        }

        // Set the LED color based on strip type
        if (drop.stripType == 1) {  // Inner
            if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                // Add to existing color (in case multiple splashes overlap)
                leds.getInner()[physicalPos] += pixelColor;
            }
        } else if (drop.stripType == 2) {  // Outer
            if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                // Add to existing color (in case multiple splashes overlap)
                leds.getOuter()[physicalPos] += pixelColor;
            }
        }
    }
}

CRGB WaterfallEffect::getWaterColor(uint8_t hue, uint8_t brightness) {
    // Create water-like colors (blues and blue-whites)

    // Use the provided hue but adjust saturation for water effect
    uint8_t saturation = 200;  // High saturation for vivid blue water

    // For brighter drops, reduce saturation to make them more white (like foam)
    if (brightness > 200) {
        saturation = 150;  // Less saturated = more white
    }

    // Create the color using FastLED's HSV system
    return CHSV(hue, saturation, brightness);
}

int WaterfallEffect::getStripLength(int stripType) {
    // Return the number of LEDs in each strip type
    switch (stripType) {
        case 1:  // Inner strips
            return INNER_LEDS_PER_STRIP;
        case 2:  // Outer strips
            return OUTER_LEDS_PER_STRIP;
        default:
            return 0;  // Invalid strip type
    }
}