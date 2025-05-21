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
          stripLength = INNER_LEDS_PER_STRIP * NUM_INNER_STRIPS;  // Total inner LEDs
          trails[i].subStrip = random(NUM_INNER_STRIPS);  // Choose one of the 3 inner strips
          break;
        case 2: 
          stripLength = OUTER_LEDS_PER_STRIP * NUM_OUTER_STRIPS;  // Total outer LEDs
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
  // Random chance to create a new trail
  if (random(10) == 0) {  // 10% chance per frame
    createNewTrail();
  }
  
  // Clear all LEDs
  leds.clearAll();
  
  // Update and draw all active trails
  for (int i = 0; i < maxTrails; i++) {
    if (trails[i].active) {
      // Get strip information
      Adafruit_NeoPixel* strip;
      int stripLength = 0;
      int ledsPerSubStrip = 0;
      
      switch (trails[i].stripId) {
        case 0:
          strip = &leds.getCore();
          stripLength = LED_STRIP_CORE_COUNT;
          ledsPerSubStrip = stripLength;
          break;
        case 1:
          strip = &leds.getInner();
          stripLength = INNER_LEDS_PER_STRIP * NUM_INNER_STRIPS;
          ledsPerSubStrip = INNER_LEDS_PER_STRIP;
          break;
        case 2:
          strip = &leds.getOuter();
          stripLength = OUTER_LEDS_PER_STRIP * NUM_OUTER_STRIPS;
          ledsPerSubStrip = OUTER_LEDS_PER_STRIP;
          break;
        case 3:
          strip = &leds.getRing();
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
        
        // For inner/outer strips with multiple sections, keep trail within one section
        if (trails[i].stripId == 1 || trails[i].stripId == 2) {
          // Get substrip-local position
          int subStripPos = logicalPos % ledsPerSubStrip;
          
          // Map to physical position within the substrip
          int physicalPos = leds.mapPositionToPhysical(trails[i].stripId, subStripPos, currentSubStrip);
          
          // Calculate the absolute LED position on the strip
          if (trails[i].stripId == 1) { // Inner
            physicalPos += currentSubStrip * INNER_LEDS_PER_STRIP;
          } else { // Outer
            physicalPos += currentSubStrip * OUTER_LEDS_PER_STRIP;
          }
          
          // Calculate brightness based on position in trail (fade toward end)
          int brightness = 255 * (trails[i].length - j) / trails[i].length;
          
          // Set pixel color with fading brightness but consistent saturation
          strip->setPixelColor(physicalPos, leds.colorHSV(trails[i].hue, 255, brightness));
        } else {
          // For core and ring strips, simpler mapping
          int physicalPos = leds.mapPositionToPhysical(trails[i].stripId, logicalPos, 0);
          
          // Calculate brightness based on position in trail (fade toward end)
          int brightness = 255 * (trails[i].length - j) / trails[i].length;
          
          // Set pixel color with fading brightness but consistent saturation
          strip->setPixelColor(physicalPos, leds.colorHSV(trails[i].hue, 255, brightness));
        }
      }
      
      // Move the trail head
      if (trails[i].direction) {
        trails[i].position = (trails[i].position + 1) % stripLength;
      } else {
        trails[i].position = (trails[i].position - 1) % stripLength;
        if (trails[i].position < 0) trails[i].position += stripLength;  // Handle negative wraparound
      }
      
      // Randomly deactivate trail (with low probability)
      if (random(50) == 0) {  // 2% chance per frame
        trails[i].active = false;
      }
    }
  }
  
  // Update all strips to show changes
  leds.showAll();
}