#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include <SparkFun_STHS34PF80_Arduino_Library.h> // Include the official SparkFun library
#include <LEDPatterns.h> // Include from library directory using angle brackets

// Pin Definitions are defined in platformio.ini as build flags:
// LED_PIN, LED_COUNT, I2C_SDA, I2C_SCL

// Sensor scaling constants
const uint8_t PRESENCE_LOG_SCALE_FACTOR = 60;  // Multiplier for log-scaled presence values
const uint8_t MOTION_LOG_SCALE_FACTOR = 70;    // Multiplier for log-scaled motion values

// Detection threshold constants
const uint16_t PRESENCE_THRESHOLD_DEFAULT = 100;  // Default threshold for presence detection
const uint8_t MOTION_THRESHOLD_DEFAULT = 50;     // Default threshold for motion detection
const uint8_t HYSTERESIS_DEFAULT = 25;          // Default hysteresis value

// Minimum absolute value to consider as a valid reading (to filter noise)
const uint16_t PRESENCE_MIN_VALUE = 70;        // Ignore presence values below this threshold
const uint16_t MOTION_MIN_VALUE = 70;          // Increased from 40 to 70 to filter more baseline noise

// Debounce settings to prevent flickering
const uint8_t DEBOUNCE_COUNT = 3;              // Number of consecutive readings required to change state
const uint16_t DEBOUNCE_THRESHOLD = 10;        // Threshold for considering a value stable

// Smoothing constants
const float SMOOTHING_FACTOR = 0.2;            // Higher value = more responsive (0.2 = 80% old value, 20% new value)

// Debug flags - greatly reduced
const bool DEBUG_PRINTS = true;               // Only show essential prints to reduce serial overhead
const bool DEBUG_TRANSITIONS = true;          // Show transition state information

// LED pattern intensity thresholds - keeping these for intensity calculations
const uint8_t INTENSITY_LOW = 64;     // Threshold for low intensity effects
const uint8_t INTENSITY_MEDIUM = 128; // Threshold for medium intensity effects
const uint8_t INTENSITY_HIGH = 192;   // Threshold for high intensity effects
const uint8_t INTENSITY_MAX = 255;    // Maximum intensity value

// LED brightness and speed ranges
const uint8_t BRIGHTNESS_MIN = 80;    // Minimum brightness for any effect (not too dim)
const uint8_t BRIGHTNESS_MAX = 220;   // Maximum brightness (avoid full brightness)
const uint8_t SPEED_MIN = 20;         // Minimum animation speed
const uint8_t SPEED_MAX = 100;        // Maximum animation speed

// Color range constants
const uint8_t HUE_COOL = 160;   // Blue (for low presence/no motion)
const uint8_t HUE_NEUTRAL = 96; // Green (for medium presence)
const uint8_t HUE_WARM = 0;     // Red (for high motion)

// Glow effect spread constants
const uint8_t SPREAD_MIN = 3;  // Minimum spread for gentle glow
const uint8_t SPREAD_MAX = 8;  // Maximum spread for gentle glow

// Transition settings - greatly simplified
const uint16_t TRANSITION_DURATION = 300; // Shortened transition duration for responsiveness
const uint16_t MIN_UPDATE_INTERVAL = 150; // Increased to prevent too frequent updates
const uint16_t MIN_TRANSITION_REST = 350; // Minimum time between transitions to ensure they complete

// LED Array
CRGB leds[LED_COUNT];

// Create instances
STHS34PF80_I2C presenceSensor; // Using the correct class name from the official SparkFun library
LEDPatterns ledPatterns(leds, LED_COUNT);

// Sensor Variables
bool presenceDetected = false;
bool motionDetected = false;
uint8_t motionIntensity = 0;
uint8_t presenceIntensity = 0;
int16_t presenceValue = 0;
int16_t motionValue = 0;

// Smoothed intensity values
float smoothedPresenceIntensity = 0;
float smoothedMotionIntensity = 0;

// Debouncing variables
uint8_t presenceDetectionCount = 0;
uint8_t presenceNonDetectionCount = 0;
uint8_t motionDetectionCount = 0;
uint8_t motionNonDetectionCount = 0;
int16_t lastPresenceValue = 0;
int16_t lastMotionValue = 0;

// Store threshold values locally since there are no getter methods
// Actually, there are getter methods, but we'll keep these variables for easy access
uint16_t presenceThreshold = PRESENCE_THRESHOLD_DEFAULT;
uint8_t motionThreshold = MOTION_THRESHOLD_DEFAULT;
uint8_t hysteresis = HYSTERESIS_DEFAULT;

