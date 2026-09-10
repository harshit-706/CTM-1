"""
CTM-1 Virtual Plant Simulator
==============================
Models correlated physical state for industrial assets.
Exposes a control/query HTTP API on port 8082.
Pushes telemetry to the C++ Edge Core on port 8080.
"""

import time
import json
import math
import random
import copy
import threading
import os
from http.server import BaseHTTPRequestHandler, HTTPServer
import urllib.request
import urllib.error

EDGE_URL = os.environ.get("EDGE_HTTP_URL", "http://localhost:8080")
TICK_RATE = int(os.environ.get("SIMULATOR_TICK_RATE_HZ", "10"))
TICK_INTERVAL = 1.0 / TICK_RATE

# ──────────────────────────────────────────────────────────
# Global state  (guarded by `assets_lock` for thread-safety)
# ──────────────────────────────────────────────────────────
assets: dict = {}
assets_lock = threading.Lock()
edge_online = False
event_log: list = []          # last 200 events for the UI
MAX_EVENT_LOG = 200

# ──────────────────────────────────────────────────────────
# Scenarios
# ──────────────────────────────────────────────────────────
AVAILABLE_SCENARIOS = [
    {"id": "NORMAL",            "label": "Normal Operation",     "description": "All assets run at nominal parameters."},
    {"id": "HIGH_LOAD",         "label": "High Load",            "description": "Increase load on all mechanical assets to 90%."},
    {"id": "VIBRATION_DRIFT",   "label": "Vibration Drift",      "description": "Gradually increase vibration on target asset."},
    {"id": "PRESSURE_DROP",     "label": "Pressure Drop",        "description": "Simulate downstream pressure loss."},
    {"id": "ENERGY_WASTE",      "label": "Energy Waste",         "description": "Inject efficiency degradation on target."},
    {"id": "OVERCURRENT",       "label": "Overcurrent Trip",     "description": "VFD overcurrent fault causing machine stop."},
    {"id": "VFD_TRIP",          "label": "VFD Trip",             "description": "Variable-frequency drive trips offline."},
    {"id": "COMMUNICATION_LOSS","label": "Communication Loss",   "description": "Stop telemetry emission from target."},
    {"id": "SENSOR_FAILURE",    "label": "Sensor Failure",       "description": "Mark sensor quality as BAD."},
]

active_scenarios: dict = {}   # scenario_id -> {target, start_time}


def log_event(msg: str):
    entry = {"ts": time.strftime("%H:%M:%S"), "msg": msg}
    event_log.append(entry)
    if len(event_log) > MAX_EVENT_LOG:
        event_log.pop(0)
    print(f"[{entry['ts']}] {msg}", flush=True)


# ──────────────────────────────────────────────────────────
# Telemetry sender
# ──────────────────────────────────────────────────────────
def send_telemetry(payload: dict):
    global edge_online
    try:
        req = urllib.request.Request(
            f"{EDGE_URL}/api/v1/telemetry",
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"},
        )
        urllib.request.urlopen(req, timeout=0.5)
        if not edge_online:
            log_event("Edge Core connection restored.")
            edge_online = True
    except urllib.error.URLError:
        if edge_online:
            log_event("EDGE OFFLINE: Telemetry delivery failed.")
            edge_online = False
    except Exception as e:
        if edge_online:
            log_event(f"EDGE OFFLINE: {e}")
            edge_online = False


# ──────────────────────────────────────────────────────────
# Asset models
# ──────────────────────────────────────────────────────────

