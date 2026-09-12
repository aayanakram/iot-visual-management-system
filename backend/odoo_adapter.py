"""Replace MockOdooAdapter with a real API implementation when an instance exists."""

from abc import ABC, abstractmethod
from dataclasses import dataclass

from .protocol import StationEvent, device_id, make_command


class OdooAdapter(ABC):
    @abstractmethod
    def handle_event(self, event: StationEvent,
                     received_at_ns: int | None = None) -> list[tuple[str, str]]:
        """Apply an event and return (command topic, JSON) publications.

        received_at_ns is wall-clock nanoseconds recorded at broker ingress, or
        None when a caller has no ingress time to report.

        A future real adapter should load ODOO_URL, ODOO_DATABASE, ODOO_USER
        and ODOO_API_KEY from the environment, resolve station/source mappings,
        then call the API supported by that Odoo version. No live API is used here.
        """


@dataclass
class Workstation:
    button_pressed: bool = False
    task_done: bool = False
    target_count: int = 0
    progress_percent: int = 0
    desired_output: bool = False
    free_heap_bytes: int = 0
    last_device_uptime_ms: int = 0
    fault_code: int = 0
    # Wall-clock ingress time of the most recent event, in nanoseconds. The
    # station reports uptime only, so this is the sole date-bearing field.
    received_at_ns: int = 0


class MockOdooAdapter(OdooAdapter):
    def __init__(self):
        self.workstations: dict[str, Workstation] = {}

    def handle_event(self, event, received_at_ns=None):
        state = self.workstations.setdefault(event.device_id, Workstation())
        state.last_device_uptime_ms = event.timestamp_ms
        if received_at_ns is not None:
            state.received_at_ns = received_at_ns
        kind = event.event_type
        # Inputs carry absolute values, so immediate QoS 1 duplicates are idempotent.
        if kind in ("button_pressed", "button_released"):
            state.button_pressed = bool(event.value)
        elif kind == "toggle_changed":
            state.task_done = bool(event.value)
            state.desired_output = state.task_done
            return [make_command(event.device_id, state.desired_output)]
        elif kind == "encoder_changed":
            state.target_count = event.value
        elif kind == "analog_changed":
            state.progress_percent = event.value
        elif kind == "heartbeat":
            state.free_heap_bytes = event.value
        elif kind == "fault_detected":
            state.fault_code = event.value
        elif kind == "sync_requested":
            return [make_command(event.device_id, state.desired_output, "set_state")]
        return []

    def set_remote_state(self, station, on):
        """Demonstrate a simulated ERP-side change without a station input."""
        publication = make_command(station, on, "set_state")
        self.workstations.setdefault(device_id(station), Workstation()).desired_output = on
        return publication
