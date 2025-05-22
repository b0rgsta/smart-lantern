// src/leds/effects/TrailsEffect.cpp

#include "TrailsEffect.h"

TrailsEffect::TrailsEffect(LEDController& ledController, int maxTrails, int trailLength) :
  Effect(ledController),
  maxTrails(maxTrails),
  trailLength(trailLength)
{
  trails = new Trail[maxTrails];
  reset();
}

TrailsEffect::~TrailsEffect() {
  delete[] trails;
}

void TrailsEffect::reset() {
  for (int i = 0; i < maxTrails; i++) {
    trails[i].active = false;
  }
}

void TrailsEffect::createNewTrail() {
  // Find an inactive trail slot
  for (int i = 0; i < maxTrails; i++) {
    if (!trails[i].active) {
      // Pick a random strip
      trails[i].stripId = random(4);  // 0-3 for the 4 strips

      // Get the selected strip's length and set subStrip if needed
      int stripLength = 0;
      switch (trails[i].stripId) {
        case 0:
          stripLength = LED_STRIP_CORE_COUNT;
          trails[i].subStrip = 0;  // Not used for core
          break;
        case 1:
          stripLength = INNER_LEDS_PER_STRIP;  // Length of one inner strip
          trails[i].subStrip = random(NUM_INNER_STRIPS);  // Choose one of the 3 inner strips
          break;
        case 2:
          stripLength = OUTER_LEDS_PER_STRIP;  // Length of one outer strip
          trails[i].subStrip = random(NUM_OUTER_STRIPS);  // Choose one of the 3 outer strips
          break;
        case 3:
          stripLength = LED_STRIP_RING_COUNT;
          trails[i].subStrip = 0;  // Not used for ring
          break;
      }

      // Make sure the strip is long enough for a trail
      if (stripLength < trailLength) continue;

      // Random starting position
      trails[i].position = random(stripLength);

      // Random direction
      trails[i].direction = random(2) == 1;

      // Full trail length
      trails[i].length = trailLength;

      // Random hue (full color spectrum)
      trails[i].hue = random(65536);

      // Activate the trail
      trails[i].active = true;

      // Only create one trail per function call
      return;
    }
  }
}

void TrailsEffect::update() {
    // Target 120 FPS for ultra-smooth trail movement
    if (!shouldUpdate(8)) {  // 8ms = 125 FPS (close to 120)
        return;
    }

    // Random chance to create a new trail (1% chance per update to account for higher frame rate)
    if (random(100) == 0) {  // Reduced from 10% to 1% since we're updating 3x more often
        createNewTrail();
    }

    // Clear all LEDs
    leds.clearAll();

    // Update and draw all active trails
    for (int i = 0; i < maxTrails; i++) {
        if (trails[i].active) {
            // Get strip information
            CRGB* strip;
            int stripLength = 0;
            int ledsPerSubStrip = 0;

            switch (trails[i].stripId) {
                case 0:
                    strip = leds.getCore();
                    stripLength = LED_STRIP_CORE_COUNT;
                    ledsPerSubStrip = stripLength;
                    break;
                case 1:
                    strip = leds.getInner();
                    stripLength = INNER_LEDS_PER_STRIP;
                    ledsPerSubStrip = stripLength;
                    break;
                case 2:
                    strip = leds.getOuter();
                    stripLength = OUTER_LEDS_PER_STRIP;
                    ledsPerSubStrip = stripLength;
                    break;
                case 3:
                    strip = leds.getRing();
                    stripLength = LED_STRIP_RING_COUNT;
                    ledsPerSubStrip = stripLength;
                    break;
            }

            // Draw the trail with fade
            for (int j = 0; j < trails[i].length; j++) {
                // Calculate logical position with direction
                int logicalPos;
                if (trails[i].direction) {
                    logicalPos = (trails[i].position - j) % stripLength;
                    if (logicalPos < 0) logicalPos += stripLength;  // Handle negative wraparound
                } else {
                    logicalPos = (trails[i].position + j) % stripLength;
                }

                // For inner/outer strips, determine which substrip this pixel belongs to
                int currentSubStrip = trails[i].subStrip;

                // Calculate brightness based on position in trail (fade toward end)
                int brightness = 255 * (trails[i].length - j) / trails[i].length;

                // Create color using FastLED's HSV conversion
                CHSV hsv(trails[i].hue >> 8, 255, brightness); // FastLED uses 0-255 range for hue
                CRGB rgb;
                hsv2rgb_rainbow(hsv, rgb);

                // Set pixel
                int physicalPos = leds.mapPositionToPhysical(trails[i].stripId, logicalPos, currentSubStrip);

                // For inner/outer strips with multiple sections, adjust the physical position
                if (trails[i].stripId == 1) { // Inner
                    physicalPos += currentSubStrip * INNER_LEDS_PER_STRIP;
                } else if (trails[i].stripId == 2) { // Outer
                    physicalPos += currentSubStrip * OUTER_LEDS_PER_STRIP;
                }

                // Set the pixel color
                switch (trails[i].stripId) {
                    case 0:
                        leds.getCore()[physicalPos] = rgb;
                        break;
                    case 1:
                        leds.getInner()[physicalPos] = rgb;
                        break;
                    case 2:
                        leds.getOuter()[physicalPos] = rgb;
                        break;
                    case 3:
                        leds.getRing()[physicalPos] = rgb;
                        break;
                }
            }

            // Move the trail head
            if (trails[i].direction) {
                trails[i].position = (trails[i].position + 1) % stripLength;
            } else {
                trails[i].position = (trails[i].position - 1) % stripLength;
                if (trails[i].position < 0) trails[i].position += stripLength;  // Handle negative wraparound
            }

            // Randomly deactivate trail (lower probability due to higher frame rate)
            if (random(600) == 0) {  // Changed from 50 to 600 to maintain same deactivation rate
                trails[i].active = false;
            }
        }
    }

    // Update all strips to show changes
    leds.showAll();
}