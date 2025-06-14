// src/leds/effects/TechnoOrangeEffect.cpp

#include "RegalEffect.h"

RegalEffect::RegalEffect(LEDController& ledController) : Effect(ledController) {
    // Initialize animation state and timing
    innerState = FILLING_UP;
    coreState = CORE_WAITING;
    innerAnimationStartTime = millis();
    coreAnimationStartTime = millis();
    innerFillPosition = 0;
    coreFillPosition = 0;
    outerBreathingStartTime = millis();

    // Initialize shimmer timing
    lastShimmerUpdate = millis();

    // Allocate memory for shimmer values array
    coreShimmerValues = new float[LED_STRIP_CORE_COUNT];

    // Initialize all shimmer values to 1.0 (no effect initially)
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        coreShimmerValues[i] = 1.0f;
    }

    // Don't call leds.clearAll() here as instructed
    Serial.println("TechnoOrangeEffect created - animated inner wave, shimmering core purple wave, breathing outer gradient, breathing ring");
}

RegalEffect::~RegalEffect() {
    // Clean up allocated shimmer array
    if (coreShimmerValues) {
        delete[] coreShimmerValues;
    }
}

void RegalEffect::reset() {
    // Reset all animation states to beginning
    innerState = FILLING_UP;
    coreState = CORE_WAITING;
    innerAnimationStartTime = millis();
    coreAnimationStartTime = millis();
    innerFillPosition = 0;
    coreFillPosition = 0;
    outerBreathingStartTime = millis();

    // Reset shimmer values
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        coreShimmerValues[i] = 1.0f;
    }
    lastShimmerUpdate = millis();

    Serial.println("TechnoOrangeEffect reset - all animations restarted");
}

void RegalEffect::update() {
    // Update the animations for inner, core, and outer strips
    updateInnerAnimation();
    updateCoreAnimation();
    updateOuterAnimation();

    // Update ring breathing animation (opposite to outer strips)
    updateRingAnimation();

    // Show all the colors on the LED strips
    leds.showAll();
}

