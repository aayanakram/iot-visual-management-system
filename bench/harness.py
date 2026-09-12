"""Measurement infrastructure: simulated station publishers and the real middleware.

What is real here: the MQTT transport (a local mosquitto), the paho client
configuration from `backend.__main__.create_client`, `backend.service.Middleware`,
`backend.service.bind_callbacks`, `backend.protocol` validation and
`backend.odoo_adapter.MockOdooAdapter`.

What is not real: the publisher. It is `backend.demo.SimulatedStation` driving a
paho client, **not** the C++ firmware. No ESP32 executes, so nothing here
measures device-side polling, debounce, serialization or the firmware's offline
buffer. These numbers characterize broker plus middleware only.

Both the publishers and the middleware run inside this one process, so every
measurement comes from a single clock and no cross-host skew is involved.

Latency is measured with `time.perf_counter_ns`, not `time.time_ns`. On the
Windows host used for the recorded run, `time.time_ns` advances in steps of
about 1 ms (`GetSystemTimeAsFileTime`), which is coarser than the quantity being
measured and would report most samples as zero. `perf_counter_ns`
(`QueryPerformanceCounter`) advances in steps of about 200 ns. The wall-clock
`received_at_ns` the middleware stamps on each Workstation is still
`time.time_ns`, because that field records *when* an event arrived rather than a
duration, and 1 ms is ample for a date.
"""

import math
import os
import threading
import time
from collections import deque

import paho.mqtt.client as mqtt

from backend.demo import SimulatedStation
from backend.odoo_adapter import MockOdooAdapter
from backend.protocol import command_topic, event_topic
from backend.service import Middleware, bind_callbacks

# The wire contract pins source_id to event_type, so a station can address only
# these four logical sources regardless of how many physical controls it carries.
INPUT_KINDS = (
    ("button_pressed", 1),
    ("toggle_changed", 2),
    ("encoder_changed", 3),
    ("analog_changed", 4),
)


def percentile(values_sorted, fraction):
    """Nearest-rank percentile. Avoids interpolating between samples that were
    never observed, which matters for small latency sets."""
    if not values_sorted:
        return None
    rank = max(1, math.ceil(fraction * len(values_sorted)))
    return values_sorted[rank - 1]


def summarize(samples):
    if not samples:
        return {"count": 0, "p50_ms": None, "p95_ms": None,
                "p99_ms": None, "max_ms": None, "mean_ms": None}
    ordered = sorted(samples)
    return {
        "count": len(ordered),
        "p50_ms": round(percentile(ordered, 0.50) / 1000.0, 3),
        "p95_ms": round(percentile(ordered, 0.95) / 1000.0, 3),
        "p99_ms": round(percentile(ordered, 0.99) / 1000.0, 3),
        "max_ms": round(ordered[-1] / 1000.0, 3),
        "mean_ms": round(sum(ordered) / len(ordered) / 1000.0, 3),
    }


class MiddlewareUnderTest:
    """The shipped middleware, with a timing wrapper around its publish hook.

    `Middleware` already takes its transport as an injected callable, so timing
    the generated command needs no change to shipped code.
    """

    def __init__(self, host, port, client_id="bench-backend"):
        self.host = host
        self.port = port
        self.adapter = MockOdooAdapter()
        self.ingress_to_command_us = []
        self.events_received = 0
        self.commands_published = 0
        self.publish_refusals = 0
        self.connections = 0
        self.first_connect_ns = None
        self.last_connect_ns = None
        # Set at ingress and read during the publish that the same on_message
        # call triggers. paho drives one network thread per client, so this is
        # never interleaved between two messages.
        self._ingress_perf_ns = None
        self._lock = threading.Lock()

        os.environ["MQTT_CLIENT_ID"] = client_id
        from backend.__main__ import create_client
        self.client = create_client()
        self.client.reconnect_delay_set(min_delay=1, max_delay=5)

        self.middleware = Middleware(self.adapter, self._timed_publish)
        bind_callbacks(self.client, self.middleware)
        self._wrap_callbacks()

    def _wrap_callbacks(self):
        inner_connect = self.client.on_connect
        inner_message = self.client.on_message

        def on_connect(client, userdata, flags, reason_code, properties):
            inner_connect(client, userdata, flags, reason_code, properties)
            if not reason_code.is_failure:
                with self._lock:
                    self.connections += 1
                    self.last_connect_ns = time.time_ns()
                    if self.first_connect_ns is None:
                        self.first_connect_ns = self.last_connect_ns

        def on_message(client, userdata, message):
            self._ingress_perf_ns = time.perf_counter_ns()
            with self._lock:
                self.events_received += 1
            inner_message(client, userdata, message)

        self.client.on_connect = on_connect
        self.client.on_message = on_message

    def _timed_publish(self, topic, payload):
        """Publish a generated command and record ingress-to-command latency.

        Wall-clock ingress is recorded by the shipped middleware on the
        Workstation; the interval itself is taken from the monotonic clock
        captured when this same message entered on_message.
        """
        ingress_perf_ns = self._ingress_perf_ns
        result = self.client.publish(topic, payload, qos=1, retain=False)
        accepted = result.rc == mqtt.MQTT_ERR_SUCCESS
        with self._lock:
            if not accepted:
                self.publish_refusals += 1
            else:
                if ingress_perf_ns is not None:
                    self.ingress_to_command_us.append(
                        (time.perf_counter_ns() - ingress_perf_ns) / 1000.0)
                self.commands_published += 1
        return accepted

    def start(self):
        self.client.connect_async(self.host, self.port, keepalive=60)
        self.client.loop_start()

    def stop(self):
        self.client.loop_stop()
        try:
            self.client.disconnect()
        except Exception:
            pass

    def wait_connected(self, timeout=15):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.client.is_connected():
                return True
            time.sleep(0.05)
        return False


