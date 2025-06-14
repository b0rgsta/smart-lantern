// src/leds/effects/RgbPatternEffect.h

#ifndef RGB_PATTERN_EFFECT_H
#define RGB_PATTERN_EFFECT_H

#include "Effect.h"

/**
 * RgbPatternEffect - Creates an upward-moving RGB pattern on core strips
 *
 * Features:
 * - Pattern: Red dots, gap, green dots, gap, blue dots, gap
 * - Dots transition in size from 2 to 6 LEDs as they move up
 * - Pattern continuously moves upward on all 3 core segments
 * - All core segments show the same synchronized animation
 * - Smooth scrolling and size transitions
 */
class RgbPatternEffect : public Effect {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller
     */
    RgbPatternEffect(LEDController& ledController);

    /**
     * Update the animation
     */
    void update() override;

    /**
     * Reset the effect to initial state
     */
    void reset() override;

    /**
     * Get the name of this effect
     */
    String getName() const override { return "RGB Pattern Effect"; }

private:
    // Pattern constants
    static const int BASE_DOT_SIZE = 2;      // Minimum dot size
    static const int MAX_DOT_SIZE = 8;       // Maximum dot size (increased from 4)
    static const int GAP_SIZE = 6;          // Gap between color dots

    // Base pattern spacing (using max size for calculations)
    static const int PATTERN_SPACING = MAX_DOT_SIZE + GAP_SIZE;  // Space for one color + gap (16)
    static const int PATTERN_LENGTH = PATTERN_SPACING * 3;       // Total pattern length (48)

    // Animation variables
    float scrollPosition;                  // Current scroll position (float for smooth movement)
    float sizePhase;                      // Phase for size transition (0 to 2*PI)
    unsigned long lastUpdateTime;          // Last time the animation was updated

    // Animation speed
    static constexpr float SCROLL_SPEED = 0.15f;     // Speed of upward movement (LEDs per frame)
    static constexpr float SIZE_SPEED = 0.02f;       // Speed of size transitions

    /**
     * Calculate the current dot size based on the size phase
     * @return Current dot size (between BASE_DOT_SIZE and MAX_DOT_SIZE)
     */
    int getCurrentDotSize();

    /**
     * Check if a position is part of a color dot
     * @param position Position to check
     * @param dotSize Current size of dots
     * @param colorIndex Which color (0=red, 1=green, 2=blue)
     * @return True if position is part of the specified color dot
     */
    bool isColorDot(float position, int dotSize, int colorIndex);

    /**
     * Get the color for a specific position
     * @param position Position to check
     * @param dotSize Current size of dots
     * @return The color for that position (red, green, blue, or black)
     */
    CRGB getColorAtPosition(float position, int dotSize);

    /**
     * Draw the pattern on a single core segment
     * @param segment Which core segment to draw on (0, 1, or 2)
     */
    void drawSegment(int segment);
};

#endif // RGB_PATTERN_EFFECT_H