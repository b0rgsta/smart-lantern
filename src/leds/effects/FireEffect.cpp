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
    // Control animation speed - reduce from 60ms to 40ms for faster movement
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime < 40) {  // Changed from 60ms to 40ms
        return; // Skip this frame to control the animation speed
    }

    // Update the fire simulation
    updateFireBase();

    // Render the fire
    renderFire();

    // Show the changes
    leds.showAll();

    // Update timing
    lastUpdateTime = currentTime;
}

void FireEffect::updateFireBase() {
    // Adjusted fire parameters for faster movement
    int cooling = 15;  // Reduced cooling for less rapid changes but more intense fire
    int sparking = 90; // Higher spark chance for more activity

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

        // Cool down every cell a little
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            // More cooling at top, less at bottom
            int coolAmount;
            if (i < INNER_LEDS_PER_STRIP * 0.3) {
                coolAmount = random(0, cooling / 5); // Reduced cooling for hotter base
            } else if (i < INNER_LEDS_PER_STRIP * 0.7) {
                coolAmount = random(0, cooling / 3);
            } else {
                coolAmount = random(0, cooling / 2);
            }

            if (tempHeat[i] > coolAmount) {
                tempHeat[i] = tempHeat[i] - coolAmount;
            } else {
                tempHeat[i] = 0;
            }
        }

        // Heat rises upward - faster mixing for more active flames
        for (int i = INNER_LEDS_PER_STRIP - 1; i >= 2; i--) {
            tempHeat[i] = (tempHeat[i] * 1 + tempHeat[i-1] * 3 + tempHeat[i-2] * 2) / 6; // Adjusted weights for faster rising
        }

        // Randomly ignite new sparks at the bottom
        if (random8() < sparking) {
            int y = random8(5); // Just the bottom few LEDs
            tempHeat[y] = tempHeat[y] + random8(60, 140); // Larger sparks for more activity

            // Occasionally add a white-hot spark (250+ is white in our color mapping)
            if (random8() < 30) {  // 30% chance for an extra hot spark
                tempHeat[y] = min(255, tempHeat[y] + random8(30, 60));
            }
        }

        // Copy back to main array
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            heatInner[segmentStart + i] = tempHeat[i];
        }
    }

    // ====== OUTER STRIPS ======
    // Process each outer strip segment separately (similar changes as inner strips)
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        // Create temporary array for this segment
        byte tempHeat[OUTER_LEDS_PER_STRIP];
        memset(tempHeat, 0, OUTER_LEDS_PER_STRIP);

        // Copy current data
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            tempHeat[i] = heatOuter[segmentStart + i];
        }

        // Cool down every cell a little
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            // More cooling at top, less at bottom
            int coolAmount;
            if (i < OUTER_LEDS_PER_STRIP * 0.3) {
                coolAmount = random(0, cooling / 5); // Reduced cooling for hotter base
            } else if (i < OUTER_LEDS_PER_STRIP * 0.7) {
                coolAmount = random(0, cooling / 3);
            } else {
                coolAmount = random(0, cooling / 2);
            }

            if (tempHeat[i] > coolAmount) {
                tempHeat[i] = tempHeat[i] - coolAmount;
            } else {
                tempHeat[i] = 0;
            }
        }

        // Heat rises upward - faster mixing for more active flames
        for (int i = OUTER_LEDS_PER_STRIP - 1; i >= 2; i--) {
            tempHeat[i] = (tempHeat[i] * 1 + tempHeat[i-1] * 3 + tempHeat[i-2] * 2) / 6; // Adjusted weights for faster rising
        }

        // Randomly ignite new sparks at the bottom
        if (random8() < sparking) {
            int y = random8(5); // Just the bottom few LEDs
            tempHeat[y] = tempHeat[y] + random8(60, 140); // Larger sparks for more activity

            // Occasionally add a white-hot spark (250+ is white in our color mapping)
            if (random8() < 30) {  // 30% chance for an extra hot spark
                tempHeat[y] = min(255, tempHeat[y] + random8(30, 60));
            }
        }

        // Copy back to main array
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            heatOuter[segmentStart + i] = tempHeat[i];
        }
    }

    // Prevent any isolated bright LEDs at the top
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        // Get top pixel index and a few pixels below
        int topIdx = (segment + 1) * INNER_LEDS_PER_STRIP - 1;

        // If any of the top 5 LEDs are lit but the ones below are not,
        // douse them to prevent isolated bright spots
        for (int i = 0; i < 5 && (topIdx - i) > 5; i++) {
            int curr = topIdx - i;
            int below = topIdx - i - 2;

            if (heatInner[curr] > 0 && heatInner[below] == 0) {
                heatInner[curr] = 0;
            }
        }
    }

    // Same for outer strips
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        // Get top pixel index and a few pixels below
        int topIdx = (segment + 1) * OUTER_LEDS_PER_STRIP - 1;

        // If any of the top 5 LEDs are lit but the ones below are not,
        // douse them to prevent isolated bright spots
        for (int i = 0; i < 5 && (topIdx - i) > 5; i++) {
            int curr = topIdx - i;
            int below = topIdx - i - 2;

            if (heatOuter[curr] > 0 && heatOuter[below] == 0) {
                heatOuter[curr] = 0;
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

    // Render inner strips
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
                    leds.getInner()[physicalPos] = color;
                }
            }
        }
    }

    // Render outer strips
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
                    leds.getOuter()[physicalPos] = color;
                }
            }
        }
    }

    // FORCE-ON TEST for third outer strip
    if (true) {
        for (int i = 0; i < 10; i++) {
            int segment = 2; // Third segment
            int physicalPos = i + segment * OUTER_LEDS_PER_STRIP;

            // Set to bright red
            CRGB color = CRGB(255, 0, 0);
            leds.getOuter()[physicalPos] = color;
        }
    }
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