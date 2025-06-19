// File: src/leds/effects/GradientEffect.h

#ifndef GRADIENT_EFFECT_H
#define GRADIENT_EFFECT_H

#include "Effect.h"
#include <vector>

// Structure to define a color at a specific position in a gradient
struct GradientPoint {
    uint32_t color;      // The color at this point (in 0xRRGGBB format)
    float position;      // Position (0.0 to 1.0) along the strip

    // Constructor for easy initialization
    GradientPoint(uint32_t c, float pos) : color(c), position(pos) {}
};

// Define a Gradient as a vector of gradient points
typedef std::vector<GradientPoint> Gradient;

/**
 * GradientEffect - Applies color gradients to LED strips
 *
 * This effect allows you to create smooth color transitions across LED strips.
 * You can apply the same gradient to multiple strips or use different gradients
 * for each strip type. The outer strips automatically get a fade-to-black overlay
 * for ambient lighting effects.
 *
 * Features:
 * - Smooth color interpolation between gradient points
 * - Individual gradient control for each strip type
 * - Automatic fade overlay on outer strips (now fades to 90% black)
 * - Multiple predefined gradient patterns
 * - Easy gradient reversal for opposing effects
 */
class GradientEffect : public Effect {
public:
    // Constructor with one gradient applied to selected strips
    GradientEffect(LEDController& ledController,
                  const Gradient& gradient,
                  bool applyToCore = true,
                  bool applyToInner = true,
                  bool applyToOuter = true,
                  bool applyToRing = true);

    // Constructor with individual gradients for each strip type
    // Pass empty gradient to disable a specific strip
    GradientEffect(LEDController& ledController,
                  const Gradient& coreGradient,
                  const Gradient& innerGradient,
                  const Gradient& outerGradient,
                  const Gradient& ringGradient);

    /**
     * Update the effect - applies gradients and fade overlay
     * Called every frame to render the gradient effect
     */
    void update() override;

    /**
     * Reset the effect to initial state
     * For gradients, there's nothing to reset since they're static
     */
    void reset() override;

    /**
     * Get the name of this effect for debugging/display
     * @return The effect name as a string
     */
    String getName() const override;

    // Setters for individual strip gradients (allows runtime changes)
    void setCoreGradient(const Gradient& gradient) { coreGradient = gradient; }
    void setInnerGradient(const Gradient& gradient) { innerGradient = gradient; }
    void setOuterGradient(const Gradient& gradient) { outerGradient = gradient; }
    void setRingGradient(const Gradient& gradient) { ringGradient = gradient; }

    // Setter to apply the same gradient to all strips
    void setAllGradients(const Gradient& gradient);

    // Predefined gradient creators for easy use
    static Gradient createRainbowGradient(int numPoints = 6);     // Full rainbow with customizable points
    static Gradient createFirstHalfRainbowGradient();            // Red to cyan (first half of spectrum)
    static Gradient createSecondHalfRainbowGradient();           // Cyan to red (second half of spectrum)
    static Gradient createFireGradient();                        // Fire colors (red/orange/yellow)
    static Gradient createBlueToWhiteGradient();                 // Cool blue to warm white
    static Gradient createSunsetGradient();                      // Sunset colors (orange/peach/purple)
    static Gradient createCoreChristmasGradient();               // Christmas colors for core
    static Gradient createOuterChristmasGradient();              // Christmas colors for outer
    static Gradient createPurpleToBlueGradient();                // Purple to blue transition
    static Gradient createBlueToPurpleGradient();                // Blue to purple transition

    // Helper method to reverse any gradient (flips positions)
    static Gradient reverseGradient(const Gradient& gradient);

private:
    // Individual gradients for each strip type
    Gradient coreGradient;
    Gradient innerGradient;
    Gradient outerGradient;
    Gradient ringGradient;

    // Core gradient application methods
    void applyGradient(CRGB* strip, int count, const Gradient& gradient);
    void applyGradientToPosition(CRGB* strip, int index, float position, const Gradient& gradient);

    // Special effect for outer strips - fades to 90% black for ambient lighting
    void applyOuterBlackFadeOverlay();

    // Color interpolation helper
    CRGB interpolateColors(const CRGB& color1, const CRGB& color2, float ratio);
};

#endif // GRADIENT_EFFECT_H