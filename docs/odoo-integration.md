# Odoo Integration

The delivered integration uses `backend.odoo_adapter.OdooAdapter`, an abstract interface with `handle_event(StationEvent) -> list[(topic, JSON)]`. `MockOdooAdapter` implements it with an in-memory dictionary of `Workstation` objects keyed by device ID. No Odoo server, database, credentials or live API calls are involved.

```text
Station JSON -> protocol validation -> Middleware -> MockOdooAdapter
                                                      |
                                  workstation fields / generated command
                                                      |
                      station command topic <- Middleware publisher
```

| Station input | Simulated workstation field | Demonstrated behavior |
|---|---|---|
| Source 1: button pressed/released | `button_pressed` | Assign boolean operator input state |
| Source 2: toggle | `task_done`, `desired_output` | Assign task status; generate `set_output` matching done state |
| Source 3: encoder | `target_count` | Assign absolute count; does not increment on duplicate reception |
| Source 4: analog | `progress_percent` | Assign progress 0–100 |
| Heartbeat | `free_heap_bytes`, `last_device_uptime_ms` | Store reported health observation |
| Fault | `fault_code` | Store a simulated fault code |
| Sync request | Existing `desired_output` | Generate `set_state`; no full reconciliation protocol |

All handled events update `last_device_uptime_ms`, which is the event's generation uptime rather than server time. `set_remote_state(device_id, on)` demonstrates a simulated ERP-side change and returns a `set_state` publication. `python -m backend.demo` demonstrates both directions, including buffered replay, without requiring a broker.

Mock mappings are intentionally small and readable; they are not real Odoo model names or record identifiers. Two immediate deliveries of an absolute input value leave the same state. There is no persistent deduplication or conflict resolution, and all workstation data disappears when the Python process restarts. If publication fails, the service reports the failure; it does not roll back mock state or provide a durable retry transaction.

## Future real adapter

Implement `OdooAdapter.handle_event` in a separate class and select it where `MockOdooAdapter` is constructed in `backend/__main__.py`. Keep ERP-specific logic out of firmware drivers. That adapter should:

1. Read `ODOO_URL`, `ODOO_DATABASE`, `ODOO_USER` and `ODOO_API_KEY` from environment/secret configuration. These are suggested extension variables, not currently consumed settings.
2. Define an explicit `(device_id, source_id)` to Odoo model/record/field mapping for the agreed workflow.
3. Choose the API supported by the actual deployed Odoo version, such as its JSON-RPC/XML-RPC/custom API, and test authentication and permissions there.
4. Apply absolute state changes with an agreed retry, duplicate and conflict policy.
5. Return validated commands, and add polling/webhook handling for independent ERP changes when required.

There is no implemented real adapter, Odoo polling, webhook endpoint, mapping UI or API credential provisioning. No live Odoo integration was validated. A future real adapter requires its own integration tests against a test instance.
