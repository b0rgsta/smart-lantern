// src/leds/effects/MatrixEffect.h

#ifndef MATRIX_EFFECT_H
#define MATRIX_EFFECT_H

#include "Effect.h"
#include <vector>

// Structure to represent a falling drop
struct Drop {
    float position;     // Current position (float for smooth movement)
    float speed;        // Drop speed
    uint8_t hue;        // Color hue
    uint8_t brightness; // Current brightness
    bool isActive;      // Whether this drop is active
    bool isWhite;       // Special white flashing drops
};

// Structure for continuous matrix ring trails
struct MatrixRingTrail {
    float position;     // Current head position around the ring
    float speed;        // Movement speed (pixels per frame)
    uint8_t hue;        // Color hue for this trail
    bool active;        // Whether this trail is active
    unsigned long creationTime; // When this trail was created
    int trailLength;    // Length of the trail
};

class MatrixEffect : public Effect {
public:
    MatrixEffect(LEDController& ledController);
    ~MatrixEffect();

    void update() override;
    void reset() override;

    String getName() const override { return "Matrix Effect"; }
private:
    // Constants for the effect
    static const uint8_t TRAIL_LENGTH = 15;            // Length of each drop's trail
    static const uint8_t TRAIL_BRIGHTNESS = 60;        // Base brightness of the trails
    static const uint8_t MAX_DROPS_PER_STRIP = 5;      // Maximum number of active drops per strip
    static const uint8_t FLICKER_INTENSITY = 155;      // How much drops can flicker
    static const uint8_t FLICKER_CHANCE = 30;          // Chance of a flicker (out of 100)
    static const uint8_t WHITE_FLASH_CHANCE = 5;       // Chance of a white flash (out of 100)
    static const uint8_t WHITE_FLASH_MIN = 100;        // Minimum brightness for white flashes
    static const uint8_t HUE_ROTATION_SPEED = 1;       // Internal counter increment (1 per frame for 0.025 effective speed)

    // Arrays to hold drops for each strip type
    std::vector<Drop> coreDrops[3];  // Changed from single vector to array of 3
    std::vector<Drop> innerDrops[NUM_INNER_STRIPS];
    std::vector<Drop> outerDrops[NUM_OUTER_STRIPS];
    std::vector<Drop> ringDrops;

    // Ring trail system for continuous trails
    std::vector<MatrixRingTrail> ringTrails;
    static const uint8_t MAX_RING_TRAILS = 3;           // Maximum number of active ring trails
    static const uint8_t RING_TRAIL_LENGTH = 8;         // Length of each ring trail
    static const unsigned long RING_TRAIL_FADEIN = 1000;       // 1 second to fade in
    static const unsigned long RING_TRAIL_LIFESPAN = 6000;     // 6 seconds at full brightness
    static const unsigned long RING_TRAIL_FADEOUT = 2000;      // 2 seconds to fade out completely
    unsigned long lastRingTrailCreateTime;              // Last time we created a ring trail

    // Color palette - updated to use CRGB
    static const uint8_t NUM_COLORS = 5;
    CRGB colorPalette[NUM_COLORS];

    // State variables
    uint16_t hueCounter;    // Counter for precise hue rotation (scaled to achieve 0.3 speed)
    uint8_t baseHue;        // Current base hue calculated from counter
    unsigned long lastUpdate;
    unsigned long lastHueUpdate;

    // Speed range (in pixels per frame)
    static constexpr float MIN_SPEED = 0.1f;
    static constexpr float MAX_SPEED = 0.3f;

    // Helper methods
    void updateColorPalette();
    void createDrop(int stripType, int subStrip = 0);
    void updateStrip(int stripType, int subStrip = 0);
    void renderDrop(Drop& drop, int stripType, int subStrip, int stripLength);

    // Ring-specific methods for continuous trails
    void updateRingTrails();
    void createNewRingTrail();
    void drawRingTrails();
};

#endif // MATRIX_EFFECT_H