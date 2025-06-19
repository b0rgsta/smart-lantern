// File: src/leds/effects/GradientEffect.cpp

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
    // Nothing to reset for gradient effects
}

// Main update method that applies gradients and fade overlay
void GradientEffect::update() {
    // Apply gradients to each strip type
    applyGradient(leds.getCore(), LED_STRIP_CORE_COUNT, coreGradient);
    applyGradient(leds.getInner(), LED_STRIP_INNER_COUNT, innerGradient);
    applyGradient(leds.getOuter(), LED_STRIP_OUTER_COUNT, outerGradient);
    
    // Skip ring if disabled
    if (!skipRing) {
        applyGradient(leds.getRing(), LED_STRIP_RING_COUNT, ringGradient);
    }

    // Apply fade overlay to outer strips (now fades to 90% black instead of complete black)
    applyOuterBlackFadeOverlay();

    // Display all LED changes
    leds.showAll();
}

// Apply black fade overlay to outer strips for ambient lighting effect
// MODIFIED: Now fades to 90% black (10% brightness) instead of complete black
void GradientEffect::applyOuterBlackFadeOverlay() {
    // Only apply fade if outer gradient is not empty (outer strips are active)
    if (outerGradient.empty()) {
        return;
    }

    // Apply fade overlay to each outer strip segment
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int segmentStart = segment * OUTER_LEDS_PER_STRIP;

        for (int i = 0; i < OUTER_LEDS_PER_STRIP; i++) {
            int ledIndex = segmentStart + i;

            // Start fade at 45% up the strip (same as fire effects for consistency)
            float fadeStartPosition = OUTER_LEDS_PER_STRIP * 0.45f;

            if (i >= fadeStartPosition) {
                // Calculate fade progress from fade start to top of strip
                float fadeProgress = (float(i) - fadeStartPosition) / (OUTER_LEDS_PER_STRIP - fadeStartPosition);

                // Apply cubic fade for smooth dramatic transition
                fadeProgress = fadeProgress * fadeProgress * fadeProgress;

                // Calculate fade factor: 1.0 = full brightness, 0.1 = 90% black (10% brightness)
                // CHANGED: Minimum brightness is now 0.1 instead of 0.0
                float fadeFactor = 1.0f - (fadeProgress * 0.9f);  // Fade from 1.0 to 0.1

                // Apply fade to the existing LED color
                leds.getOuter()[ledIndex].nscale8_video((uint8_t)(255 * fadeFactor));

                // MODIFIED: Top 10% of strip fades to 90% black instead of complete black
                if (i >= OUTER_LEDS_PER_STRIP * 0.90f) {
                    // Scale the current color to 10% brightness (90% black)
                    leds.getOuter()[ledIndex].nscale8_video(26);  // 26/255 ≈ 10% brightness
                }
            }
        }
    }
}

// Apply a gradient to any LED strip (maintains original segmented approach)
void GradientEffect::applyGradient(CRGB* strip, int count, const Gradient& gradient) {
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

    // Determine which strip we're dealing with by checking count
    bool isCore = (count == LED_STRIP_CORE_COUNT);
    bool isInner = (count == LED_STRIP_INNER_COUNT);
    bool isOuter = (count == LED_STRIP_OUTER_COUNT);

    if (isCore) {
        // For the core strip, divide it into 3 segments
        int coreSegmentLength = LED_STRIP_CORE_COUNT / 3;

        // First segment (0 to coreSegmentLength-1)
        for (int i = 0; i < coreSegmentLength; i++) {
            // Position within this segment (0.0 to 1.0)
            float position = (float)i / (coreSegmentLength - 1);
            applyGradientToPosition(strip, i, position, gradient);
        }

        // Second segment (coreSegmentLength to 2*coreSegmentLength-1)
        for (int i = 0; i < coreSegmentLength; i++) {
            int index = coreSegmentLength + i;
            // Position within this segment (0.0 to 1.0)
            float position = (float)i / (coreSegmentLength - 1);
            applyGradientToPosition(strip, index, position, gradient);
        }

        // Third segment (2*coreSegmentLength to end)
        for (int i = 0; i < (LED_STRIP_CORE_COUNT - 2 * coreSegmentLength); i++) {
            int index = 2 * coreSegmentLength + i;
            // Position within this segment (0.0 to 1.0)
            float position = (float)i / ((LED_STRIP_CORE_COUNT - 2 * coreSegmentLength) - 1);
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
                float position = (float)i / (INNER_LEDS_PER_STRIP - 1);
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
                float position = (float)i / (OUTER_LEDS_PER_STRIP - 1);
                applyGradientToPosition(strip, index, position, gradient);
            }
        }
    } else {
        // For ring strip (the only remaining option), apply gradient across entire strip
        for (int i = 0; i < count; i++) {
            // Calculate position along the strip (0.0 to 1.0)
            float position = (float)i / (count - 1);
            applyGradientToPosition(strip, i, position, gradient);
        }
    }
}

