"""Cross-language tests against the compiled firmware serializer/parser."""
import json
import os
from pathlib import Path
import subprocess
import unittest

from backend.odoo_adapter import MockOdooAdapter
from backend.protocol import event_topic, make_command, parse_command, parse_event


class FirmwareContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        firmware = Path(os.getenv("FIRMWARE_DIR", Path(__file__).resolve().parents[1] / "firmware"))
        cls.exe = firmware / "build-host" / ("host_tests.exe" if os.name == "nt" else "host_tests")
        if not cls.exe.exists():
            raise RuntimeError("Run python firmware/tests/run_host_tests.py before the full test suite")

    def test_cpp_event_to_backend_to_cpp_command(self):
        payload = subprocess.check_output([str(self.exe), "--event"], text=True)
        event = parse_event(event_topic("station_01"), payload)
        adapter = MockOdooAdapter()
        topic, command = adapter.handle_event(event)[0]
        result = subprocess.check_output([str(self.exe), "--command", command], text=True)
        self.assertEqual(result.strip(), "1")
        self.assertTrue(adapter.workstations["station_01"].task_done)
        self.assertTrue(topic.endswith("/station_01/commands"))

    def test_python_generated_commands_accepted_by_cpp(self):
        for kind in ("set_output", "set_state"):
            for on in (True, False):
                _, payload = make_command("station_01", on, kind)
                actual = subprocess.check_output([str(self.exe), "--command", payload], text=True)
                self.assertEqual(actual.strip(), str(int(on)))

    def test_shared_rejection_corpus(self):
        payloads = ['{}', '[]', '{', '{"command":"unknown","value":1}',
                    '{"command":"set_output","value":2}',
                    '{"command":"set_output","value":true}junk',
                    '{"command":"set_output","value":true,"value":false}',
                    '{"command":"set_output\\u0000junk","value":true}']
        for value in (None, "true", [], {}, 0.5, -1):
            payloads.append(json.dumps(dict(command="set_state", value=value)))
        for payload in payloads:
            with self.subTest(payload=payload):
                with self.assertRaises(ValueError):
                    parse_command(payload)
                result = subprocess.run([str(self.exe), "--command", payload], capture_output=True)
                self.assertEqual(result.returncode, 2)
