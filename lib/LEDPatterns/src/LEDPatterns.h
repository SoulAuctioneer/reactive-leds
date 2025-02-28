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
    PATTERN_FIRE,          // Fire effect
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
     * @brief Apply a fire effect
     * 
     * @param cooling Rate of cooling (20-100)
     * @param sparking Rate of sparking (50-200)
     */
    void fire(uint8_t cooling = 55, uint8_t sparking = 120);
    
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
    
private:
    CRGB* _leds;           // Pointer to the LED array
    uint16_t _numLeds;     // Number of LEDs
    uint32_t _lastUpdate;  // Last update time for animations
    uint8_t _step;         // Current step in animations
    
    // Internal state for fire effect
    uint8_t* _heat;        // Array for fire effect heat values
};

#endif // LED_PATTERNS_H 