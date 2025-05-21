#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Config.h"

class LEDController {
public:
    LEDController();

    void begin();
    void clearAll();
    void setBrightness(uint8_t brightness);
    uint32_t colorHSV(uint16_t hue, uint8_t sat, uint8_t val);
    int mapPositionToPhysical(int stripId, int logicalPos, int subStrip);

    // Methods to access LED strips
    Adafruit_NeoPixel& getCore() { return stripCore; }
    Adafruit_NeoPixel& getInner() { return stripInner; }
    Adafruit_NeoPixel& getOuter() { return stripOuter; }
    Adafruit_NeoPixel& getRing() { return stripRing; }

    // Update to display changes on all strips
    void showAll();

private:
    Adafruit_NeoPixel stripCore;
    Adafruit_NeoPixel stripInner;
    Adafruit_NeoPixel stripOuter;
    Adafruit_NeoPixel stripRing;
};

#endif // LED_CONTROLLER_H