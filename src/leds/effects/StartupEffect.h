#ifndef STARTUP_EFFECT_H
#define STARTUP_EFFECT_H

#include "Effect.h"

class StartupEffect : public Effect {
public:
    StartupEffect(LEDController& ledController);

    void update() override;
    void reset() override;

    bool isComplete() { return complete; }

    String getName() const override { return "Startup Effect"; }
private:
    int position;
    unsigned long lastUpdate;
    bool complete;
};

#endif // STARTUP_EFFECT_H