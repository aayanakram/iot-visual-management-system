# IoT Visual Management Station

A software prototype connecting physical visual-management controls to a simulated Odoo workstation. It addresses the delay and duplicate data entry that can occur when a physical task board and an ERP system are updated separately.

The firmware targets ESP32-S3 with ESP-IDF 6.1, C++ and FreeRTOS. Python middleware validates MQTT events and updates an in-memory mock Odoo adapter. A toggle can mark a simulated task done and generate an LED command; a simulated ERP change can independently set the station output.

**No physical hardware was built or tested. No live Odoo instance was tested.** GPIO assignments are prototype mappings. Software test evidence and remaining validation work are recorded in [docs/testing.md](docs/testing.md).

## Architecture

```text
Button / toggle / encoder / analog slider
             |
 InputManager -> input drivers -> EventQueue
                                     |
                                Application
                               /     |      \
                     StateManager    |    OutputManager -> LedDriver
                                     |           ^
                          JSON / MqttManager      |
                              |      ^           |
                        Wi-Fi / MQTT broker      |
                              |      |           |
                       Python middleware         |
                              |      |           |
                        Mock Odoo adapter        |
                              +-- commands -> EventQueue -> Application

When publishing is unavailable: Application -> bounded RAM FIFO -> retry
```

Two application tasks handle input polling and event processing. ESP-IDF owns the Wi-Fi/MQTT service tasks. There is no separate output or telemetry task. A 30-second heartbeat uses the event-processing task and the existing event schema.

## Implemented scope

- Active-low button and toggle inputs, 30 ms debounce and initial state reporting.
- Polled quadrature encoder with invalid-transition rejection and absolute edge count.
- ADC one-shot input scaled to 0–100, with a 2-percentage-point deadband and initial reporting.
- One digital LED output, controlled through `set_output` or `set_state` commands.
- Central event sequencing, monotonic millisecond timestamps, cJSON serialization and strict command shape/value checks.
- Wi-Fi reconnect requests, ESP-MQTT automatic reconnect and command resubscription.
- A 64-event offline FIFO that keeps only the newest buffered value per input source, a bounded ESP-MQTT outbox, and periodic retry after temporary publication failures.
- Python 3.10+ middleware using Paho MQTT 2.1.0; no web server or database.
- An Odoo adapter interface, mock workstation mapping, host firmware tests and a deterministic software demo.

The firmware dependencies are `espressif/mqtt` 1.1.0 and `espressif/cjson` 1.7.19~2. The Component Manager manifest and lock file are retained. See [firmware architecture](docs/firmware-architecture.md) for module responsibilities.

## Repository

```text
firmware/main/          ESP-IDF firmware and configuration
firmware/tests/        Actual C++ logic tests with small host-only shims
backend/               MQTT middleware, protocol, mock Odoo and demo
tests/                 Python tests and Python/C++ contract checks
docs/                  Architecture, setup, protocol and validation records
hardware/              Conceptual wiring, component recommendations and pin map
```

## MQTT contract

| Direction | Topic |
|---|---|
| Station to middleware | `kaizen/stations/<device_id>/events` |
| Middleware to station | `kaizen/stations/<device_id>/commands` |

Events and commands use QoS 1 and are not retained. Heartbeats use the events topic. There are no separate state, telemetry, heartbeat or configuration topics.

```json
{"device_id":"station_01","sequence_id":1,"event_type":"toggle_changed","source_id":2,"value":1,"timestamp_ms":1234,"firmware_version":"0.1.0"}
```

```json
{"command":"set_output","value":true}
```

`set_state` has the same boolean output semantics and generates `RemoteStateUpdate` internally. Commands require exactly `command` and `value`; numeric 0/1 are also accepted. The current station has one output, so there is no `output_id` field. `timestamp_ms` is uptime when the event was generated, not Unix time. Sequence IDs start at 1 on boot and may wrap; they are not globally unique. See [the full MQTT contract](docs/mqtt-integration.md).

## Build and run

In an activated ESP-IDF 6.1 shell, from the repository root:

```sh
cd firmware
idf.py build
```

The CMake default target is `esp32s3`. For an existing build configured for another chip, explicitly use `idf.py set-target esp32s3`. Before a future hardware demo, copy `main/config/DeviceConfig.local.hpp.example` to the ignored `DeviceConfig.local.hpp` and enter a unique station ID, Wi-Fi settings and broker URI. Placeholder configuration is sufficient for compilation. No physical flashing is part of the recorded validation.

From the repository root, set up the backend:

```sh
python -m venv .venv
```

Activate with `.venv\Scripts\Activate.ps1` in PowerShell or `source .venv/bin/activate` on POSIX, then:

```sh
python -m pip install -r backend/requirements.txt
python -m backend.demo
```

The demo needs neither a broker nor Odoo: it simulates offline inputs, ordered replay, the resulting output command, and an independent remote state change. It is an in-process simulation, not an MQTT transport test.

For an actual broker connection, export `MQTT_HOST` and `MQTT_PORT` (defaults `localhost:1883`) and run:

```sh
python -m backend
```

Optional variables are `MQTT_CLIENT_ID`, `MQTT_USERNAME`, `MQTT_PASSWORD` and `MQTT_CA_FILE`. `backend/.env.example` documents them; `.env` files are not automatically loaded. See [setup instructions](docs/setup-deployment.md) for broker-assisted simulation and [Odoo integration](docs/odoo-integration.md) for the real-adapter extension point.

## Tests

After the firmware build downloads cJSON, use a host C++ compiler (`g++`, or set `CXX` to a compatible compiler path):

```sh
python firmware/tests/run_host_tests.py
python -m unittest discover -s tests -v
python -m backend.demo
```

The full Python suite requires the host executable: it passes firmware-produced JSON through the Python adapter and returns generated commands to the actual C++ parser. Tests fail with a setup error if that executable is missing. See [test results and boundaries](docs/testing.md).

## Offline behavior and limitations

Local input state processing continues without MQTT. The FIFO preserves original timestamps and sequence IDs and retries oldest first. Because every payload is absolute state, a newer value from an input source replaces the older buffered value from that source, so replay ends at the station's current value rather than at a stale one; intermediate values from during the outage are not delivered. It is **volatile RAM and does not survive reboot**. Heartbeats are discarded when they cannot be sent rather than filling this queue. A successful publish means the MQTT client accepted the message, not that Odoo applied it. QoS 1 can duplicate messages; mock assignments are idempotent for immediate duplicates, but durable deduplication and conflict resolution are absent.

The broker and backend must be available to receive live events. A backend outage while the broker remains connected is not detected by the station. Commands are not durably stored for offline stations. Network/GPIO values remain placeholders, encoder polling may miss fast transitions, ADC scaling is uncalibrated, and the mock backend loses state on restart. There is no production provisioning, OTA, PKI, custom PCB or production security validation. TLS credential provisioning remains future work. [Known limitations](docs/known-limitations.md) gives the full scope.

Useful next steps are physical ESP32-S3 validation, a real Odoo adapter, NVS-backed event persistence if needed, TLS credential provisioning, and multi-station deployment testing.
