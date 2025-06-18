// src/leds/NotificationSystem.h
#ifndef NOTIFICATION_SYSTEM_H
#define NOTIFICATION_SYSTEM_H

#include <Arduino.h>
#include <FastLED.h>
#include "LEDController.h"

/**
 * NotificationSystem - Handles visual notifications on LED strips
 *
 * This system provides a clean way to show temporary notifications
 * on specific sections of LED strips without interfering with
 * the main effects. Notifications fade in and out smoothly.
 *
 * Usage:
 * - Call showRainbowNotification() to display a linear rainbow
 * - Call update() in your main loop to handle animation timing
 * - Notifications automatically fade out after their duration
 */
class NotificationSystem {
public:
    /**
     * Constructor
     * @param ledController Reference to the LED controller for drawing
     */
    NotificationSystem(LEDController& ledController);

    /**
     * Update notification animations
     * Call this every frame to handle fade timing
     */
    void update();

    /**
     * Show a linear rainbow notification spanning the notification section
     * Creates a smooth rainbow gradient across the specified LED range
     *
     * @param stripType Which strip to show on (0=core, 1=inner, 2=outer, 3=ring)
     * @param startLED Starting LED position in the strip
     * @param length Number of LEDs to use for the rainbow
     * @param duration How long to show the notification (milliseconds)
     * @param brightness Brightness level (0-255)
     */
    void showRainbowNotification(int stripType, int startLED, int length,
                                unsigned long duration = 2000, uint8_t brightness = 200);

    /**
     * Show a solid color notification
     *
     * @param stripType Which strip to show on (0=core, 1=inner, 2=outer, 3=ring)
     * @param startLED Starting LED position in the strip
     * @param length Number of LEDs to use
     * @param color Color to display (CRGB format)
     * @param duration How long to show the notification (milliseconds)
     */
    void showSolidNotification(int stripType, int startLED, int length,
                              CRGB color, unsigned long duration = 2000);

    /**
     * Check if any notification is currently active
     * @return true if a notification is currently showing
     */
    bool isActive() const { return notificationActive; }

    /**
     * Clear all active notifications immediately
     */
    void clear();

private:
    LEDController& leds;                    // Reference to LED controller

    // Notification state
    bool notificationActive;                // Whether a notification is currently showing
    unsigned long notificationStartTime;   // When the current notification started
    unsigned long notificationDuration;    // How long the notification should last

    // Notification display parameters
    int notifyStripType;                    // Which strip is showing notification
    int notifyStartLED;                     // Starting LED position
    int notifyLength;                       // Number of LEDs in notification
    uint8_t notifyBrightness;              // Maximum brightness for notification
    bool isRainbowNotification;            // Whether this is a rainbow or solid notification
    CRGB solidColor;                       // Color for solid notifications

    // Animation parameters
    static const unsigned long FADE_IN_TIME = 300;   // 300ms fade in
    static const unsigned long FADE_OUT_TIME = 500;  // 500ms fade out

    /**
     * Calculate current notification brightness based on timing
     * Handles fade in at start and fade out at end
     * @return Brightness multiplier (0.0 to 1.0)
     */
    float calculateNotificationBrightness();

    /**
     * Draw the current notification on the appropriate strip
     * @param brightnessMultiplier Overall brightness (0.0 to 1.0)
     */
    void drawNotification(float brightnessMultiplier);

    /**
     * Create a rainbow color for a specific position in the notification
     * @param position Position within the notification (0 to notifyLength-1)
     * @param brightnessMultiplier Overall brightness multiplier
     * @return CRGB color for this position
     */
    CRGB getRainbowColorAtPosition(int position, float brightnessMultiplier);

    /**
     * Get pointer to the appropriate LED strip array
     * @param stripType Strip type (0=core, 1=inner, 2=outer, 3=ring)
     * @return Pointer to the LED array, or nullptr if invalid
     */
    CRGB* getStripPointer(int stripType);

    /**
     * Get the maximum number of LEDs for a strip type
     * @param stripType Strip type (0=core, 1=inner, 2=outer, 3=ring)
     * @return Maximum LED count for this strip
     */
    int getStripLength(int stripType);
};

#endif // NOTIFICATION_SYSTEM_H