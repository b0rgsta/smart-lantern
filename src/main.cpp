/*
* Smart Lantern - Main Program
 *
 * Uses a class-based architecture for improved organization
 */

#include <Arduino.h>
#include "SmartLantern.h"

// Create the SmartLantern instance
SmartLantern lantern;

void setup() {
    Serial.begin(115200);
    Serial.println("Smart Lantern Starting...");

    // Initialize random seed
    randomSeed(analogRead(0));

    // Initialize the lantern
    lantern.begin();
}

void loop() {
    // Update the lantern - this handles everything
    lantern.update();

    // Small delay for animation speed
    delay(50);
}