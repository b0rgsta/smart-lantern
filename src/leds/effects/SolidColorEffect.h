// src/leds/effects/SolidColorEffect.h

#ifndef SOLID_COLOR_EFFECT_H
#define SOLID_COLOR_EFFECT_H

#include "Effect.h"

class SolidColorEffect : public Effect {
public:
    // Constructor with a single color for all strips
    SolidColorEffect(LEDController& ledController, uint32_t color);

    // Constructor with individual colors for each strip
    // Pass COLOR_NONE (0xFF000000) to turn off a specific strip
    SolidColorEffect(LEDController& ledController,
                     uint32_t coreColor,
                     uint32_t innerColor,
                     uint32_t outerColor,
                     uint32_t ringColor);

    void update() override;
    void reset() override;

    // Setters for individual strip colors
    void setCoreColor(uint32_t color);
    void setInnerColor(uint32_t color);
    void setOuterColor(uint32_t color);
    void setRingColor(uint32_t color);

    // Setter for all strips at once
    void setAllColors(uint32_t color);

    String getName() const override;

    // Special color value to indicate a strip should be turned off
    static const uint32_t COLOR_NONE = 0xFF000000;
    // Predefined white color temperatures
    static const uint32_t COLD_WHITE = 0xF0F8FF;     // RGB(240, 248, 255) - Slight blue tint
    static const uint32_t NATURAL_WHITE = 0xFFFFFF;  // RGB(255, 255, 255) - Pure white
    static const uint32_t WARM_WHITE = 0xFFE8C0;     // RGB(255, 232, 192) - Slight yellow/orange tint

private:

    // Colors for each strip
    uint32_t coreColor;
    uint32_t innerColor;
    uint32_t outerColor;
    uint32_t ringColor;

    // Helper methods
    bool isValidColor(uint32_t color) const;
    void applyColor(CRGB* strip, int count, uint32_t color);
};

#endif // SOLID_COLOR_EFFECT_H