// Variables for tracking parameter changes
uint8_t lastHue = HUE_COOL;
uint8_t lastBrightness = BRIGHTNESS_MIN;
uint8_t lastSpeed = SPEED_MIN;
uint8_t lastSpread = SPREAD_MIN;
uint32_t lastPatternChange = 0;

/**
 * Test Serial connection with a simple sequence of characters
 */
void testSerial() {
  // Send a sequence of different characters to test the serial connection
  Serial.println();
  Serial.println("---------------------");
  Serial.println("Serial Test Sequence:");
  Serial.println("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  Serial.println("1234567890");
  Serial.println("!@#$%^&*()_+");
  Serial.println("Serial test complete");
  Serial.println("---------------------");
  Serial.println();
}

/**
 * Initialize the I2C communication for sensors
 */
void initI2C() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000); // Set I2C clock to 100kHz
  Serial.println("I2C initialized");
}

/**
 * Initialize the LED strip
 */
void initLEDs() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
  FastLED.setBrightness(80); // Increased from 50 to 80 for more vibrant effects
  FastLED.clear();
  FastLED.show();
  Serial.println("LEDs initialized");
}

/**
 * Check for LED-related hardware issues
 */
void diagnoseHardwareIssues() {
  // Check power settings for FastLED
  Serial.println("\n----- LED Hardware Diagnostics -----");
  Serial.print("LED Count: ");
  Serial.println(LED_COUNT);
  
  // Calculate power requirements
  uint32_t maxCurrentMa = LED_COUNT * 60; // WS2812B can draw up to 60mA per LED at full white
  Serial.print("Maximum potential current: ");
  Serial.print(maxCurrentMa);
  Serial.println(" mA");
  
  // Check if current limiting is needed
  if (maxCurrentMa > 500) {
    Serial.println("WARNING: High power LEDs - consider power limiting with FastLED.setMaxPowerInVoltsAndMilliamps()");
  }
  
  // Set a reasonable power limit to prevent brownouts
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500); 
  Serial.println("Power limited to 500mA for safety");
  
  Serial.println("-----------------------------------\n");
}

/**
 * Apply the elegant glow pattern with parameters modulated by presence and motion values
 * 
 * @param presenceIntensity Presence intensity value (0-255)
 * @param motionIntensity Motion intensity value (0-255)
 */
