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
 * @brief Structure to track individual glow points with simplified fields
 */
struct GlowPoint {
    uint16_t pos;             // Position of the glow point
    uint8_t hue;              // Hue value
    uint8_t spread;           // Spread of the glow (how many LEDs it affects)
    uint8_t maxIntensity;     // Maximum intensity to reach
    uint8_t currentIntensity; // Current intensity of the glow point (0-255)
    int8_t state;             // State: 1=growing, 0=stable, -1=fading
    bool active;              // Whether this glow point is active
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
 * @brief Simplified class for generating LED patterns
 */
class LEDPatterns {
public:
    /**
     * @brief Constructor - initializes the LED pattern handler
     * 
     * @param leds Pointer to CRGB array
     * @param numLeds Number of LEDs in the array
     */
    LEDPatterns(CRGB* leds, uint16_t numLeds);
    
    /**
     * @brief Destructor - cleans up allocated memory
     */
    ~LEDPatterns();
    
    /**
     * @brief Sets all LEDs to a solid color
     * 
     * @param color The CRGB color to set
     */
    void solid(CRGB color);
    
    /**
     * @brief Sets all LEDs to a solid color (HSV version)
     * 
     * @param color The CHSV color to set
     */
    void solid(CHSV color);
    
    /**
     * @brief Creates a gradient between two colors across the LED strip
     * 
     * @param startColor First color in the gradient
     * @param endColor Second color in the gradient
     */
    void gradient(CRGB startColor, CRGB endColor);
    
    /**
     * @brief Legacy gradient function with CHSV colors for backward compatibility
     * 
     * @param startColor First color in the gradient (CHSV)
     * @param endColor Second color in the gradient (CHSV)
     */
    void gradient(CHSV startColor, CHSV endColor);
    
    /**
     * @brief Captures the current LED state to use as the starting point for transitions
     */
    void captureCurrentState();
    
    /**
     * @brief Starts a transition from the previous state to the current state
     * 
     * @param duration Duration of the transition in milliseconds
     */
    void startTransition(uint16_t duration);
    
    /**
     * @brief Updates active transitions by blending between previous and current states
     * 
     * @return True if a transition is still in progress, false otherwise
     */
    bool updateTransitions();
    
    /**
     * @brief Creates a gentle breathing/glowing effect with organic movement
     * 
     * @param hue Base hue of the glow effect
     * @param brightness Maximum brightness of the glow points
     * @param speed Speed of the animation (higher = faster)
     * @param spread Width of each glow point in LEDs
     */
    void gentleGlow(uint8_t hue, uint8_t brightness, uint8_t speed, uint8_t spread = 5);
    
    /**
     * @brief Apply a twinkle effect
     * 
     * @param color Base color
     * @param chance Chance of a twinkle (1-100)
     */
    void twinkle(CHSV color, uint8_t chance = 10);
    
    // Stubs for backward compatibility - empty implementations
    void breathing(CHSV color, uint8_t speed = 10) {}
    void rainbow(uint8_t speed = 10) {}
    void chase(CHSV color, CHSV bgColor, uint8_t size = 3, uint8_t speed = 10) {}
    void pulse(CHSV color, uint8_t speed = 10) {}
    void fire(uint8_t cooling = 55, uint8_t sparking = 120) {}

private:
    CRGB* leds;                      // Pointer to the LED array
    CRGB* prevLeds;                  // Previous LED state for transitions
    uint16_t numLeds;                // Number of LEDs
    uint8_t currentPatternIndex;     // Current pattern index
    
    // Glow effect variables
    GlowPoint glowPoints[MAX_GLOW_POINTS]; // Array of active glow points
    
    // Transition variables
    uint32_t transitionStartTime;    // When the transition started
    uint16_t transitionDuration;     // Duration of the transition in ms
    bool transitionActive;           // Whether a transition is in progress
    
    /**
     * @brief Adds a new glow point to the glow effect if one is available
     * 
     * @param pos Position for the new glow point
     * @param hue Hue for the glow point
     * @param spread Spread of the glow effect in LEDs
     * @param intensity Maximum intensity of the glow point
     * @return Index of the new glow point, or -1 if none available
     */
    int8_t addGlowPoint(uint16_t pos, uint8_t hue, uint8_t spread, uint8_t intensity);
    
    /**
     * @brief Updates a glow point's state and intensity
     * 
     * @param index Index of the glow point to update
     * @param speed Speed of the glow effect (higher = faster)
     * @return True if the glow point is still active, false if it's been removed
     */
    bool updateGlowPoint(uint8_t index, uint8_t speed);
    
    /**
     * @brief Renders a glow point into the LED array
     * 
     * @param index Index of the glow point to render
     */
    void renderGlowPoint(uint8_t index);
};

#endif // LED_PATTERNS_H 