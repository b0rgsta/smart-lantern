#ifndef TRAILS_EFFECT_H
#define TRAILS_EFFECT_H

#include "Effect.h"

// Structure to track active trails
struct Trail {
    int stripId;         // Which strip (0=core, 1=inner, 2=outer, 3=ring)
    int position;        // Current head position
    int length;          // Trail length (pixels)
    int hue;             // Color hue (0-65535)
    bool active;         // Is trail active?
    bool direction;      // Direction (true=forward, false=backward)
    int subStrip;        // For inner/outer: which of the 3 strips (0-2)
};

class TrailsEffect : public Effect {
public:
    TrailsEffect(LEDController& ledController, int maxTrails = 20, int trailLength = 10);
    ~TrailsEffect();

    void update() override;
    void reset() override;

    String getName() const override { return "Trails Effect"; }
private:
    void createNewTrail();

    Trail* trails;
    int maxTrails;
    int trailLength;
};

#endif // TRAILS_EFFECT_H