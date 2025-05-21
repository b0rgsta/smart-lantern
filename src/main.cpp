// src/main.cpp

#include <Arduino.h>
#include "SmartLantern.h"

// Create the SmartLantern instance
SmartLantern lantern;

// Variables for FPS calculation
unsigned long frameCount = 0;
unsigned long lastFpsTime = 0;
float currentFps = 0.0;

void setup() {
    Serial.begin(115200);
    Serial.println("Smart Lantern Starting...");

    // Initialize random seed
    // set pin 13 as input to read noise
    pinMode(13, INPUT);
    randomSeed(analogRead(13));

    // Initialize the lantern
    lantern.begin();

    // Initialize FPS timer
    lastFpsTime = millis();
}

void loop() {
    // Update the lantern - this handles everything
    lantern.update();

    // Increment frame counter
    frameCount++;

    // Calculate FPS every second
    unsigned long currentTime = millis();
    if (currentTime - lastFpsTime >= 1000) { // Every second
        // Calculate FPS
        currentFps = frameCount * 1000.0 / (currentTime - lastFpsTime);

        // Print FPS
        Serial.print("FPS: ");
        Serial.println(currentFps, 2); // Print with 2 decimal places

        // Reset counters
        frameCount = 0;
        lastFpsTime = currentTime;
    }
}