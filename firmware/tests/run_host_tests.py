"""Compile actual firmware logic with small host-only ESP/FreeRTOS shims."""
import os
from pathlib import Path
import shutil
import subprocess

firmware = Path(__file__).resolve().parents[1]
compiler = shutil.which(os.getenv("CXX", "g++"))
if not compiler:
    raise SystemExit("Install a host C++ compiler or set CXX to its executable path")
cjson = firmware / "managed_components/espressif__cjson/cJSON"
if not (cjson / "cJSON.c").exists():
    raise SystemExit("Run idf.py build first to download the locked cJSON dependency")
build = firmware / "build-host"
build.mkdir(exist_ok=True)
includes = [firmware / "tests/host/stubs", cjson]
includes += [firmware / "main" / name for name in
             ("app", "events", "state", "outputs", "drivers", "storage", "network", "config")]
sources = [firmware / "tests/host/test_core.cpp", cjson / "cJSON.c"]
sources += [firmware / "main" / name for name in
            ("network/MessageSerializer.cpp", "storage/OfflineEventStore.cpp",
             "state/StateManager.cpp", "outputs/OutputManager.cpp", "drivers/LedDriver.cpp",
             "events/EventQueue.cpp", "app/Application.cpp", "drivers/DigitalInput.cpp",
             "drivers/RotaryEncoder.cpp")]
exe = build / ("host_tests.exe" if os.name == "nt" else "host_tests")
subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-pedantic",
                *["-I" + str(p) for p in includes], *map(str, sources), "-o", str(exe)], check=True)
subprocess.run([str(exe)], check=True)
