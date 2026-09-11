# System Requirements

## 1. Project Overview

This project develops the embedded electronics and software architecture for an IoT-connected physical visual management station intended for operational environments such as manufacturing facilities, warehouses, logistics environments, and public-sector organizations.

The system connects physical visual-management controls to an Odoo ERP system.

Operators interact with physical controls such as toggles, push buttons, rotary encoders, sliders, and other interface modules. These interactions are detected by an ESP32-S3 embedded controller and transmitted to the backend through Wi-Fi and MQTT.

Communication is bidirectional. Changes made in Odoo can also be transmitted back to the station and represented using LEDs, indicators, or other physical outputs.

---

## 2. Target Deployment

A visual-management station may contain approximately 30–40 physical controls and indicators.

A production deployment may contain multiple stations, with each station operating using its own embedded controller.

The architecture must therefore support:

- approximately 30–40 interfaces per station
- multiple physical interface types
- one ESP32-S3 controller per station
- multiple independently addressed stations
- intermittent Wi-Fi connectivity
- local operation during temporary network outages
- synchronization with the backend after connectivity is restored
- deployment in dusty environments with possible water splashes
- cost-effective replication across multiple stations

---

## 3. Embedded Controller

The selected main microcontroller is the ESP32-S3.

The ESP32-S3 shall be responsible for:

- acquiring physical input states
- processing digital and analog inputs
- maintaining local station state
- generating internal application events
- controlling LEDs and indicators
- managing Wi-Fi connectivity
- communicating through MQTT
- maintaining persistent configuration
- buffering events during connectivity loss
- reporting telemetry and heartbeat information
- recovering from common communication and firmware faults

The firmware will be developed using ESP-IDF and C++.

FreeRTOS will be used to provide concurrent execution of major firmware functions.

---

## 4. Digital Inputs

The system shall support digital interfaces including:

- push buttons
- toggle switches
- selector switches
- magnetic position sensors
- limit switches where required

Digital inputs shall include appropriate debouncing.

GPIO interrupts may be used where they improve responsiveness or reduce unnecessary polling.

Input drivers shall be separated from higher-level application and communication logic.

---

## 5. Rotary Encoders

The system shall support rotary encoders where required.

Encoder processing shall provide:

- quadrature decoding
- clockwise and counter-clockwise direction detection
- position or count tracking
- appropriate handling of invalid transitions
- reliable operation during normal user interaction

Interrupt-based acquisition or ESP32 peripheral support may be used where appropriate.

---

## 6. Analog Inputs

The system shall support analog interfaces such as sliders and potentiometers.

Analog input processing shall include:

- ADC acquisition
- calibration
- scaling
- filtering
- range validation
- deadband or hysteresis

Deadband and filtering shall be used to prevent normal electrical noise from generating excessive network events.

---

## 7. Output Interfaces

The station shall support physical outputs including:

- LEDs
- status indicators
- PWM-controlled indicators where required

Outputs may be controlled by:

- local station events
- current station state
- remote backend or Odoo commands

The output hardware layer shall remain independent from MQTT and ERP-specific application logic.

---

## 8. Event-Driven Architecture

The firmware shall use an event-driven architecture.

Physical input changes shall generate internal software events.

Events shall be processed by the application state-management layer before communication or output actions are performed.

Remote MQTT messages shall also be converted into internal events before changing the local station state.

The architecture shall avoid directly coupling hardware drivers to network operations.

For example, a button driver shall report that a button changed state rather than directly publishing an MQTT message.

---

## 9. Local State Management

The ESP32-S3 shall maintain a local representation of the station state.

The local state manager shall track relevant:

- input states
- output states
- station configuration
- connectivity state
- synchronization state
- fault state

Valid state transitions shall be defined where required.

The state manager shall provide a consistent interface between physical hardware state and remote ERP state.

---

## 10. Wi-Fi Connectivity

The station shall use Wi-Fi for network communication.

The firmware shall provide:

- initial Wi-Fi connection
- connection-state monitoring
- disconnection detection
- automatic reconnection
- recovery without requiring a manual restart

