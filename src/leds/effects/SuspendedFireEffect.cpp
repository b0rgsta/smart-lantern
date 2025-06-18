// src/leds/effects/SuspendedFireEffect.cpp

#include "SuspendedFireEffect.h"
#include "../LEDController.h"
#include "../../Config.h"

SuspendedFireEffect::SuspendedFireEffect(LEDController& ledController)
    : Effect(ledController), intensity(80) {  // Default intensity set to 80%

    // Allocate memory for heat simulation arrays
    // These track the "heat" at each LED position for realistic fire simulation
    heatCore = new unsigned char[LED_STRIP_CORE_COUNT];
    heatInner = new unsigned char[LED_STRIP_INNER_COUNT];
    heatOuter = new unsigned char[LED_STRIP_OUTER_COUNT];

    // Initialize all heat arrays to zero (no heat = black)
    memset(heatCore, 0, LED_STRIP_CORE_COUNT);
    memset(heatInner, 0, LED_STRIP_INNER_COUNT);
    memset(heatOuter, 0, LED_STRIP_OUTER_COUNT);

    // Initialize flame height arrays
    for (int i = 0; i < NUM_INNER_STRIPS; i++) {
        innerFlameHeights[i] = 0.8f;   // Start at 80% height (inner strips higher)
        innerHeightTargets[i] = 0.8f;
    }
    for (int i = 0; i < NUM_OUTER_STRIPS; i++) {
        outerFlameHeights[i] = 0.75f;  // Start at 75% height (outer strips shorter)
        outerHeightTargets[i] = 0.75f;
    }

    lastUpdateTime = millis();
    lastHeightUpdate = millis();

    // Call reset to set up initial suspended fire state
    reset();
}

SuspendedFireEffect::~SuspendedFireEffect() {
    // Clean up allocated memory to prevent memory leaks
    delete[] heatCore;
    delete[] heatInner;
    delete[] heatOuter;
}

void SuspendedFireEffect::reset() {
    Serial.println("SuspendedFireEffect: Initializing suspended fire effect");

    // Initialize heat values for suspended fire (flames hang from top)
    // The fire base is now at the TOP of each strip, flames descend downward

    // Core strip remains off (same as FireEffect)
    memset(heatCore, 0, LED_STRIP_CORE_COUNT);

    // Inner strips - Initialize with hot base at TOP (high index = top of strip)
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            int index = segment * INNER_LEDS_PER_STRIP + i;

            // Calculate position from TOP (inverted from FireEffect)
            // High index = top of strip (hot base), low index = bottom (cool flames)
            float percentFromTop = (float)(INNER_LEDS_PER_STRIP - 1 - i) / INNER_LEDS_PER_STRIP;

            // Heat decreases as we go down from the top
            if (percentFromTop < 0.2) {
                // Very hot base at the top
                heatInner[index] = random(220, 255);
            }
            else if (percentFromTop < 0.4) {
                // Hot zone below the base
                heatInner[index] = random(180, 220);
            }
            else if (percentFromTop < 0.7) {
                // Medium flame zone
                heatInner[index] = 140;
            }
            else {
                // Cool bottom area
                heatInner[index] = 100;
            }
        }
    }

    // Outer strips - Initialize with hot base at TOP (consistent with inner strips)
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int index = segment * OUTER_LEDS_PER_STRIP + i;

            // Calculate position from TOP (inverted from FireEffect)
            float percentFromTop = (float)(OUTER_LEDS_PER_STRIP - 1 - i) / OUTER_LEDS_PER_STRIP;

            // Heat decreases as we go down from the top
            if (percentFromTop < 0.2) {
                // Very hot base at the top with white-hot elements
                heatOuter[index] = random(240, 255);
            }
            else if (percentFromTop < 0.4) {
                // Hot zone below the base
                heatOuter[index] = random(210, 240);
            }
            else if (percentFromTop < 0.7) {
                // Medium flame zone
                heatOuter[index] = 170;
            }
            else {
                // Cool bottom area
                heatOuter[index] = 110;
            }
        }
    }

    lastUpdateTime = millis();
}

void SuspendedFireEffect::update() {
    // Target 50 FPS for smooth suspended fire animation
    if (!shouldUpdate(20)) {
        return;
    }

    // Update dynamic flame heights every 100ms for natural variation
    updateFlameHeights();

    // Update the suspended fire simulation
    updateSuspendedFireBase();

    // Render the suspended fire
    renderSuspendedFire();

    // Show the changes
    leds.showAll();
}

