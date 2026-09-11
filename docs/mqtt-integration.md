# MQTT Integration

## 1. Overview

MQTT is the selected primary communication protocol between the ESP32-S3 visual-management stations and the system backend.

MQTT is suitable for this architecture because it provides:

- lightweight communication
- publish/subscribe messaging
- bidirectional communication
- asynchronous communication
- multi-device routing
- simple integration with IoT backends

---

## 2. Architecture

```text
ESP32-S3
    |
    | MQTT over Wi-Fi/TLS
    |
    v
MQTT Broker
    |
    v
Backend / Middleware
    |
    v
Odoo ERP
```

The ESP32 does not communicate directly with the Odoo database.

---

## 3. Topic Structure

The proposed MQTT hierarchy is:

```text
kaizen/stations/{device_id}/events
kaizen/stations/{device_id}/state
kaizen/stations/{device_id}/telemetry
kaizen/stations/{device_id}/heartbeat
kaizen/stations/{device_id}/commands
kaizen/stations/{device_id}/config
```

Example for `station_07`:

```text
kaizen/stations/station_07/events
kaizen/stations/station_07/state
kaizen/stations/station_07/telemetry
kaizen/stations/station_07/heartbeat
kaizen/stations/station_07/commands
kaizen/stations/station_07/config
```

---

## 4. Device-to-Backend Topics

The station may publish to:

```text
/events
/state
/telemetry
/heartbeat
```

### Events

Used for individual physical or logical state-change events.

### State

Used for current station-state information or synchronization.

### Telemetry

Used for operational health and diagnostic information.

### Heartbeat

Used for periodic online/alive indication.

---

## 5. Backend-to-Device Topics

The station may subscribe to:

```text
/commands
/config
```

### Commands

Used for remote actions such as:

- changing an LED
- setting an output state
- requesting synchronization
- triggering supported device actions

### Configuration

Used for station-specific configuration updates where remote configuration is supported.

---

## 6. Event Payload

The proposed message format is JSON.

Example:

```json
{
  "device_id": "station_07",
  "sequence_id": 1427,
  "event_type": "toggle_changed",
  "input_id": "task_03",
  "value": "done",
  "timestamp": 1788894000,
  "firmware_version": "0.1.0"
}
```

Fields may include:

- device ID
- sequence ID
- event type
- physical interface ID
- new value
- timestamp
- firmware version

---

## 7. Remote Command Payload

Example:

```json
{
  "device_id": "station_07",
  "sequence_id": 7731,
  "command": "set_output",
  "output_id": "task_03_led",
  "value": true
}
```

---

## 8. Sequence Identifiers

Sequence identifiers are intended to assist with:

- duplicate detection
- event ordering
- retries
- synchronization
- debugging

The exact acknowledgement and deduplication policy will be finalized during backend integration.

---

## 9. Connection Recovery

When MQTT communication is lost, the station should:

```text
Detect MQTT loss
       |
       v
Continue local operation
       |
       v
Buffer relevant outgoing events
       |
       v
Restore Wi-Fi if required
       |
       v
Reconnect to broker
       |
       v
Restore subscriptions
       |
       v
Restore synchronization
       |
       v
Transmit pending events
```

Recovery should occur automatically.

---

## 10. Offline Buffering

If an event cannot be published because the network or broker is unavailable, the firmware shall store the event according to the configured offline-buffering strategy.

When communication returns, pending events shall be processed in a defined order.

The final design will define:

- queue size
- flash behaviour
- ordering
- expiry policy
- retry limits
- duplicate protection
- conflict handling

---

## 11. MQTT Security

Production MQTT communication should use TLS.

Credentials and private keys shall not be committed to source control.

The exact certificate and credential-provisioning method remains to be finalized.

---

## 12. Quality of Service

MQTT QoS settings will be selected according to message type.

Events that represent operational changes may require stronger delivery guarantees than periodic telemetry.

Final QoS selection will be validated during system testing.

---

## 13. Multi-Device Scalability

Each station uses a unique device ID.

This identifier is included in both:

- MQTT topic paths
- message payloads

This allows the backend to independently route and manage messages from multiple physical stations.