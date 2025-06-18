// src/leds/effects/GradientEffect.h

#ifndef GRADIENT_EFFECT_H
#define GRADIENT_EFFECT_H

#include "Effect.h"
#include <vector>

// Structure to define a color at a specific position
struct GradientPoint {
    uint32_t color;      // The color at this point
    float position;      // Position (0.0 to 1.0) along the strip

    // Constructor for easy initialization
    GradientPoint(uint32_t c, float pos) : color(c), position(pos) {}
};

// Define a Gradient as a vector of gradient points
typedef std::vector<GradientPoint> Gradient;

class GradientEffect : public Effect {
public:
    // Constructor with one gradient for all strips
    GradientEffect(LEDController& ledController,
                  const Gradient& gradient,
                  bool applyToCore = true,
                  bool applyToInner = true,
                  bool applyToOuter = true,
                  bool applyToRing = true);

    // Constructor with individual gradients for each strip
    // Pass empty gradient to turn off a specific strip
    GradientEffect(LEDController& ledController,
                  const Gradient& coreGradient,
                  const Gradient& innerGradient,
                  const Gradient& outerGradient,
                  const Gradient& ringGradient);

    void update() override;
    void reset() override;

    // Setters for individual strip gradients
    void setCoreGradient(const Gradient& gradient);
    void setInnerGradient(const Gradient& gradient);
    void setOuterGradient(const Gradient& gradient);
    void setRingGradient(const Gradient& gradient);

    // Setter for all strips at once
    void setAllGradients(const Gradient& gradient);

    String getName() const override;

    // Predefined gradients for easy use
    static Gradient createRainbowGradient(int numPoints = 6);
    static Gradient createFirstHalfRainbowGradient();    // Red to cyan (first half of spectrum)
    static Gradient createSecondHalfRainbowGradient();   // Cyan to red (second half of spectrum)
    static Gradient createFireGradient();
    static Gradient createBlueToWhiteGradient();
    static Gradient createSunsetGradient();
    static Gradient createCoreChristmasGradient();
    static Gradient createOuterChristmasGradient();
    static Gradient createPurpleToBlueGradient();
    static Gradient createBlueToPurpleGradient();

    // Helper method to reverse a gradient
    static Gradient reverseGradient(const Gradient& gradient);

private:
    // Gradients for each strip
    Gradient coreGradient;
    Gradient innerGradient;
    Gradient outerGradient;
    Gradient ringGradient;

    // Helper methods
    void applyGradient(CRGB* strip, int count, const Gradient& gradient);
    void applyGradientToPosition(CRGB* strip, int index, float position, const Gradient& gradient);
    void applyOuterBlackFadeOverlay();  // Apply black fade overlay to outer strips for ambient effect
    CRGB interpolateColors(const CRGB& color1, const CRGB& color2, float ratio);
};

#endif // GRADIENT_EFFECT_H