void SuspendedFireEffect::updateSuspendedFireBase() {
    // Fire simulation parameters (same as FireEffect)
    int cooling = 12;   // Heat loss rate
    int sparking = 110; // Spark generation chance

    // ====== INNER STRIPS ======
    // Process each inner strip segment separately
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        int segmentStart = segment * INNER_LEDS_PER_STRIP;

        // Create temporary array for this segment
        byte tempHeat[INNER_LEDS_PER_STRIP];
        memset(tempHeat, 0, INNER_LEDS_PER_STRIP);

        // Copy current data
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            tempHeat[i] = heatInner[segmentStart + i];
        }

        // Cool down every cell (same cooling logic as FireEffect)
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            int coolAmount;
            if (i > INNER_LEDS_PER_STRIP * 0.6) {  // Top 40% (hot base area)
                coolAmount = random(0, cooling / 6);
            } else if (i > INNER_LEDS_PER_STRIP * 0.2) {  // Middle area
                coolAmount = random(0, cooling / 4);
            } else {  // Bottom area (cool flames)
                coolAmount = random(0, cooling / 3);
            }

            if (tempHeat[i] > coolAmount) {
                tempHeat[i] = tempHeat[i] - coolAmount;
            } else {
                tempHeat[i] = 0;
            }
        }

        // Heat flows DOWNWARD from top (inverted from FireEffect)
        // High index to low index (top to bottom)
        for (int i = 0; i <= INNER_LEDS_PER_STRIP - 3; i++) {
            tempHeat[i] = (tempHeat[i] * 1 + tempHeat[i+1] * 4 + tempHeat[i+2] * 3) / 8;
        }

        // Randomly ignite new sparks at the TOP (inverted from FireEffect)
        if (random8() < sparking) {
            int y = INNER_LEDS_PER_STRIP - 1 - random8(7);  // Top 7 LEDs can spark
            tempHeat[y] = tempHeat[y] + random8(80, 160);

            // More frequent white-hot sparks at the top
            if (random8() < 40) {
                tempHeat[y] = min(255, tempHeat[y] + random8(40, 80));
            }
        }

        // Copy back to main array
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            heatInner[segmentStart + i] = tempHeat[i];
        }

        // Apply dynamic flame height cutoff for this segment
        applyFlameHeightCutoff(segment, true); // true = inner strip
    }

    // ====== OUTER STRIPS ======
    // Process each outer strip segment separately (same logic as inner)
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        // Create temporary array for this segment
        byte tempHeat[OUTER_LEDS_PER_STRIP];
        memset(tempHeat, 0, OUTER_LEDS_PER_STRIP);

        // Copy current data
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            tempHeat[i] = heatOuter[segmentStart + i];
        }

        // Cool down every cell (same cooling logic)
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int coolAmount;
            if (i > OUTER_LEDS_PER_STRIP * 0.6) {  // Top 40% (hot base area)
                coolAmount = random(0, cooling / 6);
            } else if (i > OUTER_LEDS_PER_STRIP * 0.2) {  // Middle area
                coolAmount = random(0, cooling / 4);
            } else {  // Bottom area (cool flames)
                coolAmount = random(0, cooling / 3);
            }

            if (tempHeat[i] > coolAmount) {
                tempHeat[i] = tempHeat[i] - coolAmount;
            } else {
                tempHeat[i] = 0;
            }
        }

        // Heat flows DOWNWARD from top (inverted from FireEffect)
        for (int i = 0; i <= OUTER_LEDS_PER_STRIP - 3; i++) {
            tempHeat[i] = (tempHeat[i] * 1 + tempHeat[i+1] * 4 + tempHeat[i+2] * 3) / 8;
        }

        // Randomly ignite new sparks at the TOP (inverted from FireEffect)
        if (random8() < sparking) {
            int y = OUTER_LEDS_PER_STRIP - 1 - random8(7);  // Top 7 LEDs can spark
            tempHeat[y] = tempHeat[y] + random8(80, 160);

            // More frequent white-hot sparks at the top
            if (random8() < 40) {
                tempHeat[y] = min(255, tempHeat[y] + random8(40, 80));
            }
        }

        // Copy back to main array
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            heatOuter[segmentStart + i] = tempHeat[i];
        }

        // Apply dynamic flame height cutoff for this segment
        applyFlameHeightCutoff(segment, false); // false = outer strip
    }
}

