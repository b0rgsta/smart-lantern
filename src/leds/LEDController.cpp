#include "LEDController.h"

LEDController::LEDController() :
  stripCore(LED_STRIP_CORE_COUNT, LED_STRIP_CORE_PIN, NEO_GRB + NEO_KHZ800),
  stripInner(LED_STRIP_INNER_COUNT, LED_STRIP_INNER_PIN, NEO_GRB + NEO_KHZ800),
  stripOuter(LED_STRIP_OUTER_COUNT, LED_STRIP_OUTER_PIN, NEO_GRB + NEO_KHZ800),
  stripRing(LED_STRIP_RING_COUNT, LED_STRIP_RING_PIN, NEO_GRB + NEO_KHZ800)
{
}

void LEDController::begin() {
  stripCore.begin();
  stripInner.begin();
  stripOuter.begin();
  stripRing.begin();
  
  // Set default brightness to 30%
  setBrightness(77);
  
  // Clear all LEDs initially
  clearAll();
  
  Serial.println("LED strips initialized");
}

void LEDController::clearAll() {
  stripCore.clear();
  stripInner.clear();
  stripOuter.clear();
  stripRing.clear();
  
  showAll();
}

void LEDController::showAll() {
  stripCore.show();
  stripInner.show();
  stripOuter.show();
  stripRing.show();
}

void LEDController::setBrightness(uint8_t brightness) {
  stripCore.setBrightness(brightness);
  stripInner.setBrightness(brightness);
  stripOuter.setBrightness(brightness);
  stripRing.setBrightness(brightness);
}

uint32_t LEDController::colorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
  return stripCore.gamma32(stripCore.ColorHSV(hue, sat, val));
}

int LEDController::mapPositionToPhysical(int stripId, int logicalPos, int subStrip) {
  int physicalPos = logicalPos;
  
  switch (stripId) {
    case 0: // Core strip
      // No additional mapping needed
      break;
      
    case 1: // Inner strips
      physicalPos = logicalPos % INNER_LEDS_PER_STRIP;
      if (subStrip % 2 == 1) {
        physicalPos = INNER_LEDS_PER_STRIP - 1 - physicalPos;
      }
      break;
      
    case 2: // Outer strips
      physicalPos = logicalPos % OUTER_LEDS_PER_STRIP;
      if (subStrip % 2 == 1) {
        physicalPos = OUTER_LEDS_PER_STRIP - 1 - physicalPos;
      }
      break;
      
    case 3: // Ring strip
      // No additional mapping needed
      break;
  }
  
  return physicalPos;
}