/**
 * @file LEDPatterns.cpp
 * @brief Implementation file for LED patterns for WS2812B LED strips
 * 
 * Simplified version that focuses only on the gentle glow effect 
 * with minimal transitions to improve responsiveness.
 */

#include "LEDPatterns.h"

/**
 * Constructor - initializes the LED pattern handler
 * 
 * @param leds Pointer to the CRGB LED array
 * @param numLeds Number of LEDs in the array
 */
LEDPatterns::LEDPatterns(CRGB* leds, uint16_t numLeds) {
    this->leds = leds;
    this->numLeds = numLeds;
    this->currentPatternIndex = 0;
    this->transitionActive = false;
    this->transitionStartTime = 0;
    this->transitionDuration = 0;
    
    // Initialize previous state array
    this->prevLeds = new CRGB[numLeds];
    for (uint16_t i = 0; i < numLeds; i++) {
        this->prevLeds[i] = CRGB::Black;
    }
    
    // Initialize glow points array with empty glow points
    for (uint8_t i = 0; i < MAX_GLOW_POINTS; i++) {
        glowPoints[i].active = false;
    }
}

/**
 * Destructor - cleans up allocated memory
 */
LEDPatterns::~LEDPatterns() {
    delete[] this->prevLeds;
}

/**
 * Sets all LEDs to a solid color
 * 
 * @param color The CRGB color to set
 */
void LEDPatterns::solid(CRGB color) {
    fill_solid(leds, numLeds, color);
}

/**
 * Creates a gradient between two colors across the LED strip
 * 
 * @param startColor First color in the gradient
 * @param endColor Second color in the gradient
 */
void LEDPatterns::gradient(CRGB startColor, CRGB endColor) {
    fill_gradient_RGB(leds, 0, startColor, numLeds - 1, endColor);
}

/**
 * Captures the current LED state to use as the starting point for transitions
 */
void LEDPatterns::captureCurrentState() {
    for (uint16_t i = 0; i < numLeds; i++) {
        prevLeds[i] = leds[i];
    }
}

/**
 * Starts a transition from the previous state to the current state
 * 
 * @param duration Duration of the transition in milliseconds
 */
void LEDPatterns::startTransition(uint16_t duration) {
    transitionActive = true;
    transitionStartTime = millis();
    transitionDuration = duration;
}

/**
 * Updates active transitions by blending between previous and current states
 * 
 * @return True if a transition is still in progress, false otherwise
 */
bool LEDPatterns::updateTransitions() {
    if (!transitionActive) {
        return false;
    }
    
    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - transitionStartTime;
    
    if (elapsed >= transitionDuration) {
        // Transition complete
        transitionActive = false;
        return false;
    }
    
    // Calculate progress (0.0 to 1.0)
    float progress = (float)elapsed / (float)transitionDuration;
    
    // Blend the colors based on progress
    for (uint16_t i = 0; i < numLeds; i++) {
        leds[i] = blend(prevLeds[i], leds[i], progress * 255);
    }
    
    return true;
}

/**
 * Adds a new glow point to the glow effect if one is available
 * 
 * @param pos Position for the new glow point
 * @param hue Hue for the glow point
 * @param spread Spread of the glow effect in LEDs
 * @param intensity Maximum intensity of the glow point
 * @return Index of the new glow point, or -1 if none available
 */
int8_t LEDPatterns::addGlowPoint(uint16_t pos, uint8_t hue, uint8_t spread, uint8_t intensity) {
    // Find an inactive glow point slot
    for (uint8_t i = 0; i < MAX_GLOW_POINTS; i++) {
        if (!glowPoints[i].active) {
            glowPoints[i].active = true;
            glowPoints[i].pos = pos;
            glowPoints[i].hue = hue;
            glowPoints[i].spread = spread;
            glowPoints[i].maxIntensity = intensity;
            glowPoints[i].currentIntensity = 0;
            glowPoints[i].state = 1; // Growing
            return i;
        }
    }
    return -1; // No available glow point slots
}

/**
 * Updates a glow point's state and intensity
 * 
 * @param index Index of the glow point to update
 * @param speed Speed of the glow effect (higher = faster)
 * @return True if the glow point is still active, false if it's been removed
 */
bool LEDPatterns::updateGlowPoint(uint8_t index, uint8_t speed) {
    if (!glowPoints[index].active) {
        return false;
    }
    
    // Validate state - fix if corrupted
    if (glowPoints[index].state != 1 && glowPoints[index].state != 0 && glowPoints[index].state != -1) {
        glowPoints[index].state = -1; // Force to fading state if corrupted
    }
    
    // Update intensity based on state
    uint8_t change = max(1, speed / 20); // Minimum change of 1, max of 12 for speed 255
    
    if (glowPoints[index].state == 1) { // Growing
        // Calculate new intensity but prevent overflow
        uint8_t newIntensity = glowPoints[index].currentIntensity;
        if (255 - newIntensity > change) {
            newIntensity += change;
        } else {
            newIntensity = 255;
        }
        
        // Cap at max intensity
        if (newIntensity > glowPoints[index].maxIntensity) {
            newIntensity = glowPoints[index].maxIntensity;
        }
        
        glowPoints[index].currentIntensity = newIntensity;
        
        // If reached max intensity, switch to stable state
        if (glowPoints[index].currentIntensity >= glowPoints[index].maxIntensity) {
            glowPoints[index].state = 0; // Stable
        }
    }
    else if (glowPoints[index].state == -1) { // Fading
        if (glowPoints[index].currentIntensity <= change) {
            // Intensity would go to zero or negative, remove this glow point
            glowPoints[index].active = false;
            return false;
        }
        glowPoints[index].currentIntensity -= change;
    }
    else if (glowPoints[index].state == 0) { // Stable, random chance to start fading
        if (random8() < 5) { // ~2% chance per update to start fading
            glowPoints[index].state = -1; // Start fading
        }
    }
    
    return true;
}

