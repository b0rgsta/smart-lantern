// src/leds/effects/FireEffect.cpp

#include "FireEffect.h"

FireEffect::FireEffect(LEDController& ledController) :
    Effect(ledController),
    heatCore(nullptr),
    heatInner(nullptr),
    heatOuter(nullptr),
    lastUpdateTime(0),
    intensity(70)
{
    // Allocate memory for heat arrays
    heatCore = new byte[LED_STRIP_CORE_COUNT];
    heatInner = new byte[LED_STRIP_INNER_COUNT];
    heatOuter = new byte[LED_STRIP_OUTER_COUNT];

    // Initialize with default values
    reset();
}

FireEffect::~FireEffect() {
    // Clean up allocated memory
    if (heatCore) delete[] heatCore;
    if (heatInner) delete[] heatInner;
    if (heatOuter) delete[] heatOuter;
}

void FireEffect::reset() {
    // Initialize all arrays to zero
    memset(heatCore, 0, LED_STRIP_CORE_COUNT);
    memset(heatInner, 0, LED_STRIP_INNER_COUNT);
    memset(heatOuter, 0, LED_STRIP_OUTER_COUNT);

    // Debug print
    Serial.println("FireEffect reset - initializing all strips");
    Serial.print("NUM_INNER_STRIPS: ");
    Serial.println(NUM_INNER_STRIPS);
    Serial.print("NUM_OUTER_STRIPS: ");
    Serial.println(NUM_OUTER_STRIPS);
    Serial.print("INNER_LEDS_PER_STRIP: ");
    Serial.println(INNER_LEDS_PER_STRIP);
    Serial.print("OUTER_LEDS_PER_STRIP: ");
    Serial.println(OUTER_LEDS_PER_STRIP);

    // Set initial heat values for all strips
    // Start with hot base that fades to cool top

    // Inner strips - consistent pattern for ALL 3 segments
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        Serial.print("Initializing inner segment: ");
        Serial.println(segment);

        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            int index = segment * INNER_LEDS_PER_STRIP + i;
            float percentHeight = (float)i / INNER_LEDS_PER_STRIP;

            // Heat decreases as we go up
            if (percentHeight < 0.2) {
                // Very hot base with white-hot elements
                heatInner[index] = random(230, 255);  // Higher heat for white-hot appearance
            }
            else if (percentHeight < 0.4) {
                // Hot base
                heatInner[index] = random(200, 230);
            }
            else if (percentHeight < 0.7) {
                // Medium middle
                heatInner[index] = 160;
            }
            else {
                // Cool top
                heatInner[index] = 100;
            }
        }
    }

    // Outer strips - consistent pattern for ALL 3 segments
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        Serial.print("Initializing outer segment: ");
        Serial.println(segment);

        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int index = segment * OUTER_LEDS_PER_STRIP + i;
            float percentHeight = (float)i / OUTER_LEDS_PER_STRIP;

            // Heat decreases as we go up - use slightly higher values than inner
            if (percentHeight < 0.2) {
                // Very hot base with white-hot elements
                heatOuter[index] = random(240, 255);  // Higher heat for white-hot appearance
            }
            else if (percentHeight < 0.4) {
                // Hot base
                heatOuter[index] = random(210, 240);
            }
            else if (percentHeight < 0.7) {
                // Medium middle
                heatOuter[index] = 170;
            }
            else {
                // Cool top
                heatOuter[index] = 110;
            }
        }
    }

    lastUpdateTime = millis();
}

void FireEffect::update() {
    // Target 120 FPS for ultra-smooth fire animation but slow down the simulation more
    if (!shouldUpdate(20)) {  // Changed from 16ms to 20ms = 50 FPS (25% slower than 62.5 FPS)
        return;
    }

    // Update the fire simulation
    updateFireBase();

    // Render the fire
    renderFire();

    // Show the changes
    leds.showAll();
}

