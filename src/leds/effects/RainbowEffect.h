#ifndef RAINBOW_EFFECT_H
#define RAINBOW_EFFECT_H

#include "Effect.h"

class RainbowEffect : public Effect {
public:
    RainbowEffect(LEDController& ledController);

    void update() override;
    void reset() override;

private:
    uint16_t cycle;
};

#endif // RAINBOW_EFFECT_H