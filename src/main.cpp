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

// LED pattern intensity thresholds
const uint8_t INTENSITY_LOW = 64;     // Threshold for low intensity effects (breathing)
const uint8_t INTENSITY_MEDIUM = 128; // Threshold for medium intensity effects (pulse)
const uint8_t INTENSITY_HIGH = 192;   // Threshold for high intensity effects (chase/fire/twinkle)
const uint8_t INTENSITY_MAX = 255;    // Maximum intensity value

// Color range constants
const uint8_t HUE_HIGH = 0;    // Red (for high intensity)
const uint8_t HUE_LOW = 160;   // Blue (for low intensity)

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

// Pattern selection
PatternType currentPattern = PATTERN_BREATHING;

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
  FastLED.setBrightness(50); // 0-255
  FastLED.clear();
  FastLED.show();
  Serial.println("LEDs initialized");
}

/**
 * Update LED pattern based on sensor data
 * 
 * @param presence Whether human presence is detected
 * @param motion Whether motion is detected
 * @param intensity Overall intensity value (0-255)
 */
void updateLEDPattern(bool presence, bool motion, uint8_t intensity) {
  // Calculate hue based on intensity (blue when low, red when high)
  uint8_t hue = map(intensity, 0, INTENSITY_MAX, HUE_LOW, HUE_HIGH);
  
  if (presence || motion) {
    // Presence or motion detected - select pattern based on intensity
    if (intensity < INTENSITY_LOW) {
      // Low intensity - breathing effect
      ledPatterns.breathing(CHSV(hue, 255, 255), map(intensity, 0, INTENSITY_LOW, 5, 15));
    } 
    else if (intensity < INTENSITY_MEDIUM) {
      // Medium-low intensity - pulse effect
      ledPatterns.pulse(CHSV(hue, 255, 255), map(intensity, INTENSITY_LOW, INTENSITY_MEDIUM, 5, 20));
    }
    else if (intensity < INTENSITY_HIGH) {
      // Medium-high intensity - chase effect
      ledPatterns.chase(CHSV(hue, 255, 255), CHSV(hue, 128, 64), 3, map(intensity, INTENSITY_MEDIUM, INTENSITY_HIGH, 10, 40));
    }
    else {
      // High intensity - fire effect (if motion) or twinkle effect (if presence only)
      if (motion) {
        // Modified parameters for fire effect to prevent white output at high intensity
        // First parameter: cooling (lower = more heat)
        // Second parameter: sparking (higher = more sparks)
        ledPatterns.fire(map(intensity, INTENSITY_HIGH, INTENSITY_MAX, 100, 50), map(intensity, INTENSITY_HIGH, INTENSITY_MAX, 50, 120));
      } else {
        ledPatterns.twinkle(CHSV(hue, 255, 255), map(intensity, INTENSITY_HIGH, INTENSITY_MAX, 10, 40));
      }
    }
  } 
  else {
    // No presence or motion - gentle breathing effect in blue
    ledPatterns.breathing(CHSV(HUE_LOW, 255, 128), 5);
  }
  
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
    
    // Show success pattern
    ledPatterns.gradient(CHSV(96, 255, 255), CHSV(160, 255, 255)); // Green to Blue gradient
    FastLED.show();
    delay(1000);
  }
  
  Serial.println("Setup complete");
}

void loop() {
  // Read sensor data using the correct pattern from examples
  sths34pf80_tmos_drdy_status_t dataReady;
  presenceSensor.getDataReady(&dataReady);
  
  // Always read the latest data and update LEDs whether new data is available or not
  sths34pf80_tmos_func_status_t status;
  presenceSensor.getStatus(&status);
  
  // Update presence detection based on the flag
  bool newPresenceDetected = (status.pres_flag == 1);
  presenceSensor.getPresenceValue(&presenceValue);
  
  // Check if presence is above minimum threshold
  bool presenceAboveThreshold = (abs(presenceValue) > PRESENCE_MIN_VALUE);
  
  // Apply debouncing logic for presence detection
  if (presenceAboveThreshold && newPresenceDetected) {
    // Value is above threshold - increment detection counter
    presenceDetectionCount = min(presenceDetectionCount + 1, 255);
    presenceNonDetectionCount = 0;
    
    // Only set as detected if we have enough consecutive detection frames
    if (presenceDetectionCount >= DEBOUNCE_COUNT) {
      presenceDetected = true;
      // Calculate intensity using logarithmic scaling
      float presenceScaled = abs(presenceValue);
      presenceIntensity = constrain((uint8_t)(log10(presenceScaled + 1) * PRESENCE_LOG_SCALE_FACTOR), 0, INTENSITY_MAX);
    }
  } else {
    // Value is below threshold - increment non-detection counter
    presenceNonDetectionCount = min(presenceNonDetectionCount + 1, 255);
    presenceDetectionCount = 0;
    
    // Only clear detection if we have enough consecutive non-detection frames
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
    // Value is above threshold - increment detection counter
    motionDetectionCount = min(motionDetectionCount + 1, 255);
    motionNonDetectionCount = 0;
    
    // Only set as detected if we have enough consecutive detection frames
    if (motionDetectionCount >= DEBOUNCE_COUNT) {
      motionDetected = true;
      // Calculate intensity using logarithmic scaling
      float motionScaled = abs(motionValue);
      motionIntensity = constrain((uint8_t)(log10(motionScaled + 1) * MOTION_LOG_SCALE_FACTOR), 0, INTENSITY_MAX);
    }
  } else {
    // Value is below threshold - increment non-detection counter
    motionNonDetectionCount = min(motionNonDetectionCount + 1, 255);
    motionDetectionCount = 0;
    
    // Only clear detection if we have enough consecutive non-detection frames
    if (motionNonDetectionCount >= DEBOUNCE_COUNT) {
      motionDetected = false;
      motionIntensity = 0;
    }
  }
  
  // Store current values for next comparison
  lastPresenceValue = presenceValue;
  lastMotionValue = motionValue;
  
  // Use the higher of the two intensities for the LED pattern
  uint8_t combinedIntensity = max(presenceIntensity, motionIntensity);
  
  // Only print to serial when a detection occurs (prevents serial flooding)
  if (presenceDetected || motionDetected) {
    Serial.print("Sensor: ");
    if (presenceDetected) Serial.print("Presence ");
    if (motionDetected) Serial.print("Motion ");
    Serial.print("- Presence Value: ");
    Serial.print(presenceValue);
    Serial.print(", Motion Value: ");
    Serial.print(motionValue);
    Serial.print(", Combined Intensity: ");
    Serial.println(combinedIntensity);
  }
  
  // Update LED pattern based on sensor readings
  updateLEDPattern(presenceDetected, motionDetected, combinedIntensity);
  
  // Small delay to prevent too frequent updates
  delay(10); // Reduced delay for faster response
}