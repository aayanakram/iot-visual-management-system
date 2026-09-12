# Known Limitations

This is a software prototype, not a physically commissioned or production-deployed station.

## State convergence, not an event log

Every payload this system carries is **absolute state**: a toggle position, an analog
percentage, an absolute encoder count. None of them is an increment. The goal is therefore
that the ERP ends up agreeing with the station, not that every intermediate value is
delivered.

The original offline buffer was built on the opposite assumption. It dropped the *newest*
event when full and replayed oldest-first, which is correct for an event log and wrong for
absolute state: after an outage long enough to fill 64 slots, the replay ended at the value
from the start of the outage and the ERP never learned the current one. Nothing corrected it
afterwards.

The buffer now coalesces per source, so a backlog is bounded by the number of physical
sources rather than by the length of the outage, and replay finishes at the station's current
value. The limits that remain are listed below. Deduplication, conflict resolution and
reboot durability are still absent, so this closes the staleness hole, not the wider sync
problem.

- No physical ESP32-S3, assembled station, flashing, electrical tests, Wi-Fi RF tests, hardware timing or live Odoo validation were performed.
- GPIO4/5/6/7 and LED GPIO2 are preserved prototype mappings. ADC1 channel 0 is GPIO1 on ESP32-S3. Board exposure, electrical compatibility and wiring remain unvalidated.
- There are four logical inputs and one digital LED output. Hall sensors, 30–40 interfaces, I2C expansion, PWM outputs, custom PCB and mechanical station design are conceptual future extensions.
- Inputs are polled every 10 ms plus processing time. Fast encoder edges can be missed; count is per valid quadrature edge, not necessarily per detent. Physical bounce and noise performance are unknown.
- ADC values are raw 12-bit readings scaled to percentage with a deadband. There is no voltage calibration, smoothing filter or guaranteed full-scale voltage accuracy.
- The central queue has 32 events. Full-queue submissions are logged and rejected; digital/analog values retry while pending, but encoder events and remote commands can be lost. Task stack margins and sustained load were not measured on hardware.
- The offline FIFO is capped at 64 events in volatile RAM and **does not survive reboot**. A buffered event from a physical source is replaced when a newer value arrives from that same source, so an outage of any length collapses to at most one entry per source and replay ends at the current value. Intermediate values seen during the outage are intentionally not delivered.
- Coalescing is keyed on `source_id`, so a momentary button press and its release collapse to the release if both occur while offline: the ERP then never observes that press. This is the accepted cost of treating the button as absolute state, which is how the mock adapter already maps it.
- System-source events (`fault_detected`, `sync_requested`, source 0) are discrete occurrences, so they are never coalesced. They are still dropped when the buffer is genuinely full, and that is now the only path that reaches the full-buffer rejection.
- ESP-MQTT's separate 16 KiB outbox is also volatile and uses library expiry/retransmission behavior.
- A buffered event that cannot be serialized is dropped with an error log rather than retried, so it can no longer stall every event queued behind it. The dropped event is lost.
- `backend/demo.py`'s `SimulatedStation` still models the earlier drop-new buffering policy and does not coalesce. It is a Python model used by the demo and benchmark publisher, not the firmware, and its buffering behavior no longer matches `OfflineEventStore`.
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