class BaseAsset:
    """Common behaviour shared by every virtual asset."""

    TYPE = "generic"
    INPUTS: dict = {}
    OUTPUTS: list = []

    def __init__(self, asset_id: str, label: str = ""):
        self.asset_id = asset_id
        self.label = label or asset_id
        self.running = True
        self.faults: set = set()
        self.tags: dict = {"status": 1.0}
        self.last_update = 0.0

    # ── control helpers ──
    def start(self):
        self.running = True
        self.faults.discard("overcurrent")
        self.faults.discard("vfd_trip")
        log_event(f"{self.asset_id} STARTED")

    def stop(self):
        self.running = False
        log_event(f"{self.asset_id} STOPPED")

    def inject_fault(self, fault: str):
        self.faults.add(fault)
        if fault in ("overcurrent", "vfd_trip"):
            self.running = False
        log_event(f"{self.asset_id} FAULT INJECTED: {fault}")

    def clear_fault(self, fault: str):
        self.faults.discard(fault)
        log_event(f"{self.asset_id} FAULT CLEARED: {fault}")

    def reset(self):
        self.faults.clear()
        self.running = True
        log_event(f"{self.asset_id} RESET")

    # ── serialisation ──
    def get_state(self) -> dict:
        return {
            "asset_id": self.asset_id,
            "label": self.label,
            "type": self.TYPE,
            "running": self.running,
            "faults": list(self.faults),
            "tags": copy.copy(self.tags),
            "last_update": self.last_update,
        }

    def get_identity(self) -> dict:
        return {
            "id": self.asset_id,
            "label": self.label,
            "type": self.TYPE,
            "running": self.running,
            "faults": list(self.faults),
            "source": "SIMULATED",
            "inputs": self.INPUTS,
            "outputs": self.OUTPUTS,
        }

    # ── tick (subclasses override) ──
    def tick(self):
        if "comm_loss" in self.faults:
            return  # no telemetry
        self.tags["status"] = 1.0 if self.running else 0.0
        self.tags["fault_active"] = 1.0 if len(self.faults) > 0 else 0.0
        
        payload = {
            "asset_id": self.asset_id,
            "timestamp_ns": int(time.time() * 1e9),
            "source": "SIMULATED",
            "tags": self.tags,
        }
        send_telemetry(payload)
        self.last_update = time.time()


class PumpAsset(BaseAsset):
    TYPE = "pump"
    INPUTS = {"speed_hz": {"min": 0, "max": 60, "step": 1, "unit": "Hz", "default": 50}}
    OUTPUTS = ["speed_hz", "current_a", "pressure_bar", "flow", "vibration", "power_kw", "temperature_c"]

    def __init__(self, asset_id, label=""):
        super().__init__(asset_id, label)
        self.speed_hz = 50.0

    def set_speed(self, speed):
        self.speed_hz = max(0.0, min(60.0, speed))
        self.running = self.speed_hz > 0
        self.faults.discard("overcurrent")
        log_event(f"{self.asset_id} speed -> {self.speed_hz:.1f} Hz")

    def reset(self):
        super().reset()
        self.speed_hz = 50.0

    def tick(self):
        if "comm_loss" in self.faults:
            return
        if not self.running:
            self.tags = {"status": 0.0, "speed_hz": 0, "current_a": 0, "pressure_bar": 0,
                         "flow": 0, "vibration": 0, "power_kw": 0, "temperature_c": 25.0}
        else:
            ratio = self.speed_hz / 50.0
            load_factor = 1.4 if "energy_waste" in self.faults else 1.0
            vibration_base = 2.5 if "vibration_drift" in self.faults else 1.5

            pressure  = ratio * 5.0  + random.gauss(0, 0.05)
            flow      = ratio * 100.0 + random.gauss(0, 1.0)
            current   = ratio * 15.0 * load_factor + random.gauss(0, 0.3)
            power_kw  = (current * 400 * 1.732 * 0.85) / 1000.0
            vibration = vibration_base + random.gauss(0, 0.08)
            temperature = 25.0 + ratio * 22.0 * load_factor + random.gauss(0, 0.3)

            self.tags = {
                "status": 1.0,
                "speed_hz": round(self.speed_hz, 1),
                "current_a": round(current, 2),
                "pressure_bar": round(pressure, 2),
                "flow": round(flow, 1),
                "vibration": round(vibration, 2),
                "power_kw": round(power_kw, 2),
                "temperature_c": round(temperature, 1),
            }

        payload = {
            "asset_id": self.asset_id,
            "timestamp_ns": int(time.time() * 1e9),
            "source": "SIMULATED",
            "tags": self.tags,
        }
        send_telemetry(payload)
        self.last_update = time.time()

    def get_state(self):
        s = super().get_state()
        s["speed_hz"] = self.speed_hz
        return s


