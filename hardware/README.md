# Hardware Design

> Software prototype status: no physical hardware was built or electrically validated. Current firmware uses direct GPIO/ADC polling and one digital LED; expanders, PWM, calibration and flash-backed event storage below are future design options. The offline FIFO is RAM-only and does not survive reboot.


## Overview

This directory documents the proposed hardware architecture for the IoT-connected visual management station.

The current project does not include construction or physical validation of a complete hardware prototype. Instead, this section defines a recommended controller, interface components, connection strategy, and expansion approach for a future physical implementation.

The embedded software architecture is designed around an ESP32-S3 using ESP-IDF, C++, and FreeRTOS.

## Hardware Objectives

The proposed hardware architecture is intended to support physical interfaces including:

- push buttons
- toggle switches
- magnetic position sensors
- rotary encoders
- analog sliders or potentiometers
- LEDs and status indicators

A complete station may eventually contain approximately 30–40 physical interfaces.

The architecture therefore allows additional I/O expansion where the ESP32-S3 does not provide enough practical direct connections.

## Main Controller

The proposed main controller is an ESP32-S3 development board.

The ESP32-S3 provides the capabilities required for the software prototype, including:

- integrated Wi-Fi
- digital GPIO
- analog-to-digital conversion
- PWM-capable outputs
- internal flash storage
- NVS support
- ESP-IDF support
- FreeRTOS integration
- MQTT and TLS support

A development board is sufficient for the current project.

A custom PCB could be designed during a future hardware or commercial product iteration.

## Proposed Interface Architecture

The recommended high-level connection strategy is:

```text
Push Buttons -------\
                     \
Toggle Switches ------\
                       \
Hall Sensors -----------> GPIO / GPIO Expansion ----\
                                                    \
Rotary Encoders -------------------------------------> ESP32-S3
                                                    /
Analog Sliders -------------------------------------/
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

Low-speed digital interfaces such as buttons, toggles, and Hall-effect sensors may use direct ESP32 GPIO or optional MCP23017 GPIO expanders.

Rotary encoders are preferably connected directly to ESP32-S3 GPIO where practical.

Analog sliders or potentiometers connect to ADC-capable ESP32-S3 inputs.

LEDs and status indicators may connect directly to suitable GPIO or use external output expansion where required.

## Hardware Directory Structure

```text
hardware/
├── README.md
├── architecture.md
├── bom/
│   └── recommended-components.md
├── pinout/
│   └── interface-map.md
└── schematics/
    └── conceptual-wiring.md
```

### architecture.md

Describes the overall proposed station hardware architecture, including:

- ESP32-S3 controller
- digital input expansion
- magnetic sensing
- rotary encoders
- analog inputs
- output indicators
- local storage
- power considerations
- scalability

### bom/recommended-components.md

Provides a preliminary list of recommended components for a future physical implementation.

The list is not intended to represent a finalized production Bill of Materials.

### pinout/interface-map.md

Defines the logical relationship between physical controls, electrical interfaces, and firmware modules.

Final board-specific wiring is unassigned. Existing prototype GPIO values are recorded in [the interface map](pinout/interface-map.md) and have not been physically validated.

### schematics/conceptual-wiring.md

Provides conceptual electrical connection diagrams for the proposed physical interfaces.

These diagrams are intended for architecture and planning purposes only and are not validated production schematics.

## Implementation Status

The hardware architecture documented in this directory is a proposed design.

The current project focuses primarily on:

- ESP32-S3 firmware architecture
- input and output abstractions
- Wi-Fi connectivity
- MQTT communication
- offline event buffering
- local state management
- backend integration
- Odoo integration

No complete physical visual-management station has been assembled or electrically validated as part of the project.

## Future Physical Validation

Before a future physical implementation is deployed, the hardware design should be validated for:

- supply voltage compatibility
- GPIO voltage limits
- current requirements
- sensor operation
- ADC ranges
- rotary encoder behaviour
- LED/output current
- power integrity
- grounding
- wiring distances
- electrical noise
- connector selection
- environmental protection
- mechanical integration

A complete electrical schematic and finalized pin map should be produced once the physical station design and interface quantities are confirmed.