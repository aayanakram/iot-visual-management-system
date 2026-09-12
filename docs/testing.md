# Software Validation Record

Validation date: 2026-09-11. Scope: student software prototype. No ESP32-S3 hardware, physical station or live Odoo instance was available.

## Commands and actual results

| Check | Result |
|---|---|
| `idf.py build` in activated ESP-IDF 6.1 environment | Passed for ESP32-S3 as of the previous validation run. **Not re-run after the offline-buffer change**; no ESP-IDF toolchain was available in the environment where that change was made. The changed files compile under the host C++ tests only. |
| Install `backend/requirements.txt` | Passed; Paho MQTT 2.1.0 installed in a project virtual environment |
| `python firmware/tests/run_host_tests.py` | Passed; actual firmware logic groups listed below |
| `python -m unittest discover -s tests -v` | 20 tests passed, no skips |
| `python -m backend.demo` | Passed assertions; replayed sequences 1 and 2, mock progress 75, output true from toggle then false from remote state |
| `python -m bench.run_bench --scenario all` | Completed against a local mosquitto 2.1.2. Idle station-to-command round trip 0.84 ms p50; sustained ceiling about 2 000 events/s; a 11.8 s broker outage recovered in 4.0 s with no events lost. See [performance.md](performance.md) |

## Firmware tests

The host test runner compiles these actual modules: `MessageSerializer`, `OfflineEventStore`, `EventQueue`, `Application`, `StateManager`, `OutputManager`, `LedDriver`, `DigitalInput` and `RotaryEncoder`, plus the Component Manager's locked cJSON source. Small test-only headers stand in for timers, GPIO, FreeRTOS queue/mutex/task primitives and the Application-facing MQTT transport.

The executable verifies:

- Seven-field serialization, timestamps/sequence values, valid output/state commands and malformed/unsupported command rejection.
- FIFO empty/peek/pop/clear, zero capacity, bounded overflow, preserved order and volatile fresh-store behavior.
- Newest-per-source coalescing: distinct sources stay separate, a newer value replaces an older one from the same source and moves to the back, a 101-sample slider sweep collapses to one entry carrying the current value, and system-source events are not coalesced.
- A buffered event whose serialization fails is dropped and the rest of the backlog still drains, verified by installing a failing cJSON allocation hook for exactly one allocation.
- Local state updates while disconnected, partial publish failure, ordered replay, retry without another reconnect event, and new events staying behind the backlog.
- Remote command -> Application -> logical state -> output driver, with no command echo upstream.
- Heartbeat generation with free heap/uptime and discarding unsendable heartbeats.
- Initial debounced digital state, bounce rejection, accepted transition timestamp, encoder direction and invalid quadrature transition rejection.

These tests exercise firmware code, but the queue/mutex/clock/GPIO behavior comes from host shims. They do not verify FreeRTOS timing/concurrency, ESP32 execution, electrical signals, ADC hardware or ESP-MQTT internals. AnalogInput, InputManager and the real Wi-Fi/MQTT managers are firmware-compiled and source-reviewed; their physical/network behavior remains unvalidated.

## Python and cross-language tests

The Python suite verifies event round trips, all supported event kinds, numeric/source ranges, missing/extra/duplicate fields, malformed JSON, station topic matching, valid/invalid commands, mock Odoo mappings, station isolation, repeated absolute assignments, reverse commands, publish failure visibility, callback resubscription and retained/malformed event rejection followed by recovery.

Simulation tests cover both directions, bounded overflow, loss on simulated restart, failed flush ordering and subsequent recovery. The cross-language tests run the host executable to serialize a firmware event, parse it in Python, apply it to the mock adapter, and feed generated JSON commands back into the actual C++ command parser. They also run a shared rejection corpus.

`backend.demo` uses function calls as transport. No socket, MQTT broker, real Odoo API or ESP32 is involved. Paho client construction and callback tests do not establish broker connectivity or reconnection timing. An optional broker-assisted procedure is documented in [setup-deployment.md](setup-deployment.md), but was not run during this validation.

## Remaining build warnings

ESP-IDF configuration emitted private-include dependency warnings for `wpa_supplicant` using `esp_wifi` directories, plus upstream Kconfig notes about invalid boolean defaults in Bluetooth/FATFS and duplicate Bluetooth rename mappings. These were warnings/notes, not application compile errors. No ESP-IDF/vendor sources were modified to suppress them. The application image uses roughly 90% of its 1 MiB application partition; future growth should account for that margin.

## Required future physical/live-system validation

Flashing, GPIO/power inspection, button/toggle/encoder/slider operation, analog accuracy, electrical debounce, output current, hardware timing, stack margins, Wi-Fi RF, reboot/power-cycle behavior, long-duration operation and live Odoo integration remain untested. The RAM queue intentionally loses events on reset. No RF recovery time, physical event-loss rate or production reliability claim is made.

Broker and middleware latency, throughput and outage recovery **have** now been measured, against a local mosquitto with a simulated Python publisher. Results and their boundaries are in [performance.md](performance.md). Those runs do not involve an ESP32, so they characterize broker plus middleware only and make no claim about device-side or radio timing.
