// src/leds/effects/RgbPatternEffect.h

#ifndef RGB_PATTERN_EFFECT_H
#define RGB_PATTERN_EFFECT_H

#include "Effect.h"

/**
 * RgbPatternEffect - Creates synchronized RGB patterns across all strips
 *
 * Features:
 * - Core: RGB dots moving upward with size transitions
 * - Inner: Same RGB pattern as core (synchronized with UV mapping)
 * - Ring: RGB pattern rotating continuously
 * - Outer: RGB breathing waves synchronized with dot size
 *
 * All effects use the same RGB color scheme for visual cohesion
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
    static const int MAX_DOT_SIZE = 8;       // Maximum dot size
    static const int GAP_SIZE = 6;           // Gap between color dots

    // Base pattern spacing (using max size for calculations)
    static const int PATTERN_SPACING = MAX_DOT_SIZE + GAP_SIZE;  // Space for one color + gap (14)
    static const int PATTERN_LENGTH = PATTERN_SPACING * 3;       // Total pattern length (42)

    // Core and inner animation variables (shared)
    float scrollPosition;                    // Current scroll position (float for smooth movement)
    float sizePhase;                        // Phase for size transition (0 to 2*PI)
    unsigned long lastUpdateTime;           // Last time the animation was updated

    // Ring-specific scroll position
    float ringScrollPosition;               // Current scroll position for ring

    // Animation speeds
    static constexpr float SCROLL_SPEED = 0.15f;      // Speed of upward movement (LEDs per frame)
    static constexpr float SIZE_SPEED = 0.02f;        // Speed of size transitions
    static constexpr float RING_SCROLL_SPEED = 0.155f;// Ring rotation speed

    // Outer strip breathing wave variables
    float outerBreathingPhase;                     // Current phase of breathing cycle
    static constexpr float BREATHING_SPEED = 0.02f; // Same as SIZE_SPEED for synchronization

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
     * Map a UV coordinate to pattern position
     * @param uvPosition Position from 0.0 to 1.0 along the strip
     * @return Pattern position that maps to this UV coordinate
     */
    float uvToPatternPosition(float uvPosition);

    /**
     * Get color at a specific UV position
     * @param uvPosition Position from 0.0 to 1.0 along the strip
     * @param dotSize Current size of dots
     * @return The color for that UV position
     */
    CRGB getColorAtUV(float uvPosition, int dotSize);

    /**
     * Draw the pattern on a single core segment
     * @param segment Which core segment to draw on (0, 1, or 2)
     */
    void drawCoreSegment(int segment);

    /**
     * Draw the pattern on a single inner strip segment
     * @param segment Which inner strip segment to draw on (0, 1, or 2)
     */
    void drawInnerSegment(int segment);

    /**
     * Draw the pattern on the ring strip
     */
    void drawRing();

    /**
     * Update and draw outer strip breathing waves
     */
    void updateOuterWaves();

    /**
     * Get RGB color based on color index
     * @param colorIndex 0=red, 1=green, 2=blue
     * @return The corresponding RGB color
     */
    CRGB getIndexedColor(int colorIndex);
};

#endif // RGB_PATTERN_EFFECT_H