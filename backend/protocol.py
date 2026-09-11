"""Wire contract shared by middleware and the software station simulator."""

import json
import re
from dataclasses import asdict, dataclass

BASE_TOPIC = "kaizen/stations"
INPUTS = {
    "button_pressed": (1, 1, 1),
    "button_released": (1, 0, 0),
    "toggle_changed": (2, 0, 1),
    "encoder_changed": (3, -(2**31), 2**31 - 1),
    "analog_changed": (4, 0, 100),
    "heartbeat": (0, 0, 2**31 - 1),
    "fault_detected": (0, -(2**31), 2**31 - 1),
    "sync_requested": (0, -(2**31), 2**31 - 1),
}


def device_id(value):
    if not isinstance(value, str) or not re.fullmatch(r"[A-Za-z0-9_-]{1,64}", value):
        raise ValueError("device_id must contain 1-64 letters, digits, underscores or hyphens")
    return value


def event_topic(station):
    return f"{BASE_TOPIC}/{device_id(station)}/events"


def command_topic(station):
    return f"{BASE_TOPIC}/{device_id(station)}/commands"


def _unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON field: {key}")
        result[key] = value
    return result


def _bad_constant(value):
    raise ValueError(f"invalid JSON constant: {value}")


def json_object(payload, limit=4096):
    if not isinstance(payload, (str, bytes)) or len(payload) > limit:
        raise ValueError("invalid or oversized payload")
    try:
        data = json.loads(payload, object_pairs_hook=_unique_object, parse_constant=_bad_constant)
    except (UnicodeError, json.JSONDecodeError, RecursionError) as exc:
        raise ValueError("invalid JSON") from exc
    if not isinstance(data, dict):
        raise ValueError("expected a JSON object")
    return data


def integer(value, name, low, high):
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f"{name} must be an integer in [{low}, {high}]")


@dataclass(frozen=True)
class StationEvent:
    device_id: str
    sequence_id: int
    event_type: str
    source_id: int
    value: int
    timestamp_ms: int
    firmware_version: str

    def to_json(self):
        return json.dumps(asdict(self), separators=(",", ":"))


def parse_event(topic, payload):
    data = json_object(payload)
    fields = StationEvent.__dataclass_fields__
    if set(data) != set(fields):
        raise ValueError("event fields do not match the firmware schema")
    station = device_id(data["device_id"])
    if topic != event_topic(station):
        raise ValueError("event topic does not match device_id")
    kind = data["event_type"]
    if not isinstance(kind, str) or kind not in INPUTS:
        raise ValueError("unsupported event_type")
    source, low, high = INPUTS[kind]
    integer(data["sequence_id"], "sequence_id", 0, 2**32 - 1)
    integer(data["timestamp_ms"], "timestamp_ms", 0, 2**53 - 1)
    integer(data["source_id"], "source_id", source, source)
    integer(data["value"], "value", low, high)
    version = data["firmware_version"]
    if not isinstance(version, str) or not 1 <= len(version) <= 64:
        raise ValueError("invalid firmware_version")
    return StationEvent(**data)


def parse_command(payload):
    data = json_object(payload, 512)
    if set(data) != {"command", "value"}:
        raise ValueError("command requires exactly command and value")
    if data["command"] not in ("set_output", "set_state"):
        raise ValueError("unsupported command")
    value = data["value"]
    if not (type(value) is bool or type(value) in (int, float) and value in (0, 1)):
        raise ValueError("command value must be boolean or numeric 0/1")
    return data["command"], bool(value)


def make_command(station, on, command="set_output"):
    if type(on) is not bool:
        raise ValueError("output state must be boolean")
    payload = json.dumps({"command": command, "value": on}, separators=(",", ":"))
    parse_command(payload)
    return command_topic(station), payload