void updateLEDPattern(bool presence, bool motion, uint8_t presenceIntensity, uint8_t motionIntensity) {
    static uint32_t lastUpdate = 0;
    static uint32_t lastTransitionStart = 0;
    static bool transitionActive = false;
    uint32_t now = millis();
    
    // Apply simple exponential smoothing to the intensity values
    smoothedPresenceIntensity = (SMOOTHING_FACTOR * presenceIntensity) + ((1.0 - SMOOTHING_FACTOR) * smoothedPresenceIntensity);
    smoothedMotionIntensity = (SMOOTHING_FACTOR * motionIntensity) + ((1.0 - SMOOTHING_FACTOR) * smoothedMotionIntensity);
    
    // Use the rounded versions for parameter calculations
    uint8_t smoothedPresenceInt = (uint8_t)smoothedPresenceIntensity;
    uint8_t smoothedMotionInt = (uint8_t)smoothedMotionIntensity;
    
    // Only update parameters if enough time has passed since last update
    if (now - lastUpdate < MIN_UPDATE_INTERVAL) {
        return; // Skip this update cycle
    }
    
    // Check if a transition is in progress
    bool isInTransition = ledPatterns.updateTransitions();
    
    // Update our tracking of transition state
    if (isInTransition) {
        transitionActive = true;
        if (DEBUG_TRANSITIONS && !transitionActive) {
            Serial.println("Transition started");
        }
    } else if (transitionActive) {
        // Transition just ended
        transitionActive = false;
        if (DEBUG_TRANSITIONS) {
            Serial.println("Transition completed");
        }
    }
    
    // Don't start a new transition if one is in progress or not enough rest time
    if (transitionActive || (now - lastTransitionStart < MIN_TRANSITION_REST)) {
        if (DEBUG_TRANSITIONS) {
            Serial.println("Skipping update - transition active or rest period");
        }
        return;
    }
    
    // Calculate new parameters
    uint8_t newHue;
    uint8_t newBrightness;
    uint8_t newSpeed;
    uint8_t newSpread;
    
    // Map presence to brightness (higher presence = brighter)
    newBrightness = map(smoothedPresenceInt, 0, INTENSITY_MAX, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    
    // Map motion to speed (higher motion = faster animation)
    newSpeed = map(smoothedMotionInt, 0, INTENSITY_MAX, SPEED_MIN, SPEED_MAX);
    
    // Blend hue based on both presence and motion
    if (motion && smoothedMotionInt > INTENSITY_MEDIUM) {
        // With high motion, shift toward warm colors (red/orange)
        newHue = map(smoothedMotionInt, INTENSITY_MEDIUM, INTENSITY_MAX, HUE_NEUTRAL, HUE_WARM);
    } 
    else if (presence && smoothedPresenceInt > INTENSITY_LOW) {
        // With presence but low motion, shift toward neutral colors (green)
        newHue = map(smoothedPresenceInt, INTENSITY_LOW, INTENSITY_MAX, HUE_COOL, HUE_NEUTRAL);
    }
    else {
        // Default cool color (blue) for low/no presence
        newHue = HUE_COOL;
    }
    
    // Set spread based on combined intensity
    uint8_t combinedIntensity = max(smoothedPresenceInt, smoothedMotionInt);
    newSpread = map(combinedIntensity, 0, INTENSITY_MAX, SPREAD_MIN, SPREAD_MAX);
    
    // Ensure minimum values for visibility
    if (!presence && !motion) {
        newBrightness = BRIGHTNESS_MIN + 20;
        newSpeed = SPEED_MIN + 5;
    }
    
    // Check if parameters have changed significantly enough to update pattern
    bool parametersChanged = false;
    
    // Slightly higher thresholds to reduce unnecessary updates
    if (abs((int16_t)newHue - (int16_t)lastHue) > 5 ||
        abs((int16_t)newBrightness - (int16_t)lastBrightness) > 8 ||
        abs((int16_t)newSpeed - (int16_t)lastSpeed) > 5 ||
        abs((int16_t)newSpread - (int16_t)lastSpread) > 1) {
        
        parametersChanged = true;
    }
    
    // Update the pattern if parameters have changed
    if (parametersChanged) {
        lastUpdate = now;
        lastPatternChange = now;
        lastTransitionStart = now;
        
        if (DEBUG_PRINTS) {
            Serial.print("LED params: H=");
            Serial.print(newHue);
            Serial.print(", B=");
            Serial.print(newBrightness);
            Serial.print(", S=");
            Serial.print(newSpeed);
            Serial.print(", Spread=");
            Serial.println(newSpread);
        }
        
        // Capture current state for smooth transition
        ledPatterns.captureCurrentState();
        
        // Apply the gentle glow pattern with new parameters
        ledPatterns.gentleGlow(newHue, newBrightness, newSpeed, newSpread);
        
        // Start a quick transition
        ledPatterns.startTransition(TRANSITION_DURATION);
        transitionActive = true; // Mark that we've started a transition
        
        // Update last values
        lastHue = newHue;
        lastBrightness = newBrightness;
        lastSpeed = newSpeed;
        lastSpread = newSpread;
    } else {
        // Always update transitions even if parameters haven't changed
        ledPatterns.updateTransitions();
    }
    
    // Always show
    FastLED.show();
}

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  delay(2000); // Increased delay to give more time for serial port to initialize
  Serial.println("\n\n"); // Add extra newlines to clear any initial garbage
  
  // Test the serial connection
  testSerial();
  
  Serial.println("Reactive LEDs - Starting...");
  
  // Initialize I2C
  initI2C();
  
  // Initialize LED strip
  initLEDs();
  
  // Check for LED hardware issues
  diagnoseHardwareIssues();
  
  // Initialize sensor using the SparkFun library
  if (!presenceSensor.begin()) {
    Serial.println("Failed to initialize presence sensor");
    
    // If sensor not found, blink red three times
    for (int j = 0; j < 3; j++) {
      fill_solid(leds, LED_COUNT, CRGB::Red);
      FastLED.show();
      delay(300);
      FastLED.clear();
      FastLED.show();
      delay(300);
    }
    
    // Then show a warning pattern
    ledPatterns.twinkle(CHSV(0, 255, 255), 20); // Red twinkle
    FastLED.show();
  } else {
    Serial.println("Presence sensor initialized successfully");
    
    // Configure the sensor based on the official examples
    // Enter power-down mode by setting ODR to 0
    presenceSensor.setTmosODR(STHS34PF80_TMOS_ODR_OFF);
    
    // Enable access to embedded functions registers
    presenceSensor.setMemoryBank(STHS34PF80_EMBED_FUNC_MEM_BANK);
    
    // Set thresholds and hysteresis
    presenceSensor.setPresenceThreshold(presenceThreshold);
    presenceSensor.setMotionThreshold(motionThreshold);
    presenceSensor.setPresenceHysteresis(hysteresis);
    presenceSensor.setMotionHysteresis(hysteresis);
    
    // Disable access to embedded functions registers
    presenceSensor.setMemoryBank(STHS34PF80_MAIN_MEM_BANK);
    
    // Enter continuous mode at 30Hz - using the highest available rate for maximum responsiveness
    presenceSensor.setTmosODR(STHS34PF80_TMOS_ODR_AT_30Hz);
    
    Serial.print("Presence threshold set to: ");
    Serial.println(presenceThreshold);
    Serial.print("Motion threshold set to: ");
    Serial.println(motionThreshold);
    Serial.print("Hysteresis set to: ");
    Serial.println(hysteresis);
    
    // Show success pattern - Convert to CRGB for our simplified implementation
    CRGB greenColor = CRGB(0, 255, 0); // Green
    CRGB blueColor = CRGB(0, 0, 255);  // Blue
    
    ledPatterns.gradient(greenColor, blueColor); // Use RGB version instead of CHSV
    FastLED.show();
    delay(1000);
  }
  
  Serial.println("Setup complete");
}

