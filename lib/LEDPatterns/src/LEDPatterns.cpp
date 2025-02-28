/**
 * @file LEDPatterns.cpp
 * @brief Implementation file for LED patterns for WS2812B LED strips
 */

#include "LEDPatterns.h"

/**
 * Constructor for LEDPatterns
 */
LEDPatterns::LEDPatterns(CRGB* leds, uint16_t numLeds) :
    _leds(leds),
    _numLeds(numLeds),
    _lastUpdate(0),
    _step(0),
    _heat(nullptr)
{
    // Allocate memory for fire effect if more than 0 LEDs
    if (_numLeds > 0) {
        _heat = new uint8_t[_numLeds];
        // Initialize heat array
        for (uint16_t i = 0; i < _numLeds; i++) {
            _heat[i] = 0;
        }
    }
}

/**
 * Apply a solid color pattern (RGB)
 */
void LEDPatterns::solid(CRGB color) {
    fill_solid(_leds, _numLeds, color);
}

/**
 * Apply a solid color pattern (HSV)
 */
void LEDPatterns::solid(CHSV color) {
    fill_solid(_leds, _numLeds, color);
}

/**
 * Apply a breathing effect
 */
void LEDPatterns::breathing(CHSV color, uint8_t speed) {
    // Calculate brightness based on time
    uint8_t brightness = beatsin8(speed, 0, 255);
    
    // Apply brightness to the color
    CHSV adjustedColor = CHSV(color.h, color.s, brightness);
    
    // Fill all LEDs with the adjusted color
    fill_solid(_leds, _numLeds, adjustedColor);
}

/**
 * Apply a gradient between two colors
 */
void LEDPatterns::gradient(CHSV startColor, CHSV endColor) {
    fill_gradient_HSV(_leds, 0, startColor, _numLeds - 1, endColor, SHORTEST_HUES);
}

/**
 * Apply a rainbow effect
 */
void LEDPatterns::rainbow(uint8_t speed) {
    // Get current time for animation
    uint32_t ms = millis();
    
    // Only update if enough time has passed
    if (ms - _lastUpdate >= 20) {
        _lastUpdate = ms;
        
        // Increment step based on speed
        _step += speed;
        
        // Fill the LEDs with a gradient from current step
        fill_rainbow(_leds, _numLeds, _step, 255 / _numLeds);
    }
}

/**
 * Apply a chase effect
 */
void LEDPatterns::chase(CHSV color, CHSV bgColor, uint8_t size, uint8_t speed) {
    // Start by filling with background color
    fill_solid(_leds, _numLeds, bgColor);
    
    // Get current time for animation
    uint32_t ms = millis();
    
    // Update step based on time and speed
    uint16_t step = (ms / (256 - speed)) % _numLeds;
    
    // Draw chase dots
    for (uint8_t i = 0; i < size; i++) {
        uint16_t pos = (step + i) % _numLeds;
        _leds[pos] = color;
    }
}

/**
 * Apply a pulse effect
 */
void LEDPatterns::pulse(CHSV color, uint8_t speed) {
    // Get current time
    uint32_t ms = millis();
    
    // Create a pulse using a sine wave
    uint8_t brightness = beatsin8(speed, 0, 255, ms, 0);
    
    // Propagate the pulse through the strip
    for (uint16_t i = 0; i < _numLeds; i++) {
        // Phase shift based on LED position to create a wave
        uint8_t wave = beatsin8(speed, 0, 255, ms, i * 10);
        
        // Use phase-shifted brightness for this LED
        _leds[i] = CHSV(color.h, color.s, wave);
    }
}

/**
 * Apply a fire effect
 * 
 * This is based on FastLED's Fire2012 example
 */
void LEDPatterns::fire(uint8_t cooling, uint8_t sparking) {
    // Make sure we have memory allocated for the heat array
    if (_heat == nullptr) {
        return;
    }
    
    // Get current time for animation
    uint32_t ms = millis();
    
    // Only update if enough time has passed
    if (ms - _lastUpdate >= 20) {
        _lastUpdate = ms;
        
        // Step 1: Cool down every cell a little
        for (int i = 0; i < _numLeds; i++) {
            _heat[i] = qsub8(_heat[i], random8(0, ((cooling * 10) / _numLeds) + 2));
        }
        
        // Step 2: Heat from each cell drifts up and diffuses a little
        for (int k = _numLeds - 1; k >= 2; k--) {
            _heat[k] = (_heat[k - 1] + _heat[k - 2] + _heat[k - 2]) / 3;
        }
        
        // Step 3: Randomly ignite new 'sparks' of heat near the bottom
        if (random8() < sparking) {
            int y = random8(7);
            _heat[y] = qadd8(_heat[y], random8(160, 255));
        }
        
        // Step 4: Map from heat cells to LED colors
        for (int j = 0; j < _numLeds; j++) {
            // Scale heat value into a spectrum from black to white
            CRGB color = HeatColor(_heat[j]);
            _leds[j] = color;
        }
    }
}

/**
 * Apply a twinkle effect
 */
void LEDPatterns::twinkle(CHSV color, uint8_t chance) {
    // Dim all LEDs slightly
    fadeToBlackBy(_leds, _numLeds, 10);
    
    // Randomly add bright pixels
    for (uint16_t i = 0; i < _numLeds; i++) {
        if (random8() < chance) {
            _leds[i] = CHSV(color.h, color.s, 255);
        }
    }
} 