void RegalEffect::updateInnerAnimation() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - innerAnimationStartTime;

    // Handle the current animation state
    switch (innerState) {
        case FILLING_UP:
        {
            // Calculate progress as a ratio (0.0 to 1.0)
            float progressRatio = float(elapsedTime) / INNER_FILL_TIME;
            progressRatio = min(1.0f, progressRatio); // Clamp to 1.0 max

            // Apply ease-out cubic function for deceleration effect
            // This starts fast and slows down as it approaches the end
            // Formula: 1 - (1 - x)^3
            float easedProgress = 1.0f - pow(1.0f - progressRatio, 3.0f);

            // Calculate precise position based on eased progress
            float precisePosition = easedProgress * INNER_LEDS_PER_STRIP;

            // Define fade length for smooth leading edge
            const int fadeLength = 8; // Number of LEDs to fade over at the leading edge

            // Apply the fill to all inner strip segments
            for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
                for (int led = 0; led < INNER_LEDS_PER_STRIP; led++) {
                    int ledIndex = segment * INNER_LEDS_PER_STRIP + led;

                    if (led < precisePosition - fadeLength) {
                        // LEDs below the fade zone: fully lit (bluish-purple color)
                        leds.getInner()[ledIndex] = leds.neoColorToCRGB(INNER_COLOR);
                    } else if (led <= precisePosition) {
                        // LEDs in the fade zone: gradually fade from full brightness to off
                        float distanceFromEdge = precisePosition - led; // Distance from the leading edge
                        float fadeProgress = distanceFromEdge / fadeLength; // 0.0 to 1.0
                        fadeProgress = max(0.0f, min(1.0f, fadeProgress)); // Clamp to 0-1 range

                        // Apply smooth curve for more visible and natural fade
                        fadeProgress = sqrt(fadeProgress); // Square root for gentler fade curve

                        // Calculate faded color
                        CRGB baseColor = leds.neoColorToCRGB(INNER_COLOR);
                        CRGB fadedColor = CRGB(
                            baseColor.r * fadeProgress,
                            baseColor.g * fadeProgress,
                            baseColor.b * fadeProgress
                        );

                        leds.getInner()[ledIndex] = fadedColor;
                    } else {
                        // LEDs above the fade zone: completely off (black)
                        leds.getInner()[ledIndex] = CRGB::Black;
                    }
                }
            }

            // Update the integer position for state checking
            innerFillPosition = int(precisePosition);

            // Check if filling is complete
            if (elapsedTime >= INNER_FILL_TIME) {
                innerState = HOLDING;
                innerAnimationStartTime = currentTime; // Reset timer for next phase
                Serial.println("Inner strips: Filling complete, now holding");
            }

            // Trigger core animation earlier - when inner strips are 50% filled
            if (coreState == CORE_WAITING && elapsedTime >= (INNER_FILL_TIME * 0.5f)) {
                coreState = CORE_FILLING;
                coreAnimationStartTime = currentTime;
                Serial.println("Core strips: Starting purple wave animation (inner 50% complete)");
            }
            break;
        }

        case HOLDING:
            // Keep all LEDs fully lit during hold phase
            applyColorToStrip(leds.getInner(), LED_STRIP_INNER_COUNT, INNER_COLOR);

            // Check if hold time is complete
            if (elapsedTime >= INNER_HOLD_TIME) {
                innerState = FADING_OUT;
                innerAnimationStartTime = currentTime; // Reset timer for next phase
                Serial.println("Inner strips: Hold complete, now fading out");
            }
            break;

        case FADING_OUT:
        {
            // Calculate fade progress (1.0 = full brightness, 0.0 = completely faded)
            float fadeProgress = 1.0f - (float(elapsedTime) / INNER_FADE_TIME);
            fadeProgress = max(0.0f, fadeProgress); // Don't go below 0

            // Apply faded color to all inner strips
            CRGB baseColor = leds.neoColorToCRGB(INNER_COLOR);
            CRGB fadedColor = CRGB(
                baseColor.r * fadeProgress,
                baseColor.g * fadeProgress,
                baseColor.b * fadeProgress
            );

            for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
                leds.getInner()[i] = fadedColor;
            }

            // Trigger core to start fading at the same time
            if (coreState == CORE_FILLING) {
                coreState = CORE_FADING;
                // Don't reset coreAnimationStartTime - use current fade progress
                Serial.println("Core strips: Starting fade with inner strips");
            }

            // Check if fade is complete
            if (elapsedTime >= INNER_FADE_TIME) {
                innerState = FILLING_UP;
                coreState = CORE_WAITING; // Reset core to waiting state
                innerAnimationStartTime = currentTime; // Reset timer for next cycle
                coreAnimationStartTime = currentTime; // Reset core timer too
                innerFillPosition = 0; // Reset fill position
                coreFillPosition = 0; // Reset core fill position
                Serial.println("Inner strips: Fade complete, starting new cycle");
            }
            break;
        }
    }
}

void RegalEffect::updateCoreShimmer() {
    unsigned long currentTime = millis();

    // Only update shimmer at specified intervals
    if (currentTime - lastShimmerUpdate < SHIMMER_UPDATE_INTERVAL) {
        return;
    }

    lastShimmerUpdate = currentTime;

    // Update shimmer values for each LED
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        // Higher chance for each LED to shimmer (50% instead of 30%)
        if (random(100) < 50) {  // 50% chance per frame for each LED to change
            // Create more dramatic shimmer effect with values between 0.4 and 1.6
            // This creates a ±60% brightness variation (much more noticeable)
            coreShimmerValues[i] = 0.4f + (random(120) / 100.0f);  // 0.4 to 1.6

            // Occasionally create super bright flashes (10% chance)
            if (random(100) < 10) {
                coreShimmerValues[i] = 1.8f + (random(40) / 100.0f);  // 1.8 to 2.2 for bright flashes
            }
        } else {
            // Faster return to normal brightness for more active shimmering
            if (coreShimmerValues[i] < 1.0f) {
                coreShimmerValues[i] += 0.1f;  // Faster fade up (was 0.05f)
                if (coreShimmerValues[i] > 1.0f) coreShimmerValues[i] = 1.0f;
            } else if (coreShimmerValues[i] > 1.0f) {
                coreShimmerValues[i] -= 0.1f;  // Faster fade down (was 0.05f)
                if (coreShimmerValues[i] < 1.0f) coreShimmerValues[i] = 1.0f;
            }
        }
    }
}

