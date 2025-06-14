#include "GradientEffect.h"


// Constructor with a single gradient for multiple strips
GradientEffect::GradientEffect(LEDController &ledController,
                               const Gradient &gradient,
                               bool applyToCore,
                               bool applyToInner,
                               bool applyToOuter,
                               bool applyToRing) : Effect(ledController) {
    // Apply gradient to selected strips, leave others empty
    if (applyToCore) coreGradient = gradient;
    if (applyToInner) innerGradient = gradient;
    if (applyToOuter) outerGradient = gradient;
    if (applyToRing) ringGradient = gradient;
}

// Constructor with different gradient for each strip
GradientEffect::GradientEffect(LEDController &ledController,
                               const Gradient &coreGradient,
                               const Gradient &innerGradient,
                               const Gradient &outerGradient,
                               const Gradient &ringGradient) : Effect(ledController),
                                                               coreGradient(coreGradient),
                                                               innerGradient(innerGradient),
                                                               outerGradient(outerGradient),
                                                               ringGradient(ringGradient) {
    // No additional initialization needed
}

void GradientEffect::reset() {
    // Nothing to reset
}

void GradientEffect::update() {
    // Apply gradients to each strip
    applyGradient(leds.getCore(), LED_STRIP_CORE_COUNT, coreGradient);
    applyGradient(leds.getInner(), LED_STRIP_INNER_COUNT, innerGradient);
    applyGradient(leds.getOuter(), LED_STRIP_OUTER_COUNT, outerGradient);
    if (!skipRing)
        applyGradient(leds.getRing(), LED_STRIP_RING_COUNT, ringGradient);

    // Show all changes
    leds.showAll();
}

void GradientEffect::setCoreGradient(const Gradient &gradient) {
    coreGradient = gradient;
}

void GradientEffect::setInnerGradient(const Gradient &gradient) {
    innerGradient = gradient;
}

void GradientEffect::setOuterGradient(const Gradient &gradient) {
    outerGradient = gradient;
}

void GradientEffect::setRingGradient(const Gradient &gradient) {
    ringGradient = gradient;
}

void GradientEffect::setAllGradients(const Gradient &gradient) {
    coreGradient = gradient;
    innerGradient = gradient;
    outerGradient = gradient;
    ringGradient = gradient;
}

String GradientEffect::getName() const {
    return "Gradient Effect";
}

void GradientEffect::applyGradient(CRGB *strip, int count, const Gradient &gradient) {
    // If gradient is empty, turn off the strip
    if (gradient.empty()) {
        fill_solid(strip, count, CRGB::Black);
        return;
    }

    // If only one color in gradient, fill with that color
    if (gradient.size() == 1) {
        fill_solid(strip, count, leds.neoColorToCRGB(gradient[0].color));
        return;
    }

    // Determine which strip we're dealing with
    bool isCore = (count == LED_STRIP_CORE_COUNT);
    bool isInner = (count == LED_STRIP_INNER_COUNT);
    bool isOuter = (count == LED_STRIP_OUTER_COUNT);

    if (isCore) {
        // For the core strip, we'll divide it into 3 sections
        int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;

        // First segment (0 to coreSegmentLength-1)
        for (int i = 0; i < coreSegmentLength; i++) {
            // Position within this segment (0.0 to 1.0)
            float position = (float) i / (coreSegmentLength - 1);
            applyGradientToPosition(strip, i, position, gradient);
        }

        // Second segment (coreSegmentLength to 2*coreSegmentLength-1)
        for (int i = 0; i < coreSegmentLength; i++) {
            int index = coreSegmentLength + i;
            // Position within this segment (0.0 to 1.0)
            float position = (float) i / (coreSegmentLength - 1);
            applyGradientToPosition(strip, index, position, gradient);
        }

        // Third segment (2*coreSegmentLength to end)
        for (int i = 0; i < (LED_STRIP_CORE_COUNT - 2 * coreSegmentLength); i++) {
            int index = 2 * coreSegmentLength + i;
            // Position within this segment (0.0 to 1.0)
            float position = (float) i / ((LED_STRIP_CORE_COUNT - 2 * coreSegmentLength) - 1);
            applyGradientToPosition(strip, index, position, gradient);
        }
    } else if (isInner) {
        // Apply gradient to each inner strip segment separately
        for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
            int segmentStart = segment * INNER_LEDS_PER_STRIP;

            // Apply gradient to this segment
            for (int i = 0; i < INNER_LEDS_PER_STRIP; i++) {
                int index = segmentStart + i;
                // Position within this segment (0.0 to 1.0)
                float position = (float) i / (INNER_LEDS_PER_STRIP - 1);
                applyGradientToPosition(strip, index, position, gradient);
            }
        }
    } else if (isOuter) {
        // Apply gradient to each outer strip segment separately
        for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
            int segmentStart = segment * OUTER_LEDS_PER_STRIP;

            // Apply gradient to this segment
            for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
                int index = segmentStart + i;
                // Position within this segment (0.0 to 1.0)
                float position = (float) i / (OUTER_LEDS_PER_STRIP - 1);
                applyGradientToPosition(strip, index, position, gradient);
            }
        }
    } else {
        // For ring strip (the only remaining option), apply gradient across entire strip
        for (int i = 0; i < count; i++) {
            // Calculate position along the strip (0.0 to 1.0)
            float position = (float) i / (count - 1);
            applyGradientToPosition(strip, i, position, gradient);
        }
    }
}