class MotorAsset(BaseAsset):
    TYPE = "motor"
    INPUTS = {"frequency_hz": {"min": 0, "max": 60, "step": 1, "unit": "Hz", "default": 50},
              "load_percent": {"min": 0, "max": 100, "step": 5, "unit": "%", "default": 70}}
    OUTPUTS = ["frequency_hz", "load_percent", "current_a", "power_kw", "temperature_c", "vibration_mm_s"]

    def __init__(self, asset_id, label=""):
        super().__init__(asset_id, label)
        self.frequency_hz = 50.0
        self.load_percent = 70.0

    def set_speed(self, speed):
        self.frequency_hz = max(0.0, min(60.0, speed))
        self.running = self.frequency_hz > 0
        log_event(f"{self.asset_id} frequency -> {self.frequency_hz:.1f} Hz")

    def set_load(self, load):
        self.load_percent = max(0.0, min(100.0, load))
        log_event(f"{self.asset_id} load -> {self.load_percent:.0f}%")

    def reset(self):
        super().reset()
        self.frequency_hz = 50.0
        self.load_percent = 70.0

    def tick(self):
        if "comm_loss" in self.faults:
            return
        if not self.running:
            self.tags = {"status": 0.0, "frequency_hz": 0, "load_percent": 0,
                         "current_a": 0, "power_kw": 0, "temperature_c": 25.0, "vibration_mm_s": 0}
        else:
            ratio = self.frequency_hz / 50.0
            load_f = self.load_percent / 100.0
            eff = 1.4 if "energy_waste" in self.faults else 1.0
            vib_base = 3.0 if "vibration_drift" in self.faults else 1.8

            current  = ratio * load_f * 22.0 * eff + random.gauss(0, 0.4)
            power_kw = (current * 400 * 1.732 * 0.85) / 1000.0
            temp     = 30.0 + ratio * load_f * 35.0 * eff + random.gauss(0, 0.4)
            vib      = vib_base * ratio * (0.5 + 0.5 * load_f) + random.gauss(0, 0.05)

            self.tags = {
                "status": 1.0,
                "frequency_hz": round(self.frequency_hz, 1),
                "load_percent": round(self.load_percent, 1),
                "current_a": round(current, 2),
                "power_kw": round(power_kw, 2),
                "temperature_c": round(temp, 1),
                "vibration_mm_s": round(vib, 2),
            }

        payload = {
            "asset_id": self.asset_id,
            "timestamp_ns": int(time.time() * 1e9),
            "source": "SIMULATED",
            "tags": self.tags,
        }
        send_telemetry(payload)
        self.last_update = time.time()


class VFDAsset(BaseAsset):
    TYPE = "vfd"
    INPUTS = {"output_freq_hz": {"min": 0, "max": 60, "step": 1, "unit": "Hz", "default": 50}}
    OUTPUTS = ["output_freq_hz", "output_voltage_v", "output_current_a", "dc_bus_voltage_v", "power_kw", "temperature_c"]

    def __init__(self, asset_id, label=""):
        super().__init__(asset_id, label)
        self.output_freq_hz = 50.0

    def set_speed(self, speed):
        self.output_freq_hz = max(0.0, min(60.0, speed))
        self.running = self.output_freq_hz > 0
        log_event(f"{self.asset_id} VFD freq -> {self.output_freq_hz:.1f} Hz")

    def reset(self):
        super().reset()
        self.output_freq_hz = 50.0

    def tick(self):
        if "comm_loss" in self.faults:
            return
        if not self.running:
            self.tags = {"status": 0.0, "output_freq_hz": 0, "output_voltage_v": 0,
                         "output_current_a": 0, "dc_bus_voltage_v": 0, "power_kw": 0, "temperature_c": 30.0}
        else:
            ratio = self.output_freq_hz / 50.0
            eff = 1.3 if "energy_waste" in self.faults else 1.0
            voltage  = ratio * 380.0 + random.gauss(0, 1.0)
            current  = ratio * 18.0 * eff + random.gauss(0, 0.3)
            dc_bus   = 540.0 + random.gauss(0, 2.0)
            power_kw = (voltage * current * 1.732 * 0.92) / 1000.0
            temp     = 35.0 + ratio * 15.0 * eff + random.gauss(0, 0.5)

            self.tags = {
                "status": 1.0,
                "output_freq_hz": round(self.output_freq_hz, 1),
                "output_voltage_v": round(voltage, 1),
                "output_current_a": round(current, 2),
                "dc_bus_voltage_v": round(dc_bus, 1),
                "power_kw": round(power_kw, 2),
                "temperature_c": round(temp, 1),
            }

        payload = {
            "asset_id": self.asset_id,
            "timestamp_ns": int(time.time() * 1e9),
            "source": "SIMULATED",
            "tags": self.tags,
        }
        send_telemetry(payload)
        self.last_update = time.time()


