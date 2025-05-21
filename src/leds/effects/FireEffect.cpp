#include "FireEffect.h"

FireEffect::FireEffect(LEDController& ledController) :
  Effect(ledController)
{
}


void FireEffect::update() {
    leds.showAll();
}