#include "FireEffect.h"

FireEffect::FireEffect(LEDController& ledController) :
  Effect(ledController),
  heatCore(nullptr),
  heatInner(nullptr),
  heatOuter(nullptr)
{
  // Initialize the heat arrays
  initHeatArrays();
}

void FireEffect::initHeatArrays() {
  // Allocate memory for heat arrays
  heatCore = new uint8_t[LED_STRIP_CORE_COUNT];
  heatInner = new uint8_t[LED_STRIP_INNER_COUNT];
  heatOuter = new uint8_t[LED_STRIP_OUTER_COUNT];

  // Initialize all heat values to zero
  for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
    heatCore[i] = 0;
  }

  for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
    heatInner[i] = 0;
  }

  for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
    heatOuter[i] = 0;
  }
}

void FireEffect::reset() {
  // Reset all heat values to zero
  for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
    heatCore[i] = 0;
  }

  for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
    heatInner[i] = 0;
  }

  for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
    heatOuter[i] = 0;
  }
}

void FireEffect::update() {
  // Clear all strips initially
  // leds.clearAll();

  // Create fire effect on core strip (red-orange glowing embers)
  // Lower cooling factor makes it more stable like embers
  updateFire(heatCore, LED_STRIP_CORE_COUNT, leds.getCore(), 40, 100);

  // Create fire effect on inner strips (first layer of flames)
  // Medium cooling and sparking values for moderate fire movement
  updateFire(heatInner, LED_STRIP_INNER_COUNT, leds.getInner(), 55, 120);

  // Create fire effect on outer strips (second layer of flames)
  // Higher cooling makes it more dynamic and flickery
  updateFire(heatOuter, LED_STRIP_OUTER_COUNT, leds.getOuter(), 70, 150);

  // Ring strip stays off (already cleared)

  // Show the updated LEDs
  leds.showAll();
}

void FireEffect::updateFire(uint8_t* heat, int count, Adafruit_NeoPixel& strip, uint8_t cooling, uint8_t sparking) {
  // Step 1: Cool down each LED a little
  for (int i = 0; i < count; i++) {
    // Random cooling factor creates natural variations
    // The cooling factor determines how much each pixel cools down
    int cooldown = random(0, ((cooling * 10) / count) + 2);

    // Apply the cooling (but don't go below zero)
    if (heat[i] > cooldown) {
      heat[i] = heat[i] - cooldown;
    } else {
      heat[i] = 0;
    }
  }

  // Step 2: Heat rises - simulate this by moving heat values up
  // (LEDs at index 0 are at the bottom of the fire)
  for (int i = count - 1; i >= 2; i--) {
    // Mix values with the ones below to create upward movement
    heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3;
  }

  // Step 3: Randomly ignite new sparks at the bottom of the fire
  if (random(255) < sparking) {
    int y = random(7); // Position in the bottom section
    // Create a new "hot" spark
    heat[y] = heat[y] + random(160, 255);
  }

  // Step 4: Map heat values to colors and set LEDs
  for (int i = 0; i < count; i++) {
    // Convert heat value to color
    uint32_t color = heatToColor(heat[i]);

    // Set the LED color
    if (count == LED_STRIP_INNER_COUNT) {
      // For inner strip, handle the multiple strip segments
      int stripSegment = i / INNER_LEDS_PER_STRIP;
      int localPos = i % INNER_LEDS_PER_STRIP;

      // Map logical position to physical position
      int physicalPos = leds.mapPositionToPhysical(1, localPos, stripSegment);

      // Calculate absolute LED index
      physicalPos += stripSegment * INNER_LEDS_PER_STRIP;

      strip.setPixelColor(physicalPos, color);
    }
    else if (count == LED_STRIP_OUTER_COUNT) {
      // For outer strip, handle the multiple strip segments
      int stripSegment = i / OUTER_LEDS_PER_STRIP;
      int localPos = i % OUTER_LEDS_PER_STRIP;

      // Map logical position to physical position
      int physicalPos = leds.mapPositionToPhysical(2, localPos, stripSegment);

      // Calculate absolute LED index
      physicalPos += stripSegment * OUTER_LEDS_PER_STRIP;

      strip.setPixelColor(physicalPos, color);
    }
    else {
      // For core strip, simpler mapping
      strip.setPixelColor(i, color);
    }
  }
}

uint32_t FireEffect::heatToColor(uint8_t heat) {
  // Heat value to RGB conversion
  // Scale heat down to 0-191 range for more red-orange focus
  uint8_t scaledHeat = (heat * 191) / 255;

  uint8_t r, g, b;

  // Calculate red value - red is always on for fire
  r = 255;

  // Calculate green value - increases as heat increases
  if (scaledHeat < 127) {
    // Shift range for ember effect in core
    g = scaledHeat * 1.5; // More orange in embers
  } else {
    g = scaledHeat;
  }

  // Calculate blue value - only present at very high heat
  if (scaledHeat > 170) {
    b = (scaledHeat - 170) * 3; // Small blue component for hottest parts
  } else {
    b = 0; // No blue for most of the fire
  }

  // Return the color
  return leds.getCore().Color(r, g, b);
}