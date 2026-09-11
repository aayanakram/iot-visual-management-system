# Hardware Interface Map

> Software prototype status: no physical hardware was built or electrically validated. Current firmware uses direct GPIO/ADC polling and one digital LED; expanders, PWM, calibration and flash-backed event storage below are future design options. The offline FIFO is RAM-only and does not survive reboot.


## Overview

This document defines the proposed logical connection between physical station interfaces and the ESP32-S3.

The source contains prototype GPIO assignments; the final board and physical wiring have not been selected or validated. The table below records the current software mapping.

## Current Firmware Prototype Mapping

| Function | Numeric source ID | Prototype connection |
|---|---:|---|
| Button | 1 | GPIO4, active-low with pull-up |
| Toggle | 2 | GPIO5, active-low with pull-up |
| Encoder | 3 | GPIO6 (A), GPIO7 (B) |
| Analog slider | 4 | ADC1 channel 0, ESP32-S3 GPIO1 |
| Status LED | Command source 0 | GPIO2, active-high digital output |

Values come from `firmware/main/config/DeviceConfig.hpp` and `firmware/main/main.cpp`. They are not a validated board pinout. Source IDs in MQTT are integers, not the example human-readable labels in the conceptual mapping below. No expander driver, PWM output or I2C bus is currently instantiated.

## Interface Allocation

| Physical Interface | Signal Type | Proposed Connection | Firmware Module |
|---|---|---|---|
| Push Button | Digital | ESP32 GPIO or MCP23017 | DigitalInput |
| Toggle Switch | Digital | ESP32 GPIO or MCP23017 | DigitalInput |
| Hall Sensor | Digital | ESP32 GPIO or MCP23017 | DigitalInput |
| Rotary Encoder Channel A | Digital | ESP32 GPIO | RotaryEncoder |
| Rotary Encoder Channel B | Digital | ESP32 GPIO | RotaryEncoder |
| Analog Slider | Analog | ESP32 ADC-capable GPIO | AnalogInput |
| Status LED | Digital/PWM | ESP32 GPIO or output driver | LedDriver |

## Digital Inputs

Low-speed digital controls may be connected directly to the ESP32-S3 or through an MCP23017 GPIO expander.

Example:

```text
Button / Toggle / Hall Sensor
             |
             v
         MCP23017
             |
            I2C
             |
             v
         ESP32-S3
```

The final number of expanders depends on the number of digital interfaces used on the station.

## Rotary Encoder Inputs

A quadrature rotary encoder requires two digital signals.

```text
Encoder Channel A ------> ESP32 GPIO

Encoder Channel B ------> ESP32 GPIO
```

Direct connection to the ESP32-S3 is preferred where practical because encoder signals may change more rapidly than simple buttons or toggle inputs.

## Analog Inputs

Analog sliders or potentiometers connect to ADC-capable ESP32-S3 inputs.

```text
Slider Wiper ------> ESP32 ADC
```

The analog signal must remain within the permitted input range of the selected ESP32-S3 hardware.

## I2C Expansion Bus

The proposed future architecture would reserve an I2C bus for digital or output expansion devices.

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
          +---- Optional additional device
```

The exact SDA and SCL GPIO assignments will be selected during physical hardware implementation.

## Logical Interface Mapping

Physical hardware connections should be separated from logical application identifiers.

Example:

```text
Hall Sensor
     |
GPIO / Expander Channel
     |
     v
Firmware Input ID
"task_03_toggle"
     |
     v
Backend Mapping
     |
     v
Odoo Task / Field
```

This allows hardware mappings or ERP mappings to change without tightly coupling the two systems.

## Proposed Logical IDs

Example logical identifiers may include:

```text
button_01
button_02

toggle_01
toggle_02

encoder_01

slider_01

status_led_01
```

For an operational station, identifiers may instead represent their actual function:

```text
task_03_toggle
production_target_dial
line_status_led
confirmation_button
```

## Pin Assignment Status

Final validated GPIO allocation is TBD; the source uses the prototype values listed above.

This is intentional because:

- no physical hardware is being assembled in the current project
- the exact number of each interface type is not fixed
- the final ESP32-S3 development board has not been selected

A finalized electrical pin map should be produced during future physical implementation.