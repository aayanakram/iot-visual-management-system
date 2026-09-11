"""Small MQTT callback layer; transport is injected for software tests."""

import logging

from .protocol import BASE_TOPIC, parse_event

LOG = logging.getLogger(__name__)


class Middleware:
    def __init__(self, adapter, publish):
        self.adapter = adapter
        self.publish = publish

    def handle_message(self, topic, payload):
        event = parse_event(topic, payload)
        publications = self.adapter.handle_event(event)
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
        try:
            middleware.handle_message(message.topic, message.payload)
        except ValueError as exc:
            LOG.warning("Rejected event: %s", exc)
        except Exception:
            LOG.exception("Event processing failed; no durable backend retry is implemented")

    def on_disconnect(client, userdata, flags, reason_code, properties):
        LOG.info("MQTT disconnected: %s", reason_code)

    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect
