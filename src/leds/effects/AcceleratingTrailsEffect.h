// src/leds/effects/AcceleratingTrailsEffect.h

#ifndef ACCELERATING_TRAILS_EFFECT_H
#define ACCELERATING_TRAILS_EFFECT_H

#include "Effect.h"
#include <vector>

// Structure to track active accelerating trails
struct AccelTrail {
    int stripId;         // Which strip (1=inner, 2=outer)
    float position;      // Current head position (using float for smoother acceleration)
    int length;          // Trail length (pixels)
    uint16_t hue;        // Color hue (0-65535)
    bool active;         // Is trail active?
    float velocity;      // Current velocity (pixels per update)
    float acceleration;  // Acceleration (increase in velocity per update)
    int subStrip;        // Which of the 3 strips (0-2)
};

class AcceleratingTrailsEffect : public Effect {
public:
    AcceleratingTrailsEffect(LEDController& ledController,
                            int minTrails = 6,
                            int maxTrails = 20,
                            int trailLength = 15);
    ~AcceleratingTrailsEffect();

    void update() override;
    void reset() override;

private:
    void createNewTrail();
    void ensureMinimumTrails();

    std::vector<AccelTrail> trails;
    int minTrails;      // Minimum number of active trails
    int maxTrails;      // Maximum number of active trails
    int trailLength;    // Length of each trail

    // Parameters for trail creation and movement
    float minVelocity;  // Minimum starting velocity
    float maxVelocity;  // Maximum starting velocity
    float minAccel;     // Minimum acceleration
    float maxAccel;     // Maximum acceleration
};

#endif // ACCELERATING_TRAILS_EFFECT_H