class BenchStation:
    """One simulated station: `SimulatedStation` plus a real MQTT client.

    The model's `deliver` hook is pointed at a real publish, and its `online`
    flag is driven by the client's actual connection state, so its offline
    buffering runs during the outage scenario.
    """

    def __init__(self, station_id, host, port, capacity=64):
        self.station_id = station_id
        self.host = host
        self.port = port
        self.model = SimulatedStation(station_id, capacity=capacity)
        self.model.deliver = self._deliver

        self.roundtrip_us = []
        self.awaiting = deque()
        self.published = 0
        self.publish_failures = 0
        self.buffer_drops = 0
        self.commands_received = 0
        self.reconnects = 0
        self.reconnected_at_ns = None
        self._lock = threading.RLock()

        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,
                                  client_id=f"bench-{station_id}")
        self.client.reconnect_delay_set(min_delay=1, max_delay=5)
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_command

    # -- MQTT callbacks ----------------------------------------------------

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            return
        client.subscribe(command_topic(self.station_id), qos=1)
        with self._lock:
            self.reconnects += 1
            self.reconnected_at_ns = time.perf_counter_ns()
            # Mirrors the firmware flushing its offline buffer on MqttConnected.
            self.model.reconnect()

    def _on_disconnect(self, client, userdata, flags, reason_code, properties):
        with self._lock:
            self.model.online = False

    def _on_command(self, client, userdata, message):
        received_ns = time.perf_counter_ns()
        with self._lock:
            self.commands_received += 1
            # Commands carry no correlation ID, so pair them with emitted
            # toggles in order. QoS 1 on one topic from one broker preserves
            # per-station ordering, which is what makes this valid.
            if self.awaiting:
                _, published_ns = self.awaiting.popleft()
                self.roundtrip_us.append((received_ns - published_ns) / 1000.0)

    # -- publishing --------------------------------------------------------

    def _deliver(self, event):
        if not self.client.is_connected():
            return False
        published_ns = time.perf_counter_ns()
        result = self.client.publish(event_topic(self.station_id),
                                     event.to_json(), qos=1, retain=False)
        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            with self._lock:
                self.publish_failures += 1
            return False
        with self._lock:
            self.published += 1
            if event.event_type == "toggle_changed":
                self.awaiting.append((event.sequence_id, published_ns))
        return True

    def emit(self, kind, source, value):
        with self._lock:
            stored = self.model.emit(kind, source, value)
            if not stored:
                self.buffer_drops += 1
            return stored

    @property
    def buffered(self):
        with self._lock:
            return len(self.model.pending)

    # -- lifecycle ---------------------------------------------------------

    def start(self):
        self.client.connect_async(self.host, self.port, keepalive=60)
        self.client.loop_start()

    def stop(self):
        self.client.loop_stop()
        try:
            self.client.disconnect()
        except Exception:
            pass

    def wait_connected(self, timeout=15):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.client.is_connected():
                return True
            time.sleep(0.05)
        return False


def build_stations(count, host, port, prefix="station"):
    return [BenchStation(f"{prefix}_{index + 1:02d}", host, port)
            for index in range(count)]


def next_input(step, inputs_per_station):
    """Map a step index onto one of the four addressable sources.

    A station with 35 physical controls still has only these four source IDs
    available on the wire, so a larger input count changes the message rate but
    not the addressing.
    """
    kind, source = INPUT_KINDS[step % len(INPUT_KINDS)]
    if kind == "button_pressed":
        value = 1
    elif kind == "toggle_changed":
        value = step % 2
    elif kind == "encoder_changed":
        value = step % 1000
    else:
        value = (step * 7) % 101
    slot = step % max(1, inputs_per_station)
    return kind, source, value, slot
