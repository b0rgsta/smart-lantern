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