void RegalEffect::updateCoreAnimation() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - coreAnimationStartTime;

    // Update shimmer effect for all core states except waiting
    if (coreState != CORE_WAITING) {
        updateCoreShimmer();
    }

    // Handle the current core animation state
    switch (coreState) {
        case CORE_WAITING:
            // Core stays off while waiting for inner strips to reach top
            for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
                leds.getCore()[i] = CRGB::Black;
            }
            break;

        case CORE_FILLING:
        {
            // Core needs to fade in completely before inner strips start fading
            // Inner fills for 2s, holds for 1s, then fades
            // Core starts at 1s (50% of inner fill), so it has 2s before fade starts
            // Use 1.5s for fade-in to ensure it completes with some margin
            const unsigned long CORE_FADE_IN_TIME = 1500; // 1.5 seconds

            // Calculate fade-in progress (0.0 to 1.0)
            float fadeInProgress = float(elapsedTime) / CORE_FADE_IN_TIME;
            fadeInProgress = min(1.0f, fadeInProgress); // Clamp to 1.0 max

            // Apply smooth ease-in-out function for natural fade
            // Using smoothstep: 3x^2 - 2x^3
            float smoothProgress = fadeInProgress * fadeInProgress * (3.0f - 2.0f * fadeInProgress);

            // Apply the fade-in to all core LEDs simultaneously
            CRGB baseColor = leds.neoColorToCRGB(CORE_PURPLE_COLOR);

            for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
                // Apply shimmer multiplier to create dazzling effect
                float shimmerMultiplier = coreShimmerValues[i];

                // Calculate color with fade-in progress, 45% max brightness, and shimmer
                CRGB fadedColor = CRGB(
                    baseColor.r * smoothProgress * 0.45f * shimmerMultiplier,
                    baseColor.g * smoothProgress * 0.45f * shimmerMultiplier,
                    baseColor.b * smoothProgress * 0.45f * shimmerMultiplier
                );

                leds.getCore()[i] = fadedColor;
            }

            // Core filling doesn't complete on its own - it gets interrupted by fade
            break;
        }

        case CORE_FADING:
        {
            // Calculate fade progress to match inner strips exactly
            // Get the current time since inner strips started fading
            unsigned long innerFadeStartTime = innerAnimationStartTime; // This is when inner fade started
            unsigned long timeSinceInnerFadeStarted = millis() - innerFadeStartTime;

            // Calculate fade progress (1.0 = full brightness, 0.0 = completely faded)
            float fadeProgress = 1.0f - (float(timeSinceInnerFadeStarted) / INNER_FADE_TIME);
            fadeProgress = max(0.0f, fadeProgress); // Don't go below 0

            // Apply faded purple color to all core LEDs at 45% brightness with shimmer
            CRGB baseColor = leds.neoColorToCRGB(CORE_PURPLE_COLOR);

            for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
                // Apply shimmer during fade for continued dazzle effect
                float shimmerMultiplier = coreShimmerValues[i];

                CRGB fadedColor = CRGB(
                    baseColor.r * fadeProgress * 0.45f * shimmerMultiplier,
                    baseColor.g * fadeProgress * 0.45f * shimmerMultiplier,
                    baseColor.b * fadeProgress * 0.45f * shimmerMultiplier
                );

                leds.getCore()[i] = fadedColor;
            }
            break;
        }
    }
}