class CompressorAsset(BaseAsset):
    TYPE = "compressor"
    INPUTS = {"speed_hz": {"min": 0, "max": 60, "step": 1, "unit": "Hz", "default": 50}}
    OUTPUTS = ["speed_hz", "discharge_pressure_bar", "suction_pressure_bar", "current_a",
               "power_kw", "temperature_c", "vibration_mm_s"]

    def __init__(self, asset_id, label=""):
        super().__init__(asset_id, label)
        self.speed_hz = 50.0

    def set_speed(self, speed):
        self.speed_hz = max(0.0, min(60.0, speed))
        self.running = self.speed_hz > 0
        log_event(f"{self.asset_id} compressor speed -> {self.speed_hz:.1f} Hz")

    def reset(self):
        super().reset()
        self.speed_hz = 50.0

    def tick(self):
        if "comm_loss" in self.faults:
            return
        if not self.running:
            self.tags = {"status": 0.0, "speed_hz": 0, "discharge_pressure_bar": 0,
                         "suction_pressure_bar": 0, "current_a": 0, "power_kw": 0,
                         "temperature_c": 25.0, "vibration_mm_s": 0}
        else:
            ratio = self.speed_hz / 50.0
            eff = 1.35 if "energy_waste" in self.faults else 1.0
            vib_base = 3.2 if "vibration_drift" in self.faults else 2.0

            discharge = ratio * 8.0 + random.gauss(0, 0.1)
            suction   = 1.0 + random.gauss(0, 0.05)
            current   = ratio * 25.0 * eff + random.gauss(0, 0.5)
            power_kw  = (current * 400 * 1.732 * 0.88) / 1000.0
            temp      = 40.0 + ratio * 30.0 * eff + random.gauss(0, 0.5)
            vib       = vib_base * ratio + random.gauss(0, 0.08)

            self.tags = {
                "status": 1.0,
                "speed_hz": round(self.speed_hz, 1),
                "discharge_pressure_bar": round(discharge, 2),
                "suction_pressure_bar": round(suction, 2),
                "current_a": round(current, 2),
                "power_kw": round(power_kw, 2),
                "temperature_c": round(temp, 1),
                "vibration_mm_s": round(vib, 2),
            }

        payload = {
            "asset_id": self.asset_id,
            "timestamp_ns": int(time.time() * 1e9),
            "source": "SIMULATED",
            "tags": self.tags,
        }
        send_telemetry(payload)
        self.last_update = time.time()


class FanAsset(BaseAsset):
    TYPE = "fan"
    INPUTS = {"speed_rpm": {"min": 0, "max": 3600, "step": 50, "unit": "RPM", "default": 1800}}
    OUTPUTS = ["speed_rpm", "airflow_cfm", "current_a", "power_kw", "vibration_mm_s"]

    def __init__(self, asset_id, label=""):
        super().__init__(asset_id, label)
        self.speed_rpm = 1800.0

    def set_speed(self, speed):
        self.speed_rpm = max(0.0, min(3600.0, speed))
        self.running = self.speed_rpm > 0
        log_event(f"{self.asset_id} fan speed -> {self.speed_rpm:.0f} RPM")

    def reset(self):
        super().reset()
        self.speed_rpm = 1800.0

    def tick(self):
        if "comm_loss" in self.faults:
            return
        if not self.running:
            self.tags = {"status": 0.0, "speed_rpm": 0, "airflow_cfm": 0,
                         "current_a": 0, "power_kw": 0, "vibration_mm_s": 0}
        else:
            ratio = self.speed_rpm / 1800.0
            airflow = ratio * 5000.0 + random.gauss(0, 20)
            current = ratio * 8.0 + random.gauss(0, 0.2)
            power_kw = (current * 400 * 1.732 * 0.8) / 1000.0
            vib = (1.2 if "vibration_drift" not in self.faults else 2.8) * ratio + random.gauss(0, 0.04)

            self.tags = {
                "status": 1.0,
                "speed_rpm": round(self.speed_rpm, 0),
                "airflow_cfm": round(airflow, 0),
                "current_a": round(current, 2),
                "power_kw": round(power_kw, 2),
                "vibration_mm_s": round(vib, 2),
            }

        payload = {
            "asset_id": self.asset_id,
            "timestamp_ns": int(time.time() * 1e9),
            "source": "SIMULATED",
            "tags": self.tags,
        }
        send_telemetry(payload)
        self.last_update = time.time()