// Helper method to apply gradient at a specific position
void GradientEffect::applyGradientToPosition(CRGB *strip, int index, float position, const Gradient &gradient) {
    // Find the gradient points to interpolate between
    int lowerIndex = 0;
    int upperIndex = 0;

    for (int j = 0; j < gradient.size() - 1; j++) {
        if (position >= gradient[j].position && position <= gradient[j + 1].position) {
            lowerIndex = j;
            upperIndex = j + 1;
            break;
        }
    }

    // Convert colors from uint32_t to CRGB
    CRGB color1 = leds.neoColorToCRGB(gradient[lowerIndex].color);
    CRGB color2 = leds.neoColorToCRGB(gradient[upperIndex].color);

    // Calculate interpolation ratio
    float lowerPos = gradient[lowerIndex].position;
    float upperPos = gradient[upperIndex].position;
    float ratio = (position - lowerPos) / (upperPos - lowerPos);

    // Apply the interpolated color
    strip[index] = interpolateColors(color1, color2, ratio);
}

// Helper for color interpolation
CRGB GradientEffect::interpolateColors(const CRGB &color1, const CRGB &color2, float ratio) {
    // Linear interpolation between two colors
    uint8_t r = color1.r + (color2.r - color1.r) * ratio;
    uint8_t g = color1.g + (color2.g - color1.g) * ratio;
    uint8_t b = color1.b + (color2.b - color1.b) * ratio;

    return CRGB(r, g, b);
}

// Static method to create a rainbow gradient
Gradient GradientEffect::createRainbowGradient(int numPoints) {
    Gradient gradient;

    for (int i = 0; i < numPoints; i++) {
        float position = (float) i / (numPoints - 1);
        // Calculate hue (0-255) based on position
        uint8_t hue = 255 * position;
        // Convert HSV to RGB (full saturation and value)
        CRGB rgbColor = CHSV(hue, 255, 255);
        // Convert to uint32_t format
        uint32_t color = ((uint32_t) rgbColor.r << 16) | ((uint32_t) rgbColor.g << 8) | rgbColor.b;

        gradient.push_back(GradientPoint(color, position));
    }

    return gradient;
}

// Static method to create a fire gradient (red to yellow)
Gradient GradientEffect::createFireGradient() {
    Gradient gradient;

    // Dark red at the bottom
    gradient.push_back(GradientPoint(0x800000, 0.0f));
    // Bright red in the middle
    gradient.push_back(GradientPoint(0xFF0000, 0.3f));
    // Orange
    gradient.push_back(GradientPoint(0xFF8000, 0.6f));
    // Yellow at the top
    gradient.push_back(GradientPoint(0xFFFF00, 1.0f));

    return gradient;
}

