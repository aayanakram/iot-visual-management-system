"""Local MQTT broker lifecycle for the benchmark.

The outage scenario has to stop and restart the broker mid-run, so the harness
owns the broker process rather than attaching to an ambient one. Three backends
are supported, tried in this order unless one is named explicitly:

    native    a mosquitto executable on PATH, run as a child process
    docker    an eclipse-mosquitto container the harness creates and removes
    external  an already-running broker; the outage scenario is skipped

Nothing here is part of the shipped station or middleware. It exists only so a
measurement run is reproducible.
"""

import shutil
import socket
import subprocess
import time
from pathlib import Path

CONTAINER = "kaizen-bench-broker"
CONFIG = "listener {port}\nallow_anonymous true\nmax_queued_messages 10000\n"

# Installers commonly leave mosquitto off PATH, so check the usual locations too.
KNOWN_PATHS = (
    r"C:\Program Files\mosquitto\mosquitto.exe",
    r"C:\Program Files (x86)\mosquitto\mosquitto.exe",
    "/usr/sbin/mosquitto",
    "/usr/local/sbin/mosquitto",
    "/opt/homebrew/sbin/mosquitto",
)


def find_mosquitto():
    found = shutil.which("mosquitto")
    if found:
        return found
    for candidate in KNOWN_PATHS:
        if Path(candidate).exists():
            return candidate
    return None


def reachable(host, port, timeout=0.5):
    try:
        with socket.create_connection((host, port), timeout=timeout):
            return True
    except OSError:
        return False


def wait_until(predicate, timeout, interval=0.2):
    """Poll predicate until true. Returns seconds waited, or None on timeout."""
    deadline = time.monotonic() + timeout
    start = time.monotonic()
    while time.monotonic() < deadline:
        if predicate():
            return time.monotonic() - start
        time.sleep(interval)
    return None


class Broker:
    """A broker the harness can stop and start again."""

    def __init__(self, host="localhost", port=1883, backend=None, work_dir=None):
        self.host = host
        self.port = port
        self.work_dir = Path(work_dir or Path(__file__).resolve().parent / "run")
        self.backend = backend or self._detect()
        self._process = None

    def _detect(self):
        if reachable(self.host, self.port):
            return "external"
        if find_mosquitto():
            return "native"
        if shutil.which("docker"):
            return "docker"
        raise RuntimeError(
            "No broker available. Install mosquitto, or start one yourself and "
            "re-run with --broker external."
        )

    @property
    def restartable(self):
        return self.backend in ("native", "docker")

    # -- lifecycle ---------------------------------------------------------

    def start(self):
        if self.backend == "external":
            if not reachable(self.host, self.port):
                raise RuntimeError(f"No broker reachable at {self.host}:{self.port}")
            return
        if self.backend == "native":
            self.work_dir.mkdir(parents=True, exist_ok=True)
            config = self.work_dir / "bench-mosquitto.conf"
            config.write_text(CONFIG.format(port=self.port), encoding="utf-8")
            self._process = subprocess.Popen(
                [find_mosquitto(), "-c", str(config)],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            subprocess.run(["docker", "rm", "-f", CONTAINER],
                           capture_output=True, timeout=60)
            inline = CONFIG.format(port=self.port).replace("\n", "\\n")
            subprocess.run(
                ["docker", "run", "-d", "--name", CONTAINER,
                 "-p", f"{self.port}:{self.port}", "eclipse-mosquitto:2",
                 "sh", "-c",
                 f"printf '{inline}' > /mosquitto/config/bench.conf && "
                 f"exec mosquitto -c /mosquitto/config/bench.conf"],
                capture_output=True, check=True, timeout=180)
        if wait_until(lambda: reachable(self.host, self.port), timeout=30) is None:
            raise RuntimeError(f"Broker did not accept connections on port {self.port}")

    def stop_serving(self):
        """Take the broker down without discarding the harness's ability to
        bring it back. Returns immediately once the port stops accepting."""
        if self.backend == "native" and self._process is not None:
            self._process.terminate()
        elif self.backend == "docker":
            subprocess.run(["docker", "stop", "-t", "0", CONTAINER],
                           capture_output=True, timeout=120)
        else:
            raise RuntimeError("An external broker cannot be cycled by the harness")
        wait_until(lambda: not reachable(self.host, self.port), timeout=30)

    def resume_serving(self):
        if self.backend == "native":
            config = self.work_dir / "bench-mosquitto.conf"
            self._process = subprocess.Popen(
                [find_mosquitto(), "-c", str(config)],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        elif self.backend == "docker":
            subprocess.run(["docker", "start", CONTAINER],
                           capture_output=True, timeout=180)
        else:
            raise RuntimeError("An external broker cannot be cycled by the harness")
        return wait_until(lambda: reachable(self.host, self.port), timeout=60)

    def shutdown(self):
        if self.backend == "native" and self._process is not None:
            self._process.terminate()
            try:
                self._process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self._process.kill()
            self._process = None
        elif self.backend == "docker":
            subprocess.run(["docker", "rm", "-f", CONTAINER],
                           capture_output=True, timeout=120)

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *_):
        self.shutdown()
        return False