class SensorAsset(BaseAsset):
    """Generic sensor – reads a parent-like value but doesn't actuate."""
    TYPE = "sensor"
    INPUTS = {}
    OUTPUTS = ["value", "quality"]

    def __init__(self, asset_id, label="", sensor_type="temp_sensor"):
        super().__init__(asset_id, label)
        self.sensor_type = sensor_type
        self.TYPE = sensor_type  # preserve the original type from topology

    def tick(self):
        if "comm_loss" in self.faults:
            return
        quality = "BAD" if "sensor_failure" in self.faults else "GOOD"
        # Generate plausible sensor reading
        val = 25.0 + random.gauss(0, 0.5)
        self.tags = {"status": 1.0, "value": round(val, 2), "quality": quality}

        payload = {
            "asset_id": self.asset_id,
            "timestamp_ns": int(time.time() * 1e9),
            "source": "SIMULATED",
            "tags": self.tags,
        }
        send_telemetry(payload)
        self.last_update = time.time()

    def get_identity(self):
        d = super().get_identity()
        d["type"] = self.sensor_type
        return d


class EnergyMeterAsset(BaseAsset):
    TYPE = "energy_meter"
    INPUTS = {}
    OUTPUTS = ["voltage_v", "current_a", "power_kw", "energy_kwh", "power_factor", "frequency_hz"]

    def __init__(self, asset_id, label=""):
        super().__init__(asset_id, label)
        self.energy_kwh = 0.0

    def tick(self):
        if "comm_loss" in self.faults:
            return
        voltage = 400.0 + random.gauss(0, 2.0)
        current = 45.0 + random.gauss(0, 1.5)
        pf = 0.88 + random.gauss(0, 0.02)
        power_kw = (voltage * current * 1.732 * pf) / 1000.0
        self.energy_kwh += power_kw * TICK_INTERVAL / 3600.0
        freq = 50.0 + random.gauss(0, 0.05)

        self.tags = {
            "status": 1.0,
            "voltage_v": round(voltage, 1),
            "current_a": round(current, 2),
            "power_kw": round(power_kw, 2),
            "energy_kwh": round(self.energy_kwh, 3),
            "power_factor": round(pf, 3),
            "frequency_hz": round(freq, 2),
        }

        payload = {
            "asset_id": self.asset_id,
            "timestamp_ns": int(time.time() * 1e9),
            "source": "SIMULATED",
            "tags": self.tags,
        }
        send_telemetry(payload)
        self.last_update = time.time()


# ──────────────────────────────────────────────────────────
# Asset factory
# ──────────────────────────────────────────────────────────
SENSOR_TYPES = {"temp_sensor", "pressure_sensor", "flow_sensor", "vibration_sensor"}

def create_asset(asset_id: str, asset_type: str, label: str = "") -> BaseAsset:
    t = asset_type.lower()
    if t == "pump":
        return PumpAsset(asset_id, label)
    elif t == "motor":
        return MotorAsset(asset_id, label)
    elif t in ("vfd", "soft_starter"):
        return VFDAsset(asset_id, label)
    elif t == "compressor":
        return CompressorAsset(asset_id, label)
    elif t == "fan":
        return FanAsset(asset_id, label)
    elif t == "energy_meter":
        return EnergyMeterAsset(asset_id, label)
    elif t in SENSOR_TYPES:
        return SensorAsset(asset_id, label, sensor_type=t)
    else:
        a = BaseAsset(asset_id, label)
        a.TYPE = t
        return a


# ──────────────────────────────────────────────────────────
# HTTP API
# ──────────────────────────────────────────────────────────