The system must account for temporary Wi-Fi blind spots that may occur in warehouse and industrial environments.

---

## 11. MQTT Communication

MQTT is the selected primary device messaging protocol.

The station shall:

- publish physical events
- publish station state
- publish telemetry
- publish heartbeat messages
- subscribe to remote commands
- subscribe to configuration or state updates where required

The MQTT architecture shall support multiple independently addressed stations.

---

## 12. Network Security

Production communication should use MQTT over TLS.

Sensitive information shall not be committed to the source repository.

This includes:

- Wi-Fi passwords
- MQTT passwords
- API credentials
- private keys
- production certificates containing private information

The final credential-provisioning method remains to be determined.

---

## 13. Persistent Storage

ESP32 NVS and flash storage shall be used where appropriate.

Persistent information may include:

- device ID
- firmware information
- station configuration
- input mappings
- output mappings
- network configuration
- backend configuration
- last-known state
- pending offline events

Relevant information shall survive resets and power cycles where required.

---

## 14. Offline Operation

The station shall continue performing its core local functions during temporary loss of Wi-Fi, MQTT, or backend connectivity.

Physical events generated while offline shall not be silently lost.

The system shall provide an offline event-buffering mechanism.

When connectivity returns, the system shall attempt to transmit pending events and restore synchronization with the backend.

---

## 15. Synchronization and Reconciliation

The system shall provide a method for handling differences between local station state and remote ERP state.

The synchronization architecture should support:

- sequence identifiers
- timestamps where appropriate
- duplicate detection
- event ordering
- retry handling

A final conflict-resolution policy will be defined once the required Odoo workflows are confirmed.

---

## 16. Fault Recovery

The firmware shall recover automatically from common failures where possible.

Relevant failures include:

- Wi-Fi loss
- MQTT broker loss
- backend unavailability
- malformed incoming messages
- invalid commands
- stalled firmware tasks
- unexpected MCU resets

ESP32 watchdog functionality shall be used where appropriate.

---

## 17. Telemetry and Heartbeat

The station shall periodically report operational health information.

Telemetry may include:

- device ID
- firmware version
- uptime
- Wi-Fi RSSI
- Wi-Fi connection state
- MQTT connection state
- current device state
- synchronization state
- last successful backend synchronization
- active fault status

Periodic heartbeat messages shall allow the backend to determine whether a station is online and responsive.

---

## 18. Configuration

The system shall avoid unnecessary hardcoded station-specific values.

Configurable parameters may include:

- device ID
- station name
- input mappings
- output mappings
- MQTT topic configuration
- backend configuration
- network configuration
- station-specific behaviour

The final configuration interface will allow non-technical users to map physical station interfaces to ERP objects without modifying firmware.

---

## 19. OTA Firmware Updates

Remote firmware updates are a planned capability.

Where implemented, OTA functionality should provide:

- firmware version tracking
- firmware integrity checking
- controlled installation
- controlled reboot
- rollback or recovery protection where feasible

OTA functionality may be treated as a stretch feature for the initial prototype if required by project time constraints.

---

## 20. Scalability

The architecture shall not assume that only one visual-management station exists.

Each station shall have a unique device identifier.

MQTT topics and backend routing shall allow multiple stations to communicate simultaneously.

The architecture should allow different stations to use different interface mappings without requiring separate firmware builds.

---

## 21. Environmental Considerations

The target environment may contain:

- dust
- water splashes
- electrically noisy equipment
- metal structures that reduce Wi-Fi performance

The prototype electronics and final enclosure design shall account for these conditions where practical.

A formal IP rating has not yet been specified.

---

## 22. Validation Requirements

The completed prototype shall be tested for:

- digital input operation
- encoder operation
- analog input operation
- output operation
- Wi-Fi connectivity
- MQTT communication
- bidirectional communication
- offline operation
- automatic reconnection
- event buffering
- synchronization recovery
- reboot behaviour
- malformed messages
- duplicate messages
- power cycling

Performance measurements should include event latency and reconnection performance where practical.