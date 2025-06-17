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
        drop.trailLength = 0;  // Reset trail length too
        drop.currentFrame = 0; // Reset fade-in frame counter
        drop.fadeInFrames = 0; // Reset fade-in duration
        drop.maxBrightness = 0; // Reset max brightness
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

    // Set background water color on all LEDs first (constant waterfall base)
    fillBackgroundWater();

    // Randomly create new water drops
    if (random(100) < DROP_CREATE_CHANCE) {
        createNewDrop();
    }

    // Update and draw all active drops (these are the bright moving drops)
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

void WaterfallEffect::fillBackgroundWater() {
    // Create a subtle blue-white background water color (more muted than before)
    // This makes all LEDs look like flowing water at low brightness
    CRGB backgroundWater = getWaterColor(160, 30);  // Even dimmer and more subtle

    // Fill all inner strips with background water
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        leds.getInner()[i] = backgroundWater;
    }

    // Fill all outer strips with background water
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        leds.getOuter()[i] = backgroundWater;
    }

    // Core and ring strips stay off to focus on the waterfall
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

            // FLIPPED: Start the drop BELOW the strip so the trail can enter gradually from bottom
            // Position it so only the head starts at the bottom edge (position 0)
            drop.position = 0 - drop.trailLength;

            // Eliminate small trails - start with medium-sized drops minimum
            int dropType = random(100);
            if (dropType < 50) {
                // 50% chance: Medium drops (12-28 pixels) - no more small dots
                drop.trailLength = 12 + random(17);
            } else if (dropType < 80) {
                // 30% chance: Large streams (30-55 pixels)
                drop.trailLength = 30 + random(26);
            } else {
                // 20% chance: Massive waterfalls (60-90 pixels!)
                drop.trailLength = 60 + random(31);
            }

            // Bigger drops (longer trails) fall faster for realism
            float sizeSpeedBonus = (drop.trailLength - 12) * 0.002f;  // Adjusted for new minimum size
            drop.speed = MIN_START_SPEED + sizeSpeedBonus +
                        (random(100) / 100.0f) * (MAX_START_SPEED - MIN_START_SPEED);

            // All drops experience the same gravity (reduced)
            drop.acceleration = GRAVITY;

            // Water drops are blue-ish with some variation but more subtle
            drop.hue = 140 + random(40);  // Blue range (140-180 in FastLED hue)

            // Set maximum brightness for realistic water (not too bright/vibrant)
            drop.maxBrightness = 160 + random(70);  // 160-230 range (more subtle than before)

            // Start with zero brightness - will fade in gradually
            drop.brightness = 0;

            // Random fade-in duration (longer fade for larger drops, but cap it reasonably)
            drop.fadeInFrames = 8 + min(8, drop.trailLength / 3);  // 8-16 frames, capped to prevent very long fades
            drop.currentFrame = 0;  // Start at frame 0

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

    // Update fade-in effect for new drops
    if (drop.currentFrame < drop.fadeInFrames) {
        drop.currentFrame++;

        // Calculate fade-in progress (0.0 to 1.0)
        float fadeProgress = (float)drop.currentFrame / drop.fadeInFrames;

        // Apply smooth easing curve to fade-in (starts slow, speeds up, then slows down)
        fadeProgress = fadeProgress * fadeProgress * (3.0f - 2.0f * fadeProgress);  // Smoothstep function

        // Set current brightness based on fade-in progress
        drop.brightness = drop.maxBrightness * fadeProgress;
    } else {
        // Drop is fully faded in, use maximum brightness
        drop.brightness = drop.maxBrightness;
    }

    // Update drop physics (gravity makes it go faster)
    drop.speed += drop.acceleration;

    // Cap the maximum speed (terminal velocity)
    if (drop.speed > MAX_SPEED) {
        drop.speed = MAX_SPEED;
    }

    // FLIPPED: Move the drop UP by its current speed (instead of down)
    drop.position += drop.speed;

    // FLIPPED: Check if drop has hit the TOP of the strip
    // But don't remove it immediately - let the trail finish extending past the top
    int stripLength = getStripLength(drop.stripType);
    if (drop.position >= stripLength + drop.trailLength) {
        // Drop and its entire trail have moved past the top - start splash effect
        drop.hasSplashed = true;
        drop.splashFrame = 0;
        drop.position = stripLength - 1;  // Reset to top for splash
    }
}

