// src/leds/effects/FireEffect.cpp

#include "FireEffect.h"

FireEffect::FireEffect(LEDController& ledController) :
    Effect(ledController),
    heatCore(nullptr),
    heatInner(nullptr),
    heatOuter(nullptr),
    lastEmberTime(0),
    intensity(70),             // Default fire intensity (0-100)
    windDirection(128),        // Default wind direction (128 = straight up)
    windStrength(10),          // Default wind strength (0-100)
    baseFlickerRate(30),       // Default flicker rate (0-100)
    lastUpdateTime(0),
    animationSpeed(1.0)        // Default animation speed multiplier
{
    // Initialize heat arrays and particles
    initHeatArrays();

    // Pre-create some embers
    for (int i = 0; i < 5; i++) {
        createEmber();
    }
}

FireEffect::~FireEffect() {
    // Clean up allocated memory
    if (heatCore) delete[] heatCore;
    if (heatInner) delete[] heatInner;
    if (heatOuter) delete[] heatOuter;
}

void FireEffect::initHeatArrays() {
    // Allocate memory for heat arrays
    heatCore = new byte[LED_STRIP_CORE_COUNT];
    heatInner = new byte[LED_STRIP_INNER_COUNT];
    heatOuter = new byte[LED_STRIP_OUTER_COUNT];

    // Initialize heat values with gradient (hotter at bottom)
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        // Hotter at bottom (closer to 0 index)
        int height = LED_STRIP_CORE_COUNT - i - 1;
        heatCore[i] = constrain(255 - (height * 255 / LED_STRIP_CORE_COUNT) * 1.2, 0, 255);
    }

    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        // Calculate which segment this pixel belongs to
        int segment = i / INNER_LEDS_PER_STRIP;
        int localPos = i % INNER_LEDS_PER_STRIP;

        // Hotter at bottom of each segment
        int height = INNER_LEDS_PER_STRIP - localPos - 1;
        heatInner[i] = constrain(255 - (height * 255 / INNER_LEDS_PER_STRIP) * 1.2, 0, 255);
    }

    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        // Calculate which segment this pixel belongs to
        int segment = i / OUTER_LEDS_PER_STRIP;
        int localPos = i % OUTER_LEDS_PER_STRIP;

        // Hotter at bottom of each segment
        int height = OUTER_LEDS_PER_STRIP - localPos - 1;
        heatOuter[i] = constrain(255 - (height * 255 / OUTER_LEDS_PER_STRIP) * 1.2, 0, 255);
    }
}

void FireEffect::reset() {
    // Reset all heat values
    initHeatArrays();

    // Clear all particles
    embers.clear();
    smoke.clear();

    // Reset timing variables
    lastUpdateTime = millis();
    lastEmberTime = millis();
}

void FireEffect::update() {
    // Get current time
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - lastUpdateTime;

    // Calculate dynamic "frame delta" for smoother animation
    // regardless of actual update frequency
    float deltaTime = elapsed / 33.0f; // Normalize to ~30fps

    // Update core fire simulation
    updateFireBase();

    // Update particle effects
    updateEmbers();
    updateSmoke();

    // Render the fire
    renderFire();

    // Show all strips
    leds.showAll();

    // Update timing
    lastUpdateTime = currentTime;
}