/**
 * Renders a glow point into the LED array
 * 
 * @param index Index of the glow point to render
 */
void LEDPatterns::renderGlowPoint(uint8_t index) {
    if (!glowPoints[index].active) {
        return;
    }
    
    // Calculate the intensity value as a fraction of 255
    uint8_t intensity = glowPoints[index].currentIntensity;
    uint8_t hue = glowPoints[index].hue;
    uint8_t spread = glowPoints[index].spread;
    uint16_t pos = glowPoints[index].pos;
    
    // Create a "glow" that spans multiple LEDs with falloff from center
    for (int16_t i = -spread; i <= spread; i++) {
        // Calculate position with wraparound
        int16_t actualPos = (pos + i) % numLeds;
        if (actualPos < 0) actualPos += numLeds;
        
        // Calculate falloff based on distance from center (quadratic falloff)
        float falloff = 1.0 - (float)(i * i) / (float)((spread + 1) * (spread + 1));
        if (falloff < 0) falloff = 0;
        
        // Calculate color with intensity factored in
        CHSV hsv(hue, 240, intensity * falloff);
        CRGB rgb;
        hsv2rgb_rainbow(hsv, rgb);
        
        // Blend with existing color (additive blending)
        leds[actualPos] += rgb;
    }
}

/**
 * Creates a gentle breathing/glowing effect with organic movement
 * 
 * @param hue Base hue of the glow effect
 * @param brightness Maximum brightness of the glow points
 * @param speed Speed of the animation (higher = faster)
 * @param spread Width of each glow point in LEDs
 */
void LEDPatterns::gentleGlow(uint8_t hue, uint8_t brightness, uint8_t speed, uint8_t spread) {
    // Clear LEDs first
    fill_solid(leds, numLeds, CRGB::Black);
    
    // Update existing glow points
    for (uint8_t i = 0; i < MAX_GLOW_POINTS; i++) {
        if (glowPoints[i].active) {
            // Update the glow point state
            bool stillActive = updateGlowPoint(i, speed);
            
            // If still active, render it
            if (stillActive) {
                renderGlowPoint(i);
            }
        }
    }
    
    // Count active glow points
    uint8_t activeCount = 0;
    for (uint8_t i = 0; i < MAX_GLOW_POINTS; i++) {
        if (glowPoints[i].active) {
            activeCount++;
        }
    }
    
    // Add new glow points periodically based on speed
    if (activeCount < MAX_GLOW_POINTS) {
        uint8_t threshold = map(speed, 0, 255, 250, 180); // Faster speed = lower threshold = more frequent additions
        if (random8() > threshold) {
            uint16_t pos = random16(numLeds);
            uint8_t hueVar = random8(10); // Small hue variation
            uint8_t spreadVar = random8(2); // Small spread variation
            uint8_t intensityVar = random8(20); // Intensity variation
            
            addGlowPoint(pos, 
                        hue + hueVar - 5, // +/- 5 hue variation
                        spread + spreadVar - 1, // +/- 1 spread variation
                        brightness - intensityVar); // Slightly reduced random intensity
        }
    }
}

/**
 * Apply a solid color pattern (HSV)
 * Used for error indicators
 */
void LEDPatterns::solid(CHSV color) {
    fill_solid(leds, numLeds, color);
}

/**
 * Legacy gradient function with CHSV colors for backward compatibility
 * 
 * @param startColor First color in the gradient (CHSV)
 * @param endColor Second color in the gradient (CHSV)
 */
void LEDPatterns::gradient(CHSV startColor, CHSV endColor) {
    // Convert CHSV to CRGB and use the RGB version
    CRGB startRGB, endRGB;
    hsv2rgb_rainbow(startColor, startRGB);
    hsv2rgb_rainbow(endColor, endRGB);
    
    gradient(startRGB, endRGB);
}

/**
 * Apply a twinkle effect
 * Used for error indicator
 */
void LEDPatterns::twinkle(CHSV color, uint8_t chance) {
    // Dim all LEDs slightly
    fadeToBlackBy(leds, numLeds, 10);
    
    // Randomly add bright pixels
    for (uint16_t i = 0; i < numLeds; i++) {
        if (random8() < chance) {
            leds[i] = CHSV(color.h, color.s, 255);
        }
    }
} 