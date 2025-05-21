#ifndef EFFECT_H
#define EFFECT_H

#include "../LEDController.h"

class Effect {
public:
    Effect(LEDController& ledController) : leds(ledController) {}
    virtual ~Effect() {}

    virtual void update() = 0;  // Must be implemented by derived classes
    virtual void reset() {}     // Optional to implement

protected:
    LEDController& leds;
};

#endif // EFFECT_H