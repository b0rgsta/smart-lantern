// src/leds/MPR121LEDHandler.cpp

#include "MPR121LEDHandler.h"
#include <math.h>

MPR121LEDHandler::MPR121LEDHandler(LEDController& ledController) :
    leds(ledController),
    feedbackStartTime(0),
    feedbackDuration(0),
    feedbackActive(false)
{
    // Constructor - no LED clearing here as requested
}

void MPR121LEDHandler::showTemperatureState(int state, unsigned long showTime) {
    // Get the color for this temperature state
    uint32_t color = getStateColor(state);

    // Set up feedback timing
    feedbackStartTime = millis();
    feedbackDuration = showTime;
    feedbackActive = true;

    // Apply the feedback display
    applyFeedbackToRing(color);

    // Show the changes immediately
    leds.showAll();

    // Debug output
    String stateNames[] = {"OFF", "LOW (18°C)", "MEDIUM (10°C)", "HIGH (5°C)"};
    Serial.print("Temperature feedback: ");
    Serial.println(stateNames[constrain(state, 0, 3)]);
}

void MPR121LEDHandler::showLightState(int state, unsigned long showTime) {
    // Get the color for this light sensor state
    uint32_t color = getStateColor(state);

    // Set up feedback timing
    feedbackStartTime = millis();
    feedbackDuration = showTime;
    feedbackActive = true;

    // Apply the feedback display
    applyFeedbackToRing(color);

    // Show the changes immediately
    leds.showAll();

    // Debug output
    String stateNames[] = {"OFF", "LOW", "MEDIUM", "HIGH"};
    Serial.print("Light sensor feedback: ");
    Serial.println(stateNames[constrain(state, 0, 3)]);
}

void MPR121LEDHandler::showModeSelection(int currentMode, int totalModes, unsigned long showTime) {
    // Set up feedback timing
    feedbackStartTime = millis();
    feedbackDuration = showTime;
    feedbackActive = true;

    // Apply the selection display for modes
    applySelectionToRing(currentMode, totalModes);

    // Show the changes immediately
    leds.showAll();

    // Debug output
    Serial.print("Mode selection feedback: ");
    Serial.print(currentMode + 1); // Display as 1-based for user friendliness
    Serial.print(" of ");
    Serial.println(totalModes);
}

void MPR121LEDHandler::showEffectSelection(int currentEffect, int totalEffects, unsigned long showTime) {
    // Set up feedback timing
    feedbackStartTime = millis();
    feedbackDuration = showTime;
    feedbackActive = true;

    // Apply the selection display for effects
    applySelectionToRing(currentEffect, totalEffects);

    // Show the changes immediately
    leds.showAll();

    // Debug output
    Serial.print("Effect selection feedback: ");
    Serial.print(currentEffect + 1); // Display as 1-based for user friendliness
    Serial.print(" of ");
    Serial.println(totalEffects);
}

void MPR121LEDHandler::update() {
    // Check if feedback should be cleared due to timeout
    if (feedbackActive) {
        unsigned long currentTime = millis();

        // Check if feedback duration has expired
        if (currentTime - feedbackStartTime >= feedbackDuration) {
            clearFeedback();
        }
    }
}

void MPR121LEDHandler::clearFeedback() {
    if (feedbackActive) {
        // Clear only the button face LEDs (38-55) on the ring
        for (int i = BUTTON_FACE_START; i <= BUTTON_FACE_END; i++) {
            leds.getRing()[i] = CRGB::Black;
        }

        // Update the display
        leds.showAll();

        // Mark feedback as inactive
        feedbackActive = false;

        Serial.println("Button feedback cleared");
    }
}

bool MPR121LEDHandler::isFeedbackActive() const {
    return feedbackActive;
}

uint32_t MPR121LEDHandler::getStateColor(int state) {
    // Return appropriate color based on state
    switch (constrain(state, 0, 3)) {
        case 0:  return STATE_OFF_COLOR;   // Red for off
        case 1:  return STATE_LOW_COLOR;   // Blue for low
        case 2:  return STATE_MED_COLOR;   // Yellow for medium
        case 3:  return STATE_HIGH_COLOR;  // Orange for high
        default: return STATE_OFF_COLOR;   // Default to red
    }
}