class SimControlHandler(BaseHTTPRequestHandler):

    def _send_json(self, status, data):
        body = json.dumps(data, default=str).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Connection", "close")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def log_message(self, format, *args):
        pass  # suppress default stderr logging

    # ── GET ──

    def do_GET(self):
        if self.path == "/health":
            self._send_json(200, {"status": "ok", "edge_online": edge_online,
                                   "asset_count": len(assets), "tick_rate_hz": TICK_RATE})

        elif self.path == "/api/v1/assets":
            with assets_lock:
                result = [a.get_identity() for a in assets.values()]
            self._send_json(200, result)

        elif self.path.startswith("/api/v1/assets/"):
            aid = self.path.split("/api/v1/assets/")[1]
            with assets_lock:
                asset = assets.get(aid)
            if asset:
                self._send_json(200, asset.get_state())
            else:
                self._send_json(404, {"error": f"Asset {aid} not found"})

        elif self.path == "/api/v1/state":
            with assets_lock:
                result = {k: v.get_state() for k, v in assets.items()}
            self._send_json(200, result)

        elif self.path == "/api/v1/scenarios":
            self._send_json(200, {"available": AVAILABLE_SCENARIOS,
                                   "active": {k: {"target": v["target"],
                                                   "elapsed_s": round(time.time() - v["start_time"], 1)}
                                               for k, v in active_scenarios.items()}})

        elif self.path == "/api/v1/events":
            self._send_json(200, event_log[-50:])

        else:
            self._send_json(404, {"error": "Not found"})

    # ── POST ──

    def do_POST(self):
        content_length = int(self.headers.get("Content-Length", 0))
        post_data = self.rfile.read(content_length) if content_length else b""
        try:
            payload = json.loads(post_data.decode("utf-8")) if post_data else {}
        except json.JSONDecodeError:
            self._send_json(400, {"error": "Invalid JSON"})
            return

        # ── topology push from Edge ──
        if self.path == "/api/v1/topology":
            with assets_lock:
                assets.clear()
                for node in payload.get("nodes", []):
                    aid = node.get("id")
                    ntype = node.get("type", "generic")
                    nlabel = node.get("label", aid)
                    if aid:
                        assets[aid] = create_asset(aid, ntype, nlabel)
            log_event(f"Topology applied. Total assets: {len(assets)}")
            self._send_json(200, {"status": "topology_applied", "asset_count": len(assets)})

        # ── control commands ──
        elif self.path in ("/api/v1/control", "/control"):
            cmd = payload.get("command")
            aid = payload.get("asset_id")

            with assets_lock:
                asset = assets.get(aid)

            if not asset:
                self._send_json(404, {"error": f"Asset {aid} not found"})
                return

            if cmd == "start":
                asset.start()
            elif cmd == "stop":
                asset.stop()
            elif cmd == "set_speed":
                speed = payload.get("speed", payload.get("value", 50))
                if hasattr(asset, "set_speed"):
                    asset.set_speed(float(speed))
            elif cmd == "set_load":
                if hasattr(asset, "set_load"):
                    asset.set_load(float(payload.get("value", 70)))
            elif cmd == "inject_fault":
                asset.inject_fault(payload.get("fault", "generic"))
            elif cmd == "clear_fault":
                asset.clear_fault(payload.get("fault", "generic"))
            elif cmd == "reset":
                asset.reset()
            else:
                self._send_json(400, {"error": f"Unknown command: {cmd}"})
                return

            self._send_json(200, {"status": "ok", "asset": asset.get_state()})

        # ── scenario start ──
        elif self.path.startswith("/api/v1/scenarios/") and self.path.endswith("/start"):
            parts = self.path.split("/")
            scenario_id = parts[4]
            target = payload.get("target")

            if scenario_id == "NORMAL":
                with assets_lock:
                    for a in assets.values():
                        a.reset()
                active_scenarios.clear()
                log_event("Scenario NORMAL activated: all assets reset.")
            elif target:
                with assets_lock:
                    asset = assets.get(target)
                if asset:
                    _apply_scenario(scenario_id, asset)
                    active_scenarios[scenario_id] = {"target": target, "start_time": time.time()}
                else:
                    self._send_json(404, {"error": f"Target asset {target} not found"})
                    return
            else:
                self._send_json(400, {"error": "Missing 'target' asset_id"})
                return

            self._send_json(200, {"status": "scenario_started", "scenario": scenario_id})

        # ── scenario stop ──
        elif self.path.startswith("/api/v1/scenarios/") and self.path.endswith("/stop"):
            parts = self.path.split("/")
            scenario_id = parts[4]
            info = active_scenarios.pop(scenario_id, None)
            if info:
                with assets_lock:
                    asset = assets.get(info["target"])
                    if asset:
                        _clear_scenario(scenario_id, asset)
            self._send_json(200, {"status": "scenario_stopped", "scenario": scenario_id})

        # ── fault injection ──
        elif self.path == "/api/v1/faults":
            aid = payload.get("asset_id")
            fault = payload.get("fault")
            with assets_lock:
                asset = assets.get(aid)
            if asset and fault:
                asset.inject_fault(fault)
                self._send_json(200, {"status": "fault_injected"})
            else:
                self._send_json(400, {"error": "Missing asset_id or fault"})

        # ── global reset ──
        elif self.path == "/api/v1/reset":
            with assets_lock:
                for a in assets.values():
                    a.reset()
            active_scenarios.clear()
            log_event("GLOBAL RESET performed.")
            self._send_json(200, {"status": "reset"})

        else:
            self._send_json(404, {"error": "Not found"})