void FireEffect::updateFireBase() {
    // Adjusted fire parameters for higher flames
    int cooling = 12;  // Reduced cooling further to keep heat longer (was 15)
    int sparking = 110; // Increased spark chance for more activity (was 90)

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

        // Cool down every cell a little - reduced cooling for higher flames
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            // Less cooling at all levels to preserve heat higher up
            int coolAmount;
            if (i < INNER_LEDS_PER_STRIP * 0.4) {
                coolAmount = random(0, cooling / 6); // Even less cooling for hotter base (was /5)
            } else if (i < INNER_LEDS_PER_STRIP * 0.8) {
                coolAmount = random(0, cooling / 4); // Less cooling in middle (was /3)
            } else {
                coolAmount = random(0, cooling / 3); // Less cooling at top (was /2)
            }

            if (tempHeat[i] > coolAmount) {
                tempHeat[i] = tempHeat[i] - coolAmount;
            } else {
                tempHeat[i] = 0;
            }
        }

        // Heat rises upward - stronger mixing for higher flames
        for (int i = INNER_LEDS_PER_STRIP - 1; i >= 2; i--) {
            tempHeat[i] = (tempHeat[i] * 1 + tempHeat[i-1] * 4 + tempHeat[i-2] * 3) / 8; // Stronger upward heat flow (was /6)
        }

        // Randomly ignite new sparks at the bottom - more and bigger sparks
        if (random8() < sparking) {
            int y = random8(7); // Slightly more bottom LEDs can spark (was 5)
            tempHeat[y] = tempHeat[y] + random8(80, 160); // Even larger sparks (was 60-140)

            // More frequent white-hot sparks
            if (random8() < 40) {  // 40% chance for an extra hot spark (was 30%)
                tempHeat[y] = min(255, tempHeat[y] + random8(40, 80)); // Bigger white-hot sparks (was 30-60)
            }
        }

        // Copy back to main array
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            heatInner[segmentStart + i] = tempHeat[i];
        }
    }

    // ====== OUTER STRIPS ======
    // Process each outer strip segment separately (same changes as inner strips)
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        // Create temporary array for this segment
        byte tempHeat[OUTER_LEDS_PER_STRIP];
        memset(tempHeat, 0, OUTER_LEDS_PER_STRIP);

        // Copy current data
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            tempHeat[i] = heatOuter[segmentStart + i];
        }

        // Cool down every cell a little - reduced cooling for higher flames
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            // Less cooling at all levels to preserve heat higher up
            int coolAmount;
            if (i < OUTER_LEDS_PER_STRIP * 0.4) {
                coolAmount = random(0, cooling / 6); // Even less cooling for hotter base (was /5)
            } else if (i < OUTER_LEDS_PER_STRIP * 0.8) {
                coolAmount = random(0, cooling / 4); // Less cooling in middle (was /3)
            } else {
                coolAmount = random(0, cooling / 3); // Less cooling at top (was /2)
            }

            if (tempHeat[i] > coolAmount) {
                tempHeat[i] = tempHeat[i] - coolAmount;
            } else {
                tempHeat[i] = 0;
            }
        }

        // Heat rises upward - stronger mixing for higher flames
        for (int i = OUTER_LEDS_PER_STRIP - 1; i >= 2; i--) {
            tempHeat[i] = (tempHeat[i] * 1 + tempHeat[i-1] * 4 + tempHeat[i-2] * 3) / 8; // Stronger upward heat flow (was /6)
        }

        // Randomly ignite new sparks at the bottom - more and bigger sparks
        if (random8() < sparking) {
            int y = random8(7); // Slightly more bottom LEDs can spark (was 5)
            tempHeat[y] = tempHeat[y] + random8(80, 160); // Even larger sparks (was 60-140)

            // More frequent white-hot sparks
            if (random8() < 40) {  // 40% chance for an extra hot spark (was 30%)
                tempHeat[y] = min(255, tempHeat[y] + random8(40, 80)); // Bigger white-hot sparks (was 30-60)
            }
        }

        // Copy back to main array
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            heatOuter[segmentStart + i] = tempHeat[i];
        }
    }

    // Force fade to black at the top of strips - reduce heat values based on position
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        int segmentStart = segment * INNER_LEDS_PER_STRIP;

        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            int idx = segmentStart + i;

            // Apply fade reduction starting from 45% up the strip (was 60% - much lower for more black)
            float fadeStartPosition = INNER_LEDS_PER_STRIP * 0.45f;

            if (i >= fadeStartPosition) {
                // Calculate how far up the strip we are (0.0 to 1.0)
                float fadeProgress = (float(i) - fadeStartPosition) / (INNER_LEDS_PER_STRIP - fadeStartPosition);

                // Apply much gentler fade to heat values
                fadeProgress = fadeProgress * fadeProgress; // Square for more aggressive fade

                // Reduce heat value more aggressively (75% reduction max instead of 50%)
                float heatReduction = fadeProgress * 0.75f; // Up to 75% heat reduction at the top (was 50%)
                heatInner[idx] = heatInner[idx] * (1.0f - heatReduction);
            }
        }

        // Make top area more black
        int topIdx = (segment + 1) * INNER_LEDS_PER_STRIP - 1;
        for (int i = 0; i < 5; i++) { // Clear top 5 LEDs (was 3)
            int curr = topIdx - i;
            if (curr >= segmentStart && curr < segmentStart + INNER_LEDS_PER_STRIP) {
                if (i < 3) {
                    heatInner[curr] = heatInner[curr] * 0.4f; // More aggressive reduction (was 0.7f)
                } else {
                    heatInner[curr] = heatInner[curr] * 0.6f; // More reduction (was 0.8f)
                }
            }
        }
    }

    // Same fade to black logic for outer strips
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int idx = segmentStart + i;

            // Apply fade reduction starting from 45% up the strip (was 60% - much lower for more black)
            float fadeStartPosition = OUTER_LEDS_PER_STRIP * 0.45f;

            if (i >= fadeStartPosition) {
                // Calculate how far up the strip we are (0.0 to 1.0)
                float fadeProgress = (float(i) - fadeStartPosition) / (OUTER_LEDS_PER_STRIP - fadeStartPosition);

                // Apply much gentler fade to heat values
                fadeProgress = fadeProgress * fadeProgress; // Square for more aggressive fade

                // Reduce heat value more aggressively (75% reduction max instead of 50%)
                float heatReduction = fadeProgress * 0.75f; // Up to 75% heat reduction at the top (was 50%)
                heatOuter[idx] = heatOuter[idx] * (1.0f - heatReduction);
            }
        }

        // Only prevent isolated LEDs at the very very top
        int topIdx = (segment + 1) * OUTER_LEDS_PER_STRIP - 1;
        for (int i = 0; i < 3; i++) { // Only clear top 3 LEDs (was 8)
            int curr = topIdx - i;
            if (curr >= segmentStart && curr < segmentStart + OUTER_LEDS_PER_STRIP) {
                if (i < 2) {
                    heatOuter[curr] = heatOuter[curr] * 0.7f; // Don't force to zero, just reduce (was 0)
                } else {
                    heatOuter[curr] = heatOuter[curr] * 0.8f; // Mild reduction (was 0.3f)
                }
            }
        }
    }
}

