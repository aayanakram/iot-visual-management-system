# Recommended Components

> Software prototype status: no physical hardware was built or electrically validated. Current firmware uses direct GPIO/ADC polling and one digital LED; expanders, PWM, calibration and flash-backed event storage below are future design options. The offline FIFO is RAM-only and does not survive reboot.


## Overview

This document provides a preliminary list of hardware components recommended for a future physical implementation of the visual management station.

This is not a finalized manufacturing Bill of Materials.

Exact components and quantities depend on the final mechanical design and number of physical interfaces used on each station.

## Recommended Components

| Function | Recommended Component | Purpose | Requirement |
|---|---|---|---|
| Main controller | ESP32-S3 development board | Main embedded controller and Wi-Fi connectivity | Required |
| Digital I/O expansion | MCP23017 | Adds 16 digital I/O channels through I2C | Optional |
| Push-button input | Momentary push button | Physical operator input | As required |
| Toggle input | Toggle or selector mechanism | Physical state selection | As required |
| Magnetic sensing | 3.3 V-compatible digital Hall-effect sensor | Detects magnets in physical controls | As required |
| Rotary input | Incremental quadrature rotary encoder | Dial/selection input | As required |
| Analog input | Linear potentiometer or slider | Variable-value physical input | As required |
| Status output | LED | Visual state indication | As required |
| LED protection | Current-limiting resistor | Limits LED current | Required with conventional LEDs |
| Output expansion | External LED/PWM driver | Provides additional output channels | Optional |
| Analog expansion | External ADC | Provides additional analog channels if required | Optional |
| Power | Regulated ESP32-compatible supply | Powers the station electronics | Required |

## ESP32-S3

The ESP32-S3 is the proposed main controller.

It was selected because it provides:

- integrated Wi-Fi
- digital GPIO
- analog-to-digital conversion
- PWM-capable outputs
- hardware peripherals suitable for input processing
- internal flash
- NVS support
- ESP-IDF support
- FreeRTOS integration
- MQTT and TLS support

A development board is sufficient for the current software prototype.

A custom PCB could be developed during a future product iteration.

## MCP23017 GPIO Expander

The MCP23017 is recommended if the final station requires more digital interfaces than can conveniently be connected directly to the ESP32-S3.

Each MCP23017 provides 16 additional digital I/O channels through I2C.

For example:

```text
2 x MCP23017
      =
32 additional digital I/O channels
```

Typical interfaces suitable for GPIO expansion include:

- push buttons
- toggle switches
- selector switches
- digital Hall-effect sensors

Rotary encoder channels should preferably be connected directly to suitable ESP32-S3 GPIO where practical.

## Magnetic Sensors

A digital Hall-effect sensor is recommended for physical controls that use embedded magnets.

The exact Hall-effect sensor has not been selected because this depends on:

- magnet strength
- magnet dimensions
- sensing distance
- mechanical geometry
- required switching behaviour

The selected device should be compatible with the ESP32-S3 logic voltage.

## Rotary Encoder

A standard incremental quadrature rotary encoder is recommended for physical dial inputs.

The encoder should provide:

- Channel A
- Channel B

An integrated push switch may also be used if required.

## Analog Slider

A linear potentiometer or slider potentiometer may be used for variable-value controls.

A nominal resistance such as 10 kOhm provides a reasonable starting point for future hardware evaluation.

The potentiometer wiper would connect to an ESP32-S3 ADC-capable input.

## Status Indicators

Standard LEDs may be used for basic visual feedback.

Each conventional LED should use an appropriate current-limiting resistor.

If the final station contains a large number of indicators, an external LED or PWM driver may be introduced.

## Final Component Selection

The components listed here are recommendations only.

Before production use, components should be evaluated for:

- electrical compatibility
- mechanical compatibility
- environmental requirements
- availability
- cost
- reliability
- manufacturability