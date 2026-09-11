# Hardware Interface Map

## Overview

This document defines the proposed logical connection between physical station interfaces and the ESP32-S3.

Exact GPIO numbers are intentionally not assigned because the final development board and physical interface quantities have not yet been finalized.

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

The proposed architecture reserves an I2C bus for digital or output expansion devices.

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

Exact GPIO allocation is currently TBD.

This is intentional because:

- no physical hardware is being assembled in the current project
- the exact number of each interface type is not fixed
- the final ESP32-S3 development board has not been selected

A finalized electrical pin map should be produced during future physical implementation.