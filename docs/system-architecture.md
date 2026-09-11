# System Architecture

## 1. Overview

The IoT-connected visual management station is designed as a distributed embedded system connecting physical operational interfaces with an Odoo ERP environment.

The architecture is divided into the following major layers:

1. Physical interface layer
2. Hardware interface layer
3. ESP32-S3 embedded controller
4. Network communication layer
5. MQTT broker
6. Backend/middleware
7. Odoo ERP

---

## 2. High-Level Architecture

```text
PHYSICAL VISUAL MANAGEMENT STATION

Physical Interfaces
------------------------------------------------
Toggles | Buttons | Encoders | Sliders | Sensors
------------------------------------------------
                    |
                    v
Hardware Interface Layer
------------------------------------------------
GPIO | ADC | Encoder Acquisition | Output Drivers
------------------------------------------------
                    |
                    v
ESP32-S3
------------------------------------------------
ESP-IDF + C++ + FreeRTOS

Input Drivers
Event Processing
State Manager
Output Manager
Wi-Fi Manager
MQTT Manager
Persistent Storage
Offline Event Buffer
Telemetry
Fault Recovery
------------------------------------------------
                    |
               Wi-Fi + TLS
                    |
                    v
               MQTT Broker
                    |
                    v
           Backend / Middleware
                    |
                Odoo API
                    |
                    v
                Odoo ERP
```

Communication is bidirectional.

Station-to-ERP communication transfers physical state changes into Odoo.

ERP-to-station communication transfers remote state or command information back to physical outputs.

---

## 3. Physical Interface Layer

The physical visual-management board may contain approximately 30–40 interfaces.

The mechanical design may include:

- oversized toggles
- push buttons
- rotary controls
- sliders
- selector interfaces
- LEDs
- other indicators

Some physical controls are expected to use magnets embedded in 3D-printed mechanical components.

Magnetic sensors mounted behind or within the station can detect the physical position of these controls without requiring the mechanical component itself to contain a conventional electrical switch.

---

## 4. Hardware Interface Layer

The hardware interface layer converts physical interactions into electrical signals suitable for the ESP32-S3.

Interface types may include:

- digital GPIO signals
- magnetic sensor outputs
- rotary encoder channels
- analog voltage signals
- digital output signals
- PWM output signals

Additional I/O expansion may be required depending on the final number and type of controls used on each station.

The final GPIO and peripheral allocation will be determined after the exact interface quantities are confirmed.

---

## 5. Embedded Controller

Each station uses one ESP32-S3 as its primary embedded controller.

The controller is responsible for:

- physical input acquisition
- input filtering and debouncing
- event generation
- state management
- output control
- Wi-Fi connectivity
- MQTT communication
- persistent configuration
- offline buffering
- synchronization
- telemetry
- fault recovery

ESP-IDF is used as the firmware framework.

C++ is used for the main application architecture.

FreeRTOS provides task scheduling and asynchronous execution.

---

## 6. Event-Driven Data Flow

The firmware architecture separates physical hardware from networking.

A physical input does not directly perform an MQTT operation.

Instead:

```text
Physical Input
      |
      v
Hardware Driver
      |
      v
Internal Event
      |
      v
FreeRTOS Event Queue
      |
      v
Event Processor
      |
      v
State Manager
     / \
    /   \
   v     v
Output   Network
Manager  Manager
```

This architecture reduces coupling and allows hardware, application logic, and communications to evolve independently.

---

## 7. Station-to-Odoo Data Flow

Example physical interaction:

```text
Operator moves toggle
        |
        v
Sensor detects new position
        |
        v
ESP32 input driver
        |
        v
Internal event
        |
        v
State manager
        |
        v
MQTT event
        |
        v
MQTT broker
        |
        v
Middleware
        |
        v
Odoo API
        |
        v
Task / Work Order / KPI Updated
```

---

## 8. Odoo-to-Station Data Flow

Example remote update:

```text
Odoo task status changes
        |
        v
Middleware
        |
        v
MQTT command
        |
        v
ESP32 MQTT manager
        |
        v
Internal event
        |
        v
State manager
        |
        v
Output manager
        |
        v
LED / Status Indicator Updated
```

---

## 9. Backend / Middleware

The preferred architecture uses an intermediary software layer between embedded devices and Odoo.

```text
ESP32-S3
   |
 MQTT
   |
   v
Middleware
   |
Odoo API
   |
   v
Odoo ERP
```

The ESP32 does not directly modify the Odoo database.

The middleware may be responsible for:

- MQTT communication
- device identification
- device routing
- message validation
- duplicate detection
- device-to-Odoo mapping
- Odoo authentication
- Odoo API operations
- command generation
- retry handling
- synchronization
- logging

This prevents ERP-specific implementation details from being tightly coupled to embedded firmware.

---

## 10. Offline Architecture

Warehouse Wi-Fi may be temporarily unavailable due to environmental conditions such as metal racks and equipment.

The station must therefore remain operational during temporary outages.

```text
Physical Event
      |
      v
Local Processing
      |
      v
State Manager
      |
      v
Network Available?
   /          \
 YES           NO
  |             |
  v             v
MQTT       Offline Buffer
Publish          |
                 |
           Connectivity
             Restored
                 |
                 v
            MQTT Publish
```

The device shall automatically attempt to recover network connectivity without requiring operator intervention.

---

## 11. Persistent Storage

ESP32 NVS and flash storage provide persistent storage for configuration and selected runtime information.

Potential persistent data includes:

- unique station ID
- station configuration
- input mappings
- output mappings
- Wi-Fi configuration
- MQTT configuration
- last known station state
- firmware metadata
- pending offline events

---

## 12. Multi-Station Architecture

The system must support multiple visual-management stations.

Example:

```text
Station 01 ----\
Station 02 -----\
Station 03 ------\
Station 04 ------- > MQTT Broker -> Backend -> Odoo
...              /
Station 20 ------/
```

Each station uses a unique device ID.

MQTT topics and backend routing use this device ID to separate messages originating from different stations.

The architecture therefore does not require a separate backend implementation for every station.

---

## 13. Security Architecture

Production communication should use MQTT over TLS.

Sensitive credentials shall not be stored directly in the public source repository.

Security architecture should provide appropriate handling of:

- Wi-Fi credentials
- MQTT authentication
- TLS certificates
- backend credentials
- Odoo credentials

---

## 14. Design Principles

The system architecture follows the following principles:

- modularity
- separation of concerns
- hardware abstraction
- event-driven execution
- asynchronous communication
- loose coupling between drivers and networking
- graceful degradation during network failure
- automatic recovery
- persistent configuration
- scalable device addressing
- observable device behaviour
- maintainability