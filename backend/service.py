"""Small MQTT callback layer; transport is injected for software tests."""

import logging
import time

from .protocol import BASE_TOPIC, parse_event

LOG = logging.getLogger(__name__)


class Middleware:
    def __init__(self, adapter, publish):
        self.adapter = adapter
        self.publish = publish

    def handle_message(self, topic, payload, received_at_ns=None):
        """Apply one station event.

        received_at_ns is wall-clock time at broker ingress. The station's own
        timestamp_ms is monotonic uptime, which orders events within one boot of
        one device but cannot be compared across devices or turned into a date,
        so the first wall-clock reading in the system is taken here.
        """
        if received_at_ns is None:
            received_at_ns = time.time_ns()
        event = parse_event(topic, payload)
        publications = self.adapter.handle_event(event, received_at_ns)
        for command_topic, command in publications:
            if not self.publish(command_topic, command):
                raise RuntimeError("MQTT did not accept the generated command")
        LOG.info("Station %s: %s=%s seq=%s", event.device_id,
                 event.event_type, event.value, event.sequence_id)
        return publications


def bind_callbacks(client, middleware):
    def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            LOG.error("MQTT connection rejected: %s", reason_code)
            return
        result, _ = client.subscribe(f"{BASE_TOPIC}/+/events", qos=1)
        if result != 0:
            LOG.error("MQTT event subscription request failed: %s", result)
        else:
            LOG.info("MQTT connected; event subscription requested")

    def on_message(client, userdata, message):
        if message.retain:
            LOG.warning("Ignoring retained event on %s", message.topic)
            return
        received_at_ns = time.time_ns()
        try:
            middleware.handle_message(message.topic, message.payload, received_at_ns)
        except ValueError as exc:
            LOG.warning("Rejected event: %s", exc)
        except Exception:
            LOG.exception("Event processing failed; no durable backend retry is implemented")

    def on_disconnect(client, userdata, flags, reason_code, properties):
        LOG.info("MQTT disconnected: %s", reason_code)

    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect
