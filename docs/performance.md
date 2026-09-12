# Measured Performance

Measurement date: 2026-09-11. Harness: [`bench/`](../bench/README.md). Raw output:
[`bench/results/`](../bench/results).

## What these numbers cover, and what they do not

**The publisher is not the firmware.** Every run below was driven by
`backend.demo.SimulatedStation` publishing through a paho client. No ESP32
executed. Nothing here measures GPIO polling, the 30 ms debounce, the 10 ms input
task period, cJSON serialization on-device, Wi-Fi association, RF recovery, or
the firmware's own `OfflineEventStore`. **These are broker and middleware
numbers.** A station-to-LED figure on real hardware will be larger by the
device-side and radio time, which remains unmeasured.

Real in these runs: a local mosquitto 2.1.2 broker over TCP, the paho client
built by `backend.__main__.create_client`, `backend.service.Middleware` and
`bind_callbacks`, `backend.protocol` validation, and `MockOdooAdapter`. The only
addition to shipped code paths is a timing wrapper around the publish callable
that `Middleware` already accepts by injection.

Also not covered: a real Odoo adapter. `MockOdooAdapter` is a dictionary
assignment. A real adapter performing a JSON-RPC call per event would dominate
every latency figure here, and because `on_message` runs synchronously on paho's
network thread, it would also lower the throughput ceiling. Treat the middleware
numbers as a floor, not a forecast.

### Host

| | |
|---|---|
| OS | Windows 11 (10.0.26200) |
| CPU | AMD64 family 23, 12 logical cores |
| Python | 3.12.10, paho-mqtt 2.1.0 |
| Broker | mosquitto 2.1.2, local, plaintext, `allow_anonymous true` |
| Topology | Publishers, broker and middleware all on one machine over loopback |

Loopback removes the network entirely. A real deployment adds Wi-Fi and switch
latency to every figure.

### Metrics

- **ingress to command** — from the middleware receiving a station event to
  having published the generated command. Validation, adapter work and paho
  publish enqueue. Excludes both broker hops.
- **station round trip** — from a station publishing an event to that same
  station receiving the resulting command. Both broker hops plus the middleware.
  This is the figure that corresponds to how long a person waits for the LED.

Intervals use `time.perf_counter_ns` (about 200 ns granularity here).
`time.time_ns` was measured on this host at about 1 ms granularity, which is
coarser than the quantity being measured; an earlier draft of this harness using
it reported most samples as 0.000 ms. The wall-clock `received_at_ns` now stamped
on each `Workstation` still uses `time.time_ns`, because that field records when
an event arrived rather than a duration.

Only `toggle_changed` and `sync_requested` generate commands, so latency samples
come from toggles. Commands carry no correlation ID, so each received command is
paired with the oldest unmatched toggle from that station; QoS 1 on one topic
through one broker preserves per-station ordering, which is what makes that
valid.

## Scenario A: one station, idle system

One station, 1 toggle per second, 60 s, 59 samples.

| Metric | p50 | p95 | p99 | max |
|---|---:|---:|---:|---:|
| Ingress to command | 0.196 ms | 0.306 ms | 0.360 ms | 0.360 ms |
| Station round trip | 0.837 ms | 1.100 ms | 1.186 ms | 1.186 ms |

All 59 events were received and all 59 commands delivered. On an unloaded system
the middleware contributes roughly 0.2 ms and the two broker hops roughly 0.6 ms.
Sub-millisecond over loopback; the shop-floor figure will be dominated by Wi-Fi,
not by this software.

## Scenario B: twenty stations, ramped load

20 stations, 35 inputs each, 8 s per step, offered load ramped. Events cycle
through all four sources, so roughly one event in four is a toggle.

| Target ev/s | Achieved ev/s | Published | Received | Delivered | Ingress p50 | Ingress p99 | Round trip p50 | Round trip p99 |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 100 | 99.8 | 798 | 798 | 100% | 0.136 ms | 0.373 ms | 1.022 ms | 1.524 ms |
| 250 | 249.5 | 1 996 | 1 996 | 100% | 0.133 ms | 0.493 ms | 1.846 ms | 2.867 ms |
| 500 | 499.0 | 3 992 | 3 992 | 100% | 0.107 ms | 0.615 ms | 2.975 ms | 4.566 ms |
| 1 000 | 998.0 | 7 984 | 7 984 | 100% | 0.099 ms | 0.690 ms | 5.011 ms | 9.034 ms |
| 2 000 | 1 996.0 | 15 968 | 15 968 | 100% | 0.093 ms | 0.670 ms | 8.339 ms | 51.787 ms |
| 4 000 | 3 712.1 | 29 812 | 15 904 | 53% | 0.105 ms | 0.813 ms | 5 719.862 ms | 6 425.227 ms |
| 8 000 | 3 746.7 | 38 872 | 17 744 | 46% | 0.109 ms | 0.906 ms | 7 645.906 ms | 8 357.171 ms |

**The ceiling is between 2 000 and 4 000 events/s, and it is the middleware's
consume rate.** Three things converge on that reading:

1. Per-event middleware cost never degrades. Ingress-to-command p50 stays at
   0.09–0.14 ms at every step, including the saturated ones. The middleware is
   not getting slower per event; it is being handed more than it can take.