uint32_t SuspendedFireEffect::heatToColor(unsigned char heat) {
    // Convert heat value to fire colors (same as FireEffect)
    // white-yellow-orange-red color gradient based on heat intensity
    CRGB color;

    if (heat <= 0) {
        // No heat = black
        color = CRGB(0, 0, 0);
    }
    else if (heat < 70) {
        // Low heat = dark red
        uint8_t red = map(heat, 0, 70, 0, 160);
        color = CRGB(red, 0, 0);
    }
    else if (heat < 140) {
        // Medium heat = red
        uint8_t red = map(heat, 70, 140, 160, 255);
        uint8_t green = map(heat, 70, 140, 0, 40);
        color = CRGB(red, green, 0);
    }
    else if (heat < 210) {
        // High heat = orange-yellow
        uint8_t red = 255;
        uint8_t green = map(heat, 140, 210, 40, 120);
        color = CRGB(red, green, 0);
    }
    else {
        // Very high heat = white-hot core
        uint8_t red = 255;
        uint8_t green = map(heat, 210, 255, 120, 255);
        uint8_t blue = map(heat, 210, 255, 0, 220);
        color = CRGB(red, green, blue);
    }

    return (uint32_t)color.r << 16 | (uint32_t)color.g << 8 | color.b;
}

void SuspendedFireEffect::renderSuspendedFire() {
    // Clear all strips first
    leds.clearAll();

    // Core strip remains off intentionally (same as FireEffect)

    // Render inner strips
    // NOTE: Black gradient overlay remains in original position (fades to black at TOP)
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            int idx = segment * INNER_LEDS_PER_STRIP + i;

            if (heatInner[idx] > 0) {
                // Get physical position using LED mapping
                int physicalPos = mapLEDPosition(1, i, segment);
                physicalPos += segment * INNER_LEDS_PER_STRIP;

                // Make sure we're in bounds
                if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                    // Set the LED color based on heat
                    uint32_t colorVal = heatToColor(heatInner[idx]);
                    CRGB color = leds.neoColorToCRGB(colorVal);

                    // Apply BLACK GRADIENT OVERLAY (unchanged from FireEffect)
                    // This creates the fade to black at the TOP regardless of flame direction
                    float fadeStartPosition = INNER_LEDS_PER_STRIP * 0.45f;

                    if (i >= fadeStartPosition) {
                        // Calculate fade factor with aggressive fading
                        float fadeProgress = (float(i) - fadeStartPosition) / (INNER_LEDS_PER_STRIP - fadeStartPosition);
                        fadeProgress = fadeProgress * fadeProgress * fadeProgress; // Cube for dramatic fade
                        float fadeFactor = 1.0f - fadeProgress;

                        // Apply fade by reducing all color components
                        color.r = color.r * fadeFactor;
                        color.g = color.g * fadeFactor;
                        color.b = color.b * fadeFactor;

                        // Force top 10% to be completely black
                        if (i >= INNER_LEDS_PER_STRIP * 0.90f) {
                            color.r = 0;
                            color.g = 0;
                            color.b = 0;
                        }
                    }

                    leds.getInner()[physicalPos] = color;
                }
            }
        }
    }

    // Render outer strips
    // NOTE: Black gradient overlay remains in original position (fades to black at TOP)
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int idx = segment * OUTER_LEDS_PER_STRIP + i;

            if (heatOuter[idx] > 0) {
                // Map logical position to physical LED
                int physicalPos = mapLEDPosition(2, i, segment);
                physicalPos += segment * OUTER_LEDS_PER_STRIP;

                // Make sure we're in bounds
                if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                    // Set the LED color based on heat
                    uint32_t colorVal = heatToColor(heatOuter[idx]);
                    CRGB color = leds.neoColorToCRGB(colorVal);

                    // Apply BLACK GRADIENT OVERLAY (unchanged from FireEffect)
                    // This creates the fade to black at the TOP regardless of flame direction
                    float fadeStartPosition = OUTER_LEDS_PER_STRIP * 0.45f;

                    if (i >= fadeStartPosition) {
                        // Calculate fade factor with aggressive fading
                        float fadeProgress = (float(i) - fadeStartPosition) / (OUTER_LEDS_PER_STRIP - fadeStartPosition);
                        fadeProgress = fadeProgress * fadeProgress * fadeProgress; // Cube for dramatic fade
                        float fadeFactor = 1.0f - fadeProgress;

                        // Apply fade by reducing all color components
                        color.r = color.r * fadeFactor;
                        color.g = color.g * fadeFactor;
                        color.b = color.b * fadeFactor;

                        // Force top 10% to be completely black
                        if (i >= OUTER_LEDS_PER_STRIP * 0.90f) {
                            color.r = 0;
                            color.g = 0;
                            color.b = 0;
                        }
                    }

                    leds.getOuter()[physicalPos] = color;
                }
            }
        }
    }
}

