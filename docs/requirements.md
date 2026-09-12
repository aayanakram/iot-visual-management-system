# Prototype Requirements and Scope

The prototype connects physical visual-management inputs to a simulated ERP workstation and demonstrates a reverse path to a status output. A larger board with 30–40 controls, multiple deployed stations and a live Odoo system remains a future deployment objective.

| Requirement | Delivered scope | Evidence / boundary |
|---|---|---|
| ESP32-S3 firmware | ESP-IDF 6.1, C++, Component Manager dependencies and lock | Firmware build; no flashing |
| Event-driven processing | Drivers -> EventQueue -> Application -> StateManager/MQTT/output | Selected actual C++ logic tested on host |
| Digital inputs | One button and toggle; active-low, debounce, initial state | Host debounce test; no electrical validation |
| Rotary input | One two-channel polled quadrature encoder | Host direction/invalid-transition tests; no timing validation |
| Analog input | ADC1 channel 0, raw percentage, 2-point deadband | Firmware compilation; ADC hardware remains unvalidated |
| Output | One active-high digital LED; remote state/output events | Host state/output tests; physical output unvalidated |
| MQTT integration | Two station-specific topics, JSON, QoS 1, reconnect/resubscribe | Protocol/contract and callback tests; broker behavior requires a broker test |
| Offline operation | 64-event bounded RAM FIFO, newest-per-source coalescing, ordered retry, logged overflow | Actual FIFO/application host tests; no reboot persistence |
| Health | 30-second application heartbeat containing free heap and uptime | Actual application loop with a host timer shim |
| Backend | Python/Paho event validation and routing | Python tests and deterministic demo |
| ERP demonstration | Odoo adapter interface plus mock workstation mappings and reverse commands | Mock only; no live Odoo validation |
| Configuration | Firmware defaults/ignored local header; backend environment variables | No provisioning UI or production secret store |
| Documentation | Implemented architecture, JSON, setup, limitations, conceptual hardware | See linked documents and testing record |

Software acceptance requires `idf.py build` to report `Project build complete`, the host C++ tests to pass, Python tests including the firmware contract to pass, and the demo assertions to pass. Current results are recorded in [testing.md](testing.md); compilation alone does not establish physical or live-system readiness.

Excluded from the current prototype scope are OTA, PKI, certificate provisioning infrastructure, flash-backed event storage, a custom PCB, a production web application/database, live Odoo deployment and production validation. Analog calibration/filtering beyond the deadband, durable deduplication, conflict resolution, mapping configuration UI and environmental protections are future requirements, not delivered features.
