// src/leds/effects/WaterfallEffect.cpp

#include "WaterfallEffect.h"
#include "../../SmartLantern.h"

// Constructor - Initialize the waterfall effect
WaterfallEffect::WaterfallEffect(LEDController& ledController) : Effect(ledController) {
    // Reserve space for all possible water drops to avoid memory allocation during runtime
    waterDrops.reserve(MAX_DROPS);

    // Create empty drop objects (all inactive initially)
    for (int i = 0; i < MAX_DROPS; i++) {
        WaterDrop drop;
        drop.isActive = false;  // Start inactive
        drop.hasSplashed = false;
        waterDrops.push_back(drop);
    }
}

// Destructor - Clean up (vector automatically handles memory)
WaterfallEffect::~WaterfallEffect() {
    // Vector destructor automatically cleans up memory
}

// Reset effect to starting state
void WaterfallEffect::reset() {
    // Mark all drops as inactive (this clears the effect)
    for (auto& drop : waterDrops) {
        drop.isActive = false;
        drop.hasSplashed = false;
    }
}

// Main update function - called every frame
void WaterfallEffect::update() {
    // Smoother frame rate for fluid transitions
    if (!shouldUpdate(33)) {  // 33ms = 30 FPS (smoother than 20 FPS)
        return;
    }

    // Step 1: Fill background with subtle water color
    fillBackgroundWater();

    // Step 2: More frequent drop creation (25% increase in trail amount)
    if (random(100) < 19) {  // Increased from 15 to 19 (25% more: 15 * 1.25 = 18.75, rounded to 19)
        createNewDrop();
    }

    // Step 3: Update and draw all active drops
    for (auto& drop : waterDrops) {
        if (drop.isActive) {
            // Update the drop's position and properties
            updateDrop(drop);

            // Draw the drop (or its splash) on the LEDs
            if (drop.hasSplashed) {
                drawSplash(drop);
            } else {
                drawDrop(drop);
            }
        }
    }

    // Step 4: Show all the changes on the LED strips
    leds.showAll();
}

// Fill all LEDs with subtle background water color
void WaterfallEffect::fillBackgroundWater() {
    // Create a brighter blue-white background water color (30% minimum brightness)
    // This makes all LEDs look like flowing water at higher brightness
    CRGB backgroundWater = getWaterColor(160, 77);  // Increased from 30 to 77 (30% of 255)

    // Fill all inner strips with background water
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        leds.getInner()[i] = backgroundWater;
    }

    // Fill all outer strips with background water
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        leds.getOuter()[i] = backgroundWater;
    }

    // Clear core and ring strips (focus attention on waterfall)
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        leds.getCore()[i] = CRGB::Black;
    }

    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        leds.getRing()[i] = CRGB::Black;
    }
}

// Create a new water drop with random properties
void WaterfallEffect::createNewDrop() {
    // Find an inactive drop slot to reuse
    for (auto& drop : waterDrops) {
        if (!drop.isActive) {
            // Initialize this drop with random properties

            // Choose which strip type (inner or outer only)
            drop.stripType = random(1, 3);  // 1 or 2 (inner or outer)

            // Choose which segment of that strip type (0, 1, or 2)
            drop.subStrip = random(3);

            // Start the drop below the strip so trail enters gradually
            drop.position = 0 - drop.trailLength;

            // ENHANCED: More varied trail lengths with some very long trails
            int dropType = random(100);
            if (dropType < 35) {
                // 35% chance: Medium drops (15-35 pixels)
                drop.trailLength = 15 + random(21);
            } else if (dropType < 60) {
                // 25% chance: Large streams (40-70 pixels)
                drop.trailLength = 40 + random(31);
            } else if (dropType < 85) {
                // 25% chance: Very long waterfalls (75-120 pixels)
                drop.trailLength = 75 + random(46);
            } else {
                // 15% chance: Massive cascading waterfalls (125-180 pixels!)
                drop.trailLength = 125 + random(56);
            }

            // 6x faster initial speeds and size bonus (50% faster than 4x)
            float sizeSpeedBonus = (drop.trailLength - 15) * 0.01728f;  // 50% faster: 0.01152f -> 0.01728f
            drop.speed = 0.06912f + sizeSpeedBonus +
                        (random(100) / 100.0f) * (0.27648f - 0.06912f);  // 50% faster: 0.04608f-0.18432f -> 0.06912f-0.27648f

            // 6x stronger gravity (50% faster than 4x)
            drop.acceleration = 0.010368f;  // 50% stronger: 0.006912f -> 0.010368f

            // Water drops are blue-ish with some variation
            drop.hue = 140 + random(40);  // Blue range (140-180)

            // Longer trails get slightly brighter for visual impact
            int brightnessBonus = min(50, drop.trailLength / 3);
            drop.maxBrightness = 160 + random(70) + brightnessBonus;

            // Start with zero brightness - will fade in gradually
            drop.brightness = 0;

            // Longer trails take more time to fade in for smoother appearance
            drop.fadeInFrames = 12 + min(18, drop.trailLength / 6);  // 12-30 frames for smoother fade-in
            drop.currentFrame = 0;

            // Mark as active and not splashed yet
            drop.isActive = true;
            drop.hasSplashed = false;
            drop.splashFrame = 0;

            // Only create one drop per function call
            return;
        }
    }
    // If all slots are full, that's fine - no new drop created
}

