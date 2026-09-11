# Known Limitations

This is a software prototype, not a physically commissioned or production-deployed station.

- No physical ESP32-S3, assembled station, flashing, electrical tests, Wi-Fi RF tests, hardware timing or live Odoo validation were performed.
- GPIO4/5/6/7 and LED GPIO2 are preserved prototype mappings. ADC1 channel 0 is GPIO1 on ESP32-S3. Board exposure, electrical compatibility and wiring remain unvalidated.
- There are four logical inputs and one digital LED output. Hall sensors, 30–40 interfaces, I2C expansion, PWM outputs, custom PCB and mechanical station design are conceptual future extensions.
- Inputs are polled every 10 ms plus processing time. Fast encoder edges can be missed; count is per valid quadrature edge, not necessarily per detent. Physical bounce and noise performance are unknown.
- ADC values are raw 12-bit readings scaled to percentage with a deadband. There is no voltage calibration, smoothing filter or guaranteed full-scale voltage accuracy.
- The central queue has 32 events. Full-queue submissions are logged and rejected; digital/analog values retry while pending, but encoder events and remote commands can be lost. Task stack margins and sustained load were not measured on hardware.
- The offline FIFO is capped at 64 events in volatile RAM, drops new events when full, and **does not survive reboot**. ESP-MQTT's separate 16 KiB outbox is also volatile and uses library expiry/retransmission behavior.
- MQTT enqueue acceptance does not prove delivery to a broker, backend or Odoo. There is no end-to-end acknowledgement, persistent session, durable backend retry, durable command queue or offline backend detection when the broker stays connected.
- Sequence IDs reset on boot and wrap at uint32. Uptime is not wall-clock time. There is no boot ID or durable deduplication; stale replays and concurrent ERP/station changes have no reconciliation policy.
- Heartbeats report free heap, uptime and firmware identity through the ordinary event schema. They are discarded when they cannot be sent. There is no full health subsystem, task watchdog integration or backend freshness alarm.
- Command handling supports one boolean output via `set_output` or `set_state`. It rejects retained, fragmented and oversized commands. There is no command acknowledgement or physical output-readback verification; StateManager records requested output state.
- Backend storage is an in-memory mock; it has no real Odoo client, database, mapping UI, Odoo webhook or polling mechanism. Workstation records are not bounded by a configured station registry; the prototype assumes a trusted small demo deployment.
- Firmware network settings are placeholders. An ignored local header supports development values; production provisioning, broker ACLs, TLS device credential installation and secret lifecycle management are not implemented.
- There is no OTA, PKI, NVS-backed event persistence, production configuration service, environmental rating, EMC certification or multi-station deployment validation.
- Host tests exercise selected actual C++ logic with test-only shims. They do not emulate FreeRTOS scheduling, GPIO electronics, ADC hardware, ESP-MQTT transport or RF recovery. The default demo is in-process and does not contact an MQTT broker.
- The validated firmware image has roughly 10% free space in its 1 MiB application partition; larger future features may require a revised partition plan.

Sensible next work is physical ESP32-S3 validation, a real Odoo test deployment, NVS-backed buffering if reboot durability is required, TLS credential provisioning and multi-station/reconnect/load testing. Power architecture, enclosure design, sensor selection and final wiring should be resolved during physical development.