uint8_t MPR121LEDHandler::calculateBellCurveBrightness(int position) {
    // Calculate bell curve (Gaussian distribution) for smooth brightness falloff
    // Position should be 0 to BUTTON_FACE_COUNT-1

    // Center of the bell curve (middle of button face)
    float center = (BUTTON_FACE_COUNT - 1) / 2.0f;

    // Standard deviation - controls width of bell curve
    // Smaller value = sharper peak, larger value = wider spread
    float standardDeviation = BUTTON_FACE_COUNT / 4.0f;

    // Distance from center
    float distance = position - center;

    // Calculate Gaussian function: e^(-(distance^2)/(2*sigma^2))
    float exponent = -(distance * distance) / (2 * standardDeviation * standardDeviation);
    float gaussianValue = exp(exponent);

    // Convert to brightness (0-255 range)
    // Add minimum brightness so even edges are slightly visible
    uint8_t minBrightness = 20;  // Minimum 8% brightness at edges
    uint8_t maxBrightness = 255; // Full brightness at center

    uint8_t brightness = minBrightness + (gaussianValue * (maxBrightness - minBrightness));

    return brightness;
}

void MPR121LEDHandler::applyFeedbackToRing(uint32_t color) {
    // First, clear the entire ring to ensure clean feedback display
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        leds.getRing()[i] = CRGB::Black;
    }

    // Convert base color to CRGB for manipulation
    CRGB baseColor = leds.neoColorToCRGB(color);

    // Apply bell curve pattern to button face LEDs (38-55)
    for (int i = 0; i < BUTTON_FACE_COUNT; i++) {
        // Calculate ring LED index
        int ringIndex = BUTTON_FACE_START + i;

        // Calculate brightness for this position using bell curve
        uint8_t brightness = calculateBellCurveBrightness(i);

        // Create colored LED with calculated brightness
        CRGB ledColor = baseColor;

        // Scale the color by brightness (maintains color ratio while dimming)
        ledColor.nscale8_video(brightness);

        // Set the LED
        leds.getRing()[ringIndex] = ledColor;
    }
}

void MPR121LEDHandler::applySelectionToRing(int selectedIndex, int totalItems) {
    // First, clear the entire ring to ensure clean display
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        leds.getRing()[i] = CRGB::Black;
    }

    // Calculate how many LEDs each item should occupy
    // We divide the display area equally among all items
    float ledsPerItem = (float)BUTTON_FACE_COUNT / totalItems;

    // Calculate the center position for the selected item
    float selectedCenter = (selectedIndex + 0.5f) * ledsPerItem;

    // Determine how many LEDs to light up for this item
    // We want only a fraction of the space allocated to each item
    int ledsToLight = max(1, (int)(ledsPerItem / totalItems)); // At least 1 LED, but scale down for many items

    // For better visual effect, cap the maximum at 3 LEDs per item
    ledsToLight = min(ledsToLight, 3);

    // Get the color for this selected item
    CRGB itemColor = getItemColor(selectedIndex, totalItems);

    // Light up the LEDs around the center position with mini bell curve
    for (int i = 0; i < ledsToLight; i++) {
        // Calculate the LED position relative to center
        int ledOffset = i - (ledsToLight - 1) / 2; // Centers the LEDs around selectedCenter
        int ledPosition = (int)(selectedCenter + ledOffset);

        // Make sure we stay within the display area
        if (ledPosition >= 0 && ledPosition < BUTTON_FACE_COUNT) {
            int ringIndex = BUTTON_FACE_START + ledPosition;

            // Calculate brightness using mini bell curve
            uint8_t brightness = calculateMiniBellCurveBrightness(i, ledsToLight);

            // Apply brightness to the color
            CRGB ledColor = itemColor;
            ledColor.nscale8_video(brightness);

            // Set the LED
            leds.getRing()[ringIndex] = ledColor;
        }
    }
}

CRGB MPR121LEDHandler::getItemColor(int itemIndex, int totalItems) {
    // Calculate hue based on position in the spectrum
    // We spread the items evenly across the full hue range (0-255 in FastLED)
    uint8_t hue = (itemIndex * 255) / totalItems;

    // Return color with full saturation and brightness
    // The brightness will be adjusted later by the mini bell curve
    return CHSV(hue, 255, 255);
}

uint8_t MPR121LEDHandler::calculateMiniBellCurveBrightness(int position, int groupSize) {
    // For very small groups, just return full brightness
    if (groupSize <= 1) {
        return 255;
    }

    // Calculate bell curve for this mini group
    // Center of the mini bell curve
    float center = (groupSize - 1) / 2.0f;

    // Smaller standard deviation for tighter bell curve
    float standardDeviation = groupSize / 3.0f;

    // Distance from center
    float distance = position - center;

    // Calculate Gaussian function
    float exponent = -(distance * distance) / (2 * standardDeviation * standardDeviation);
    float gaussianValue = exp(exponent);

    // Convert to brightness with higher minimum for visibility
    uint8_t minBrightness = 100;  // Minimum 40% brightness at edges
    uint8_t maxBrightness = 255;  // Full brightness at center

    uint8_t brightness = minBrightness + (gaussianValue * (maxBrightness - minBrightness));

    return brightness;
}