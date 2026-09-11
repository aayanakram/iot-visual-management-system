"""Deterministic in-process simulation, with no MQTT sockets or physical hardware."""

import json
from collections import deque
from dataclasses import asdict

from .odoo_adapter import MockOdooAdapter
from .protocol import StationEvent, command_topic, event_topic, parse_command
from .service import Middleware


class SimulatedStation:
    def __init__(self, station="station_01", capacity=64):
        self.station = station
        self.capacity = capacity
        self.pending = deque()
        self.online = False
        self.sequence = 1
        self.output = False
        self.local_values = {}
        self.deliver = None

    def emit(self, kind, source, value):
        self.local_values[source] = value
        event = StationEvent(self.station, self.sequence, kind, source, value,
                             self.sequence * 100, "0.1.0")
        self.sequence += 1
        self.flush()
        if self.online and not self.pending and self.deliver(event):
            return True
        if kind == "heartbeat":
            return False
        if len(self.pending) >= self.capacity:
            return False
        self.pending.append(event)
        return True

    def flush(self):
        while self.online and self.pending:
            if not self.deliver(self.pending[0]):
                break
            self.pending.popleft()

    def reconnect(self):
        self.online = True
        self.flush()

    def receive(self, topic, payload):
        if not self.online or topic != command_topic(self.station):
            return False
        _, self.output = parse_command(payload)
        return True


def run_demo():
    station = SimulatedStation()
    adapter = MockOdooAdapter()
    middleware = Middleware(adapter, station.receive)
    observed = []

    def deliver(event):
        middleware.handle_message(event_topic(station.station), event.to_json())
        observed.append(event.sequence_id)
        return True

    station.deliver = deliver
    assert station.emit("toggle_changed", 2, 1)
    assert station.emit("analog_changed", 4, 75)
    assert len(station.pending) == 2 and not adapter.workstations
    station.reconnect()
    assert observed == [1, 2] and station.output and not station.pending
    assert adapter.workstations[station.station].progress_percent == 75
    assert station.receive(*adapter.set_remote_state(station.station, False))
    assert not station.output
    result = {"simulation": "in-process; no broker, ESP32 or live Odoo",
              "replayed_sequences": observed, "station_output": station.output,
              "mock_workstation": asdict(adapter.workstations[station.station])}
    print(json.dumps(result, indent=2))
    return result


if __name__ == "__main__":
    run_demo()
