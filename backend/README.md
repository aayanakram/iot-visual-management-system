# Prototype MQTT Middleware

Run from the repository root so Python can resolve the `backend` package:

```sh
python -m pip install -r backend/requirements.txt
python -m backend.demo
python -m backend
```

Use a virtual environment as described in [setup instructions](../docs/setup-deployment.md). The demo runs in-process without a broker or Odoo. The last command starts the real Paho MQTT client and requires a reachable broker; defaults are `localhost:1883`.

`protocol.py` validates the firmware's seven-field event JSON, station topics, event/source/value mappings and two-field commands. `service.py` routes valid events to `OdooAdapter` and publishes generated commands. `odoo_adapter.py` supplies the interface and `MockOdooAdapter`, which stores simulated workstation state in RAM. `__main__.py` configures Paho, subscriptions and reconnect. `demo.py` provides deterministic station/offline/reverse-command simulation.

The live client subscribes to `kaizen/stations/+/events`, ignores retained events, and generates non-retained QoS 1 commands on `kaizen/stations/<device_id>/commands`. Configure `MQTT_HOST`, `MQTT_PORT`, `MQTT_CLIENT_ID`, optional `MQTT_USERNAME`/`MQTT_PASSWORD` and optional `MQTT_CA_FILE` through environment variables. `.env.example` is documentation, not an automatically loaded configuration file.

The mock maps button to pressed state, toggle to task-done and desired LED state, encoder to target count, analog to progress, and heartbeat to reported heap/uptime. A toggle generates `set_output`; `set_remote_state` demonstrates an independent ERP change with `set_state`. There is no actual Odoo connection, web application, database, persistent deduplication or durable retry queue. See [the integration contract and extension points](../docs/odoo-integration.md).

Tests, including the firmware JSON contract, run from the root:

```sh
python firmware/tests/run_host_tests.py
python -m unittest discover -s tests -v
```
