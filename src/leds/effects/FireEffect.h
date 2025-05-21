#ifndef FIRE_EFFECT_H
#define FIRE_EFFECT_H

#include "Effect.h"

class FireEffect : public Effect {
public:
    FireEffect(LEDController& ledController);

    void update() override;
};

#endif // FIRE_EFFECT_H