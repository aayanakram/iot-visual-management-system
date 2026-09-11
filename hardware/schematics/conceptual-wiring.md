# Conceptual Wiring

> Software prototype status: no physical hardware was built or electrically validated. Current firmware uses direct GPIO/ADC polling and one digital LED; expanders, PWM, calibration and flash-backed event storage below are future design options. The offline FIFO is RAM-only and does not survive reboot.


## Overview

This document provides conceptual connection diagrams for the proposed physical interfaces used by the IoT-connected visual management station.

These diagrams are intended to describe the recommended electrical architecture only.

They are not validated production schematics, and no physical hardware implementation is claimed as part of the current project.

## Push Button

A push button can be implemented as an active-low digital input.

```text
3.3 V
  |
Pull-Up
  |
  +---------------- ESP32 / GPIO Expander Input
  |
Button
  |
 GND
```

When the button is pressed, the input is pulled toward ground.

Software debouncing is handled by the firmware.

## Toggle or Selector Switch

A toggle or selector switch can also be read as a digital input.

```text
3.3 V
  |
Pull-Up
  |
  +---------------- ESP32 / GPIO Expander Input
  |
Toggle Switch
  |
 GND
```

The firmware detects a change in the digital state and generates an application event.

## Magnetic Hall Sensor

A digital Hall-effect sensor can be used to detect a magnet embedded in an oversized physical control.

```text
              3.3 V
                |
         +--------------+
Magnet   | Hall Sensor  |
  --->   |              |
         +------+-------+
                |
              Output
                |
                v
       ESP32 GPIO / MCP23017
                |
               GND
```

The exact electrical connection depends on the selected Hall-effect sensor.

The selected sensor must be compatible with the supply voltage and logic levels used by the ESP32-S3.

## Rotary Encoder

A quadrature rotary encoder uses two digital channels.

```text
Rotary Encoder

Channel A ----------------------> ESP32 GPIO

Channel B ----------------------> ESP32 GPIO

Common -------------------------> GND
```

The firmware uses Channel A and Channel B to determine:

- clockwise movement
- counter-clockwise movement
- position or count changes

Direct connection to the ESP32-S3 is preferred where practical.

## Analog Slider

A potentiometer-style slider can be connected as a voltage divider.

```text
3.3 V
  |
  |
+----------------+
| Potentiometer  |
+-------+--------+
        |
       Wiper --------------------> ESP32 ADC
        |
        |
       GND
```

The firmware processing path may include:

```text
ADC Reading
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

This prevents small ADC fluctuations from generating unnecessary state-change events.

## Status LED

A conventional LED may be driven from a suitable ESP32-S3 digital output.

```text
ESP32 GPIO
     |
Current-Limiting Resistor
     |
    LED
     |
    GND
```

If the selected indicator requires more current than an ESP32 GPIO can safely provide, an external transistor or driver circuit should be used.

## GPIO Expansion

For a station containing many digital controls, an MCP23017 GPIO expander can be used.

```text
Button -----------\
Toggle ------------\
Hall Sensor --------> MCP23017
Selector ----------/      |
                         I2C
                          |
                          v
                      ESP32-S3
```

Multiple MCP23017 devices may share the same I2C bus using different hardware addresses.

## Shared I2C Bus

A single I2C bus can support multiple expansion devices.

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
          +---- Optional output expander
          |
          +---- Optional external ADC
```

The exact SDA and SCL GPIO assignments will be selected during future physical implementation.

## Complete Conceptual Station

```text
Push Buttons -------\
                     \
Toggle Switches ------\
                       \
Hall Sensors -----------> GPIO Expansion ----\
                                              \
Rotary Encoders -------------------------------> ESP32-S3
                                              /
Analog Sliders -------------------------------/
                                             |
                                             +---- LEDs / Indicators
                                             |
                                             +---- Flash / NVS
                                             |
                                             +---- Wi-Fi
                                                     |
                                                     v
                                                MQTT Broker
```

## Power

For software development using an ESP32-S3 development board, the board may be powered through its supported USB input.

The final physical station power architecture has not yet been specified.

A future implementation should use an appropriately regulated power supply selected according to:

- available installation power
- ESP32-S3 requirements
- sensor requirements
- number of indicators
- total current consumption

## Future Electrical Validation

Before physical implementation, a complete electrical schematic should be produced and checked for:

- supply voltage compatibility
- ESP32 GPIO voltage limits
- GPIO output current limits
- pull-up and pull-down requirements
- sensor output types
- input filtering
- electrical protection
- LED current limiting
- grounding
- wiring distance
- electrical noise
- connector selection
- power integrity

No physical electrical validation is claimed as part of the current project.
## Implemented ADC Scope

The firmware currently scales raw 12-bit ADC readings to 0-100 and applies a 2-percentage-point deadband. Calibration and smoothing shown in the conceptual pipeline are future work. A 3.3 V potentiometer supply is conceptual wiring, not a claim that the ADC accurately measures the entire rail-to-rail range; verify the selected board and ADC input range before connecting hardware.
