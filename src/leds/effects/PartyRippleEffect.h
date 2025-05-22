// src/leds/effects/PartyRippleEffect.h

#ifndef PARTY_RIPPLE_EFFECT_H
#define PARTY_RIPPLE_EFFECT_H

#include "Effect.h"
#include <vector>

// Structure to track trails
struct PartyTrail {
    float position;     // Current head position (float for smooth movement)
    int length;         // Trail length
    uint16_t hue;       // Color hue (0-65535)
    bool active;        // Is trail active?
    bool direction;     // Direction (true=forward, false=backward)
    int subStrip;       // Which segment (0-2)
};

// Structure to track ripples
struct Ripple {
    int position;       // Center position
    int size;           // Current size (radius)
    int maxSize;        // Maximum size before splitting
    bool hasSplit;      // Whether this ripple has split yet
    bool active;        // Is ripple active
    uint16_t hue;       // Color hue
    int leftPosition;   // Position of left side after split
    int rightPosition;  // Position of right side after split
    int width;          // Width of ripple
};

class PartyRippleEffect : public Effect {
public:
    PartyRippleEffect(LEDController& ledController);
    ~PartyRippleEffect();

    void update() override;
    void reset() override;

    String getName() const override { return "Party Ripple Effect"; }

private:
    // Trail parameters
    static const int MAX_OUTER_TRAILS = 20;
    static const int MAX_INNER_TRAILS = 20;
    static const int TRAIL_LENGTH = 10;

    // Ripple parameters
    static const int MAX_RIPPLES = 10;
    static const int RIPPLE_MAX_SIZE = 5;
    static const int RIPPLE_WIDTH = 5;

    // Timing
    unsigned long lastRippleTime;
    unsigned long rippleInterval;  // 2 seconds

    // Collections
    std::vector<PartyTrail> outerTrails;
    std::vector<PartyTrail> innerTrails;
    std::vector<Ripple> ripples;

    // Helper methods
    void createOuterTrail();
    void createInnerTrail();
    void createRipple();
    void updateOuterTrails();
    void updateInnerTrails();
    void updateRipples();

    // Color helpers
    CRGB getTrailHeadColor(uint16_t hue, bool isInner);
    CRGB getTrailColor(uint16_t hue, float brightness, bool isInner);
    CRGB getRippleColor(uint16_t hue, float brightness);
};

#endif // PARTY_RIPPLE_EFFECT_H