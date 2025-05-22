#ifndef EFFECT_H
#define EFFECT_H

#include "../LEDController.h"

/**
 * Base class for all LED effects
 *
 * This class provides frame rate independence by tracking time between updates.
 * Child classes should use deltaTime for consistent animation speeds.
 */
class Effect {
public:
    /**
     * Constructor - creates an effect that works with the given LED controller
     * @param ledController Reference to the LED controller to use
     */
    Effect(LEDController& ledController) : leds(ledController), lastUpdateTime(0) {}

    /**
     * Virtual destructor - allows proper cleanup of child classes
     */
    virtual ~Effect() {}

    /**
     * Update the effect - must be implemented by child classes
     * This is called every frame and should use deltaTime for frame rate independence
     */
    virtual void update() = 0;

    /**
     * Reset the effect to its initial state - optional to implement
     */
    virtual void reset() { lastUpdateTime = millis(); }

    /**
     * Get the name of this effect - must be implemented by child classes
     * @return String containing the effect name for debugging/display
     */
    virtual String getName() const = 0;
    /**
     * Check if ring LEDs should be skipped (for button feedback)
     * @param buttonFeedbackActive True if button feedback is currently showing
     */
    virtual void setSkipRing(bool skipRing) { this->skipRing = skipRing; }

protected:
    bool skipRing = false;
    LEDController& leds;        // Reference to LED controller for drawing
    unsigned long lastUpdateTime;  // Time of last update in milliseconds

    /**
     * Get time elapsed since last update in milliseconds
     * Use this for frame rate independent animations
     * @return Milliseconds elapsed since last update call
     */
    unsigned long getDeltaTime() {
        unsigned long currentTime = millis();
        unsigned long deltaTime = currentTime - lastUpdateTime;
        lastUpdateTime = currentTime;
        return deltaTime;
    }

    /**
     * Check if enough time has passed for next animation step
     * @param intervalMs Minimum milliseconds between animation steps
     * @return True if enough time has passed, false otherwise
     */
    bool shouldUpdate(unsigned long intervalMs) {
        unsigned long currentTime = millis();
        if (currentTime - lastUpdateTime >= intervalMs) {
            lastUpdateTime = currentTime;
            return true;
        }
        return false;
    }
};

#endif // EFFECT_H