int SuspendedFireEffect::mapLEDPosition(int stripType, int position, int subStrip) {
    // Use the LED controller's mapping function for consistent LED positioning
    return leds.mapPositionToPhysical(stripType, position, subStrip);
}

void SuspendedFireEffect::setIntensity(byte newIntensity) {
    // Clamp intensity to valid range (0-100)
    intensity = constrain(newIntensity, 0, 100);
}

void SuspendedFireEffect::updateFlameHeights() {
    unsigned long currentTime = millis();

    // Update flame height targets every 100-300ms for natural variation
    if (currentTime - lastHeightUpdate >= 150) {
        lastHeightUpdate = currentTime;

        // Update inner strip flame heights with individual variation
        for (int i = 0; i < NUM_INNER_STRIPS; i++) {
            // Generate new random target height between 60% and 100% (inner strips go higher)
            float minHeight = 0.60f;
            float maxHeight = 1.0f;
            innerHeightTargets[i] = minHeight + (random(0, 100) / 100.0f) * (maxHeight - minHeight);
        }

        // Update outer strip flame heights with individual variation
        for (int i = 0; i < NUM_OUTER_STRIPS; i++) {
            // Generate new random target height between 55% and 95% (outer strips are shorter)
            float minHeight = 0.55f;
            float maxHeight = 0.95f;
            outerHeightTargets[i] = minHeight + (random(0, 100) / 100.0f) * (maxHeight - minHeight);
        }
    }

    // Smoothly interpolate current heights toward targets
    float lerpSpeed = 0.05f; // Adjust this for faster/slower height changes

    for (int i = 0; i < NUM_INNER_STRIPS; i++) {
        innerFlameHeights[i] += (innerHeightTargets[i] - innerFlameHeights[i]) * lerpSpeed;
    }

    for (int i = 0; i < NUM_OUTER_STRIPS; i++) {
        outerFlameHeights[i] += (outerHeightTargets[i] - outerFlameHeights[i]) * lerpSpeed;
    }
}

void SuspendedFireEffect::applyFlameHeightCutoff(int segment, bool isInnerStrip) {
    // Apply individual flame height cutoff for each strip segment

    if (isInnerStrip) {
        // Apply cutoff to inner strip segment
        int segmentStart = segment * INNER_LEDS_PER_STRIP;
        float currentHeight = innerFlameHeights[segment];

        // Calculate cutoff position (flames hang from top downward)
        int cutoffPosition = (int)(INNER_LEDS_PER_STRIP * (1.0f - currentHeight));

        // Clear heat below the dynamic flame height
        for (int i = 0; i < cutoffPosition; i++) {
            int idx = segmentStart + i;
            // Gradually reduce heat toward the cutoff for smooth transition
            float fadeDistance = min(5, cutoffPosition); // Fade over 5 LEDs or less
            if (i >= cutoffPosition - fadeDistance) {
                float fadeFactor = (float)(cutoffPosition - i) / fadeDistance;
                heatInner[idx] = heatInner[idx] * fadeFactor;
            } else {
                heatInner[idx] = 0; // Complete cutoff
            }
        }
    } else {
        // Apply cutoff to outer strip segment
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;
        float currentHeight = outerFlameHeights[segment];

        // Calculate cutoff position (flames hang from top downward)
        int cutoffPosition = (int)(OUTER_LEDS_PER_STRIP * (1.0f - currentHeight));

        // Clear heat below the dynamic flame height
        for (int i = 0; i < cutoffPosition; i++) {
            int idx = segmentStart + i;
            // Gradually reduce heat toward the cutoff for smooth transition
            float fadeDistance = min(5, cutoffPosition); // Fade over 5 LEDs or less
            if (i >= cutoffPosition - fadeDistance) {
                float fadeFactor = (float)(cutoffPosition - i) / fadeDistance;
                heatOuter[idx] = heatOuter[idx] * fadeFactor;
            } else {
                heatOuter[idx] = 0; // Complete cutoff
            }
        }
    }
}