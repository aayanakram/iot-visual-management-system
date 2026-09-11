import json
import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch

from backend.demo import SimulatedStation, run_demo
from backend.odoo_adapter import MockOdooAdapter
from backend.protocol import (StationEvent, command_topic, event_topic,
                              make_command, parse_command, parse_event)
from backend.service import Middleware, bind_callbacks


def event(**changes):
    data = dict(device_id="station_01", sequence_id=1, event_type="toggle_changed",
                source_id=2, value=1, timestamp_ms=100, firmware_version="0.1.0")
    data.update(changes)
    return StationEvent(**data)


class ProtocolTests(unittest.TestCase):
    def test_round_trip(self):
        original = event()
        self.assertEqual(parse_event(event_topic(original.device_id), original.to_json()), original)

    def test_all_event_types(self):
        for kind, source, value in [("button_pressed", 1, 1), ("button_released", 1, 0),
                                    ("toggle_changed", 2, 0), ("encoder_changed", 3, -12),
                                    ("analog_changed", 4, 100), ("heartbeat", 0, 100000),
                                    ("fault_detected", 0, -1), ("sync_requested", 0, 0)]:
            with self.subTest(kind=kind):
                original = event(event_type=kind, source_id=source, value=value)
                self.assertEqual(parse_event(event_topic("station_01"), original.to_json()), original)

    def test_invalid_events(self):
        for changes in [dict(value=True), dict(value=2), dict(source_id=3),
                        dict(timestamp_ms=-1), dict(sequence_id=2**32),
                        dict(event_type=[]), dict(event_type="set_output"),
                        dict(device_id="../+"), dict(firmware_version=""), dict(value=1.5)]:
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                parse_event(event_topic("station_01"), event(**changes).to_json())

    def test_missing_extra_malformed_and_duplicate_fields(self):
        data = json.loads(event().to_json())
        del data["timestamp_ms"]
        for payload in [json.dumps(data), event().to_json()[:-1] + ',"extra":0}',
                        event().to_json()[:-1] + ',"value":0}', 'null', '[]', '{',
                        '{"value":NaN}', b'\xff', 'x' * 4097]:
            with self.subTest(payload=payload), self.assertRaises(ValueError):
                parse_event(event_topic("station_01"), payload)

    def test_topic_routing(self):
        self.assertEqual(command_topic("station_02"), "kaizen/stations/station_02/commands")
        with self.assertRaises(ValueError):
            parse_event(event_topic("station_02"), event().to_json())
        for station in ["", "+", "a/b", "a#", "a" * 65]:
            with self.assertRaises(ValueError):
                event_topic(station)

    def test_commands(self):
        for kind in ["set_output", "set_state"]:
            for value in [True, False, 0, 1, 1.0]:
                self.assertEqual(parse_command(json.dumps(dict(command=kind, value=value))),
                                 (kind, bool(value)))
        self.assertEqual(make_command("station_01", True),
                         (command_topic("station_01"), '{"command":"set_output","value":true}'))

    def test_invalid_commands(self):
        for payload in ['{}', '[]', '{', '{"command":"reboot","value":1}',
                        '{"command":"set_output","value":2}',
                        '{"command":"set_output","value":"true"}',
                        '{"command":"set_output","value":null}',
                        '{"command":"set_output","value":true}garbage',
                        '{"command":"set_output","value":true}\u0000',
                        '{"command":"set_output","value":true,"value":false}',
                        '{"command":"set_output","value":NaN}', ' ' * 513]:
            with self.subTest(payload=payload), self.assertRaises(ValueError):
                parse_command(payload)


