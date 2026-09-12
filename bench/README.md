# Benchmark Harness

Latency and scale measurement for the MQTT broker and the Python middleware.
Recorded results and their interpretation are in [docs/performance.md](../docs/performance.md).

**The publisher is not the firmware.** It is `backend.demo.SimulatedStation`
driving a paho client. No ESP32 runs, so nothing here measures device-side
polling, debounce, serialization, Wi-Fi, or the firmware's own offline buffer.
These runs characterize broker plus middleware.

## Requirements

- Python 3.10+ with `backend/requirements.txt` installed
- A broker. The harness picks one automatically:
  - `native` — a `mosquitto` executable on PATH or in a standard install
    location, run as a child process
  - `docker` — an `eclipse-mosquitto:2` container the harness creates and removes
  - `external` — a broker already listening on the target port; scenario C is
    skipped because the harness cannot restart a broker it does not own

The default port is 1884 rather than 1883, so a run does not collide with a
broker you already have running.

## Running

```sh
python -m bench.run_bench --scenario all
```

```sh
python -m bench.run_bench --scenario a --duration 60 --rate 1
python -m bench.run_bench --scenario b --stations 20 --inputs 35 --rates 100,500,2000
python -m bench.run_bench --scenario c --outage 10 --stations 5
```

Useful flags: `--broker native|docker|external`, `--host`, `--port`, `--step`
(seconds per load step in B), `--out` (results directory).

## Scenarios

| | What it does | What it answers |
|---|---|---|
| A | One station, steady toggles at a low rate | The latency floor on an idle system |
| B | 20 stations, offered load ramped upward | Where latency degrades, and whether anything is lost |
| C | Broker stopped mid-run and restarted | Reconnect time, backlog drain, events lost |

## What is measured

- **`ingress_to_command`** — from the middleware receiving a station event to it
  having published the generated command. Middleware processing plus paho
  publish enqueue. Excludes both broker hops.
- **`station_roundtrip`** — from the station publishing an event to the same
  station receiving the resulting command. Includes both broker hops and the
  middleware. This is the number that corresponds to how long a person waits for
  the LED.

Only `toggle_changed` and `sync_requested` generate commands, so latency samples
come from toggles. Other event types contribute to throughput but produce no
command and therefore no latency sample.

Intervals use `time.perf_counter_ns`, not `time.time_ns`. On Windows
`time.time_ns` advances in steps of roughly 1 ms, which is coarser than what is
being measured; `perf_counter_ns` advances in steps of roughly 200 ns. The
wall-clock `received_at_ns` that the middleware records on each Workstation is
still `time.time_ns`, since that is a date rather than a duration.

Commands carry no correlation ID, so a received command is paired with the
oldest toggle that station published and has not yet been matched. QoS 1 on a
single topic through one broker preserves per-station ordering, which is what
makes the pairing valid.

## Output

`bench/results/` holds `summary.json`, `summary.csv` and
`scenario_a_samples.csv`. Committed results are from one developer machine and
are not a claim about any other hardware.
