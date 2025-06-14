// src/leds/effects/RgbPatternEffect.h

#ifndef RGB_PATTERN_EFFECT_H
#define RGB_PATTERN_EFFECT_H

#include "Effect.h"

/**
 * RgbPatternEffect - Creates synchronized RGB patterns across all strips
 *
 * Features:
 * - Core: RGB dots moving upward with size transitions
 * - Inner: Breathing cycle through R->G->B colors (NEW!)
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

    // Core strip properties
    static const int CORE_LEDS_PER_SEGMENT = 50;

    // Note: The following are defined in Config.h:
    // INNER_LEDS_PER_STRIP = 75
    // NUM_INNER_STRIPS = 3
    // OUTER_LEDS_PER_STRIP = 75
    // NUM_OUTER_STRIPS = 3
    // LED_STRIP_RING_COUNT (for ring)

    // Animation speeds (REDUCED for slower animation)
    static constexpr float SCROLL_SPEED = 0.3f;       // Core upward scroll speed (was 0.8f)
    static constexpr float RING_SCROLL_SPEED = 0.3f;  // Ring rotation speed (reduced by 25% from 0.4f)
    static constexpr float SIZE_SPEED = 0.008f;       // Dot size change speed (was 0.015f)

    // NEW: Inner strip breathing constants
    static constexpr float INNER_BREATHING_SPEED = 0.015f;  // Speed of breathing cycle (slower)
    static constexpr float INNER_MIN_BRIGHTNESS = 0.0f;     // Minimum brightness (0% - full black)
    static constexpr float INNER_MAX_BRIGHTNESS = 0.3f;     // Maximum brightness (50% - half brightness)

    // Animation state variables
    float scrollPosition;          // Current scroll position for core/inner
    float ringScrollPosition;      // Current scroll position for ring
    float sizePhase;              // Phase for dot size animation
    unsigned long lastUpdateTime;  // Last update timestamp
    float outerBreathingPhase;    // Phase for outer strip breathing

    // NEW: Inner strip breathing state
    float innerBreathingPhase;     // Phase for inner strip breathing (0 to 9*PI for full RGB cycle with pauses)

    // Helper methods
    int getCurrentDotSize();
    CRGB getColorAtPosition(float position, int dotSize);
    float uvToPatternPosition(float uvPosition);
    CRGB getColorAtUV(float uvPosition, int dotSize);
    void drawCoreSegment(int segment);
    void drawInnerSegment(int segment);
    void drawRing();
    void updateOuterWaves();
    CRGB getIndexedColor(int colorIndex);

    // NEW: Helper method for inner strip breathing
    void updateInnerBreathing();
};

#endif // RGB_PATTERN_EFFECT_H