void RegalEffect::updateOuterAnimation() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - outerBreathingStartTime;

    // Calculate breathing progress (0.0 to 1.0 over the full cycle)
    float breathingProgress = float(elapsedTime % OUTER_BREATHING_CYCLE) / OUTER_BREATHING_CYCLE;

    // Convert progress to a sine wave for smooth breathing (0 to 2*PI)
    float sineInput = breathingProgress * 2.0f * PI;
    float sineValue = sin(sineInput); // -1.0 to 1.0

    // Convert sine wave to brightness range (20% to 100%)
    float normalizedSine = (sineValue + 1.0f) / 2.0f; // 0.0 to 1.0
    float currentBrightness = OUTER_MIN_BRIGHTNESS +
                             (normalizedSine * (OUTER_MAX_BRIGHTNESS - OUTER_MIN_BRIGHTNESS));

    // Apply gradient with breathing brightness to outer strips
    applyGradientToStrip(leds.getOuter(), LED_STRIP_OUTER_COUNT, OUTER_COLOR, currentBrightness);
}

void RegalEffect::updateRingAnimation() {
    // Skip ring if button feedback is active (effect base class handles this)
    if (skipRing) {
        return;
    }

    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - outerBreathingStartTime;

    // Calculate breathing progress (0.0 to 1.0 over the full cycle)
    // Same timing as outer strips but inverted
    float breathingProgress = float(elapsedTime % OUTER_BREATHING_CYCLE) / OUTER_BREATHING_CYCLE;

    // Convert progress to a sine wave for smooth breathing (0 to 2*PI)
    float sineInput = breathingProgress * 2.0f * PI;
    float sineValue = sin(sineInput); // -1.0 to 1.0

    // INVERT the sine value to make ring breathe opposite to outer strips
    float invertedSineValue = -sineValue; // Flip the sine wave

    // Convert inverted sine wave to brightness range (30% to 90%)
    float normalizedSine = (invertedSineValue + 1.0f) / 2.0f; // 0.0 to 1.0
    float ringBrightness = RING_MIN_BRIGHTNESS +
                          (normalizedSine * (RING_MAX_BRIGHTNESS - RING_MIN_BRIGHTNESS));

    // Apply solid red-orange color with breathing brightness to ring
    applyColorToStripWithBrightness(leds.getRing(), LED_STRIP_RING_COUNT, RING_COLOR, ringBrightness);
}

void RegalEffect::applyGradientToStrip(CRGB* strip, int count, uint32_t baseColor, float brightness) {
    // Convert base color to CRGB
    CRGB rgbColor = leds.neoColorToCRGB(baseColor);

    // Apply gradient for each outer strip segment
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        for (int led = 0; led < OUTER_LEDS_PER_STRIP; led++) {
            int ledIndex = segment * OUTER_LEDS_PER_STRIP + led;

            // Calculate gradient factor (1.0 at bottom, 0.0 at top)
            float gradientFactor = 1.0f - (float(led) / (OUTER_LEDS_PER_STRIP - 1));

            // Apply both gradient and breathing brightness
            float finalBrightness = gradientFactor * brightness;

            // Calculate final color
            CRGB finalColor = CRGB(
                rgbColor.r * finalBrightness,
                rgbColor.g * finalBrightness,
                rgbColor.b * finalBrightness
            );

            strip[ledIndex] = finalColor;
        }
    }
}

void RegalEffect::applyColorToStrip(CRGB* strip, int count, uint32_t color) {
    // Convert the 32-bit color value to CRGB format
    CRGB rgbColor = leds.neoColorToCRGB(color);

    // Set every LED in the strip to this color
    for (int i = 0; i < count; i++) {
        strip[i] = rgbColor;
    }
}

void RegalEffect::applyColorToStripWithBrightness(CRGB* strip, int count, uint32_t color, float brightness) {
    // Convert the 32-bit color value to CRGB format
    CRGB rgbColor = leds.neoColorToCRGB(color);

    // Apply brightness to the color
    CRGB finalColor = CRGB(
        rgbColor.r * brightness,
        rgbColor.g * brightness,
        rgbColor.b * brightness
    );

    // Set every LED in the strip to this color with brightness applied
    for (int i = 0; i < count; i++) {
        strip[i] = finalColor;
    }
}