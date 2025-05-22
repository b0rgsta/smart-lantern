// src/leds/MPR121LEDHandler.h

#ifndef MPR121_LED_HANDLER_H
#define MPR121_LED_HANDLER_H

#include <Arduino.h>
#include <FastLED.h>
#include "LEDController.h"
#include "Config.h"

/**
 * MPR121LEDHandler - Manages ring LED feedback for button presses
 *
 * This class handles displaying visual feedback on the ring LEDs when buttons are pressed.
 * The feedback shows the current state of temperature and light sensor buttons using
 * different colors and a bell curve brightness pattern.
 *
 * Button face range: LEDs 38-55 on the ring strip (18 LEDs total)
 * Colors:
 * - Off state: Red
 * - Low state: Blue
 * - Medium state: Yellow
 * - High state: Orange
 */
class MPR121LEDHandler {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller for ring access
     */
    MPR121LEDHandler(LEDController& ledController);

    /**
     * Show temperature button state feedback
     * @param state Temperature button state (0=off, 1=low, 2=med, 3=high)
     * @param showTime How long to show the feedback in milliseconds (default 2000ms)
     */
    void showTemperatureState(int state, unsigned long showTime = 2000);

    /**
     * Show light sensor button state feedback
     * @param state Light sensor state (0=off, 1=low, 2=med, 3=high)
     * @param showTime How long to show the feedback in milliseconds (default 2000ms)
     */
    void showLightState(int state, unsigned long showTime = 2000);

    /**
     * Show mode selection feedback
     * @param currentMode Current mode index (0-based)
     * @param totalModes Total number of available modes
     * @param showTime How long to show the feedback in milliseconds (default 2000ms)
     */
    void showModeSelection(int currentMode, int totalModes, unsigned long showTime = 2000);

    /**
     * Show effect selection feedback
     * @param currentEffect Current effect index (0-based)
     * @param totalEffects Total number of available effects for current mode
     * @param showTime How long to show the feedback in milliseconds (default 2000ms)
     */
    void showEffectSelection(int currentEffect, int totalEffects, unsigned long showTime = 2000);

    /**
     * Update the LED handler - call this every frame
     * This handles timing for when to clear feedback displays
     */
    void update();

    /**
     * Clear any active feedback display immediately
     */
    void clearFeedback();

    /**
     * Check if feedback is currently being displayed
     * @return True if feedback is active, false otherwise
     */
    bool isFeedbackActive() const;

private:
    LEDController& leds;                // Reference to LED controller

    // Button face LED range on ring strip (our "display" area)
    static const int BUTTON_FACE_START = 38;    // First LED of button face
    static const int BUTTON_FACE_END = 55;      // Last LED of button face
    static const int BUTTON_FACE_COUNT = 18;    // Total LEDs in button face (55-38+1)

    // Feedback timing
    unsigned long feedbackStartTime;    // When current feedback started
    unsigned long feedbackDuration;     // How long to show feedback
    bool feedbackActive;                // Is feedback currently active

    // State colors for different button states
    static const uint32_t STATE_OFF_COLOR = 0xFF0000;      // Red
    static const uint32_t STATE_LOW_COLOR = 0x0000FF;      // Blue
    static const uint32_t STATE_MED_COLOR = 0xFFFF00;      // Yellow
    static const uint32_t STATE_HIGH_COLOR = 0xFF8000;     // Orange

    /**
     * Get color for a given state
     * @param state The button state (0-3)
     * @return Color as 32-bit RGB value
     */
    uint32_t getStateColor(int state);

    /**
     * Calculate bell curve brightness for position within button face
     * @param position Position within button face (0 to BUTTON_FACE_COUNT-1)
     * @return Brightness value (0-255)
     */
    uint8_t calculateBellCurveBrightness(int position);

    /**
     * Apply feedback display to ring LEDs using solid color
     * @param color Base color for the feedback
     */
    void applyFeedbackToRing(uint32_t color);

    /**
     * Apply selection display to ring LEDs using dynamic layout
     * @param selectedIndex Currently selected item (0-based)
     * @param totalItems Total number of items available
     */
    void applySelectionToRing(int selectedIndex, int totalItems);

    /**
     * Get color for a specific item index using full hue spectrum
     * @param itemIndex Index of the item (0-based)
     * @param totalItems Total number of items
     * @return Color as CRGB value
     */
    CRGB getItemColor(int itemIndex, int totalItems);

    /**
     * Calculate mini bell curve brightness for a small group of LEDs
     * @param position Position within the mini group
     * @param groupSize Size of the mini group
     * @return Brightness value (0-255)
     */
    uint8_t calculateMiniBellCurveBrightness(int position, int groupSize);
};

#endif // MPR121_LED_HANDLER_H