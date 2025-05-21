// src/leds/LEDController.h
#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <FastLED.h>
#include "Config.h"

class LEDController {
public:
    LEDController();

    void begin();
    void clearAll();
    void setBrightness(uint8_t brightness);
    uint32_t colorHSV(uint16_t hue, uint8_t sat, uint8_t val);
    int mapPositionToPhysical(int stripId, int logicalPos, int subStrip);

    // Methods to access LED arrays
    CRGB* getCore() { return ledsCore; }
    CRGB* getInner() { return ledsInner; }
    CRGB* getOuter() { return ledsOuter; }
    CRGB* getRing() { return ledsRing; }

    // Update to display changes on all strips
    void showAll();

    // Helper methods for color conversion between systems
    uint32_t color(uint8_t r, uint8_t g, uint8_t b);
    CRGB neoColorToCRGB(uint32_t color);
    uint32_t CRGBToNeoColor(CRGB color);

private:
    // LED arrays for each strip
    CRGB ledsCore[LED_STRIP_CORE_COUNT];
    CRGB ledsInner[LED_STRIP_INNER_COUNT];
    CRGB ledsOuter[LED_STRIP_OUTER_COUNT];
    CRGB ledsRing[LED_STRIP_RING_COUNT];

    uint8_t brightness;
};

#endif // LED_CONTROLLER_H