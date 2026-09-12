"""Latency and scale benchmark for the MQTT broker plus Python middleware.

Run from the repository root:

    python -m bench.run_bench --scenario all

Scenarios
    A  one station at a steady low rate; the quiet-system latency floor
    B  twenty stations, ramped offered load, to find the throughput ceiling
    C  an induced broker outage, to measure reconnect and backlog drain

The publisher is `backend.demo.SimulatedStation`, not the C++ firmware. See
`bench/harness.py` and `docs/performance.md` for what that does and does not
cover.
"""

import argparse
import csv
import json
import time
from pathlib import Path

from .broker import Broker
from .harness import (BenchStation, MiddlewareUnderTest, build_stations,
                      next_input, summarize)

RESULTS = Path(__file__).resolve().parent / "results"


def pace(target_rate, duration_s, send_one):
    """Emit at a target aggregate rate and report what was actually achieved.

    Windows sleep granularity is far coarser than the inter-event gap at high
    rates, so this paces by deficit: it sends however many events the schedule
    says are now due rather than sleeping between each one. The achieved rate is
    returned so a run that the harness itself could not keep up with is visible
    rather than being reported as a system limit.
    """
    start = time.monotonic()
    sent = 0
    while True:
        elapsed = time.monotonic() - start
        if elapsed >= duration_s:
            break
        due = int(target_rate * elapsed) - sent
        if due <= 0:
            time.sleep(0.001)
            continue
        for _ in range(due):
            send_one(sent)
            sent += 1
    elapsed = time.monotonic() - start
    return sent, sent / elapsed if elapsed else 0.0


