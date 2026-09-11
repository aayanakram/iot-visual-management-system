# MQTT Integration

Only two topic forms are implemented:

| Topic | Producer | Consumer |
|---|---|---|
| `kaizen/stations/<device_id>/events` | Station | Python middleware subscribing to `kaizen/stations/+/events` |
| `kaizen/stations/<device_id>/commands` | Middleware | Matching station |

Use a unique device ID of 1–64 ASCII letters, digits, underscores or hyphens. Firmware event and command topics use `config::MQTT_BASE_TOPIC`; the Python prototype uses the same fixed root. If changing the root, update both implementations. Events and commands are QoS 1, non-retained. Retained incoming commands/events are ignored. There are no `/state`, `/telemetry`, `/heartbeat` or `/config` subscriptions/publications.

## Event JSON

```json
{"device_id":"station_01","sequence_id":1,"event_type":"toggle_changed","source_id":2,"value":1,"timestamp_ms":1234,"firmware_version":"0.1.0"}
```

The backend requires exactly these seven fields and verifies that `device_id` matches the topic. Duplicate keys, missing/unknown fields, invalid types/ranges, unsupported event/source combinations and malformed JSON are rejected. Integers are required for numeric event fields; booleans are not integers in this contract. Maximum backend payload size is 4096 bytes for MQTT byte payloads.

| `event_type` | `source_id` | `value` |
|---|---:|---|
| `button_pressed` | 1 | 1 |
| `button_released` | 1 | 0 |
| `toggle_changed` | 2 | 0 or 1 |
| `encoder_changed` | 3 | Signed 32-bit absolute quadrature edge count |
| `analog_changed` | 4 | Integer percentage, 0–100 |
| `heartbeat` | 0 | Nonnegative free heap bytes |
| `fault_detected` | 0 | Signed integer fault code; reserved producer |
| `sync_requested` | 0 | Signed integer; reserved producer |

`timestamp_ms` is monotonic device uptime at generation, not Unix/UTC time. Heartbeat timestamps also represent device uptime; delivery can be delayed by MQTT. `sequence_id` is a uint32 counter assigned centrally, starting at 1 on each boot and wrapping through 0. Discarded heartbeats/full buffers can leave gaps. There is no boot ID, global ordering, durable deduplication or cross-reboot uniqueness. Preserve original timestamps/IDs when replaying.

## Command JSON

```json
{"command":"set_output","value":true}
```

```json
{"command":"set_state","value":false}
```

`set_output` creates `SetOutput`; `set_state` creates `RemoteStateUpdate`. Both set the single logical and LED output state. The topic selects the station. No device ID, sequence, output ID or timestamp is required in the command. Commands must have exactly two fields, and value must be boolean or numeric 0/1. The C++ parser supplies the receive timestamp and leaves sequence zero. Unknown commands, extra/duplicate fields, trailing data, embedded NULs, invalid values and messages over 512 bytes are rejected. MQTT fragments are deliberately rejected rather than reassembled; normal compact commands fit in one message buffer. Remote commands are not echoed as outgoing events, and there is no application command acknowledgement.

## Recovery and delivery boundaries

ESP-MQTT retries connection every 5 seconds by configuration and resubscribes on each connection. Wi-Fi requests reconnect after a disconnection unless explicitly stopped. Middleware Paho reconnect backoff ranges from 1 to 30 seconds and subscribes in its connection callback. These API choices follow [Paho's client documentation](https://eclipse.dev/paho/files/paho.mqtt.python/html/client.html); host callback tests do not verify actual network reconnection.

The firmware stores up to 64 unsent non-heartbeat events in RAM. A full queue drops the newest event and logs the loss. Application checks retry opportunities at least once per one-second receive timeout, so a missed MQTT connection notification or temporarily full MQTT outbox does not leave the FIFO waiting forever for another reconnect. An older FIFO entry must be accepted before a newer event. ESP-MQTT has a separate 16 KiB RAM outbox; accepted QoS 1 messages can be retransmitted by the library and may expire under its default policy.

Neither queue survives reboot. Enqueue success is not confirmation of broker delivery or ERP application. MQTT cannot detect that this mock backend is down while the broker is up. The backend uses a clean session and no durable event store, and commands are not retained, so offline consumers may miss publications. Repeated absolute mock-state assignments tolerate immediate QoS 1 duplicates, but stale duplicates and independent changes can overwrite newer state. There is no conflict resolution or end-to-end delivery guarantee.

The default firmware URI is a plaintext placeholder. Production broker ACLs, authentication and device TLS credential provisioning are future work. The Python client can use configured username/password and a CA file; this does not imply production security was validated.