def _apply_scenario(scenario_id: str, asset: BaseAsset):
    mapping = {
        "HIGH_LOAD":         lambda a: setattr(a, "load_percent", 95) if hasattr(a, "load_percent") else None,
        "VIBRATION_DRIFT":   lambda a: a.inject_fault("vibration_drift"),
        "PRESSURE_DROP":     lambda a: a.inject_fault("pressure_drop"),
        "ENERGY_WASTE":      lambda a: a.inject_fault("energy_waste"),
        "OVERCURRENT":       lambda a: a.inject_fault("overcurrent"),
        "VFD_TRIP":          lambda a: a.inject_fault("vfd_trip"),
        "COMMUNICATION_LOSS":lambda a: a.inject_fault("comm_loss"),
        "SENSOR_FAILURE":    lambda a: a.inject_fault("sensor_failure"),
    }
    fn = mapping.get(scenario_id)
    if fn:
        fn(asset)
        log_event(f"Scenario {scenario_id} applied to {asset.asset_id}")


def _clear_scenario(scenario_id: str, asset: BaseAsset):
    fault_map = {
        "VIBRATION_DRIFT": "vibration_drift",
        "PRESSURE_DROP": "pressure_drop",
        "ENERGY_WASTE": "energy_waste",
        "OVERCURRENT": "overcurrent",
        "VFD_TRIP": "vfd_trip",
        "COMMUNICATION_LOSS": "comm_loss",
        "SENSOR_FAILURE": "sensor_failure",
    }
    fault = fault_map.get(scenario_id)
    if fault:
        asset.clear_fault(fault)
    if scenario_id == "HIGH_LOAD" and hasattr(asset, "load_percent"):
        asset.load_percent = 70.0
    log_event(f"Scenario {scenario_id} cleared from {asset.asset_id}")


# ──────────────────────────────────────────────────────────
# Server & main loop
# ──────────────────────────────────────────────────────────

def run_http_server():
    server = HTTPServer(("127.0.0.1", 8082), SimControlHandler)
    log_event(f"Simulator HTTP API on http://127.0.0.1:8082")
    server.serve_forever()


def main():
    log_event(f"Starting CTM-1 Virtual Plant Simulator at {TICK_RATE} Hz")

    # Seed with two default pump assets
    with assets_lock:
        assets["P-104"] = PumpAsset("P-104", "Centrifugal Pump P-104")
        assets["P-105"] = PumpAsset("P-105", "Centrifugal Pump P-105")

    threading.Thread(target=run_http_server, daemon=True).start()

    while True:
        start_time = time.time()
        with assets_lock:
            snapshot = list(assets.values())
        for asset in snapshot:
            asset.tick()
        elapsed = time.time() - start_time
        sleep_time = max(0, TICK_INTERVAL - elapsed)
        time.sleep(sleep_time)


if __name__ == "__main__":
    main()