def settle(middleware, stations, timeout=10.0):
    """Wait for in-flight commands to arrive so late samples are not lost."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        outstanding = sum(len(station.awaiting) for station in stations)
        if outstanding == 0:
            break
        time.sleep(0.1)
    time.sleep(0.5)


def scenario_a(host, port, duration_s, rate):
    """One station, steady toggles. Every event generates a command."""
    middleware = MiddlewareUnderTest(host, port)
    middleware.start()
    assert middleware.wait_connected(), "middleware did not connect"
    station = BenchStation("station_01", host, port)
    station.start()
    assert station.wait_connected(), "station did not connect"
    time.sleep(0.5)

    def send_one(index):
        station.emit("toggle_changed", 2, index % 2)

    sent, achieved = pace(rate, duration_s, send_one)
    settle(middleware, [station])

    result = {
        "scenario": "A",
        "description": f"1 station, {rate} event/s toggles, {duration_s}s",
        "stations": 1,
        "target_rate_eps": rate,
        "achieved_publish_rate_eps": round(achieved, 2),
        "events_published": station.published,
        "events_received_by_middleware": middleware.events_received,
        "commands_published": middleware.commands_published,
        "commands_received_by_station": station.commands_received,
        "publish_refusals": middleware.publish_refusals,
        "ingress_to_command": summarize(middleware.ingress_to_command_us),
        "station_roundtrip": summarize(station.roundtrip_us),
    }
    samples = [{"scenario": "A", "ingress_to_command_ms": round(v / 1000.0, 4)}
               for v in middleware.ingress_to_command_us]
    samples += [{"scenario": "A", "station_roundtrip_ms": round(v / 1000.0, 4)}
                for v in station.roundtrip_us]
    station.stop()
    middleware.stop()
    return result, samples


def scenario_b(host, port, station_count, inputs_per_station, step_s, rates):
    """Ramp offered load across many stations until something gives."""
    steps = []
    for target in rates:
        middleware = MiddlewareUnderTest(host, port)
        middleware.start()
        assert middleware.wait_connected(), "middleware did not connect"
        stations = build_stations(station_count, host, port)
        for station in stations:
            station.start()
        for station in stations:
            assert station.wait_connected(), f"{station.station_id} did not connect"
        time.sleep(0.5)

        def send_one(index):
            station = stations[index % len(stations)]
            kind, source, value, _ = next_input(index // len(stations),
                                                inputs_per_station)
            station.emit(kind, source, value)

        sent, achieved = pace(target, step_s, send_one)
        settle(middleware, stations, timeout=15.0)

        published = sum(s.published for s in stations)
        received = middleware.events_received
        steps.append({
            "scenario": "B",
            "target_rate_eps": target,
            "achieved_publish_rate_eps": round(achieved, 2),
            "stations": station_count,
            "inputs_per_station": inputs_per_station,
            "events_published": published,
            "events_received_by_middleware": received,
            "delivery_ratio": round(received / published, 4) if published else None,
            "commands_published": middleware.commands_published,
            "publish_refusals": middleware.publish_refusals,
            "station_buffer_drops": sum(s.buffer_drops for s in stations),
            "ingress_to_command": summarize(middleware.ingress_to_command_us),
            "station_roundtrip": summarize(
                [v for s in stations for v in s.roundtrip_us]),
        })
        for station in stations:
            station.stop()
        middleware.stop()
        time.sleep(1.0)

        publisher_kept_up = achieved >= 0.9 * target
        if not publisher_kept_up:
            steps[-1]["note"] = ("harness publisher could not sustain the target "
                                 "rate; this is a harness limit, not a system limit")
            break
    return steps


def scenario_c(host, port, broker, station_count, outage_s, rate):
    """Stop the broker mid-run, restart it, and watch the system recover."""
    if not broker.restartable:
        return {"scenario": "C", "skipped":
                "the broker is external, so the harness cannot cycle it"}

    middleware = MiddlewareUnderTest(host, port)
    middleware.start()
    assert middleware.wait_connected(), "middleware did not connect"
    stations = build_stations(station_count, host, port)
    for station in stations:
        station.start()
    for station in stations:
        assert station.wait_connected(), f"{station.station_id} did not connect"
    time.sleep(0.5)

    counter = [0]

    def emit_round():
        for station in stations:
            kind, source, value, _ = next_input(counter[0], 4)
            station.emit(kind, source, value)
        counter[0] += 1

    for _ in range(int(rate * 3)):
        emit_round()
        time.sleep(1.0 / rate)

    before = sum(s.published for s in stations)
    connects_before = [s.reconnects for s in stations]
    middleware_connects_before = middleware.connections

    down_perf = time.perf_counter_ns()
    broker.stop_serving()

    # Keep the station busy while it has nowhere to send, so its offline buffer
    # is exercised the way an outage on the floor would exercise it.
    outage_deadline = time.monotonic() + outage_s
    while time.monotonic() < outage_deadline:
        emit_round()
        time.sleep(1.0 / rate)
    peak_buffered = max(s.buffered for s in stations)
    drops_during_outage = sum(s.buffer_drops for s in stations)

    # Capture the moment recovery is requested, before the broker is asked to
    # come back. A client can reconnect the instant the port opens, which is
    # earlier than the harness can observe the port opening, so timing recovery
    # from "port confirmed reachable" would produce negative offsets.
    resume_requested_perf = time.perf_counter_ns()
    resume_wait = broker.resume_serving()
    measured_outage_s = (time.perf_counter_ns() - down_perf) / 1e9

    # Recovery is reconnect plus backlog drain together; the model flushes inside
    # its reconnect callback, so measuring them separately would report zero.
    recovery_deadline = time.monotonic() + 60
    recovered = False
    last_nudge = 0.0
    while time.monotonic() < recovery_deadline:
        reconnected = all(s.reconnects > connects_before[i]
                          for i, s in enumerate(stations))
        if (reconnected and middleware.connections > middleware_connects_before
                and all(s.buffered == 0 for s in stations)):
            recovered = True
            break
        # The firmware retries its backlog once per one-second loop even without
        # a fresh connection notification; nudge the model on the same cadence.
        if reconnected and time.monotonic() - last_nudge > 1.0:
            emit_round()
            last_nudge = time.monotonic()
        time.sleep(0.05)
    recovery_s = (time.perf_counter_ns() - resume_requested_perf) / 1e9

    settle(middleware, stations, timeout=15.0)

    station_reconnect_ms = sorted(
        round((s.reconnected_at_ns - resume_requested_perf) / 1e6, 1)
        for s in stations if s.reconnected_at_ns
        and s.reconnected_at_ns > resume_requested_perf)
    result = {
        "scenario": "C",
        "description": f"{station_count} station(s), {outage_s}s broker outage",
        "stations": station_count,
        "broker_backend": broker.backend,
        "events_before_outage": before,
        "outage_duration_s": round(measured_outage_s, 2),
        "broker_accepting_again_after_s": round(resume_wait, 2)
        if resume_wait is not None else None,
        "station_reconnect_after_resume_request_ms": station_reconnect_ms,
        "middleware_reconnected": middleware.connections > middleware_connects_before,
        "peak_buffered_events_per_station": peak_buffered,
        "events_dropped_by_station_buffer": drops_during_outage,
        "recovery_s_reconnect_plus_drain": round(recovery_s, 2),
        "fully_recovered": recovered,
        "events_published_total": sum(s.published for s in stations),
        "events_received_by_middleware": middleware.events_received,
        "ingress_to_command": summarize(middleware.ingress_to_command_us),
        "station_roundtrip": summarize(
            [v for s in stations for v in s.roundtrip_us]),
    }
    for station in stations:
        station.stop()
    middleware.stop()
    return result


def flatten(row):
    flat = {}
    for key, value in row.items():
        if isinstance(value, dict):
            for inner, inner_value in value.items():
                flat[f"{key}_{inner}"] = inner_value
        elif isinstance(value, list):
            flat[key] = ";".join(str(v) for v in value)
        else:
            flat[key] = value
    return flat


def write_csv(path, rows):
    if not rows:
        return
    flattened = [flatten(row) for row in rows]
    fields = []
    for row in flattened:
        for key in row:
            if key not in fields:
                fields.append(key)
    with open(path, "w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(flattened)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scenario", default="all",
                        choices=["a", "b", "c", "all"])
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=1884)
    parser.add_argument("--broker", default=None,
                        choices=["native", "docker", "external"])
    parser.add_argument("--duration", type=float, default=60.0,
                        help="scenario A duration in seconds")
    parser.add_argument("--rate", type=float, default=1.0,
                        help="scenario A event rate")
    parser.add_argument("--stations", type=int, default=20)
    parser.add_argument("--inputs", type=int, default=35)
    parser.add_argument("--step", type=float, default=8.0,
                        help="scenario B seconds per load step")
    parser.add_argument("--rates", default="50,100,250,500,1000,2000,4000")
    parser.add_argument("--outage", type=float, default=10.0)
    parser.add_argument("--out", default=str(RESULTS))
    args = parser.parse_args()

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    wanted = ["a", "b", "c"] if args.scenario == "all" else [args.scenario]

    broker = Broker(host=args.host, port=args.port, backend=args.broker)
    print(f"broker backend: {broker.backend}")
    summary, samples = [], []
    with broker:
        if "a" in wanted:
            print("scenario A ...")
            result, rows = scenario_a(args.host, args.port, args.duration, args.rate)
            summary.append(result)
            samples.extend(rows)
        if "b" in wanted:
            print("scenario B ...")
            rates = [float(r) for r in args.rates.split(",")]
            summary.extend(scenario_b(args.host, args.port, args.stations,
                                      args.inputs, args.step, rates))
        if "c" in wanted:
            print("scenario C ...")
            summary.append(scenario_c(args.host, args.port, broker,
                                      min(args.stations, 5), args.outage, 5.0))

    write_csv(out_dir / "summary.csv", summary)
    write_csv(out_dir / "scenario_a_samples.csv", samples)
    (out_dir / "summary.json").write_text(
        json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps(summary, indent=2))
    print(f"\nwrote {out_dir / 'summary.csv'}, {out_dir / 'summary.json'}")


if __name__ == "__main__":
    main()
