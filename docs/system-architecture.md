# System Architecture

The project has three implemented software parts: ESP32-S3 firmware, Python MQTT middleware, and a mock Odoo adapter. A real MQTT broker is an external runtime dependency for network operation. The included in-process demo replaces transport with function calls so it can run without a broker or hardware.

```text
Proposed physical controls
        |
ESP32-S3 firmware: InputManager -> drivers -> EventQueue -> Application
                                                         |       |
                                                    StateManager |
                                                                 v
                                                      JSON -> MQTT manager
                                                                 |
                                                          Wi-Fi / broker
                                                                 |
                                                      Python protocol validation
                                                                 |
                                                        Mock Odoo workstation
                                                                 |
                                                      Generated output/state command
                                                                 |
broker -> MQTT manager -> JSON parser -> EventQueue -> Application -> OutputManager -> LED
```

Firmware has separate acquisition and application tasks. Drivers know GPIO/ADC and logical source IDs, while the middleware knows the simulated workstation mapping. ERP model/API details do not belong in the embedded driver layer. Remote commands enter the same event-processing path as local events; they do not directly write GPIO from a network callback.

A local input updates its station state even while MQTT is unavailable. Application holds unsent events in a bounded RAM FIFO and retries in order. The FIFO, MQTT outbox and local state are volatile. NVS is initialized for ESP-IDF services, but there is no application event persistence or persistent station mapping service. Successful MQTT enqueue is not an acknowledgement from the backend.

Each station has a device ID in its event payload and topic. Backend validation checks those agree. The mock dictionary separates station state, but multi-station deployment capacity and reliability have not been measured. A duplicate absolute value can be applied again; there is no global deduplication, cross-boot identity or reconciliation algorithm.

The reverse demo maps a done toggle to a desired LED output and supports an independent mock ERP state change. Firmware commands only set a boolean output. No real Odoo authentication, model update, polling or webhook has been implemented.

The hardware design documents describe future electrical/mechanical recommendations. Existing source GPIOs are placeholders, and no physical station was built. Plaintext MQTT placeholders support a simple local prototype; TLS device credential provisioning and production deployment security are future work. See [firmware architecture](firmware-architecture.md), [MQTT contract](mqtt-integration.md), [Odoo adapter](odoo-integration.md), and [limitations](known-limitations.md).
