# Firmware Architecture

## 1. Platform

The firmware targets the ESP32-S3 microcontroller.

Development stack:

- ESP-IDF
- C++
- FreeRTOS

The application is designed as a modular event-driven embedded system rather than a monolithic polling loop.

---

## 2. Firmware Architecture

The firmware is divided into logical layers.

```text
Application Logic
        |
        v
State and Event Management
        |
        v
System Services
        |
        v
Hardware Drivers
        |
        v
ESP-IDF / ESP32-S3 Hardware
```

This separation prevents application logic from being tightly coupled to individual GPIO pins, sensors, or communication protocols.

---

## 3. Planned Source Structure

```text
firmware/
|
+-- main/
    |
    +-- app/
    |
    +-- drivers/
    |
    +-- events/
    |
    +-- state/
    |
    +-- network/
    |
    +-- storage/
    |
    +-- outputs/
    |
    +-- telemetry/
    |
    +-- config/
```

---

## 4. Hardware Drivers

Hardware drivers provide abstraction around individual physical interfaces.

Planned driver types include:

```text
DigitalInput
RotaryEncoder
AnalogInput
LedDriver
```

Drivers are responsible only for hardware interaction.

For example, a button driver may determine that a button has been pressed, but it does not need to know:

- what Odoo object the button represents
- what MQTT topic should be used
- whether Wi-Fi is connected
- whether an event must be stored offline

These responsibilities belong to higher firmware layers.

---

## 5. Event Model

Physical and remote actions are represented as software events.

A conceptual event enumeration may resemble:

```cpp
enum class EventType
{
    ButtonPressed,
    ButtonReleased,
    ToggleChanged,
    EncoderChanged,
    AnalogChanged,

    RemoteStateUpdate,

    WifiConnected,
    WifiDisconnected,

    MqttConnected,
    MqttDisconnected,

    SyncRequested,
    FaultDetected
};
```

A conceptual event structure may contain:

```cpp
struct Event
{
    EventType type;
    uint32_t sourceId;
    int32_t value;
    uint64_t timestamp;
    uint32_t sequenceId;
};
```

The exact data structure may evolve during implementation.

---

## 6. Event Processing

The preferred event path is:

```text
Hardware Driver
      |
      v
Internal Event
      |
      v
FreeRTOS Queue
      |
      v
Event Processing Task
      |
      v
State Manager
     / \
    /   \
   v     v
Output   Network
Control  Communication
```

Direct coupling such as the following should be avoided:

```cpp
if (buttonPressed)
{
    mqttPublish(...);
}
```

Instead, the hardware driver should report the state change and allow the event-processing architecture to determine the required system response.

---

## 7. FreeRTOS Task Architecture

The initial architecture uses several logical FreeRTOS tasks.

### Input Acquisition Task

Responsible for:

- reading digital inputs
- reading analog inputs
- processing encoder activity
- debouncing
- filtering
- detecting meaningful state changes
- generating input events

### Event Processing Task

Responsible for:

- receiving internal events
- validating events
- applying state transitions
- updating the local state manager
- generating network actions
- generating output actions

### Network Task

Responsible for:

- Wi-Fi connectivity
- MQTT connectivity
- MQTT publishing
- MQTT subscription handling
- reconnection
- resubscription
- transmission of queued events
- receiving remote commands

### Output Task

Responsible for:

- LEDs
- status indicators
- PWM outputs
- applying locally generated output changes
- applying remotely generated output changes

### Telemetry Task

Responsible for periodic health reporting including:

- uptime
- RSSI
- firmware version
- connectivity state
- synchronization state
- fault status
- heartbeat messages

Exact task allocation may be adjusted during implementation to avoid unnecessary task complexity.

---

## 8. Inter-Task Communication

FreeRTOS queues will be used where appropriate to transfer events between firmware components.

Additional synchronization primitives may include:

- mutexes
- semaphores
- event groups
- notifications

Synchronization mechanisms will be selected based on the requirements of each shared resource.

Shared global state should be minimized.

---

## 9. State Manager

The StateManager provides the central local representation of the visual-management station.

Responsibilities include:

- tracking current input state
- tracking current output state
- maintaining station-level state
- validating state transitions
- processing remote state updates
- detecting synchronization differences
- providing state to other firmware modules

---

## 10. Digital Input Processing

Digital input processing shall include debouncing.

Depending on the interface, acquisition may use:

- polling
- GPIO interrupts
- hardware-supported peripherals

The final approach will be selected according to the response-time and scalability requirements of the interface.

---

## 11. Rotary Encoder Processing

Rotary encoders shall use quadrature decoding.

The firmware shall determine:

- clockwise movement
- counter-clockwise movement
- position/count changes

Invalid or noisy transitions should be rejected where practical.

---

## 12. Analog Input Processing

Analog controls such as sliders shall use the ESP32 ADC.

The processing path may include:

```text
ADC Sample
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
State Change Event
```

This prevents small ADC fluctuations from continuously generating network traffic.

---

## 13. Networking

Networking functionality shall be separated into Wi-Fi and MQTT management components.

The Wi-Fi manager handles:

- network connection
- network status
- reconnection

The MQTT manager handles:

- broker connection
- subscriptions
- publishing
- incoming messages
- reconnect
- resubscription

---

## 14. Persistent Storage

Persistent storage shall use ESP32 NVS and flash where appropriate.

A configuration storage component may maintain:

- device ID
- mappings
- network settings
- backend settings
- firmware information

A separate offline event store may maintain pending events that must survive network outages or resets.

---

## 15. Fault Handling

The firmware should recover from common failures without operator intervention where possible.

Fault scenarios include:

- Wi-Fi disconnection
- MQTT disconnection
- unavailable backend
- invalid network messages
- malformed JSON
- stalled tasks
- unexpected reset

Watchdog functionality shall be used where appropriate.

---

## 16. Logging

ESP-IDF logging functionality will be used during development.

Logs should provide visibility into:

- startup
- configuration
- input events
- state transitions
- Wi-Fi state
- MQTT state
- message transmission
- synchronization
- faults
- recovery operations

Sensitive credentials must not be printed to logs.