// Static method to create a blue to white gradient
Gradient GradientEffect::createBlueToWhiteGradient() {
    Gradient gradient;

    // Deep blue at the bottom
    gradient.push_back(GradientPoint(0x0000FF, 0.0f));
    // Lighter blue in the middle
    gradient.push_back(GradientPoint(0x4040FF, 0.3f));
    // Light blue
    gradient.push_back(GradientPoint(0x8080FF, 0.6f));
    // White at the top
    gradient.push_back(GradientPoint(0xFFFFFF, 1.0f));

    return gradient;
}

// Static method to create a sunset gradient (oranges, reds, purples)
Gradient GradientEffect::createSunsetGradient() {
    Gradient gradient;

    // Yellow at the bottom
    gradient.push_back(GradientPoint(0xFFFF00, 0.0f));
    // Orange
    gradient.push_back(GradientPoint(0xFF8800, 0.25f));
    // Deep orange/red
    gradient.push_back(GradientPoint(0xFF4400, 0.5f));
    // Pink/rose
    gradient.push_back(GradientPoint(0xFF0088, 0.75f));
    // Purple at the top
    gradient.push_back(GradientPoint(0x800080, 1.0f));

    return gradient;
}


// Static method to create a Christmas gradient (red & green)
Gradient GradientEffect::createOuterChristmasGradient() {
    Gradient gradient;

    // Deep red
    gradient.push_back(GradientPoint(0xAA0000, 0.0f));
    // Bright red
    gradient.push_back(GradientPoint(0xFF0000, 0.25f));
    // Black (for transition)
    gradient.push_back(GradientPoint(0x000000, 0.5f));
    // Bright green
    gradient.push_back(GradientPoint(0x00FF00, 0.75f));
    // Deep green
    gradient.push_back(GradientPoint(0x006600, 1.0f));

    return gradient;
}

// Static method to create a Christmas gradient (red & green)
Gradient GradientEffect::createCoreChristmasGradient() {
    Gradient gradient;

    // Add red at position 0.0
    gradient.emplace_back(0x000000, 0.0f);

    // Alternate white and red, properly spaced
    for (unsigned int i = 0; i < 9; i++) {
        // White at positions 0.2, 0.6, 1.0, etc.
        gradient.emplace_back(0xFFFFFF, 0.1f + (i * 0.2f));

        // Red at positions 0.4, 0.8, etc.
        if (i < 8) {
            gradient.emplace_back(0x000000, 0.3f + (i * 0.2f));
        }
    }

    return gradient;
}

// Static method to create purple to blue gradient
Gradient GradientEffect::createPurpleToBlueGradient() {
    Gradient gradient;

    // Purple
    gradient.push_back(GradientPoint(0x800080, 0.0f));
    // Blue
    gradient.push_back(GradientPoint(0x0000FF, 1.0f));

    return gradient;
}

// Static method to create blue to purple gradient (reversed)
Gradient GradientEffect::createBlueToPurpleGradient() {
    Gradient gradient;

    // Blue
    gradient.push_back(GradientPoint(0x0000FF, 0.0f));
    // Purple
    gradient.push_back(GradientPoint(0x800080, 1.0f));

    return gradient;
}

// Helper for reversing a gradient
Gradient GradientEffect::reverseGradient(const Gradient &gradient) {
    Gradient reversed;

    for (const auto &point: gradient) {
        // Add each point with inverted position (1.0 - original position)
        reversed.push_back(GradientPoint(point.color, 1.0f - point.position));
    }

    // Sort by position to ensure proper interpolation
    std::sort(reversed.begin(), reversed.end(),
              [](const GradientPoint &a, const GradientPoint &b) {
                  return a.position < b.position;
              });

    return reversed;
}
