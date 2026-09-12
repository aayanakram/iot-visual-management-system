# Firmware Architecture

The implemented target is ESP32-S3, ESP-IDF 6.1, C++ and FreeRTOS. `main/main.cpp` creates static objects that live for the lifetime of the firmware. It initializes NVS, the event queue, outputs, inputs, MQTT configuration, the Application task, input polling, Wi-Fi/network services, and finally starts MQTT. Networking is initialized before MQTT can use the TCP/IP stack.

| Module | Implemented responsibility |
|---|---|
| `Event` | Type, numeric source, signed value, monotonic `timestampMs`, sequence ID |
| `EventQueue` | FreeRTOS queue of 32 copied events; nonblocking producers; logs full-queue drops |
| `InputManager` | 3072-byte stack, priority 4; polls button, toggle, encoder and ADC every 10 ms plus processing time |
| `DigitalInput` | Active-low pull-up GPIO, 30 ms debounce, initial debounced report, retry while an unqueued stable state remains |
| `AnalogInput` | ADC one-shot, 12-bit raw scaling to 0–100, deadband of 2 percentage points; initial report and retry on a full queue |
| `RotaryEncoder` | GPIO quadrature transition table; absolute count per valid edge; rejects two-bit transitions; saturates at int32 limits |
| `Application` | 4096-byte stack, priority 5; consumes events, updates state/output, assigns outgoing sequence IDs, publishes/buffers and retries |
| `StateManager` | Button, toggle, encoder, analog and requested output state; no ERP rules or persistence |
| `OutputManager` / `LedDriver` | Applies `SetOutput` and `RemoteStateUpdate` to one active-high digital LED; starts off |
| `WifiManager` | Network stack/event-loop initialization, WPA2 station configuration, reconnect request on loss, queue notifications |
| `MqttManager` | ESP-MQTT client, 5 s reconnect interval, per-station command subscription on connect, 16 KiB outbox, enqueue publications |
| `MessageSerializer` | Seven-field JSON events; parses two supported commands using managed cJSON |
| `OfflineEventStore` | Mutex-protected, bounded `std::deque<Event>` FIFO, capacity 64; newest-per-source coalescing, drop-new only for uncoalesced system events |
| `DeviceConfig` | Prototype mappings, queue sizes, timing and placeholder network configuration; optional ignored local header |

```text
InputManager -> drivers -> EventQueue -> Application -> StateManager
                                              |----> OutputManager -> LED
                                              |----> serializer -> MQTT
                                              |----> OfflineEventStore -> retry
Wi-Fi/MQTT callbacks -> EventQueue ------------^
MQTT command -> serializer -> EventQueue ------^
```

State and output updates occur in the Application task. ESP-IDF provides network tasks; there is no custom output, network or telemetry task. The MQTT connection flag is atomic across callback/application contexts. `esp_mqtt_client_enqueue` keeps socket transmission in the MQTT task rather than blocking the application on network writes.

Input drivers timestamp observations with `esp_timer_get_time()/1000`. Application assigns sequence IDs only to outgoing events, starting at 1 each boot. Buffered events retain these values. Connectivity and remote commands are not published upstream, avoiding command loops.

Application waits at most one second for an event, checks the FIFO after every iteration, and generates a heartbeat every 30 seconds even when inputs keep arriving. The heartbeat `value` is free heap bytes; `timestamp_ms` gives uptime and the ordinary event fields identify the station/version. It is processed directly inside Application, with no additional task. It reports application liveness, not comprehensive system health or backend acknowledgement.

The FIFO retries oldest first on connection notification, on normal loop iterations, and before new publications. A head event is removed only after MQTT accepts it, except for an event that cannot be serialized at all, which is dropped with an error rather than stalling the events behind it. MQTT acceptance is not broker PUBACK or Odoo acknowledgement. Heartbeats that cannot be sent are discarded. There is no persistent event storage: both the FIFO and MQTT outbox disappear on reboot. NVS initialization supports ESP-IDF services, not a persistent application event queue.

`FaultDetected` and `SyncRequested` have serialization/dispatch support but no normal physical producer. Fast encoder movement, electrical debounce, ADC accuracy, FreeRTOS scheduling, task stack margins, output write errors and RF behavior require physical validation. Host tests use selected real modules with small test-only shims; they do not emulate ESP32 hardware.
