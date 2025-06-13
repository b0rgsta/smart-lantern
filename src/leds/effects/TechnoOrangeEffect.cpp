// src/leds/effects/TechnoOrangeEffect.cpp

#include "TechnoOrangeEffect.h"

TechnoOrangeEffect::TechnoOrangeEffect(LEDController& ledController) : Effect(ledController) {
    // Initialize animation state and timing
    innerState = FILLING_UP;
    coreState = CORE_WAITING;
    innerAnimationStartTime = millis();
    coreAnimationStartTime = millis();
    innerFillPosition = 0;
    coreFillPosition = 0;
    outerBreathingStartTime = millis();

    // Don't call leds.clearAll() here as instructed
    Serial.println("TechnoOrangeEffect created - animated inner wave, core purple wave, breathing outer gradient");
}

void TechnoOrangeEffect::reset() {
    // Reset all animation states to beginning
    innerState = FILLING_UP;
    coreState = CORE_WAITING;
    innerAnimationStartTime = millis();
    coreAnimationStartTime = millis();
    innerFillPosition = 0;
    coreFillPosition = 0;
    outerBreathingStartTime = millis();

    Serial.println("TechnoOrangeEffect reset - all animations restarted");
}

void TechnoOrangeEffect::update() {
    // Update the animations for inner, core, and outer strips
    updateInnerAnimation();
    updateCoreAnimation();
    updateOuterAnimation();

    // Ring strip stays off (or is handled by button feedback system)
    if (!skipRing) {
        // If button feedback is not active, turn off the ring
        applyColorToStrip(leds.getRing(), LED_STRIP_RING_COUNT, 0x000000); // Black = off
    }

    // Show all the colors on the LED strips
    leds.showAll();
}

void TechnoOrangeEffect::updateInnerAnimation() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - innerAnimationStartTime;

    // Handle the current animation state
    switch (innerState) {
        case FILLING_UP:
        {
            // Calculate precise fill position using float for smoother animation
            float precisePosition = (float(elapsedTime) * INNER_LEDS_PER_STRIP) / INNER_FILL_TIME;

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

void TechnoOrangeEffect::updateCoreAnimation() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - coreAnimationStartTime;

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
            // Calculate precise fill position using float for smoother animation
            // Make core fill faster by giving it more time (3 seconds instead of 2)
            float precisePosition = (float(elapsedTime) * LED_STRIP_CORE_COUNT) / (CORE_FILL_TIME * 1.5f);

            // Define fade length for smooth leading edge
            const int fadeLength = 12; // Longer fade for more visible effect on core

            // Apply the fill to all core strip segments
            int coreSegmentLength = LED_STRIP_CORE_COUNT / 3; // Core has 3 segments

            for (int segment = 0; segment < 3; segment++) {
                for (int led = 0; led < coreSegmentLength; led++) {
                    // Use LED mapping to get the correct physical position
                    int physicalPos = leds.mapPositionToPhysical(0, led, segment);
                    physicalPos += segment * coreSegmentLength; // Add segment offset

                    // Use the same logic as inner strips - simple position comparison
                    if (led < precisePosition - fadeLength) {
                        // LEDs below the fade zone: fully lit (purple color at 45% brightness)
                        CRGB baseColor = leds.neoColorToCRGB(CORE_PURPLE_COLOR);
                        CRGB dimmedColor = CRGB(
                            baseColor.r * 0.45f,
                            baseColor.g * 0.45f,
                            baseColor.b * 0.45f
                        );
                        leds.getCore()[physicalPos] = dimmedColor;
                    } else if (led <= precisePosition) {
                        // LEDs in the fade zone: gradually fade from full brightness to off
                        float distanceFromEdge = precisePosition - led;
                        float fadeProgress = distanceFromEdge / fadeLength;
                        fadeProgress = max(0.0f, min(1.0f, fadeProgress));

                        // Apply smooth curve for natural fade
                        fadeProgress = sqrt(fadeProgress);

                        // Calculate faded purple color
                        CRGB baseColor = leds.neoColorToCRGB(CORE_PURPLE_COLOR);
                        CRGB fadedColor = CRGB(
                            baseColor.r * fadeProgress * 0.45f,
                            baseColor.g * fadeProgress * 0.45f,
                            baseColor.b * fadeProgress * 0.45f
                        );

                        leds.getCore()[physicalPos] = fadedColor;
                    } else {
                        // LEDs above the fade zone: completely off (black)
                        leds.getCore()[physicalPos] = CRGB::Black;
                    }
                }
            }

            // Update fill position for state checking
            coreFillPosition = int(precisePosition);

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

            // Apply faded purple color to all core LEDs at 15% brightness
            CRGB baseColor = leds.neoColorToCRGB(CORE_PURPLE_COLOR);
            CRGB fadedColor = CRGB(
                baseColor.r * fadeProgress * 0.45f,  // Apply both fade and 15% brightness
                baseColor.g * fadeProgress * 0.45f,  // Apply both fade and 15% brightness
                baseColor.b * fadeProgress * 0.45f   // Apply both fade and 15% brightness
            );

            for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
                leds.getCore()[i] = fadedColor;
            }
            break;
        }
    }
}

void TechnoOrangeEffect::updateOuterAnimation() {
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

void TechnoOrangeEffect::applyGradientToStrip(CRGB* strip, int count, uint32_t baseColor, float brightness) {
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

void TechnoOrangeEffect::applyColorToStrip(CRGB* strip, int count, uint32_t color) {
    // Convert the 32-bit color value to CRGB format
    CRGB rgbColor = leds.neoColorToCRGB(color);

    // Set every LED in the strip to this color
    for (int i = 0; i < count; i++) {
        strip[i] = rgbColor;
    }
}