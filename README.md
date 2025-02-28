# Reactive LEDs

This project uses an ESP32 to control WS2812B LED strips based on input from a SparkFun Human Presence and Motion Sensor (STHS34PF80). The LEDs react dynamically to both presence and motion detection, with different lighting patterns based on the type and intensity of detection.

## Overview

This project uses an ESP32 to drive WS2812B LED strips with dynamic patterns that react to environmental inputs. The system uses various sensors to modulate the LED patterns:

- **Primary Sensor**: SparkFun Human Presence and Motion Sensor (based on STHS34PF80 IR sensor)
- **Communication**: I2C interface for sensor connectivity
- **Future Expansion**: Additional I2C sensors may be incorporated

## Hardware Requirements

- ESP32 development board (ESP32-Dev)
- WS2812B LED strip (Addressable RGB LEDs)
- SparkFun Human Presence and Motion Sensor - STHS34PF80 (Qwiic)
- Power supply appropriate for your LED setup
- Jumper wires

## Hardware Configuration

- GPIO Pin for LED data: 19
- Number of LEDs: 150
- I2C SDA Pin: 21
- I2C SCL Pin: 22

## Libraries Used

- [FastLED](https://github.com/FastLED/FastLED) - For controlling the WS2812B LED strips
- [SparkFun STHS34PF80 Arduino Library](https://github.com/sparkfun/SparkFun_STHS34PF80_Arduino_Library) - For interfacing with the STHS34PF80 sensor
- Custom LEDPatterns Library - For creating various LED animation patterns

## Project Structure

- `/src/` - Source code files
    - `main.cpp` - Main application
- `/lib/` - Library files
    - `/LEDPatterns/` - Custom LED patterns library
- `/include/` - Header files
- `/test/` - Unit tests

## Development Environment

This project uses PlatformIO for development, which provides:
- Cross-platform build system
- Library management
- Testing framework
- Unified debugging

## Setup Instructions

1. Install [PlatformIO](https://platformio.org/) (recommended as an extension for VS Code)
2. Clone this repository
3. Open the project in PlatformIO
4. Build and upload to your ESP32

## Configuration

The project uses PlatformIO for configuration. The main settings can be found in `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
monitor_filters = direct
monitor_rts = 0
monitor_dtr = 0
build_flags = 
    -D LED_PIN=19
    -D LED_COUNT=150
    -D I2C_SDA=21
    -D I2C_SCL=22

; Libraries
lib_deps =
    fastled/FastLED @ ^3.5.0
    sparkfun/SparkFun STHS34PF80 Arduino Library @ ^1.0.4
```

## Custom LED Patterns Library Setup

The project includes a custom LED Patterns library that provides various animation patterns for the WS2812B LED strips. The library is configured with proper library.json metadata for PlatformIO discoverability.

Key aspects of the library configuration:
- Located in `/lib/LEDPatterns/`
- Source files in `/lib/LEDPatterns/src/`
- Properly configured `library.json` with build configuration
- Important: The library requires FastLED as a dependency

If you encounter any build issues with the custom library, check that:
1. The `library.json` file has proper `build` and `export` sections
2. Include paths are correctly set
3. Main program is importing the library with angle brackets: `#include <LEDPatterns.h>`

## Power Considerations

With 150 WS2812B LEDs, power requirements can be substantial. At full brightness (white), each LED can draw up to 60mA. Calculate your power supply needs accordingly:

- 150 LEDs × 60mA = 9A at 5V (maximum)

It's recommended to:
- Use an appropriate 5V power supply with sufficient amperage
- Connect the power supply directly to the LED strip power lines
- Share ground between the power supply, LED strip, and ESP32
