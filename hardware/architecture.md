# Hardware Architecture

## Overview

The proposed visual management station uses an ESP32-S3 as the main embedded controller.

The controller interfaces with several types of physical controls and indicators while providing integrated Wi-Fi connectivity for communication with the MQTT backend.

The hardware architecture is designed to support expansion to approximately 30–40 interfaces per station without requiring every physical interface to connect directly to the ESP32-S3.

No complete physical hardware prototype is being assembled as part of the current project. This document defines the recommended architecture for a future implementation.

## High-Level Architecture

```text
                  VISUAL MANAGEMENT STATION

        Digital Inputs            Variable Inputs
     --------------------       ---------------------
     Buttons     Toggles        Encoder       Slider
        |           |             |  |           |
        |           |             |  |           |
        v           v             v  v           v

   GPIO / Optional            ESP32 GPIO      ESP32 ADC
   GPIO Expansion              / PCNT
        |                         |              |
        +-------------+-----------+--------------+
                      |
                      v
                +-------------+
                |  ESP32-S3   |
                |-------------|
                | ESP-IDF     |
                | FreeRTOS    |
                | Wi-Fi       |
                | NVS / Flash |
                +------+------+
                       |
              +--------+--------+
              |                 |
              v                 v
        LED / Status       Local Storage
          Outputs          Offline Events
              |
              v
        Physical Status
          Indicators

                       |
                  Wi-Fi / TLS
                       |
                       v
                  MQTT Broker
```

## Main Controller

The recommended main controller is an ESP32-S3 development board.

The ESP32-S3 is suitable for the proposed system because it provides:

- integrated Wi-Fi
- digital GPIO
- analog-to-digital conversion
- PWM-capable outputs
- hardware peripherals suitable for input processing
- internal flash
- Non-Volatile Storage (NVS)
- native ESP-IDF support
- FreeRTOS integration
- MQTT and TLS support

For the current software prototype, a development board is sufficient.

A custom PCB may be developed during a future product iteration if the system moves toward commercial deployment.

## Digital Input Architecture

Push buttons, toggle switches, selector switches, and digital magnetic sensors can be represented as digital inputs.

For a small number of interfaces, these inputs may connect directly to available ESP32-S3 GPIO.

For a larger station containing many digital interfaces, external GPIO expansion is recommended.

A suitable option is the MCP23017.

Each MCP23017 provides 16 additional digital I/O channels through I2C.

For example:

```text
2 x MCP23017
      =
32 additional digital I/O channels
```

This allows the ESP32-S3 to support a larger number of low-speed physical interfaces without consuming a large number of direct GPIO pins.

Typical interfaces suitable for GPIO expansion include:

- push buttons
- toggle switches
- selector switches
- digital Hall-effect sensors

## Magnetic Input Architecture

The proposed mechanical design may use magnets embedded in oversized physical controls.

A digital Hall-effect sensor is recommended for detecting the position of these controls.

Conceptually:

```text
3D-Printed Physical Control
            |
          Magnet
            |
            v
     Hall-Effect Sensor
            |
            v
     Digital Input Signal
            |
            v
ESP32 GPIO / MCP23017
```

The exact Hall-effect sensor should be selected only after the mechanical design is finalized.

Selection depends on:

- magnet strength
- magnet dimensions
- sensing distance
- sensor placement
- desired switching behaviour

## Rotary Encoder Architecture

Rotary encoders require two digital channels for quadrature decoding.

Where practical, encoder channels should connect directly to ESP32-S3 GPIO or suitable ESP32 hardware peripherals.

Conceptually:

```text
Rotary Encoder

Channel A --------\
                   \
                    >---- ESP32-S3
                   /
Channel B --------/
```

Direct connection is preferred because encoder signals may change more rapidly than simple button or toggle inputs.

Firmware is responsible for:

- direction detection
- count tracking
- invalid transition handling
- event generation

## Analog Input Architecture

Analog sliders or potentiometers may connect directly to ADC-capable ESP32-S3 inputs.

Conceptually:

```text
3.3 V
  |
Potentiometer
  |
  +---- Wiper ----> ESP32 ADC
  |
 GND
```

The firmware processing path may include:

```text
Raw ADC Reading
      |
      v
Calibration
      |
      v
Filtering
      |
      v
Scaling
      |
      v
Deadband / Hysteresis
      |
      v
Application Event
```

Filtering and deadband processing prevent small electrical fluctuations from generating unnecessary state-change events.

An external ADC may be introduced in a future implementation if more analog channels or higher measurement performance are required.

## Output Architecture

The visual management station may use LEDs or other status indicators to reflect local or remote system state.

For a small number of outputs, indicators may be driven directly from ESP32-S3 GPIO where electrically appropriate.

For larger numbers of indicators, an external output driver or PWM expander may be introduced.

Conceptually:

```text
ESP32-S3
    |
    v
Output Driver
    |
    v
LED / Status Indicator
```

Output state may be controlled by:

- local station logic
- state transitions
- backend commands
- Odoo-derived events

## I2C Expansion Architecture

A shared I2C bus can be used for hardware expansion.

Conceptually:

```text
ESP32-S3
   |
   +---- SDA
   |
   +---- SCL
          |
          +---- MCP23017 #1
          |
          +---- MCP23017 #2
          |
          +---- Optional Output Expander
          |
          +---- Optional External ADC
```

Each device requires a unique I2C address.

The exact SDA and SCL GPIO assignments are intentionally left TBD until a specific ESP32-S3 development board is selected.

## Persistent Storage

The initial architecture uses the ESP32-S3's internal flash and NVS rather than requiring external storage.

Persistent storage may contain:

- device ID
- station configuration
- input mappings
- output mappings
- network configuration
- backend configuration
- last known state
- pending offline events

An external SD card is not required for the current software prototype.

## Offline Event Storage

If Wi-Fi or MQTT connectivity is unavailable, physical events should continue to be processed locally.

Conceptually:

```text
Physical Event
      |
      v
ESP32-S3
      |
      v
Network Available?
   /          \
 YES           NO
  |             |
  v             v
Publish      Store Event
                 |
                 v
          Connectivity Returns
                 |
                 v
            Publish Event
```

The current project implements this primarily as a firmware and software function.

Persistent flash storage may be used where queued events must survive a reset or power loss.

## Power Architecture

The final production power architecture has not been specified.

For development using an ESP32-S3 development board, the board may use its supported USB power input.

A future physical implementation should use a regulated power supply selected according to:

- available installation power
- ESP32-S3 power requirements
- connected sensor requirements
- number of indicators
- total current consumption

A production implementation should also consider:

- voltage regulation
- input protection
- grounding
- electrical noise
- connector reliability
- power integrity

## Scalability

The proposed hardware architecture is intended to scale beyond a single physical station.

Each station can use:

```text
1 x ESP32-S3
+
Optional GPIO Expansion
+
Physical Input Modules
+
Output Indicators
```

Multiple stations then communicate independently with the same MQTT/backend infrastructure using unique device identifiers.

## Future Physical Validation

Before physical deployment, the following areas should be verified:

- exact interface quantities
- final GPIO requirements
- Hall-effect sensor selection
- GPIO voltage compatibility
- ADC input range
- output current requirements
- power supply requirements
- wiring distance
- I2C bus length
- electrical noise immunity
- grounding
- connector selection
- environmental protection
- enclosure integration

The current project provides the recommended hardware architecture only and does not claim physical electrical validation.