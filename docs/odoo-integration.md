# Odoo Integration

## 1. Objective

The visual-management station must synchronize physical operational state with Odoo ERP.

The integration is bidirectional.

Physical station events may update Odoo objects.

Odoo changes may also generate commands or state updates that are transmitted back to physical stations.

---

## 2. Proposed Architecture

```text
ESP32-S3
    |
    v
MQTT Broker
    |
    v
Middleware / Backend
    |
    v
Odoo API
    |
    v
Odoo ERP
```

An intermediary backend is preferred rather than allowing embedded devices to directly modify the Odoo database.

---

## 3. Station-to-Odoo Flow

Example:

```text
Operator moves toggle from TO DO to DONE
                |
                v
Magnetic sensor detects position
                |
                v
ESP32 generates event
                |
                v
MQTT message published
                |
                v
Backend receives message
                |
                v
Backend maps input to Odoo object
                |
                v
Odoo API update
                |
                v
Task status becomes DONE
```

---

## 4. Odoo-to-Station Flow

Example:

```text
Task becomes overdue in Odoo
              |
              v
Backend receives/detects change
              |
              v
MQTT command published
              |
              v
ESP32 receives command
              |
              v
Remote event generated
              |
              v
State manager updated
              |
              v
Red status LED enabled
```

---

## 5. Backend Responsibilities

The middleware may be responsible for:

- subscribing to station MQTT messages
- publishing station commands
- validating messages
- identifying stations
- routing messages
- detecting duplicate events
- mapping physical controls to Odoo objects
- authenticating with Odoo
- performing API operations
- retrying failed Odoo operations
- maintaining synchronization information
- producing logs and diagnostics

---

## 6. Odoo Interface Options

Potential Odoo interfaces include:

- JSON-RPC
- XML-RPC
- REST or custom API
- custom Odoo modules

The final integration method depends on the available Odoo environment and project requirements.

---

## 7. Device Mapping

Physical interface identifiers should not be permanently hardcoded to specific Odoo database objects inside the low-level hardware driver.

Conceptually:

```text
station_07

task_03_toggle
        |
        v
Configuration Mapping
        |
        v
Odoo Task 183
        |
        v
Status Field
```

This mapping architecture allows station configuration to change without requiring modification of the low-level input firmware.

---

## 8. Example Station Event

```json
{
  "device_id": "station_07",
  "sequence_id": 1427,
  "event_type": "toggle_changed",
  "input_id": "task_03",
  "value": "done"
}
```

The backend interprets the physical interface ID and determines which Odoo object and field should be updated.

---

## 9. Reverse Mapping

Odoo data may similarly be mapped to physical outputs.

Example:

```text
Odoo Task 183
status = overdue
       |
       v
Configuration Mapping
       |
       v
station_07
task_03_led
       |
       v
LED ON
```

---

## 10. Synchronization

The integration must account for cases where local state and ERP state differ.

The synchronization design should consider:

- event timestamps
- sequence IDs
- duplicate requests
- offline events
- remote updates while the station is offline
- conflicting state changes
- retries

A final conflict-resolution policy remains to be defined.

---

## 11. Configuration Interface

A lightweight configuration interface is planned to allow a non-technical user to associate physical interfaces with specific ERP objects and fields.

The final interface may allow configuration of:

- station ID
- input ID
- interface type
- Odoo object
- Odoo record
- Odoo field
- output behaviour

The exact UI implementation remains to be determined.

---

## 12. Open Integration Items

The following information remains to be confirmed:

- Odoo version
- Odoo development/test environment
- selected API mechanism
- authentication method
- required Odoo models
- required Odoo fields
- permission to create custom Odoo modules
- final synchronization policy