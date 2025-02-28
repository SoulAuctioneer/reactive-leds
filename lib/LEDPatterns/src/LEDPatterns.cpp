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
    _targetLeds(nullptr),
    _previousLeds(nullptr),
    _lastGlowUpdate(0),
    _activeGlowPoints(0),
    _transitionStartTime(0),
    _transitionDuration(0),
    _isTransitioning(false)
{
    // Initialize glow points
    for (uint8_t i = 0; i < MAX_GLOW_POINTS; i++) {
        _glowPoints[i].position = 0;
        _glowPoints[i].intensity = 0;
        _glowPoints[i].maxIntensity = 0;
        _glowPoints[i].state = 0;
        _glowPoints[i].speed = 0;
        _glowPoints[i].hue = 0;
    }
    
    if (_numLeds > 0) {
        // Allocate memory for transition arrays
        _targetLeds = new CRGB[_numLeds];
        _previousLeds = new CRGB[_numLeds];
        
        // Initialize transition arrays
        for (uint16_t i = 0; i < _numLeds; i++) {
            _targetLeds[i] = CRGB::Black;
            _previousLeds[i] = CRGB::Black;
        }
    }
}

/**
 * Destructor - free allocated memory
 */
LEDPatterns::~LEDPatterns() {
    if (_targetLeds != nullptr) {
        delete[] _targetLeds;
        _targetLeds = nullptr;
    }
    
    if (_previousLeds != nullptr) {
        delete[] _previousLeds;
        _previousLeds = nullptr;
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
 * Apply a gentle glow effect 
 * Creates soft, random points of light that grow, then fade and diffuse to neighbors
 */
void LEDPatterns::gentleGlow(uint8_t baseHue, uint8_t brightness, uint8_t speed, uint8_t spread) {
    // Get current time
    uint32_t currentTime = millis();
    
    // Dim all LEDs slightly - this creates a nice fading trail effect
    fadeToBlackBy(_leds, _numLeds, 20);
    
    // Only update every 20ms (50fps) to avoid too rapid changes
    if (currentTime - _lastGlowUpdate >= 20) {
        _lastGlowUpdate = currentTime;
        
        // Chance to add a new glow point based on speed
        // Higher speed = more frequent new glow points
        if (random8() < map(speed, 1, 255, 5, 40) && _activeGlowPoints < MAX_GLOW_POINTS) {
            // Try to add a new glow point with the given parameters
            if (addGlowPoint(baseHue, brightness, speed)) {
                _activeGlowPoints++;
            }
        }
        
        // Update existing glow points and remove any that have faded out
        for (uint8_t i = 0; i < MAX_GLOW_POINTS; i++) {
            // Skip inactive glow points
            if (_glowPoints[i].state == 0 && _glowPoints[i].intensity == 0) continue;
            
            // Update this glow point and check if it's still active
            if (!updateGlowPoint(i, spread)) {
                // Glow point has faded out completely
                _glowPoints[i].state = 0;
                _glowPoints[i].intensity = 0;
                _activeGlowPoints = max(0, _activeGlowPoints - 1);
            }
        }
    }
}

/**
 * Add a new glow point at a random position
 */
bool LEDPatterns::addGlowPoint(uint8_t baseHue, uint8_t brightness, uint8_t speed) {
    // Find an inactive slot
    for (uint8_t i = 0; i < MAX_GLOW_POINTS; i++) {
        if (_glowPoints[i].state == 0 && _glowPoints[i].intensity == 0) {
            // Found an inactive slot, initialize a new glow point
            _glowPoints[i].position = random16(_numLeds);
            _glowPoints[i].intensity = 1; // Start very dim
            _glowPoints[i].maxIntensity = map(brightness, 0, 255, 100, 255); // Cap based on brightness param
            _glowPoints[i].state = 1; // Growing
            _glowPoints[i].speed = map(speed, 1, 255, 2, 15); // Map speed to a reasonable range
            
            // Vary hue slightly from the base hue (±15)
            _glowPoints[i].hue = baseHue + random8(31) - 15;
            
            return true;
        }
    }
    return false; // No inactive slots found
}

/**
 * Update an existing glow point
 */
bool LEDPatterns::updateGlowPoint(uint8_t pointIndex, uint8_t spread) {
    GlowPoint& point = _glowPoints[pointIndex];
    
    // Skip inactive points
    if (point.state == 0 && point.intensity == 0) {
        return false;
    }
    
    // Update the intensity based on current state
    if (point.state == 1) { // Growing
        // Increase intensity
        point.intensity = qadd8(point.intensity, point.speed);
        
        // If reached max intensity, transition to stable state briefly
        if (point.intensity >= point.maxIntensity) {
            point.intensity = point.maxIntensity;
            point.state = 0; // Stable
        }
    }
    else if (point.state == 0) { // Stable
        // Stay stable for a brief moment then start fading
        point.state = -1; // Start fading
    }
    else if (point.state == -1) { // Fading
        // Decrease intensity
        point.intensity = qsub8(point.intensity, point.speed / 2); // Fade slower than growth
        
        // If reached 0, mark as inactive
        if (point.intensity <= 1) {
            point.intensity = 0;
            point.state = 0;
            return false;
        }
    }
    
    // Create a color based on hue and current intensity
    CHSV color = CHSV(point.hue, 240, point.intensity);
    
    // Apply the color to the center point
    _leds[point.position] = color;
    
    // Spread to surrounding LEDs with decreasing intensity
    // Control spread based on parameter (higher spread = more diffusion)
    uint8_t maxSpread = map(spread, 1, 10, 2, 8);
    for (uint8_t i = 1; i <= maxSpread; i++) {
        // Calculate intensity falloff (quadratic falloff for more natural look)
        uint8_t falloff = (point.intensity * (maxSpread - i) * (maxSpread - i)) / (maxSpread * maxSpread);
        
        // Skip if the falloff is too small
        if (falloff < 5) continue;
        
        // Apply to LEDs to the left
        int16_t leftPos = (point.position - i + _numLeds) % _numLeds;
        _leds[leftPos] += CHSV(point.hue, 240, falloff);
        
        // Apply to LEDs to the right
        int16_t rightPos = (point.position + i) % _numLeds;
        _leds[rightPos] += CHSV(point.hue, 240, falloff);
    }
    
    return true;
}

/**
 * Capture the current LED state as a starting point for transitions
 */
void LEDPatterns::captureCurrentState() {
    // Copy current LED values to the previous array
    for (uint16_t i = 0; i < _numLeds; i++) {
        _previousLeds[i] = _leds[i];
    }
}

/**
 * Start a transition to a new pattern
 */
void LEDPatterns::startTransition(uint16_t duration) {
    // Store current state before starting transition
    captureCurrentState();
    
    // Set transition parameters
    _transitionStartTime = millis();
    _transitionDuration = duration;
    _isTransitioning = true;
}

/**
 * Generate the next frame of the current pattern without showing it
 * This method needs a function pointer to the pattern method to call
 * 
 * @param patternFunction Function pointer to the pattern function
 * @param args Variable arguments for the pattern function
 */
void LEDPatterns::generateNextFrame() {
    // This is a base implementation that doesn't do anything
    // In practice, you should use the target buffer directly from your pattern functions
    // The switching of buffers is handled in startTransition and updateTransitions
    
    // Ideally, we would use function pointers to call pattern functions
    // but this is complex due to different pattern function signatures
    // 
    // For simplicity, we rely on the pattern function writing directly
    // to the _targetLeds array when called after swapping buffers
}

/**
 * Update LED transitions for smooth pattern changes
 */
bool LEDPatterns::updateTransitions() {
    if (!_isTransitioning) {
        return false;
    }
    
    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - _transitionStartTime;
    
    // Check if transition is complete
    if (elapsed >= _transitionDuration) {
        // Copy target values to current LEDs
        for (uint16_t i = 0; i < _numLeds; i++) {
            _leds[i] = _targetLeds[i];
        }
        _isTransitioning = false;
        return false;
    }
    
    // Calculate transition progress (0-255)
    uint8_t progress = map(elapsed, 0, _transitionDuration, 0, 255);
    
    // Blend between previous and target states
    for (uint16_t i = 0; i < _numLeds; i++) {
        _leds[i] = blendColors(_previousLeds[i], _targetLeds[i], progress);
    }
    
    return true;
}

/**
 * Blend between two colors based on progress
 */
CRGB LEDPatterns::blendColors(CRGB start, CRGB end, uint8_t progress) {
    // Use the built-in FastLED function for color blending
    return blend(start, end, progress);
}

// The following helper methods can be used to apply patterns to the target buffer
/**
 * Apply solid color pattern to target buffer (RGB)
 */
void LEDPatterns::solidToTarget(CRGB color) {
    // Ensure we have a target buffer
    if (_targetLeds == nullptr) return;
    
    // Fill target buffer with color
    for (uint16_t i = 0; i < _numLeds; i++) {
        _targetLeds[i] = color;
    }
}

/**
 * Apply solid color pattern to target buffer (HSV)
 */
void LEDPatterns::solidToTarget(CHSV color) {
    if (_targetLeds == nullptr) return;
    
    // Fill target buffer with color
    for (uint16_t i = 0; i < _numLeds; i++) {
        _targetLeds[i] = color;
    }
}

/**
 * Apply breathing effect to target buffer
 */
void LEDPatterns::breathingToTarget(CHSV color, uint8_t speed) {
    if (_targetLeds == nullptr) return;
    
    // Calculate brightness based on time
    uint8_t brightness = beatsin8(speed, 0, 255);
    
    // Apply brightness to the color
    CHSV adjustedColor = CHSV(color.h, color.s, brightness);
    
    // Fill target buffer with the adjusted color
    for (uint16_t i = 0; i < _numLeds; i++) {
        _targetLeds[i] = adjustedColor;
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