// Apply gradient color to a specific LED position (uses original interpolation logic)
void GradientEffect::applyGradientToPosition(CRGB* strip, int index, float position, const Gradient& gradient) {
    // Find the gradient points to interpolate between
    int lowerIndex = 0;
    int upperIndex = 0;

    // Find the two gradient points that bracket this position
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

// Smoothly blend between two colors using linear interpolation
CRGB GradientEffect::interpolateColors(const CRGB& color1, const CRGB& color2, float ratio) {
    // Clamp ratio to valid range
    ratio = constrain(ratio, 0.0f, 1.0f);

    // Linear interpolation of RGB components
    return CRGB(
        color1.r + (color2.r - color1.r) * ratio,
        color1.g + (color2.g - color1.g) * ratio,
        color1.b + (color2.b - color1.b) * ratio
    );
}

// Static method to create first half of rainbow (red to cyan)
Gradient GradientEffect::createFirstHalfRainbowGradient() {
    Gradient gradient;

    // Build rainbow from red through yellow to cyan
    gradient.push_back(GradientPoint(0xFF0000, 0.0f));  // Red (hue 0)
    gradient.push_back(GradientPoint(0xFF8000, 0.25f)); // Orange (hue 32)
    gradient.push_back(GradientPoint(0xFFFF00, 0.5f));  // Yellow (hue 64)
    gradient.push_back(GradientPoint(0x00FF00, 0.75f)); // Green (hue 96)
    gradient.push_back(GradientPoint(0x00FFFF, 1.0f));  // Cyan (hue 128)

    return gradient;
}

// Static method to create second half of rainbow (cyan to red)
Gradient GradientEffect::createSecondHalfRainbowGradient() {
    Gradient gradient;

    // Build rainbow from cyan through magenta back to red
    gradient.push_back(GradientPoint(0x00FFFF, 0.0f));  // Cyan (hue 128)
    gradient.push_back(GradientPoint(0x0080FF, 0.25f)); // Light Blue (hue 160)
    gradient.push_back(GradientPoint(0x0000FF, 0.5f));  // Blue (hue 192)
    gradient.push_back(GradientPoint(0x8000FF, 0.75f)); // Purple (hue 224)
    gradient.push_back(GradientPoint(0xFF0000, 1.0f));  // Red (hue 256/0)

    return gradient;
}

// Static method to create fire gradient (red/orange/yellow)
Gradient GradientEffect::createFireGradient() {
    Gradient gradient;

    // Deep red at bottom transitioning to bright yellow at top
    gradient.push_back(GradientPoint(0x800000, 0.0f));  // Dark red
    gradient.push_back(GradientPoint(0xFF0000, 0.3f));  // Bright red
    gradient.push_back(GradientPoint(0xFF4000, 0.6f));  // Red-orange
    gradient.push_back(GradientPoint(0xFF8000, 0.8f));  // Orange
    gradient.push_back(GradientPoint(0xFFFF00, 1.0f));  // Yellow

    return gradient;
}

// Static method to create blue to white gradient
Gradient GradientEffect::createBlueToWhiteGradient() {
    Gradient gradient;

    // Cool blue transitioning to warm white
    gradient.push_back(GradientPoint(0x0000FF, 0.0f));  // Pure blue
    gradient.push_back(GradientPoint(0x4080FF, 0.5f));  // Light blue
    gradient.push_back(GradientPoint(0xFFFFFF, 1.0f));  // White

    return gradient;
}

// Static method to create sunset gradient
Gradient GradientEffect::createSunsetGradient() {
    Gradient gradient;

    // Warm sunset colors from deep orange to purple
    gradient.push_back(GradientPoint(0xFF4000, 0.0f));  // Deep orange
    gradient.push_back(GradientPoint(0xFF8000, 0.3f));  // Orange
    gradient.push_back(GradientPoint(0xFF8040, 0.6f));  // Peach
    gradient.push_back(GradientPoint(0x8040FF, 0.8f));  // Purple
    gradient.push_back(GradientPoint(0x400080, 1.0f));  // Deep purple

    return gradient;
}

// Static method to create Christmas gradient for core
Gradient GradientEffect::createCoreChristmasGradient() {
    Gradient gradient;

    // Traditional Christmas colors: red and green
    gradient.push_back(GradientPoint(0xFF0000, 0.0f));  // Red
    gradient.push_back(GradientPoint(0x00FF00, 0.5f));  // Green
    gradient.push_back(GradientPoint(0xFF0000, 1.0f));  // Red

    return gradient;
}

// Static method to create Christmas gradient for outer strips
Gradient GradientEffect::createOuterChristmasGradient() {
    Gradient gradient;

    // Christmas colors with gold accent
    gradient.push_back(GradientPoint(0x00FF00, 0.0f));  // Green
    gradient.push_back(GradientPoint(0xFFD700, 0.5f));  // Gold
    gradient.push_back(GradientPoint(0xFF0000, 1.0f));  // Red

    return gradient;
}

// Static method to create purple to blue gradient
Gradient GradientEffect::createPurpleToBlueGradient() {
    Gradient gradient;

    // Cool purple to blue transition
    gradient.push_back(GradientPoint(0x8000FF, 0.0f));  // Purple
    gradient.push_back(GradientPoint(0x4080FF, 0.5f));  // Purple-blue
    gradient.push_back(GradientPoint(0x0080FF, 1.0f));  // Blue

    return gradient;
}

// Static method to create blue to purple gradient
Gradient GradientEffect::createBlueToPurpleGradient() {
    Gradient gradient;

    // Reverse of purple to blue
    gradient.push_back(GradientPoint(0x0080FF, 0.0f));  // Blue
    gradient.push_back(GradientPoint(0x4080FF, 0.5f));  // Purple-blue
    gradient.push_back(GradientPoint(0x8000FF, 1.0f));  // Purple

    return gradient;
}

// Helper method to reverse any gradient
Gradient GradientEffect::reverseGradient(const Gradient& gradient) {
    Gradient reversed;

    // Iterate through original gradient in reverse order
    for (int i = gradient.size() - 1; i >= 0; i--) {
        // Flip the position: 0.0 becomes 1.0, 1.0 becomes 0.0, etc.
        float newPosition = 1.0f - gradient[i].position;
        reversed.push_back(GradientPoint(gradient[i].color, newPosition));
    }

    return reversed;
}

// Static method to create rainbow gradient with specified number of points
// This creates a full rainbow spectrum but skips green section for better color distribution
Gradient GradientEffect::createRainbowGradient(int numPoints) {
    Gradient gradient;

    // Create rainbow gradient with specified number of points
    for (int i = 0; i < numPoints; i++) {
        // Calculate position along gradient (0.0 to 1.0)
        float position = (float)i / (numPoints - 1);

        // Skip green section: Start from hue 43 (yellow) and go to hue 171 (blue)
        // This gives us yellow → orange → red → magenta → blue (skipping green)
        float hueRange = 171 - 43; // 128 hue units
        uint8_t hue = 43 + (position * hueRange);

        // Convert HSV to RGB (full saturation and value)
        CRGB rgbColor = CHSV(hue, 255, 255);

        // Convert to uint32_t format expected by GradientPoint
        uint32_t color = ((uint32_t)rgbColor.r << 16) | ((uint32_t)rgbColor.g << 8) | rgbColor.b;

        gradient.push_back(GradientPoint(color, position));
    }

    return gradient;
}

// Get the name of this effect (required by Effect base class)
String GradientEffect::getName() const {
    return "Gradient Effect";
}