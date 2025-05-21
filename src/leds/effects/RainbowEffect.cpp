#include "RainbowEffect.h"

RainbowEffect::RainbowEffect(LEDController& ledController) :
  Effect(ledController),
  cycle(0)
{
}

void RainbowEffect::reset() {
    cycle = 0;
}

void RainbowEffect::update() {
    // 65536 / 256 = 256 full rotations
    uint16_t baseHue = cycle * 256;

    // Core strip - gradient around the strip
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        int pixelHue = baseHue + (i * 65536 / LED_STRIP_CORE_COUNT);
        leds.getCore().setPixelColor(i, leds.colorHSV(pixelHue, 255, 255));
    }

    // Inner strip - gradient across all inner strips
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        int pixelHue = baseHue + (i * 65536 / LED_STRIP_INNER_COUNT);
        leds.getInner().setPixelColor(i, leds.colorHSV(pixelHue, 255, 255));
    }

    // Outer strip - gradient across all outer strips
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        int pixelHue = baseHue + (i * 65536 / LED_STRIP_OUTER_COUNT);
        leds.getOuter().setPixelColor(i, leds.colorHSV(pixelHue, 255, 255));
    }

    // Ring strip - gradient around the ring
    for (int i = 0; i < LED_STRIP_RING_COUNT; i++) {
        int pixelHue = baseHue + (i * 65536 / LED_STRIP_RING_COUNT);
        leds.getRing().setPixelColor(i, leds.colorHSV(pixelHue, 255, 255));
    }

    // Show the LEDs
    leds.showAll();

    // Increment cycle
    cycle = (cycle + 1) % 256;
}