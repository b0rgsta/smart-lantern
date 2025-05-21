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
            if (percentHeight < 0.3) {
                // Hot base
                heatInner[index] = 220;
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
            if (percentHeight < 0.3) {
                // Hot base
                heatOuter[index] = 230;
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
    // Control animation speed - only update every 60ms for slower movement
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime < 60) {
        return; // Skip this frame to slow down the animation
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
    // Simplified fire parameters
    int cooling = 20;  // Lower cooling value for less rapid changes
    int sparking = 80; // Lower spark chance for less activity

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
                coolAmount = random(0, cooling / 4); // Reduced cooling for slower changes
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

        // Heat rises upward - slower mixing for smoother flames
        for (int i = INNER_LEDS_PER_STRIP - 1; i >= 2; i--) {
            tempHeat[i] = (tempHeat[i] * 2 + tempHeat[i-1] * 3 + tempHeat[i-2]) / 6;
        }

        // Randomly ignite new sparks at the bottom
        if (random8() < sparking) {
            int y = random8(5); // Just the bottom few LEDs
            tempHeat[y] = tempHeat[y] + random8(40, 100); // Smaller sparks for less activity
        }

        // Copy back to main array
        for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
            heatInner[segmentStart + i] = tempHeat[i];
        }
    }

    // ====== OUTER STRIPS ======
    // Process each outer strip segment separately
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        // Debug outer segment info
        Serial.print("Processing outer segment ");
        Serial.print(segment);
        Serial.print(": Start idx=");
        Serial.print(segmentStart);
        Serial.print(", End idx=");
        Serial.println(segmentStart + OUTER_LEDS_PER_STRIP - 1);

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
                coolAmount = random(0, cooling / 4); // Reduced cooling for slower changes
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

        // Heat rises upward - slower mixing for smoother flames
        for (int i = OUTER_LEDS_PER_STRIP - 1; i >= 2; i--) {
            tempHeat[i] = (tempHeat[i] * 2 + tempHeat[i-1] * 3 + tempHeat[i-2]) / 6;
        }

        // Randomly ignite new sparks at the bottom
        if (random8() < sparking) {
            int y = random8(5); // Just the bottom few LEDs
            tempHeat[y] = tempHeat[y] + random8(40, 100); // Smaller sparks for less activity
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
    // Convert heat value to fire colors (red to orange)

    CRGB color;

    if (heat <= 0) {
        // No heat = black
        color = CRGB(0, 0, 0);
    }
    else if (heat < 85) {
        // Low heat = dark red
        uint8_t red = map(heat, 0, 85, 0, 160);
        color = CRGB(red, 0, 0);
    }
    else if (heat < 170) {
        // Medium heat = red
        uint8_t red = map(heat, 85, 170, 160, 255);
        uint8_t green = map(heat, 85, 170, 0, 40);
        color = CRGB(red, green, 0);
    }
    else {
        // High heat = orange-red
        uint8_t green = map(heat, 170, 255, 40, 80);
        color = CRGB(255, green, 0);
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

        Serial.print("Inner segment ");
        Serial.print(segment);
        Serial.print(": ");
        Serial.print(activePixels);
        Serial.println(" active pixels");
    }

    // Render outer strips
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int activePixels = 0;

        // CRITICAL FIX: Explicitly check and print the active heat values for third segment
        if (segment == 2) {
            Serial.println("Checking heat values in third outer segment:");
            for (int i = 0; i < 10; i++) { // Just check first 10 LEDs
                int idx = segment * OUTER_LEDS_PER_STRIP + i;
                Serial.print("Pixel ");
                Serial.print(i);
                Serial.print(": Heat = ");
                Serial.println(heatOuter[idx]);
            }
        }

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

                    // Debug output for third segment
                    if (segment == 2 && i < 5) {
                        Serial.print("Setting third segment LED ");
                        Serial.print(i);
                        Serial.print(" (physical: ");
                        Serial.print(physicalPos);
                        Serial.print(") to color: ");
                        Serial.println(colorVal, HEX);
                    }
                }
            }
        }

        Serial.print("Outer segment ");
        Serial.print(segment);
        Serial.print(": ");
        Serial.print(activePixels);
        Serial.println(" active pixels");
    }

    // FORCE-ON TEST for third outer strip
    if (true) {
        Serial.println("FORCE-ON TEST: Setting first few LEDs in third outer strip");
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

    // Debug output for unusual mappings
    if (stripType == 2 && subStrip == 2 && position < 5) {
        Serial.print("Mapping outer strip ");
        Serial.print(subStrip);
        Serial.print(", position ");
        Serial.print(position);
        Serial.print(" to physical position: ");
        Serial.println(result);
    }

    return result;
}

void FireEffect::setIntensity(byte newIntensity) {
    // Clamp intensity to 0-100
    intensity = constrain(newIntensity, 0, 100);
}