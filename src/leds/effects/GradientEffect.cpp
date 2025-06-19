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

// Updated update method to include black fade overlay for outer strips
void GradientEffect::update() {
    // Apply gradients to each strip
    applyGradient(leds.getCore(), LED_STRIP_CORE_COUNT, coreGradient);
    applyGradient(leds.getInner(), LED_STRIP_INNER_COUNT, innerGradient);
    applyGradient(leds.getOuter(), LED_STRIP_OUTER_COUNT, outerGradient);
    if (!skipRing)
        applyGradient(leds.getRing(), LED_STRIP_RING_COUNT, ringGradient);

    // Apply black fade overlay to outer strips (like fire effects)
    applyOuterBlackFadeOverlay();

    // Show all changes
    leds.showAll();
}
// Apply black fade overlay to outer strips for ambient lighting effect
void GradientEffect::applyOuterBlackFadeOverlay() {
    // Only apply if outer gradient is not empty (outer strips are active)
    if (outerGradient.empty()) {
        return;
    }

    // Apply fade-to-black overlay to outer strips
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;

            // Calculate fade starting at 45% up the strip (like fire effects)
            float fadeStartPosition = OUTER_LEDS_PER_STRIP * 0.45f;

            if (i >= fadeStartPosition) {
                // Calculate fade progress from fade start to top of strip
                float fadeProgress = (float(i) - fadeStartPosition) / (OUTER_LEDS_PER_STRIP - fadeStartPosition);

                // Apply aggressive cubic fade for dramatic black transition
                fadeProgress = fadeProgress * fadeProgress * fadeProgress;

                // Calculate fade factor (1.0 = full brightness, 0.0 = black)
                float fadeFactor = 1.0f - fadeProgress;

                // Apply fade to the existing LED color
                leds.getOuter()[ledIndex].nscale8_video((uint8_t)(255 * fadeFactor));

                // Force top 10% of strip to be completely black
                if (i >= OUTER_LEDS_PER_STRIP * 0.90f) {
                    leds.getOuter()[ledIndex] = CRGB::Black;
                }
            }
        }
    }
}
// Static method to create first half of rainbow (red to cyan)
Gradient GradientEffect::createFirstHalfRainbowGradient() {
    Gradient gradient;

    // Red (hue 0)
    gradient.push_back(GradientPoint(0xFF0000, 0.0f));
    // Orange (hue 32)
    gradient.push_back(GradientPoint(0xFF8000, 0.25f));
    // Yellow (hue 64)
    gradient.push_back(GradientPoint(0xFFFF00, 0.5f));
    // Green (hue 96)
    gradient.push_back(GradientPoint(0x00FF00, 0.75f));
    // Cyan (hue 128)
    gradient.push_back(GradientPoint(0x00FFFF, 1.0f));

    return gradient;
}

// Static method to create second half of rainbow (cyan to red)
Gradient GradientEffect::createSecondHalfRainbowGradient() {
    Gradient gradient;

    // Cyan (hue 128)
    gradient.push_back(GradientPoint(0x00FFFF, 0.0f));
    // Blue (hue 160)
    gradient.push_back(GradientPoint(0x0000FF, 0.25f));
    // Purple (hue 192)
    gradient.push_back(GradientPoint(0x8000FF, 0.5f));
    // Magenta (hue 224)
    gradient.push_back(GradientPoint(0xFF00FF, 0.75f));
    // Red (hue 255/0)
    gradient.push_back(GradientPoint(0xFF0000, 1.0f));

    return gradient;
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
        // Skip green section: Start from hue 43 (yellow) and go to hue 171 (blue)
        // This gives us yellow → orange → red → magenta → blue (skipping green)
        float hueRange = 171 - 43; // 128 hue units
        uint8_t hue = 43 + (position * hueRange);
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

    // Dark navy blue at the bottom
    gradient.push_back(GradientPoint(0x0B1426, 0.0f));
    // Medium blue
    gradient.push_back(GradientPoint(0x1E3A5F, 0.33f));
    // Light peach
    gradient.push_back(GradientPoint(0xFFCBA4, 0.67f));
    // Bright peach at the top
    gradient.push_back(GradientPoint(0xFFB07A, 1.0f));

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

Gradient GradientEffect::createPurpleToBlueGradient() {
    Gradient gradient;

    // Deep blue-purple (dark indigo) - start
    gradient.push_back(GradientPoint(0x2E1A47, 0.0f));
    // Medium purple-blue blend - 1/3 position
    gradient.push_back(GradientPoint(0x4A2C6A, 0.33f));
    // Warm orange (sunset orange) - 2/3 position
    gradient.push_back(GradientPoint(0xFF6B35, 0.67f));
    // Bright sunset orange - end
    gradient.push_back(GradientPoint(0xFF8C42, 1.0f));

    return gradient;
}

// Static method to create blue to purple gradient (reversed - now sunset orange to deep blue/purple)
Gradient GradientEffect::createBlueToPurpleGradient() {
    Gradient gradient;

    // Bright sunset orange - start
    gradient.push_back(GradientPoint(0xFF8C42, 0.0f));
    // Warm orange (sunset orange) - 1/3 position
    gradient.push_back(GradientPoint(0xFF6B35, 0.33f));
    // Medium purple-blue blend - 2/3 position
    gradient.push_back(GradientPoint(0x4A2C6A, 0.67f));
    // Deep blue-purple (dark indigo) - end
    gradient.push_back(GradientPoint(0x2E1A47, 1.0f));

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
