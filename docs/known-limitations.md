# Known Limitations and Open Items

This document tracks requirements and implementation areas that remain unconfirmed, incomplete, or outside the current prototype scope.

The contents will be updated throughout development.

---

## 1. Physical Interface Configuration

The exact quantity of each physical interface type per station has not yet been finalized.

The overall target is approximately 30–40 interfaces per board, but the final distribution between:

- toggles
- buttons
- rotary controls
- analog controls
- indicators

remains to be confirmed.

---

## 2. Sensor Selection

Magnetic sensing is currently planned for some of the oversized mechanical controls.

The exact sensor model and electrical interface have not yet been finalized.

---

## 3. I/O Expansion

The final requirement for GPIO expansion, multiplexing, or additional ADC resources depends on the completed interface allocation.

This will be determined after the exact quantity and type of physical modules are known.

---

## 4. Power Architecture

The final station power source and voltage architecture have not yet been confirmed.

The prototype power design will be finalized after the available installation power source is known.

---

## 5. Environmental Rating

The system is expected to operate in environments containing dust and occasional water splashes.

A formal IP rating has not yet been specified.

---

## 6. Production Hardware

The current embedded architecture uses the ESP32-S3.

Whether the final commercial version requires a custom PCB rather than development modules remains to be determined.

---

## 7. Offline Storage Capacity

The maximum expected duration of network loss has not yet been defined.

The required offline event-buffer capacity therefore remains TBD.

---

## 8. Conflict Resolution

A final policy has not yet been selected for cases where local station state and Odoo state are modified independently during a period of disconnection.

Possible strategies will be evaluated during synchronization implementation.

---

## 9. MQTT Infrastructure

The final production MQTT broker has not yet been selected.

The following remain to be finalized:

- broker platform
- authentication
- TLS certificate provisioning
- production topic permissions
- MQTT QoS settings

---

## 10. Odoo Integration

The final Odoo interface has not yet been selected.

Potential methods include:

- JSON-RPC
- XML-RPC
- REST/custom API
- custom Odoo module

The exact Odoo models and fields that will be controlled by the physical stations also remain to be confirmed.

---

## 11. Configuration Interface

The architecture includes a user-facing configuration system, but the final implementation has not yet been selected.

The first implementation may use a lightweight local or web-based interface.

---

## 12. OTA Updates

OTA firmware updates are included in the planned architecture.

Full production-grade OTA functionality, including rollback protection and secure firmware signing, may be outside the available time for the initial prototype.

---

## 13. Security Provisioning

TLS is planned for production MQTT communication.

The final secure provisioning method for:

- Wi-Fi credentials
- MQTT credentials
- certificates
- API credentials

has not yet been finalized.

---

## 14. Hardware Testing

Environmental, EMC, long-duration reliability, and formal certification testing are outside the current prototype scope unless additional requirements are provided.

The prototype will instead focus on functional hardware validation and representative fault testing.

---

## 15. Performance Data

Measured values for:

- network latency
- synchronization latency
- reconnect time
- event-loss rate
- long-duration uptime

are not yet available.

These will be added after an operational end-to-end prototype is completed.