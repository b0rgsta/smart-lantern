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

class MatrixEffect : public Effect {
public:
    MatrixEffect(LEDController& ledController);
    ~MatrixEffect();

    void update() override;
    void reset() override;

private:
    // Constants for the effect
    static const uint8_t TRAIL_LENGTH = 15;            // Length of each drop's trail
    static const uint8_t TRAIL_BRIGHTNESS = 60;        // Base brightness of the trails
    static const uint8_t MAX_DROPS_PER_STRIP = 5;      // Maximum number of active drops per strip
    static const uint8_t FLICKER_INTENSITY = 155;      // How much drops can flicker
    static const uint8_t FLICKER_CHANCE = 30;          // Chance of a flicker (out of 100)
    static const uint8_t WHITE_FLASH_CHANCE = 5;       // Chance of a white flash (out of 100)
    static const uint8_t WHITE_FLASH_MIN = 100;        // Minimum brightness for white flashes
    static const uint8_t HUE_ROTATION_SPEED = 1;       // Speed of hue rotation

    // Arrays to hold drops for each strip type
    std::vector<Drop> coreDrops;
    std::vector<Drop> innerDrops[NUM_INNER_STRIPS];
    std::vector<Drop> outerDrops[NUM_OUTER_STRIPS];
    std::vector<Drop> ringDrops;

    // Color palette - updated to use CRGB
    static const uint8_t NUM_COLORS = 5;
    CRGB colorPalette[NUM_COLORS];
    
    // State variables
    uint8_t baseHue;
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
};

#endif // MATRIX_EFFECT_H