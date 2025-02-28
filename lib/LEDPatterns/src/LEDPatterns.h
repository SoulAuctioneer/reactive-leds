/**
 * @file LEDPatterns.h
 * @brief Header file for LED patterns for WS2812B LED strips
 * 
 * This file contains definitions and function declarations for various
 * LED patterns that can be used with WS2812B LED strips.
 */

#ifndef LED_PATTERNS_H
#define LED_PATTERNS_H

#include <Arduino.h>
#include <FastLED.h>

// Maximum number of active glow points for gentle glow effect
#define MAX_GLOW_POINTS 5

/**
 * @brief Structure to track individual glow points
 */
struct GlowPoint {
    uint16_t position;    // Position of the glow point
    uint8_t intensity;    // Current intensity of the glow point (0-255)
    uint8_t maxIntensity; // Maximum intensity to reach
    int8_t state;         // State: 1=growing, 0=stable, -1=fading
    uint8_t speed;        // Speed of growth/fade
    uint8_t hue;          // Hue value
};

/**
 * @brief LED pattern types
 */
enum PatternType {
    PATTERN_SOLID,         // Solid color
    PATTERN_BREATHING,     // Breathing effect
    PATTERN_GRADIENT,      // Gradient between two colors
    PATTERN_RAINBOW,       // Rainbow effect
    PATTERN_CHASE,         // Chase effect
    PATTERN_PULSE,         // Pulse effect
    PATTERN_GENTLE_GLOW,   // Gentle glow effect (replacing fire)
    PATTERN_TWINKLE,       // Twinkle effect
    NUM_PATTERNS           // Total number of patterns
};

/**
 * @class LEDPatterns
 * @brief Class for generating various LED patterns
 */
class LEDPatterns {
public:
    /**
     * @brief Constructor
     * 
     * @param leds Pointer to CRGB array
     * @param numLeds Number of LEDs in the array
     */
    LEDPatterns(CRGB* leds, uint16_t numLeds);
    
    /**
     * @brief Destructor - Frees memory used for fire effect
     */
    ~LEDPatterns();
    
    /**
     * @brief Apply a solid color pattern
     * 
     * @param color Color to use (CRGB or CHSV)
     */
    void solid(CRGB color);
    void solid(CHSV color);
    
    /**
     * @brief Apply a breathing effect
     * 
     * @param color Base color
     * @param speed Speed of the effect (1-255)
     */
    void breathing(CHSV color, uint8_t speed = 10);
    
    /**
     * @brief Apply a gradient between two colors
     * 
     * @param startColor Start color
     * @param endColor End color
     */
    void gradient(CHSV startColor, CHSV endColor);
    
    /**
     * @brief Apply a rainbow effect
     * 
     * @param speed Speed of the effect (1-255)
     */
    void rainbow(uint8_t speed = 10);
    
    /**
     * @brief Apply a chase effect
     * 
     * @param color Color of the chase
     * @param bgColor Background color
     * @param size Size of the chase (number of LEDs)
     * @param speed Speed of the effect (1-255)
     */
    void chase(CHSV color, CHSV bgColor, uint8_t size = 3, uint8_t speed = 10);
    
    /**
     * @brief Apply a pulse effect
     * 
     * @param color Color of the pulse
     * @param speed Speed of the effect (1-255)
     */
    void pulse(CHSV color, uint8_t speed = 10);
    
    /**
     * @brief Apply a gentle glow effect
     * 
     * Creates soft, random points of light that grow, then fade and diffuse to neighbors
     * 
     * @param baseHue Base hue to start from (will have slight variations)
     * @param brightness Maximum brightness (0-255)
     * @param speed Speed of the effect (1-255)
     * @param spread Amount to spread to neighboring LEDs (1-10)
     */
    void gentleGlow(uint8_t baseHue, uint8_t brightness, uint8_t speed, uint8_t spread = 5);
    
    /**
     * @brief Apply a twinkle effect
     * 
     * @param color Base color
     * @param chance Chance of a twinkle (1-100)
     */
    void twinkle(CHSV color, uint8_t chance = 10);
    
    /**
     * @brief Get the number of LEDs
     * 
     * @return Number of LEDs
     */
    uint16_t getNumLeds() const {
        return _numLeds;
    }
    
    /**
     * @brief Update LED transitions for smooth pattern changes
     * @return True if transitions are still in progress
     */
    bool updateTransitions();
    
    /**
     * @brief Start a transition to a new pattern
     * @param targetPattern Function pointer to the target pattern
     * @param duration Duration of the transition in milliseconds
     */
    void startTransition(uint16_t duration);
    
    /**
     * @brief Capture the current LED state as a starting point for transitions
     */
    void captureCurrentState();
    
    /**
     * @brief Generate the next frame of the current pattern without showing it
     * Allows for pre-calculation of target states
     */
    void generateNextFrame();

    // Target buffer pattern methods - for transition effects
    
    /**
     * @brief Apply solid color pattern to target buffer (RGB)
     * @param color CRGB color to apply to target buffer
     */
    void solidToTarget(CRGB color);
    
    /**
     * @brief Apply solid color pattern to target buffer (HSV)
     * @param color CHSV color to apply to target buffer
     */
    void solidToTarget(CHSV color);
    
    /**
     * @brief Apply breathing effect to target buffer
     * @param color Base color for breathing effect
     * @param speed Speed of the breathing effect (1-255)
     */
    void breathingToTarget(CHSV color, uint8_t speed);

private:
    CRGB* _leds;                // Pointer to the LED array
    CRGB* _targetLeds;          // Target LED state for transitions
    CRGB* _previousLeds;        // Previous LED state for transitions
    uint16_t _numLeds;          // Number of LEDs
    uint32_t _lastUpdate;       // Last update time for animations
    uint8_t _step;              // Current step in animations
    
    // Glow effect variables
    GlowPoint _glowPoints[MAX_GLOW_POINTS]; // Array of active glow points
    uint32_t _lastGlowUpdate;               // Last update time for glow effect
    uint8_t _activeGlowPoints;              // Number of currently active glow points
    
    // Transition variables
    uint32_t _transitionStartTime;   // When the transition started
    uint16_t _transitionDuration;    // Duration of the transition in ms
    bool _isTransitioning;           // Whether a transition is in progress
    
    /**
     * @brief Blend between two colors based on progress (0-255)
     * @param start Starting color
     * @param end Ending color
     * @param progress Blend progress (0-255)
     * @return Blended color
     */
    CRGB blendColors(CRGB start, CRGB end, uint8_t progress);
    
    /**
     * @brief Add a new glow point at a random position
     * 
     * @param baseHue Base hue to use (will be varied slightly)
     * @param brightness Maximum brightness to reach
     * @param speed Speed of growth/fade
     * @return True if a new glow point was added, false if at capacity
     */
    bool addGlowPoint(uint8_t baseHue, uint8_t brightness, uint8_t speed);
    
    /**
     * @brief Update an existing glow point
     * 
     * @param pointIndex Index of the glow point to update
     * @param spread Amount to spread to neighboring LEDs
     * @return True if the glow point is still active, false if it has faded out
     */
    bool updateGlowPoint(uint8_t pointIndex, uint8_t spread);
};

#endif // LED_PATTERNS_H 