// Update a single drop's physics and state
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

        // Apply smooth cubic easing for gentler fade-in
        fadeProgress = fadeProgress * fadeProgress * (3.0f - 2.0f * fadeProgress);  // Smoothstep

        // Set current brightness based on fade progress
        drop.brightness = (uint8_t)(drop.maxBrightness * fadeProgress);
    } else {
        // Fully faded in - use maximum brightness
        drop.brightness = drop.maxBrightness;
    }

    // Update position (physics simulation)
    drop.position += drop.speed;
    drop.speed += drop.acceleration;  // Gravity effect

    // Cap maximum speed 6x higher (50% faster than 4x)
    if (drop.speed > 0.6912f) {  // 50% higher maximum speed: 0.4608f -> 0.6912f
        drop.speed = 0.6912f;
    }

    // Check if drop has reached the top and should splash
    int stripLength = getStripLength(drop.stripType);
    if (drop.position >= stripLength + drop.trailLength) {
        drop.hasSplashed = true;
        drop.splashFrame = 0;
    }
}

// Draw a water drop with its trailing effect
void WaterfallEffect::drawDrop(const WaterDrop& drop) {
    // Draw the drop trail from back to front (tail to head)
    for (int i = 0; i < drop.trailLength; i++) {
        // Calculate position of this part of the trail
        float trailPos = drop.position - i;

        // Skip if this part is off the strip
        if (trailPos < 0) continue;

        int stripLength = getStripLength(drop.stripType);
        if (trailPos >= stripLength) continue;

        // Calculate brightness fade for trail effect
        float distanceFromHead = (float)i / drop.trailLength;

        // Enhanced trail fade curve for smoother transitions
        float trailBrightness;
        if (distanceFromHead < 0.05f) {
            // Very bright head (first 5% of trail)
            trailBrightness = 1.0f;
        } else if (distanceFromHead < 0.2f) {
            // Smooth transition zone (next 15% of trail)
            float transitionFactor = (distanceFromHead - 0.05f) / 0.15f;
            trailBrightness = 1.0f - (transitionFactor * transitionFactor * 0.3f);  // Gentle curve down to 0.7
        } else if (distanceFromHead < 0.4f) {
            // Gradual fade in main body (next 20% of trail)
            float bodyFactor = (distanceFromHead - 0.2f) / 0.2f;
            trailBrightness = 0.7f - (bodyFactor * 0.4f);  // Linear fade from 0.7 to 0.3
        } else {
            // Smooth exponential fade for long tail (remaining 60% of trail)
            float tailFactor = (distanceFromHead - 0.4f) / 0.6f;
            trailBrightness = 0.3f * exp(-tailFactor * tailFactor * 4.0f);  // Smoother exponential using squared factor
        }

        // Apply drop's current brightness (for fade-in effect)
        uint8_t finalBrightness = (uint8_t)(drop.brightness * trailBrightness);

        // Skip if too dim to see
        if (finalBrightness < 5) continue;

        // Get the water color
        CRGB dropColor = getWaterColor(drop.hue, finalBrightness);

        // Map logical position to physical LED
        int physicalPos = leds.mapPositionToPhysical(drop.stripType, (int)trailPos, drop.subStrip);

        // Adjust for segment offset
        if (drop.stripType == 1) {  // Inner strips
            physicalPos += drop.subStrip * INNER_LEDS_PER_STRIP;
        } else if (drop.stripType == 2) {  // Outer strips
            physicalPos += drop.subStrip * OUTER_LEDS_PER_STRIP;
        }

        // Add color to the LED (additive blending for overlapping drops)
        if (drop.stripType == 1) {  // Inner
            if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                leds.getInner()[physicalPos] += dropColor;
            }
        } else if (drop.stripType == 2) {  // Outer
            if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                leds.getOuter()[physicalPos] += dropColor;
            }
        }
    }
}

// Draw splash effect when drop hits the top
void WaterfallEffect::drawSplash(const WaterDrop& drop) {
    // Calculate splash brightness (fades out over time)
    float fadeRatio = 1.0f - ((float)drop.splashFrame / SPLASH_FRAMES);
    uint8_t splashBrightness = drop.brightness * fadeRatio * 0.6f;

    // Get splash color
    CRGB splashColor = getWaterColor(drop.hue, splashBrightness);

    // Draw splash at the top of the strip
    int stripLength = getStripLength(drop.stripType);
    int physicalPos = leds.mapPositionToPhysical(drop.stripType, stripLength - 1, drop.subStrip);

    // Adjust for segment offset
    if (drop.stripType == 1) {  // Inner strips
        physicalPos += drop.subStrip * INNER_LEDS_PER_STRIP;
    } else if (drop.stripType == 2) {  // Outer strips
        physicalPos += drop.subStrip * OUTER_LEDS_PER_STRIP;
    }

    // Add splash color to the LED
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

// Generate realistic water colors (blues and blue-whites)
CRGB WaterfallEffect::getWaterColor(uint8_t hue, uint8_t brightness) {
    // Use reduced saturation for more realistic water colors
    uint8_t saturation = 120;

    // Brighter drops get less saturation (more white, like foam)
    if (brightness > 180) {
        saturation = 80;
    }

    // Create color using FastLED's HSV system
    return CHSV(hue, saturation, brightness);
}

// Get the length of a strip type
int WaterfallEffect::getStripLength(int stripType) {
    switch (stripType) {
        case 1:  // Inner strips
            return INNER_LEDS_PER_STRIP;
        case 2:  // Outer strips
            return OUTER_LEDS_PER_STRIP;
        default:
            return 0;  // Invalid strip type
    }
}