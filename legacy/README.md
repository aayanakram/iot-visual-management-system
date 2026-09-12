# Superseded Prototypes

Nothing in this directory is built, tested, or part of the delivered system. It
is kept because it records how the prototype started, not because it runs.

Both files were added late, after the ESP-IDF firmware and Python middleware in
this repository were already written. Left at the repository root they
contradicted the delivered contract, so they are parked here.

## `Model Esp code.cpp`

An early single-file Arduino sketch using `WiFi.h`, `PubSubClient` and
`ArduinoJson`. **It is not the firmware.** The delivered firmware is the
ESP-IDF C++ application under [`firmware/`](../firmware).

It disagrees with the delivered system in two ways that matter:

**A different, incompatible MQTT contract.** It publishes per-module topics and
subscribes to a work-order topic:

| This file | Delivered contract |
|---|---|
| `station1/module1/status` | `kaizen/stations/<device_id>/events` |
| `station1/module2/kpi` | `kaizen/stations/<device_id>/events` |
| `station1/module3/alert` | `kaizen/stations/<device_id>/events` |
| `station1/led_feedback` | `kaizen/stations/<device_id>/commands` |
| `erp/workorder/101` | no equivalent |

[docs/mqtt-integration.md](../docs/mqtt-integration.md) states that only two
topic forms are implemented, which is true of the delivered firmware and
middleware and was not true of the repository root while this file sat there.

**Different pin assignments.** Toggle 25, potentiometer 34, button 26, LED 27,
against the prototype mapping in
[`firmware/main/config/DeviceConfig.hpp`](../firmware/main/config/DeviceConfig.hpp)
of toggle 5, ADC1 channel 0 (GPIO1), button 4, LED 2. Neither set has been
electrically validated; treat
[hardware/pinout/interface-map.md](../hardware/pinout/interface-map.md) as the
current reference.

It also debounces at 50 ms against the delivered 30 ms, and uses `ArduinoJson`,
which is not a dependency of this project.

## `Operational DSM.html`

A standalone browser page that mimics a station and an ERP panel for
demonstration. It has no connection to the firmware, the middleware or the
broker, and shares no code with them. Useful for showing the idea on a laptop;
not evidence that anything works.

For a demonstration that exercises the real protocol, validation and adapter,
use `python -m backend.demo` instead.