void FireEffect::updateFireBase() {
    // 1. COOLING: How much does the air cool as it rises?
    // Less cooling = taller flames, more cooling = shorter flames
    // Cooler flames are less intense and redder, hotter flames are more energetic and yellow
    int cooling = map(100 - intensity, 0, 100, 20, 80);

    // 2. SPARKING: How many sparks will be added to the fire base this cycle?
    // More sparks = more active and taller fire
    int sparking = map(intensity, 0, 100, 50, 200);

    // 3. CORE STRIP (CENTER OF FIRE)
    // Core strip should have minimal cooling as it's the center of the fire
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        // Apply cooling based on height
        int heightCooling = cooling * (i * 1.0 / LED_STRIP_CORE_COUNT); // More cooling at top
        int cooldown = random8(0, heightCooling);

        // Apply the cooling - use FastLED's qadd8/qsub8 for better performance
        heatCore[i] = qsub8(heatCore[i], cooldown);
    }

    // Sparking: add new energy at the base
    if (random8() < sparking) {
        int y = random8(min(7, LED_STRIP_CORE_COUNT / 5)); // Limited to bottom section
        int sparkHeat = random8(160, 255); // Create new spark at the base
        heatCore[y] = qadd8(heatCore[y], sparkHeat); // Add the spark (with saturation)
    }

    // Heat rises: blend each cell with cells above it
    for (int i = LED_STRIP_CORE_COUNT - 1; i >= 2; i--) {
        // Get a blend of the cells around this one, with bias upward
        heatCore[i] = (heatCore[i - 1] + heatCore[i - 2] + heatCore[i - 2]) / 3;
    }

    // 4. INNER STRIPS (MAIN FLAME BODY)
    // Process each segment separately
    for (int segment = 0; segment < NUM_INNER_STRIPS; segment++) {
        int startIdx = segment * INNER_LEDS_PER_STRIP;
        int endIdx = (segment + 1) * INNER_LEDS_PER_STRIP - 1;

        // Cool each LED
        for (int i = startIdx; i <= endIdx; i++) {
            // More cooling for inner flames
            int localPos = i - startIdx;
            int heightCooling = cooling * 1.2 * (localPos * 1.0 / INNER_LEDS_PER_STRIP);
            int cooldown = random8(0, heightCooling);

            heatInner[i] = qsub8(heatInner[i], cooldown);
        }

        // Sparking at the base of each segment
        if (random8() < sparking) {
            int localY = random8(min(5, INNER_LEDS_PER_STRIP / 5));
            int sparkHeat = random8(140, 240); // Slightly cooler than core
            heatInner[startIdx + localY] = qadd8(heatInner[startIdx + localY], sparkHeat);
        }

        // Heat rises within each segment
        for (int i = endIdx; i >= startIdx + 2; i--) {
            heatInner[i] = (heatInner[i - 1] + heatInner[i - 2] + heatInner[i - 2]) / 3;
        }
    }

    // 5. OUTER STRIPS (FLAME EDGES)
    // Process each segment separately
    for (int segment = 0; segment < NUM_OUTER_STRIPS; segment++) {
        int startIdx = segment * OUTER_LEDS_PER_STRIP;
        int endIdx = (segment + 1) * OUTER_LEDS_PER_STRIP - 1;

        // Cool each LED - outer edges cool more rapidly
        for (int i = startIdx; i <= endIdx; i++) {
            // More aggressive cooling for outer edges
            int localPos = i - startIdx;
            int heightCooling = cooling * 1.5 * (localPos * 1.0 / OUTER_LEDS_PER_STRIP);
            int cooldown = random8(0, heightCooling);

            heatOuter[i] = qsub8(heatOuter[i], cooldown);
        }

        // Sparking - less frequent for outer strips
        if (random8() < sparking * 0.7) {
            int localY = random8(min(4, OUTER_LEDS_PER_STRIP / 6));
            int sparkHeat = random8(100, 200); // Cooler than inner strips
            heatOuter[startIdx + localY] = qadd8(heatOuter[startIdx + localY], sparkHeat);
        }

        // Heat rises but dissipates more quickly on the edges
        for (int i = endIdx; i >= startIdx + 2; i--) {
            heatOuter[i] = (heatOuter[i - 1] + heatOuter[i - 2] + heatOuter[i - 3]) / 3;
        }
    }
}

void FireEffect::updateEmbers() {
    unsigned long currentTime = millis();

    // Create new embers based on fire intensity
    int emberRate = map(intensity, 0, 100, 2000, 500); // Time between embers in ms
    if (currentTime - lastEmberTime > emberRate) {
        createEmber();
        lastEmberTime = currentTime;
    }

    // Pre-calculate wind factor for efficiency
    float windFactor = map(windStrength, 0, 100, 0.0f, 0.05f);
    float windOffset = (windDirection - 128) / 128.0f; // -1.0 to 1.0

    // Update all ember particles
    for (auto it = embers.begin(); it != embers.end();) {
        // Apply physics: gravity counteracted by buoyancy
        it->vy -= 0.03; // Embers float upward (negative y velocity)

        // Apply wind effect to horizontal movement
        it->vx += windOffset * windFactor;

        // Update positions
        it->x += it->vx;
        it->y += it->vy;

        // Embers cool as they rise - use FastLED's qsub8 for safety
        it->heat = qsub8(it->heat, 2);

        // Remove embers that are too cool or out of bounds
        if (it->heat < 5 || it->y < -5 || it->y > LED_STRIP_OUTER_COUNT ||
            it->x < 0 || it->x >= OUTER_LEDS_PER_STRIP) {
            it = embers.erase(it);
        } else {
            ++it;
        }
    }
}

