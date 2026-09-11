"""Run with python -m backend from the repository root."""

import logging
import os

import paho.mqtt.client as mqtt

from .odoo_adapter import MockOdooAdapter
from .service import Middleware, bind_callbacks


def create_client():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,
                         client_id=os.getenv("MQTT_CLIENT_ID", "kaizen-demo-backend"))
    username = os.getenv("MQTT_USERNAME")
    if username:
        client.username_pw_set(username, os.getenv("MQTT_PASSWORD"))
    ca_file = os.getenv("MQTT_CA_FILE")
    if ca_file:
        client.tls_set(ca_certs=ca_file)
    client.reconnect_delay_set(min_delay=1, max_delay=30)
    client.max_queued_messages_set(64)
    client.enable_logger()
    return client


def main():
    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(message)s")
    client = create_client()

    def publish(topic, payload):
        result = client.publish(topic, payload, qos=1, retain=False)
        return result.rc == mqtt.MQTT_ERR_SUCCESS

    middleware = Middleware(MockOdooAdapter(), publish)
    bind_callbacks(client, middleware)
    port = int(os.getenv("MQTT_PORT", "1883"))
    if not 1 <= port <= 65535:
        raise ValueError("MQTT_PORT must be in 1..65535")
    client.connect_async(os.getenv("MQTT_HOST", "localhost"), port, keepalive=60)
    try:
        client.loop_forever(retry_first_connection=True)
    except KeyboardInterrupt:
        pass
    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
