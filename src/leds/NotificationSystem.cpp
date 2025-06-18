// src/leds/NotificationSystem.cpp
#include "NotificationSystem.h"
#include "Config.h"

NotificationSystem::NotificationSystem(LEDController& ledController) :
    leds(ledController),
    notificationActive(false),
    notificationStartTime(0),
    notificationDuration(0),
    notifyStripType(0),
    notifyStartLED(0),
    notifyLength(0),
    notifyBrightness(200),
    isRainbowNotification(false),
    solidColor(CRGB::Black)
{
    // Constructor - clean initialization without clearing LEDs
}

void NotificationSystem::update() {
    // Only process if we have an active notification
    if (!notificationActive) {
        return;
    }

    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - notificationStartTime;

    // Check if notification has expired
    if (elapsed >= notificationDuration) {
        notificationActive = false;
        return;
    }

    // Calculate current brightness based on fade timing
    float brightnessMultiplier = calculateNotificationBrightness();

    // Draw the notification with current brightness
    drawNotification(brightnessMultiplier);
}

void NotificationSystem::showRainbowNotification(int stripType, int startLED, int length,
                                                unsigned long duration, uint8_t brightness) {
    // Validate parameters
    if (stripType < 0 || stripType > 3) {
        Serial.println("ERROR: Invalid strip type for notification");
        return;
    }

    int maxLength = getStripLength(stripType);
    if (startLED < 0 || startLED >= maxLength) {
        Serial.println("ERROR: Invalid start LED for notification");
        return;
    }

    if (length <= 0 || (startLED + length) > maxLength) {
        Serial.println("ERROR: Invalid length for notification");
        return;
    }

    // Store notification parameters
    notifyStripType = stripType;
    notifyStartLED = startLED;
    notifyLength = length;
    notifyBrightness = brightness;
    notificationDuration = duration;
    isRainbowNotification = true;

    // Start the notification
    notificationActive = true;
    notificationStartTime = millis();

    Serial.println("Rainbow notification started: Strip " + String(stripType) +
                   ", LEDs " + String(startLED) + "-" + String(startLED + length - 1) +
                   ", Duration " + String(duration) + "ms");
}

void NotificationSystem::showSolidNotification(int stripType, int startLED, int length,
                                              CRGB color, unsigned long duration) {
    // Validate parameters (same validation as rainbow)
    if (stripType < 0 || stripType > 3) {
        Serial.println("ERROR: Invalid strip type for notification");
        return;
    }

    int maxLength = getStripLength(stripType);
    if (startLED < 0 || startLED >= maxLength) {
        Serial.println("ERROR: Invalid start LED for notification");
        return;
    }

    if (length <= 0 || (startLED + length) > maxLength) {
        Serial.println("ERROR: Invalid length for notification");
        return;
    }

    // Store notification parameters
    notifyStripType = stripType;
    notifyStartLED = startLED;
    notifyLength = length;
    notifyBrightness = 255; // Full brightness for solid colors
    notificationDuration = duration;
    isRainbowNotification = false;
    solidColor = color;

    // Start the notification
    notificationActive = true;
    notificationStartTime = millis();

    Serial.println("Solid notification started: Strip " + String(stripType) +
                   ", Color RGB(" + String(color.r) + "," + String(color.g) + "," + String(color.b) + ")");
}



void NotificationSystem::clear() {
    notificationActive = false;
    Serial.println("Notifications cleared");
}

float NotificationSystem::calculateNotificationBrightness() {
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - notificationStartTime;

    // Fade in phase
    if (elapsed < FADE_IN_TIME) {
        return (float)elapsed / (float)FADE_IN_TIME;
    }

    // Fade out phase (starts FADE_OUT_TIME before end)
    unsigned long fadeOutStart = notificationDuration - FADE_OUT_TIME;
    if (elapsed >= fadeOutStart) {
        unsigned long fadeOutElapsed = elapsed - fadeOutStart;
        return 1.0f - ((float)fadeOutElapsed / (float)FADE_OUT_TIME);
    }

    // Full brightness phase (between fade in and fade out)
    return 1.0f;
}

void NotificationSystem::drawNotification(float brightnessMultiplier) {
    // Get pointer to the appropriate LED strip
    CRGB* strip = getStripPointer(notifyStripType);
    if (strip == nullptr) {
        return;
    }

    // Draw notification LEDs
    for (int i = 0; i < notifyLength; i++) {
        int ledIndex = notifyStartLED + i;

        if (isRainbowNotification) {
            // Create rainbow color for this position
            strip[ledIndex] = getRainbowColorAtPosition(i, brightnessMultiplier);
        } else {
            // Use solid color with brightness adjustment
            uint8_t adjustedBrightness = (uint8_t)(notifyBrightness * brightnessMultiplier);
            strip[ledIndex] = CRGB(
                (solidColor.r * adjustedBrightness) / 255,
                (solidColor.g * adjustedBrightness) / 255,
                (solidColor.b * adjustedBrightness) / 255
            );
        }
    }
}

CRGB NotificationSystem::getRainbowColorAtPosition(int position, float brightnessMultiplier) {
    // Calculate hue based on position within the notification length
    // Create a linear rainbow spanning the entire notification section
    uint8_t hue = (uint8_t)((position * 255) / (notifyLength - 1));

    // Full saturation for vibrant rainbow colors
    uint8_t saturation = 255;

    // Apply brightness with multiplier for fading
    uint8_t value = (uint8_t)(notifyBrightness * brightnessMultiplier);

    // Create HSV color and convert to RGB
    CHSV hsvColor(hue, saturation, value);
    CRGB rgbColor;
    hsv2rgb_rainbow(hsvColor, rgbColor);

    return rgbColor;
}

CRGB* NotificationSystem::getStripPointer(int stripType) {
    switch (stripType) {
        case 0: return leds.getCore();    // Core strip
        case 1: return leds.getInner();   // Inner strips
        case 2: return leds.getOuter();   // Outer strips
        case 3: return leds.getRing();    // Ring strip
        default:
            Serial.println("ERROR: Invalid strip type " + String(stripType));
            return nullptr;
    }
}

int NotificationSystem::getStripLength(int stripType) {
    switch (stripType) {
        case 0: return LED_STRIP_CORE_COUNT;   // Core strip
        case 1: return LED_STRIP_INNER_COUNT;  // Inner strips
        case 2: return LED_STRIP_OUTER_COUNT;  // Outer strips
        case 3: return LED_STRIP_RING_COUNT;   // Ring strip
        default:
            Serial.println("ERROR: Invalid strip type " + String(stripType));
            return 0;
    }
}