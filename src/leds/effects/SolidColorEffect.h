// src/leds/effects/SolidColorEffect.h

#ifndef SOLID_COLOR_EFFECT_H
#define SOLID_COLOR_EFFECT_H

#include "Effect.h"

class SolidColorEffect : public Effect {
public:
    SolidColorEffect(LEDController& ledController, uint32_t color);

    void update() override;
    void reset() override;
    void setColor(uint32_t color);

private:
    uint32_t color;
};

#endif // SOLID_COLOR_EFFECT_H