void loop() {
    // Read sensor data
    sths34pf80_tmos_func_status_t status;
    presenceSensor.getStatus(&status);
    
    // Check if presence is detected
    bool newPresenceDetected = (status.pres_flag == 1);
    presenceSensor.getPresenceValue(&presenceValue);
    
    // Check if presence is above minimum threshold
    bool presenceAboveThreshold = (abs(presenceValue) > PRESENCE_MIN_VALUE);
    
    // Apply debouncing logic for presence detection
    if (presenceAboveThreshold && newPresenceDetected) {
        presenceDetectionCount = min(presenceDetectionCount + 1, 255);
        presenceNonDetectionCount = 0;
        
        if (presenceDetectionCount >= DEBOUNCE_COUNT) {
            presenceDetected = true;
            // Calculate intensity using logarithmic scaling
            float presenceScaled = abs(presenceValue);
            presenceIntensity = constrain((uint8_t)(log10(presenceScaled + 1) * PRESENCE_LOG_SCALE_FACTOR), 0, INTENSITY_MAX);
        }
    } else {
        presenceNonDetectionCount = min(presenceNonDetectionCount + 1, 255);
        presenceDetectionCount = 0;
        
        if (presenceNonDetectionCount >= DEBOUNCE_COUNT) {
            presenceDetected = false;
            presenceIntensity = 0;
        }
    }
    
    // Update motion detection based on the flag
    bool newMotionDetected = (status.mot_flag == 1);
    presenceSensor.getMotionValue(&motionValue);
    
    // Check if motion is above minimum threshold
    bool motionAboveThreshold = (abs(motionValue) > MOTION_MIN_VALUE);
    
    // Apply debouncing logic for motion detection
    if (motionAboveThreshold && newMotionDetected) {
        motionDetectionCount = min(motionDetectionCount + 1, 255);
        motionNonDetectionCount = 0;
        
        if (motionDetectionCount >= DEBOUNCE_COUNT) {
            motionDetected = true;
            // Calculate intensity using logarithmic scaling
            float motionScaled = abs(motionValue);
            motionIntensity = constrain((uint8_t)(log10(motionScaled + 1) * MOTION_LOG_SCALE_FACTOR), 0, INTENSITY_MAX);
        }
    } else {
        motionNonDetectionCount = min(motionNonDetectionCount + 1, 255);
        motionDetectionCount = 0;
        
        if (motionNonDetectionCount >= DEBOUNCE_COUNT) {
            motionDetected = false;
            motionIntensity = 0;
        }
    }
    
    // Store current values for next comparison
    lastPresenceValue = presenceValue;
    lastMotionValue = motionValue;
    
    // Only print to serial when a detection occurs
    if ((presenceDetected || motionDetected) && DEBUG_PRINTS) {
        Serial.print("Sensor: ");
        if (presenceDetected) Serial.print("Presence ");
        if (motionDetected) Serial.print("Motion ");
        Serial.print("- P:");
        Serial.print(presenceValue);
        Serial.print(", M:");
        Serial.print(motionValue);
        Serial.print(", P-Int:");
        Serial.print(presenceIntensity);
        Serial.print("/");
        Serial.print(smoothedPresenceIntensity);
        Serial.print(", M-Int:");
        Serial.print(motionIntensity);
        Serial.print("/");
        Serial.println(smoothedMotionIntensity);
    }
    
    // Update LED pattern based on sensor readings
    updateLEDPattern(presenceDetected, motionDetected, presenceIntensity, motionIntensity);
    
    // Small delay to prevent too frequent updates
    delay(15); // Increased from 10ms to 15ms
}