uint32_t FireEffect::heatToColor(unsigned char heat) {
    // Convert heat value to fire colors (white-yellow-orange-red)
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
        uint8_t blue = map(heat, 210, 255, 0, 220);  // Add blue to make it whiter
        color = CRGB(red, green, blue);
    }

    return (uint32_t)color.r << 16 | (uint32_t)color.g << 8 | color.b;
}

void FireEffect::renderFire() {
    // Clear all strips first
    leds.clearAll();

    // Core strip remains off intentionally

    // Render inner strips with more obvious fade to black
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        int activePixels = 0;

        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            int idx = segment * INNER_LEDS_PER_STRIP + i;

            if (heatInner[idx] > 0) {
                activePixels++;

                // Get physical position
                int physicalPos = mapLEDPosition(1, i, segment);
                physicalPos += segment * INNER_LEDS_PER_STRIP;

                // Make sure we're in bounds
                if (physicalPos >= 0 && physicalPos < LED_STRIP_INNER_COUNT) {
                    // Set the LED color
                    uint32_t colorVal = heatToColor(heatInner[idx]);
                    CRGB color = leds.neoColorToCRGB(colorVal);

                    // Apply fade to black starting at 45% up the strip (was 60% - much lower for more black)
                    float fadeStartPosition = INNER_LEDS_PER_STRIP * 0.45f;

                    if (i >= fadeStartPosition) {
                        // Calculate fade factor with very aggressive fading
                        float fadeProgress = (float(i) - fadeStartPosition) / (INNER_LEDS_PER_STRIP - fadeStartPosition);

                        // Apply double exponential fade for extremely dramatic effect
                        fadeProgress = fadeProgress * fadeProgress * fadeProgress; // Cube for very aggressive fade

                        float fadeFactor = 1.0f - fadeProgress; // 1.0 at start, 0.0 at top

                        // Apply fade by reducing all color components
                        color.r = color.r * fadeFactor;
                        color.g = color.g * fadeFactor;
                        color.b = color.b * fadeFactor;

                        // Force more of the top to be completely black
                        if (i >= INNER_LEDS_PER_STRIP * 0.90f) { // Top 10% of strip forced black (was 1%)
                            color.r = 0;
                            color.g = 0;
                            color.b = 0;
                        }
                    }
                    // If i < fadeStartPosition, use original color (no fade)

                    leds.getInner()[physicalPos] = color;
                }
            }
        }
    }

    // Render outer strips with more obvious fade to black
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int activePixels = 0;

        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int idx = segment * OUTER_LEDS_PER_STRIP + i;

            if (heatOuter[idx] > 0) {
                activePixels++;

                // Map logical position to physical LED
                int physicalPos = mapLEDPosition(2, i, segment);
                physicalPos += segment * OUTER_LEDS_PER_STRIP;

                // Make sure we're in bounds
                if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
                    // Set the LED color
                    uint32_t colorVal = heatToColor(heatOuter[idx]);
                    CRGB color = leds.neoColorToCRGB(colorVal);

                    // Apply fade to black starting at 45% up the strip (was 60% - much lower for more black)
                    float fadeStartPosition = OUTER_LEDS_PER_STRIP * 0.45f;

                    if (i >= fadeStartPosition) {
                        // Calculate fade factor with very aggressive fading
                        float fadeProgress = (float(i) - fadeStartPosition) / (OUTER_LEDS_PER_STRIP - fadeStartPosition);

                        // Apply double exponential fade for extremely dramatic effect
                        fadeProgress = fadeProgress * fadeProgress * fadeProgress; // Cube for very aggressive fade

                        float fadeFactor = 1.0f - fadeProgress; // 1.0 at start, 0.0 at top

                        // Apply fade by reducing all color components
                        color.r = color.r * fadeFactor;
                        color.g = color.g * fadeFactor;
                        color.b = color.b * fadeFactor;

                        // Force more of the top to be completely black
                        if (i >= OUTER_LEDS_PER_STRIP * 0.90f) { // Top 10% of strip forced black (was 1%)
                            color.r = 0;
                            color.g = 0;
                            color.b = 0;
                        }
                    }
                    // If i < fadeStartPosition, use original color (no fade)

                    leds.getOuter()[physicalPos] = color;
                }
            }
        }
    }

    // FORCE-ON TEST for third outer strip with updated fade

}

int FireEffect::mapLEDPosition(int stripType, int position, int subStrip) {
    // Use the LED controller's mapping function with debug output
    int result = leds.mapPositionToPhysical(stripType, position, subStrip);

    return result;
}

void FireEffect::setIntensity(byte newIntensity) {
    // Clamp intensity to 0-100
    intensity = constrain(newIntensity, 0, 100);
}