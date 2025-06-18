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

void MPR121LEDHandler::showEffectSelectionSmart(int currentEffect, int totalEffects, bool isPartyMode, unsigned long showTime) {
    // Set up feedback timing
    feedbackStartTime = millis();
    feedbackDuration = showTime;
    feedbackActive = true;

    // Check if this is the party cycle effect (index 0 in party mode)
    if (isPartyMode && currentEffect == 0) {
        // Use special party cycle display
        applyPartyCycleDisplay();
        Serial.println("Effect selection feedback: Party Cycle (All Effects)");
    } else {
        // Use normal selection display for all other effects
        applySelectionToRing(currentEffect, totalEffects);
        Serial.print("Effect selection feedback: ");
        Serial.print(currentEffect + 1);
        Serial.print(" of ");
        Serial.println(totalEffects);
    }

    // Show the changes immediately
    leds.showAll();
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
        // Clear only the button face LEDs (11-22) on the ring
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
        case 0:  return STATE_OFF_COLOR;    // Red
        case 1:  return STATE_LOW_COLOR;    // Blue
        case 2:  return STATE_MED_COLOR;    // Yellow
        case 3:  return STATE_HIGH_COLOR;   // Orange
        default: return STATE_OFF_COLOR;    // Fallback to red
    }
}

uint8_t MPR121LEDHandler::calculateBellCurveBrightness(int position) {
    // Calculate bell curve brightness for the button face
    // Position should be 0 to BUTTON_FACE_COUNT-1

    // Center position of the bell curve
    float center = (BUTTON_FACE_COUNT - 1) / 2.0f;

    // Calculate distance from center
    float distance = abs(position - center);

    // Bell curve calculation (Gaussian-like)
    // Using a simpler formula: brightness = max * exp(-distance^2 / width^2)
    float width = BUTTON_FACE_COUNT / 3.0f; // Controls the width of the bell
    float normalizedDistance = distance / width;
    float brightness = exp(-normalizedDistance * normalizedDistance);

    // Scale to 0-255 range with minimum brightness of 30
    return (uint8_t)(30 + brightness * 225);
}

void MPR121LEDHandler::applyFeedbackToRing(uint32_t color) {
    // Convert 32-bit color to CRGB
    CRGB baseColor = CRGB(
        (color >> 16) & 0xFF,  // Red
        (color >> 8) & 0xFF,   // Green
        color & 0xFF           // Blue
    );

    // Clear the entire ring first
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        leds.getRing()[i] = CRGB::Black;
    }

    // Apply bell curve brightness across the button face
    for (int i = 0; i < BUTTON_FACE_COUNT; i++) {
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

void MPR121LEDHandler::applyPartyCycleDisplay() {
    // First, clear the entire ring to ensure clean display
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        leds.getRing()[i] = CRGB::Black;
    }

    // Representative colors for each party effect (same as PartyCycleEffect)
    // Order: lust, emerald, suspendedPartyFire, codeRed, matrix,
    // technoOrange, rainbowTrance, partyFire, rainbow, future, futureRainbow, rgbPattern
    CRGB effectColors[] = {
        CRGB(255, 20, 147),    // Lust Effect - Deep pink/magenta
        CRGB(0, 255, 127),     // Emerald City - Emerald green
        CRGB(255, 69, 0),      // Suspended Party Fire - Orange red
        CRGB(220, 20, 60),     // Code Red - Crimson red
        CRGB(0, 255, 0),       // Matrix - Bright green
        CRGB(255, 140, 0),     // Techno Orange (Regal) - Dark orange
        CRGB(138, 43, 226),    // Rainbow Trance - Blue violet
        CRGB(255, 0, 0),       // Party Fire - Red
        CRGB(255, 255, 0),     // Rainbow - Yellow (center of rainbow)
        CRGB(0, 191, 255),     // Future - Deep sky blue
        CRGB(255, 0, 255),     // Future Rainbow - Magenta
        CRGB(128, 0, 128)      // RGB Pattern - Purple
    };

    int numEffects = sizeof(effectColors) / sizeof(effectColors[0]);

    // Static brightness for button feedback (no breathing during feedback)
    uint8_t brightness = 180; // Bright enough to be clearly visible

    // Distribute effect colors across the notification LEDs
    for (int i = 0; i < BUTTON_FACE_COUNT; i++) {
        int ringIndex = BUTTON_FACE_START + i;

        // Map LED position to effect index
        // This spreads all effects across the 12 LEDs
        int effectIndex = (i * numEffects) / BUTTON_FACE_COUNT;
        if (effectIndex >= numEffects) effectIndex = numEffects - 1;

        // Get the base color for this effect
        CRGB baseColor = effectColors[effectIndex];

        // Apply consistent brightness
        baseColor.nscale8_video(brightness);

        // Set the LED
        leds.getRing()[ringIndex] = baseColor;
    }
}

CRGB MPR121LEDHandler::getItemColor(int itemIndex, int totalItems) {
    // Generate a color for the item using hue spectrum
    // Map item index to hue (0-255 in FastLED)
    uint8_t hue = (itemIndex * 255) / totalItems;

    // Use full saturation and brightness
    CHSV hsvColor(hue, 255, 200); // Bright but not max to avoid overpowering

    // Convert to RGB
    CRGB rgbColor;
    hsv2rgb_rainbow(hsvColor, rgbColor);

    return rgbColor;
}

uint8_t MPR121LEDHandler::calculateMiniBellCurveBrightness(int position, int groupSize) {
    // Calculate bell curve brightness for a small group of LEDs
    if (groupSize <= 1) {
        return 255; // Single LED gets full brightness
    }

    // Center position of the mini bell curve
    float center = (groupSize - 1) / 2.0f;

    // Calculate distance from center
    float distance = abs(position - center);

    // Bell curve calculation for small groups
    float width = groupSize / 2.5f; // Controls the width of the mini bell
    float normalizedDistance = distance / width;
    float brightness = exp(-normalizedDistance * normalizedDistance);

    // Scale to 0-255 range with minimum brightness of 60 for small groups
    return (uint8_t)(60 + brightness * 195);
}