class AdapterTests(unittest.TestCase):
    def test_mapping_and_remote_state(self):
        adapter = MockOdooAdapter()
        cases = [("button_pressed", 1, 1, "button_pressed", True),
                 ("button_released", 1, 0, "button_pressed", False),
                 ("encoder_changed", 3, -4, "target_count", -4),
                 ("analog_changed", 4, 42, "progress_percent", 42),
                 ("heartbeat", 0, 120000, "free_heap_bytes", 120000),
                 ("fault_detected", 0, 9, "fault_code", 9)]
        for kind, source, value, field, expected in cases:
            adapter.handle_event(event(event_type=kind, source_id=source, value=value))
            self.assertEqual(getattr(adapter.workstations["station_01"], field), expected)
        first = adapter.handle_event(event())
        self.assertEqual(first, adapter.handle_event(event()))
        self.assertTrue(adapter.workstations["station_01"].task_done)
        self.assertEqual(parse_command(adapter.set_remote_state("station_01", False)[1]),
                         ("set_state", False))
        self.assertEqual(parse_command(adapter.handle_event(event(event_type="sync_requested",
                          source_id=0, value=0))[0][1]), ("set_state", False))

    def test_station_isolation(self):
        adapter = MockOdooAdapter()
        adapter.handle_event(event())
        adapter.handle_event(event(device_id="station_02", value=0))
        self.assertTrue(adapter.workstations["station_01"].task_done)
        self.assertFalse(adapter.workstations["station_02"].task_done)

    def test_publish_failure_visible(self):
        service = Middleware(MockOdooAdapter(), lambda *_: False)
        with self.assertRaises(RuntimeError):
            service.handle_message(event_topic("station_01"), event().to_json())


class SimulationTests(unittest.TestCase):
    def test_bidirectional_demo(self):
        with patch("builtins.print"):
            result = run_demo()
        self.assertEqual(result["replayed_sequences"], [1, 2])
        self.assertFalse(result["station_output"])

    def test_overflow_and_reboot(self):
        station = SimulatedStation(capacity=2)
        self.assertTrue(station.emit("toggle_changed", 2, 1))
        self.assertTrue(station.emit("analog_changed", 4, 50))
        self.assertFalse(station.emit("encoder_changed", 3, 4))
        self.assertEqual(station.local_values[3], 4)
        self.assertEqual([e.sequence_id for e in station.pending], [1, 2])
        self.assertFalse(station.emit("heartbeat", 0, 120000))
        self.assertEqual(len(SimulatedStation().pending), 0)

    def test_failed_flush_preserves_order_and_retries_without_reconnect(self):
        station = SimulatedStation()
        station.emit("toggle_changed", 2, 1)
        station.deliver = lambda _: False
        station.reconnect()
        station.emit("analog_changed", 4, 10)
        self.assertEqual([e.sequence_id for e in station.pending], [1, 2])
        received = []
        station.deliver = lambda e: received.append(e.sequence_id) or True
        station.flush()
        self.assertEqual(received, [1, 2])
        self.assertFalse(station.pending)

    def test_wrong_topic_does_not_change_output(self):
        station = SimulatedStation()
        station.online = True
        self.assertFalse(station.receive(*make_command("station_02", True)))
        self.assertFalse(station.output)


class MqttCallbackTests(unittest.TestCase):
    def test_resubscribe_on_each_connection(self):
        client = Mock()
        client.subscribe.return_value = (0, 1)
        bind_callbacks(client, Mock())
        for _ in range(2):
            client.on_connect(client, None, None, SimpleNamespace(is_failure=False), None)
        self.assertEqual(client.subscribe.call_count, 2)
        client.subscribe.assert_called_with("kaizen/stations/+/events", qos=1)

    def test_callback_rejects_retained_and_bad_events_then_recovers(self):
        client = Mock()
        publish = Mock(return_value=True)
        adapter = MockOdooAdapter()
        bind_callbacks(client, Middleware(adapter, publish))
        message = SimpleNamespace(topic=event_topic("station_01"), payload=event().to_json(), retain=True)
        with self.assertLogs("backend.service", level="WARNING"):
            client.on_message(client, None, message)
        self.assertFalse(adapter.workstations)
        message.retain = False
        message.payload = "{"
        with self.assertLogs("backend.service", level="WARNING"):
            client.on_message(client, None, message)
        message.payload = event().to_json()
        client.on_message(client, None, message)
        publish.assert_called_once()

    def test_declared_paho_dependency_and_client_configuration(self):
        from backend.__main__ import create_client
        with patch.dict("os.environ", {}, clear=True):
            client = create_client()
        self.assertEqual(client._max_queued_messages, 64)
        self.assertEqual(client._reconnect_min_delay, 1)
        self.assertEqual(client._reconnect_max_delay, 30)


if __name__ == "__main__":
    unittest.main()