void WaterfallEffect::drawDrop(const WaterDrop& drop) {
    // Get the strip length to ensure we're drawing within bounds
    int stripLength = getStripLength(drop.stripType);

    // Draw the drop with its fading trail
    for (int i = 0; i < drop.trailLength; i++) {
        // FLIPPED: Calculate position for this part of the trail
        // For upward moving drops: i=0 should be the leading edge (top/front)
        // i=trailLength-1 should be the tail (bottom/back)
        float trailPosition = drop.position - i;  // Trail extends downward from the leading edge
        int ledPosition = (int)trailPosition;

        // Make sure position is valid
        if (ledPosition < 0 || ledPosition >= stripLength) {
            continue; // Skip this trail pixel if it's out of bounds
        }

        // Map the logical position to physical LED position
        int physicalPos = leds.mapPositionToPhysical(drop.stripType, ledPosition, drop.subStrip);

        // Adjust for the segment offset (each segment has multiple strips)
        if (drop.stripType == 1) {  // Inner strips
            physicalPos += drop.subStrip * INNER_LEDS_PER_STRIP;
        } else if (drop.stripType == 2) {  // Outer strips
            physicalPos += drop.subStrip * OUTER_LEDS_PER_STRIP;
        }

        // Calculate brightness fade for this trail position
        // Head of drop is brightest (i=0), tail fades out (i=trailLength-1)
        float fadeRatio = 1.0f - ((float)i / drop.trailLength);

        // Apply a curve to the fade for more natural look
        fadeRatio = fadeRatio * fadeRatio;  // Square the ratio for exponential fade

        // Calculate final brightness for this trail pixel
        uint8_t trailBrightness = drop.brightness * fadeRatio;

        // Get the water color for this trail position
        CRGB waterColor = getWaterColor(drop.hue, trailBrightness);

        // Set the LED color based on strip type (add to background, don't replace)
        if (drop.stripType == 1) {  // Inner
            if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                // Add the trail color to the existing background water
                leds.getInner()[physicalPos] += waterColor;
            }
        } else if (drop.stripType == 2) {  // Outer
            if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                // Add the trail color to the existing background water
                leds.getOuter()[physicalPos] += waterColor;
            }
        }
    }
}
void WaterfallEffect::drawSplash(const WaterDrop& drop) {
    // FLIPPED: Create a subtle splash effect that spreads outward at the TOP
    // instead of at the bottom

    // Calculate splash intensity based on animation frame (fades quickly)
    float fadeRatio = 1.0f - ((float)drop.splashFrame / SPLASH_FRAMES);
    uint8_t splashBrightness = drop.brightness * fadeRatio * 0.6f;  // Make splash dimmer

    // FLIPPED: Only draw splash at the very TOP of the strip (instead of bottom)
    // Get the water color for splash
    CRGB splashColor = getWaterColor(drop.hue, splashBrightness);

    // Map the logical position to physical LED position for the TOP
    int stripLength = getStripLength(drop.stripType);
    int physicalPos = leds.mapPositionToPhysical(drop.stripType, stripLength - 1, drop.subStrip);

    // Adjust for the segment offset
    if (drop.stripType == 1) {  // Inner strips
        physicalPos += drop.subStrip * INNER_LEDS_PER_STRIP;
    } else if (drop.stripType == 2) {  // Outer strips
        physicalPos += drop.subStrip * OUTER_LEDS_PER_STRIP;
    }

    // Add splash to the LED at the top
    if (drop.stripType == 1) {  // Inner
        if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
            leds.getInner()[physicalPos] += splashColor;
        }
    } else if (drop.stripType == 2) {  // Outer
        if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
            leds.getOuter()[physicalPos] += splashColor;
        }
    }
}

CRGB WaterfallEffect::getWaterColor(uint8_t hue, uint8_t brightness) {
    // Create more subtle water-like colors (blues and blue-whites)

    // Use reduced saturation for more subtle, realistic water colors
    uint8_t saturation = 120;  // Much lower saturation for subtle colors

    // For brighter drops, reduce saturation even more to make them more white (like foam)
    if (brightness > 180) {
        saturation = 80;  // Very low saturation = more white/subtle
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