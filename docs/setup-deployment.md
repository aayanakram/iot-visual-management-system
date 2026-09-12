# Setup and Local Demonstration

The validated scope is software-only. You need ESP-IDF 6.1 with ESP32-S3 tools, Python 3.10+, and a host C++ compiler for firmware logic tests. A board, broker and Odoo instance are not needed for compilation or the in-process demo. A broker is required to run the MQTT middleware against a network transport.

## Firmware

Use an activated ESP-IDF shell and start from the repository root:

```sh
cd firmware
idf.py --version
idf.py build
```

The default CMake target is ESP32-S3. If a previous build targeted another chip, use `idf.py set-target esp32s3` first; that command regenerates target configuration. `sdkconfig.defaults` sets a 1000 Hz FreeRTOS tick and a 4096-byte ESP main task stack. `sdkconfig` is local/generated and ignored. Component Manager retrieves the versions in `main/idf_component.yml` and `dependencies.lock`; keep the lock file, while `managed_components/` and `build/` are ignored. First-time dependency retrieval requires network access or a populated cache.

On the validation Windows machine, the installed environment is activated with:

```powershell
. C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1
$env:PYTHONUTF8 = '1'
idf.py build
```

That path is machine-specific. Other installations should use their own ESP-IDF shell or export script. A bare shell may not have `idf.py` on PATH.

For future hardware operation, copy `main/config/DeviceConfig.local.hpp.example` to `main/config/DeviceConfig.local.hpp` and edit the four macros. This optional local header is ignored by Git. Keep device IDs to 1–64 letters, digits, underscores or hyphens and assign a different ID to each active station. Firmware defaults remain placeholders. `idf.py menuconfig` configures ESP-IDF settings; it does not provide station Wi-Fi/broker fields for this prototype.

The firmware currently requires a WPA2-compatible Wi-Fi access point. A plaintext `mqtt://` URI is suitable for a controlled local prototype; no device credential provisioning or production TLS setup is supplied. Do not put real network secrets in tracked source or example files.

## Python backend

From the repository root:

```sh
python -m venv .venv
```

Activate the environment with `.venv\Scripts\Activate.ps1` in PowerShell or `source .venv/bin/activate` on POSIX, then:

```sh
python -m pip install -r backend/requirements.txt
python -m backend.demo
```

The demo uses an in-process simulated station, transport callbacks and the real protocol/middleware/mock adapter. It asserts ordered offline replay, a toggle-driven output command, analog mapping and an independent mock ERP output change. It does not connect to MQTT or Odoo.

To connect the backend to a broker, configure environment variables. PowerShell example:

```powershell
$env:MQTT_HOST = 'localhost'
$env:MQTT_PORT = '1883'
python -m backend
```

POSIX example:

```sh
MQTT_HOST=localhost MQTT_PORT=1883 python -m backend
```

| Variable | Default / meaning |
|---|---|
| `MQTT_HOST` | `localhost` |
| `MQTT_PORT` | `1883`; use the broker's actual TLS port when configuring TLS |
| `MQTT_CLIENT_ID` | `kaizen-demo-backend`; make unique if running multiple clients |
| `MQTT_USERNAME` | Optional broker username |
| `MQTT_PASSWORD` | Optional broker password |
| `MQTT_CA_FILE` | Optional CA file; setting it enables Paho TLS certificate verification |

`.env.example` is a reference only: export variables yourself. There is no dotenv dependency or automatic `.env` loading. The backend retries initial connection and later reconnects; stop with Ctrl+C. No Odoo variables are currently needed because the selected adapter is a mock.

## Benchmark harness

The procedure below was written for manual use and is now automated in `bench/`,
which starts its own broker, drives simulated stations through it into the real
middleware, and records latency, throughput and outage recovery. See
[bench/README.md](../bench/README.md) and [performance.md](performance.md).

```sh
python -m bench.run_bench --scenario all
```

## Optional broker-assisted software check

This procedure was **not** part of recorded validation. Start a local MQTT broker you control and run `python -m backend`. With Mosquitto CLI tools installed, open a subscriber first:

```sh
mosquitto_sub -h localhost -t kaizen/stations/station_01/commands -q 1 -v
```

Save the event example from [mqtt-integration.md](mqtt-integration.md) as a UTF-8 file `event.json`, then publish it in another terminal (no retain flag):

```sh
mosquitto_pub -h localhost -t kaizen/stations/station_01/events -q 1 -f event.json
```

Expected command JSON is `{"command":"set_output","value":true}`. This verifies middleware through a broker with a software publisher/subscriber; it still does not validate ESP32 networking or physical output. Add the broker's authentication/TLS options when required. Use a local broker you control for this demonstration.

## Automated software checks

From the root, after the firmware build:

```sh
python firmware/tests/run_host_tests.py
python -m unittest discover -s tests -v
python -m backend.demo
```

The host runner locates `g++` by default; set `CXX` to a compatible compiler executable if needed. It builds `firmware/build-host/host_tests[.exe]` from actual C++ modules plus locked cJSON and test-only shims. Python contract tests require this executable. If testing an alternate checkout layout, `FIRMWARE_DIR` can specify its firmware directory. The recorded environment used MinGW GCC 6.3.0 for host tests and the ESP-IDF Xtensa toolchain for the firmware.

## Future physical bring-up

Select a board and inspect the conceptual [interface map](../hardware/pinout/interface-map.md), supply, grounding, voltage limits and LED resistor before connecting anything. Validate ADC range and pin availability against that board. Only when hardware is available and wiring has been checked, use `idf.py -p <PORT> flash monitor` from `firmware/`.

Future validation should cover initial input states, debounce, encoder edge rate/detents, ADC scaling, output behavior, Wi-Fi loss/recovery, broker loss/resubscription, FIFO overflow and reset loss. A future live Odoo adapter requires a separate test instance and validated API mapping. No flashing, electrical checks, RF checks or live Odoo testing are claimed here.
