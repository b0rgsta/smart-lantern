// src/leds/effects/TemperatureColorEffect.cpp

#include "TemperatureColorEffect.h"

TemperatureColorEffect::TemperatureColorEffect(LEDController& ledController,
                                             uint16_t temperatureK,
                                             bool enableCore,
                                             bool enableInner,
                                             bool enableOuter,
                                             bool enableRing) :
    Effect(ledController),
    temperature(temperatureK),
    coreEnabled(enableCore),
    innerEnabled(enableInner),
    outerEnabled(enableOuter),
    ringEnabled(enableRing),
    needsUpdate(true)  // Need to update on first frame
{
    // Calculate the RGB color from temperature
    calculatedColor = kelvinToRGB(temperature);

    // Log the creation
    Serial.print("TemperatureColorEffect created with temperature: ");
    Serial.print(temperature);
    Serial.println("K");
}

void TemperatureColorEffect::reset() {
    // For static effect, just mark that we need to update
    needsUpdate = true;
}

void TemperatureColorEffect::update() {
    if (!shouldUpdate(500)) {
        return;
    }

    // Clear all LEDs first
    leds.clearAll();

    // Apply color to core strip if enabled
    if (coreEnabled) {
        applySolidColor(leds.getCore(), LED_STRIP_CORE_COUNT, calculatedColor);
    }

    // Apply color to inner strips if enabled
    if (innerEnabled) {
        applySolidColor(leds.getInner(), LED_STRIP_INNER_COUNT, calculatedColor);
    }

    // Apply color with fade to outer strips if enabled
    if (outerEnabled) {
        applyFadeToOuter(leds.getOuter(), LED_STRIP_OUTER_COUNT, calculatedColor);
    }

    // Apply color to ring strip if enabled (unless skipped for button feedback)
    if (ringEnabled && !skipRing) {
        applySolidColor(leds.getRing(), LED_STRIP_RING_COUNT, calculatedColor);
    }

    // Show the LEDs
    leds.showAll();

    // Mark update as complete
    needsUpdate = false;
}

void TemperatureColorEffect::setTemperature(uint16_t temperatureK) {
    // Only update if temperature actually changed
    if (temperature != temperatureK) {
        temperature = temperatureK;
        calculatedColor = kelvinToRGB(temperature);
        needsUpdate = true;

        Serial.print("Temperature changed to: ");
        Serial.print(temperature);
        Serial.println("K");
    }
}

String TemperatureColorEffect::getName() const {
    // Include temperature in the name for debugging
    return "Temperature Color (" + String(temperature) + "K)";
}

CRGB TemperatureColorEffect::kelvinToRGB(uint16_t kelvin) {
    // Algorithm based on Tanner Helland's implementation
    // Constrain input to reasonable range
    kelvin = constrain(kelvin, 1000, 40000);

    // Temperature in hundreds of Kelvin
    float temp = kelvin / 100.0f;

    float red, green, blue;

    // Calculate red
    if (temp <= 66) {
        red = 255;
    } else {
        // Note: the formula uses 329.698727446 as a magic number
        red = temp - 60;
        red = 329.698727446f * pow(red, -0.1332047592f);
        red = constrain(red, 0, 255);
    }

    // Calculate green
    if (temp <= 66) {
        green = temp;
        green = 99.4708025861f * log(green) - 161.1195681661f;
        green = constrain(green, 0, 255);
    } else {
        green = temp - 60;
        green = 288.1221695283f * pow(green, -0.0755148492f);
        green = constrain(green, 0, 255);
    }

    // Calculate blue
    if (temp >= 66) {
        blue = 255;
    } else if (temp <= 19) {
        blue = 0;
    } else {
        blue = temp - 10;
        blue = 138.5177312231f * log(blue) - 305.0447927307f;
        blue = constrain(blue, 0, 255);
    }

    return CRGB((uint8_t)red, (uint8_t)green, (uint8_t)blue);
}

void TemperatureColorEffect::applySolidColor(CRGB* strip, int count, CRGB color) {
    // Simply fill the entire strip with the solid color
    fill_solid(strip, count, color);
}

void TemperatureColorEffect::applyFadeToOuter(CRGB* strip, int count, CRGB color) {
    // Apply fade to each of the 3 outer strip segments independently
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        // Apply fade within this segment
        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;

            // Calculate position ratio (0.0 at bottom, 1.0 at top)
            float positionRatio = (float)i / (OUTER_LEDS_PER_STRIP - 1);

            // Linear fade from bottom to top
            // Bottom (i=0) = full brightness (1.0)
            // Top (i=max) = no brightness (0.0)
            float brightness = 1.0f - positionRatio;

            // Optional: Apply a curve to make the fade more dramatic
            // Uncomment one of these for different fade curves:

            // Square curve - faster fade at the top
            // brightness = brightness * brightness;

            // Square root curve - faster fade at the bottom
            // brightness = sqrt(brightness);

            // S-curve - slow at edges, fast in middle
            // brightness = brightness * brightness * (3.0f - 2.0f * brightness);

            // Apply brightness to the color
            CRGB fadedColor = CRGB(
                (uint8_t)(color.r * brightness),
                (uint8_t)(color.g * brightness),
                (uint8_t)(color.b * brightness)
            );

            // Set the LED
            if (ledIndex < count) {
                strip[ledIndex] = fadedColor;
            }
        }
    }
}