void FireEffect::updateSmoke() {
    // Smoke simulation will be minimal for now
    // Smoke will just be handled as a dark haze at the top of the fire
    // This is a placeholder for more advanced smoke effects
}

void FireEffect::createEmber() {
    Ember newEmber;

    // Embers spawn from the base of the fire
    newEmber.x = random(OUTER_LEDS_PER_STRIP);
    newEmber.y = OUTER_LEDS_PER_STRIP - random(5);  // Near the bottom

    // Random velocity, mostly upward
    newEmber.vx = (random(20) - 10) / 40.0f;  // Small horizontal movement
    newEmber.vy = -random(10, 25) / 80.0f;    // Upward movement

    // Hot embers with some color variation
    newEmber.heat = random(150, 255);
    newEmber.hue = random(0, 3000);  // Reddish-orange range

    // Add to ember list
    embers.push_back(newEmber);
}

void FireEffect::renderFire() {
    // 1. First clear all strips
    leds.clearAll();

    // 2. Render core strip
    for (int i = 0; i < LED_STRIP_CORE_COUNT; i++) {
        if (heatCore[i] > 0) {
            int physicalPos = mapLEDPosition(0, i);
            leds.getCore()[physicalPos] = HeatColor(heatCore[i]);
        }
    }

    // 3. Render inner strips
    for (int i = 0; i < LED_STRIP_INNER_COUNT; i++) {
        if (heatInner[i] > 0) {
            int segment = i / INNER_LEDS_PER_STRIP;
            int localPos = i % INNER_LEDS_PER_STRIP;
            int physicalPos = mapLEDPosition(1, localPos, segment);
            physicalPos += segment * INNER_LEDS_PER_STRIP;
            leds.getInner()[physicalPos] = HeatColor(heatInner[i]);
        }
    }

    // 4. Render outer strips
    for (int i = 0; i < LED_STRIP_OUTER_COUNT; i++) {
        if (heatOuter[i] > 0) {
            int segment = i / OUTER_LEDS_PER_STRIP;
            int localPos = i % OUTER_LEDS_PER_STRIP;
            int physicalPos = mapLEDPosition(2, localPos, segment);
            physicalPos += segment * OUTER_LEDS_PER_STRIP;
            leds.getOuter()[physicalPos] = HeatColor(heatOuter[i]);
        }
    }

    // 5. Render embers (as individual bright pixels)
    for (const auto& ember : embers) {
        // Calculate which segment this ember belongs to
        int segment = static_cast<int>(ember.x) / OUTER_LEDS_PER_STRIP;
        int localPos = static_cast<int>(ember.x) % OUTER_LEDS_PER_STRIP;
        int verticalPos = static_cast<int>(ember.y);

        // Skip if out of bounds
        if (segment >= NUM_OUTER_STRIPS || verticalPos >= OUTER_LEDS_PER_STRIP) continue;

        // Create ember color - use FastLED's optimized functions
        CRGB emberColor;

        // Embers are brighter and more orange-yellow
        if (ember.heat > 180) {
            // Very hot embers get a bit of blue/white in the center
            emberColor = CRGB(255, scale8(ember.heat, 180), scale8(ember.heat - 180, 80));
        }
        else {
            // Normal hot embers are orange/yellow
            emberColor = CRGB(255, scale8(ember.heat, 150), 0);
        }

        // Calculate physical position
        int physicalPos = mapLEDPosition(2, localPos, segment);
        physicalPos += segment * OUTER_LEDS_PER_STRIP;

        // Draw ember on outer strips if position is valid
        if (physicalPos >= 0 && physicalPos < LED_STRIP_OUTER_COUNT) {
            leds.getOuter()[physicalPos] = emberColor;
        }
    }
}

int FireEffect::mapLEDPosition(int stripType, int position, int subStrip) {
    // Use the LED controller's mapping function
    return leds.mapPositionToPhysical(stripType, position, subStrip);
}

void FireEffect::setIntensity(byte newIntensity) {
    // Clamp intensity to 0-100
    intensity = constrain(newIntensity, 0, 100);

    // Adjust animation speed based on intensity
    animationSpeed = map(intensity, 0, 100, 0.7, 1.3);
}