2. Received counts plateau. The 4 000 and 8 000 steps delivered 15 904 and 17 744
   events in 8 s, about 1 990 and 2 220 per second. That is the same ceiling in
   both, and it matches the highest rate that sustained 100% delivery.
3. Round trip jumps three orders of magnitude, from 8 ms to 5.7 s, which is queue
   growth rather than slower processing.

**The 53% and 46% figures are not event loss.** `publish_refusals` and
`station_buffer_drops` were zero throughout. Those events were accepted by the
broker and were still queued when the step ended and the clients were torn down
after the settle window. The correct reading is that the backlog grew without
bound, not that messages were discarded. A longer settle window would have
delivered more of them; it would not have changed the ceiling.

The publisher plateaus at about 3 700 events/s regardless of whether 4 000 or
8 000 is requested, which is TCP backpressure from the broker rather than a
Python pacing limit. The harness flags the 8 000 step as one it could not offer
at the requested rate, so that row should be read as a second sample of the
saturated regime, not as an independent data point.

### What this means for 20 boards of 30-40 inputs

The deployment target generates events only when a control moves. At 2 000
events/s the system was comfortable. That is 100 events/s per station, or roughly
three events per second per input across 35 inputs — far above what humans
produce on a task board, and enough headroom that steady-state operation is not
in question on this hardware.

The caveat is the burst case, not the steady state. A slider swept by hand
produces up to one event per 10 ms poll, so a handful of simultaneously moving
analog inputs across 20 stations approaches the measured ceiling. Once past it,
latency degrades into seconds rather than dropping messages, so the visible
symptom on the floor would be a lagging board rather than a wrong one.

### A contract limit found while building this

The wire contract pins `source_id` to `event_type` (`backend/protocol.py:92`), so
a station can address exactly four logical sources: button, toggle, encoder,
analog. A 30-40 input station cannot currently identify which of its 12 toggles
moved. Scenario B therefore reproduces the message *volume* that 20 boards of 35
inputs would generate, but not their addressing. Supporting the real input count
needs a contract change, and that change is a prerequisite for the deployment
target rather than an optimization. This is not recorded elsewhere in the docs.

## Scenario C: broker outage

5 stations at 5 events/s each. The broker was stopped mid-run and restarted.

| | |
|---|---:|
| Outage duration | 11.78 s |
| Broker accepting connections again after restart | 0.50 s |
| Station reconnect after resume was requested | 3 939, 3 939, 3 940, 3 958, 3 958 ms |
| Middleware reconnected | yes |
| Peak events buffered per station | 50 |
| Events dropped by the station buffer | 0 |
| Recovery, reconnect plus backlog drain | 3.97 s |
| Events published / received | 325 / 325 |

Round trip during this run was p50 52.3 ms, p99 77.2 ms, inflated because the
replayed backlog arrives as a burst; those samples are queueing time, not
steady-state latency.

**Reconnect time is set by client backoff, not by broker availability.** The
broker accepted connections 0.5 s after being asked to restart, but clients took
a further 3.9 s, because paho had backed off across repeated failures during the
outage. Two consequences for the delivered system:

- This harness sets `reconnect_delay_set(min_delay=1, max_delay=5)`. The shipped
  backend uses 1 to 30 s (`backend/__main__.py:21`), so a production backend can
  take substantially longer than 3.9 s to return after a long outage. That is
  worth measuring against the shipped setting before quoting a recovery figure.
- The firmware uses a fixed 5 s `reconnect_timeout_ms`
  (`firmware/main/network/MqttManager.cpp:46`), so it does not back off and
  should recover within about 5 s regardless of outage length. That asymmetry is
  unverified on hardware.

**The buffer depth result is about the model, not the firmware.** Zero events
were dropped because 50 buffered events fit inside the 64-slot buffer. At 5
events/s per station, 64 slots covers roughly 13 s of outage. A station with 35
inputs in active use would fill it in about a second. Note that
`SimulatedStation` still implements the **pre-coalescing** drop-new policy, so
this scenario measures the old behavior; the firmware's `OfflineEventStore` now
keeps one entry per source and would not overflow at all here. Recorded in
[known-limitations.md](known-limitations.md).

## Summary

| Question | Answer on this host |
|---|---|
| Idle latency, station to LED | 0.84 ms p50, 1.19 ms max, loopback only |
| Middleware cost per event | 0.09-0.20 ms p50, flat under load |
| Sustained throughput ceiling | about 2 000 events/s, limited by middleware consume rate |
| Behavior past the ceiling | unbounded queue growth and multi-second latency, not message loss |
| Recovery from a broker outage | under 4 s here, governed by client backoff settings |
| Events lost across a 12 s outage | none, with the buffer at 78% of capacity |

## What still needs measuring

- The same scenarios against real ESP32-S3 hardware over Wi-Fi. Every figure
  here excludes the device and the radio.
- The throughput ceiling with a real Odoo adapter in place of the mock, which is
  the change most likely to move it.
- The shipped 1-30 s backend reconnect backoff, rather than the 1-5 s this
  harness sets.
- Sustained multi-hour operation. The longest run here was 60 s.
- Behavior with TLS enabled; all runs were plaintext.
