#!/usr/bin/env python3
"""
@file       verdict_checker.py
@brief      SIL verdict checker — executes scenario YAML files against the
            Docker-based SIL platform and evaluates pass/fail verdicts
@aspice     SWE.5 — SW Component Verification & Integration
@iso        ISO 26262 Part 6, Section 10 — Software Integration & Testing
@date       2026-02-24

This module implements a TUV-grade SIL test verdict checker for the
taktflow-embedded platform.  It reads scenario definitions from YAML files,
orchestrates fault injection via the REST API, monitors CAN bus and MQTT
telemetry for expected responses, and produces JUnit XML reports with
full ISO 26262 traceability.

Observation channels:
    1. CAN bus (python-can on vcan0) — primary real-time observation
    2. MQTT (paho-mqtt)              — secondary telemetry observation
    3. Fault Injection REST API      — scenario control

Vehicle states (from plant_sim/simulator.py):
    0=INIT, 1=RUN, 2=DEGRADED, 3=LIMP, 4=SAFE_STOP, 5=SHUTDOWN
"""

from __future__ import annotations

import argparse
import json
import logging
import os
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, field
from enum import IntEnum
from pathlib import Path
from typing import Any, Optional

import can
import paho.mqtt.client as paho_mqtt
import requests
import yaml
from junit_xml import TestCase, TestSuite

# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [VERDICT] %(levelname)-5s %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("verdict_checker")

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

# ANSI color codes for terminal output
RED = "\033[0;31m"
GREEN = "\033[0;32m"
YELLOW = "\033[1;33m"
CYAN = "\033[0;36m"
BOLD = "\033[1m"
NC = "\033[0m"  # No Color


class VehicleState(IntEnum):
    """Vehicle state codes — must match plant_sim/simulator.py."""
    INIT = 0
    RUN = 1
    DEGRADED = 2
    LIMP = 3
    SAFE_STOP = 4
    SHUTDOWN = 5


# Human-readable name mapping
VEHICLE_STATE_NAMES: dict[str, int] = {
    "INIT": VehicleState.INIT,
    "RUN": VehicleState.RUN,
    "DEGRADED": VehicleState.DEGRADED,
    "LIMP": VehicleState.LIMP,
    "SAFE_STOP": VehicleState.SAFE_STOP,
    "SHUTDOWN": VehicleState.SHUTDOWN,
}

# CAN IDs for monitoring (from plant_sim/simulator.py)
CAN_VEHICLE_STATE = 0x100
CAN_STEERING_STATUS = 0x200
CAN_BRAKE_STATUS = 0x201
CAN_MOTOR_STATUS = 0x300
CAN_MOTOR_CURRENT = 0x301
CAN_MOTOR_TEMP = 0x302
CAN_BATTERY_STATUS = 0x303
CAN_DTC_BROADCAST = 0x500

# Default CAN channel
DEFAULT_CAN_CHANNEL = "vcan0"

# Default timeouts
DEFAULT_SCENARIO_TIMEOUT_SEC = 60
DEFAULT_STATE_WAIT_TIMEOUT_SEC = 10

# Motor RPM byte positions in Motor_Status (0x300)
MOTOR_RPM_BYTE_LO = 2
MOTOR_RPM_BYTE_HI = 3


# ---------------------------------------------------------------------------
# Data classes
# ---------------------------------------------------------------------------

@dataclass
class VerdictEvidence:
    """Evidence collected during verdict evaluation."""
    description: str
    expected: str
    observed: str
    passed: bool
    timestamp: float = 0.0
    details: str = ""


@dataclass
class ScenarioResult:
    """Result of a single scenario execution."""
    scenario_id: str
    scenario_name: str
    description: str
    verifies: list[str]
    aspice: str
    passed: bool
    duration_sec: float
    verdicts: list[VerdictEvidence] = field(default_factory=list)
    error: Optional[str] = None


# ---------------------------------------------------------------------------
# CAN Bus Monitor
# ---------------------------------------------------------------------------

class CANBusMonitor:
    """Threaded CAN bus monitor that captures messages and tracks state.

    Runs a background listener on vcan0 and maintains:
      - Current vehicle state (from 0x100)
      - Latest CAN messages by arbitration ID
      - History of state transitions with timestamps
      - Motor RPM tracking
    """

    def __init__(self, channel: str = DEFAULT_CAN_CHANNEL) -> None:
        self.channel = channel
        self._bus: Optional[can.Bus] = None
        self._thread: Optional[threading.Thread] = None
        self._running = False
        self._lock = threading.Lock()

        # Monitored state
        self._vehicle_state: int = VehicleState.INIT
        self._state_transitions: list[tuple[float, int]] = []
        self._latest_messages: dict[int, tuple[float, can.Message]] = {}
        self._message_history: dict[int, list[tuple[float, can.Message]]] = {}
        self._motor_rpm: int = 0
        self._can_ids_seen: set[int] = set()

    def start(self) -> None:
        """Start the CAN bus listener thread."""
        if self._running:
            return
        try:
            self._bus = can.interface.Bus(
                channel=self.channel,
                interface="socketcan",
            )
        except Exception as exc:
            log.error("Failed to open CAN bus '%s': %s", self.channel, exc)
            raise
        self._running = True
        self._thread = threading.Thread(
            target=self._listener_loop,
            daemon=True,
            name="can-monitor",
        )
        self._thread.start()
        log.info("CAN bus monitor started on %s", self.channel)

    def stop(self) -> None:
        """Stop the CAN bus listener thread."""
        self._running = False
        if self._thread is not None:
            self._thread.join(timeout=3.0)
        if self._bus is not None:
            self._bus.shutdown()
            self._bus = None
        log.info("CAN bus monitor stopped")

    def reset(self) -> None:
        """Clear all captured state for a new scenario."""
        with self._lock:
            self._state_transitions.clear()
            self._latest_messages.clear()
            self._message_history.clear()
            self._motor_rpm = 0
            self._can_ids_seen.clear()
            self._vehicle_state = VehicleState.INIT

    @property
    def vehicle_state(self) -> int:
        """Current vehicle state (thread-safe read)."""
        with self._lock:
            return self._vehicle_state

    @property
    def motor_rpm(self) -> int:
        """Current motor RPM (thread-safe read)."""
        with self._lock:
            return self._motor_rpm

    @property
    def state_transitions(self) -> list[tuple[float, int]]:
        """Copy of state transition history [(timestamp, state), ...]."""
        with self._lock:
            return list(self._state_transitions)

    def get_latest_message(self, can_id: int) -> Optional[tuple[float, can.Message]]:
        """Get the most recent message for a CAN ID, or None."""
        with self._lock:
            return self._latest_messages.get(can_id)

    def get_message_history(self, can_id: int) -> list[tuple[float, can.Message]]:
        """Get all captured messages for a CAN ID."""
        with self._lock:
            return list(self._message_history.get(can_id, []))

    def has_seen_can_id(self, can_id: int) -> bool:
        """Check if a specific CAN ID has been observed."""
        with self._lock:
            return can_id in self._can_ids_seen

    def wait_for_state(
        self,
        target_state: int,
        timeout_sec: float = DEFAULT_STATE_WAIT_TIMEOUT_SEC,
    ) -> bool:
        """Block until vehicle reaches the target state or timeout.

        Args:
            target_state: The VehicleState value to wait for.
            timeout_sec: Maximum seconds to wait.

        Returns:
            True if target state was reached, False on timeout.
        """
        deadline = time.monotonic() + timeout_sec
        while time.monotonic() < deadline:
            if self.vehicle_state == target_state:
                return True
            time.sleep(0.05)  # 50ms polling
        return False

    def wait_for_can_message(
        self,
        can_id: int,
        timeout_sec: float = 5.0,
    ) -> Optional[tuple[float, can.Message]]:
        """Wait until a specific CAN ID is received or timeout.

        Args:
            can_id: The CAN arbitration ID to wait for.
            timeout_sec: Maximum seconds to wait.

        Returns:
            (timestamp, message) tuple or None on timeout.
        """
        # Record the current message count for this ID
        with self._lock:
            initial_count = len(self._message_history.get(can_id, []))

        deadline = time.monotonic() + timeout_sec
        while time.monotonic() < deadline:
            with self._lock:
                history = self._message_history.get(can_id, [])
                if len(history) > initial_count:
                    return history[-1]
            time.sleep(0.05)
        return None

    def wait_for_motor_rpm_zero(self, timeout_sec: float = 5.0) -> bool:
        """Wait until motor RPM reaches 0 or timeout.

        Args:
            timeout_sec: Maximum seconds to wait.

        Returns:
            True if RPM reached 0, False on timeout.
        """
        deadline = time.monotonic() + timeout_sec
        while time.monotonic() < deadline:
            if self.motor_rpm == 0:
                return True
            time.sleep(0.05)
        return False

    def _listener_loop(self) -> None:
        """Background loop that reads CAN messages."""
        assert self._bus is not None
        while self._running:
            try:
                msg = self._bus.recv(timeout=0.1)
            except can.CanError:
                continue
            if msg is None:
                continue

            ts = time.monotonic()
            arb_id = msg.arbitration_id

            with self._lock:
                self._can_ids_seen.add(arb_id)
                self._latest_messages[arb_id] = (ts, msg)

                # Append to history (cap at 1000 per ID to limit memory)
                if arb_id not in self._message_history:
                    self._message_history[arb_id] = []
                hist = self._message_history[arb_id]
                hist.append((ts, msg))
                if len(hist) > 1000:
                    hist[:] = hist[-500:]

                # Track vehicle state from 0x100
                if arb_id == CAN_VEHICLE_STATE and len(msg.data) >= 3:
                    new_state = msg.data[2] & 0x0F
                    if new_state != self._vehicle_state:
                        self._state_transitions.append((ts, new_state))
                        log.debug(
                            "Vehicle state: %s -> %s",
                            _state_name(self._vehicle_state),
                            _state_name(new_state),
                        )
                    self._vehicle_state = new_state

                # Track motor RPM from 0x300
                if arb_id == CAN_MOTOR_STATUS and len(msg.data) >= 4:
                    self._motor_rpm = (
                        msg.data[MOTOR_RPM_BYTE_LO]
                        | (msg.data[MOTOR_RPM_BYTE_HI] << 8)
                    )


# ---------------------------------------------------------------------------
# MQTT Monitor
# ---------------------------------------------------------------------------

class MQTTMonitor:
    """MQTT subscriber that captures telemetry messages for verdict checking.

    Subscribes to taktflow/# and stores the latest message per topic.
    """

    def __init__(
        self,
        host: str = "localhost",
        port: int = 1883,
    ) -> None:
        self._host = host
        self._port = port
        self._client: Optional[paho_mqtt.Client] = None
        self._lock = threading.Lock()
        self._messages: dict[str, tuple[float, dict[str, Any]]] = {}
        self._connected = False

    def start(self) -> None:
        """Connect to MQTT broker and start listening."""
        self._client = paho_mqtt.Client(
            paho_mqtt.CallbackAPIVersion.VERSION2,
            client_id="taktflow-sil-verdict-checker",
        )
        self._client.on_connect = self._on_connect
        self._client.on_message = self._on_message
        try:
            self._client.connect(self._host, self._port, keepalive=30)
            self._client.loop_start()
            log.info(
                "MQTT monitor connecting to %s:%d", self._host, self._port
            )
        except Exception as exc:
            log.warning("MQTT monitor failed to connect: %s", exc)

    def stop(self) -> None:
        """Disconnect from MQTT broker."""
        if self._client is not None:
            self._client.loop_stop()
            self._client.disconnect()
            self._client = None
        log.info("MQTT monitor stopped")

    def reset(self) -> None:
        """Clear all captured messages for a new scenario."""
        with self._lock:
            self._messages.clear()

    def get_latest(self, topic: str) -> Optional[dict[str, Any]]:
        """Get the latest parsed JSON message for a topic, or None."""
        with self._lock:
            entry = self._messages.get(topic)
            return entry[1] if entry is not None else None

    def wait_for_message(
        self,
        topic: str,
        timeout_sec: float = 5.0,
    ) -> Optional[dict[str, Any]]:
        """Wait for a message on a specific topic.

        Args:
            topic: MQTT topic to wait for.
            timeout_sec: Maximum seconds to wait.

        Returns:
            Parsed JSON dict or None on timeout.
        """
        deadline = time.monotonic() + timeout_sec
        while time.monotonic() < deadline:
            with self._lock:
                entry = self._messages.get(topic)
                if entry is not None:
                    return entry[1]
            time.sleep(0.1)
        return None

    def _on_connect(
        self,
        client: paho_mqtt.Client,
        userdata: Any,
        flags: Any,
        rc: Any,
        properties: Any = None,
    ) -> None:
        """MQTT on_connect callback."""
        if hasattr(rc, 'value'):
            rc_val = rc.value
        else:
            rc_val = rc
        if rc_val == 0:
            self._connected = True
            client.subscribe("taktflow/#", qos=0)
            log.info("MQTT monitor connected and subscribed to taktflow/#")
        else:
            log.warning("MQTT connect failed with rc=%s", rc)

    def _on_message(
        self,
        client: paho_mqtt.Client,
        userdata: Any,
        msg: paho_mqtt.MQTTMessage,
    ) -> None:
        """MQTT on_message callback."""
        try:
            payload = json.loads(msg.payload.decode("utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            payload = {"raw": msg.payload.hex()}

        with self._lock:
            self._messages[msg.topic] = (time.monotonic(), payload)


# ---------------------------------------------------------------------------
# Scenario Executor
# ---------------------------------------------------------------------------

class ScenarioExecutor:
    """Executes scenario steps and evaluates verdicts.

    Reads scenario YAML definitions and orchestrates:
      1. Setup (reset, wait for RUN state)
      2. Step execution (fault injection, docker commands, waits)
      3. Verdict evaluation (CAN state, messages, motor shutdown, MQTT)
      4. Teardown (reset, restart containers)
    """

    def __init__(
        self,
        can_monitor: CANBusMonitor,
        mqtt_monitor: MQTTMonitor,
        fault_api_url: str = "http://localhost:8091",
        default_timeout_sec: int = DEFAULT_SCENARIO_TIMEOUT_SEC,
    ) -> None:
        self._can = can_monitor
        self._mqtt = mqtt_monitor
        self._fault_api_url = fault_api_url
        self._default_timeout_sec = default_timeout_sec

    def execute_scenario(self, scenario_path: Path) -> ScenarioResult:
        """Execute a single scenario YAML file and return the result.

        Args:
            scenario_path: Path to the scenario YAML file.

        Returns:
            ScenarioResult with pass/fail verdicts and evidence.
        """
        # Load scenario YAML
        with open(scenario_path, "r", encoding="utf-8") as fh:
            scenario = yaml.safe_load(fh)

        scenario_id = scenario.get("id", scenario_path.stem)
        scenario_name = scenario.get("name", scenario_path.stem)
        description = scenario.get("description", "")
        verifies = scenario.get("verifies", [])
        aspice = scenario.get("aspice", "SWE.5")
        timeout_sec = scenario.get(
            "timeout_sec", self._default_timeout_sec
        )

        log.info(
            "%s--- Scenario: %s (%s) ---%s",
            BOLD, scenario_name, scenario_id, NC,
        )
        log.info("  Description: %s", description)
        log.info("  Verifies: %s", ", ".join(verifies))
        log.info("  Timeout: %ds", timeout_sec)

        start_time = time.monotonic()

        # Reset monitors for fresh observation
        self._can.reset()
        self._mqtt.reset()

        # Execute setup steps
        try:
            setup_steps = scenario.get("setup", [])
            for step in setup_steps:
                self._execute_step(step, timeout_sec)
        except Exception as exc:
            return ScenarioResult(
                scenario_id=scenario_id,
                scenario_name=scenario_name,
                description=description,
                verifies=verifies,
                aspice=aspice,
                passed=False,
                duration_sec=time.monotonic() - start_time,
                error=f"Setup failed: {exc}",
            )

        # Reset monitors again after setup to capture only scenario events
        self._can.reset()
        self._mqtt.reset()

        # Record the observation start time
        observation_start = time.monotonic()

        # Execute scenario steps
        try:
            steps = scenario.get("steps", [])
            for step in steps:
                self._execute_step(step, timeout_sec)
        except Exception as exc:
            return ScenarioResult(
                scenario_id=scenario_id,
                scenario_name=scenario_name,
                description=description,
                verifies=verifies,
                aspice=aspice,
                passed=False,
                duration_sec=time.monotonic() - start_time,
                error=f"Step execution failed: {exc}",
            )

        # Evaluate verdicts
        verdict_defs = scenario.get("verdicts", [])
        verdict_results: list[VerdictEvidence] = []

        for vdef in verdict_defs:
            evidence = self._evaluate_verdict(
                vdef,
                observation_start=observation_start,
                timeout_sec=timeout_sec,
            )
            verdict_results.append(evidence)

        # Execute teardown steps
        teardown_steps = scenario.get("teardown", [])
        for step in teardown_steps:
            try:
                self._execute_step(step, timeout_sec)
            except Exception as exc:
                log.warning("Teardown step failed (non-fatal): %s", exc)

        duration = time.monotonic() - start_time
        all_passed = all(v.passed for v in verdict_results)

        # Log verdict summary
        for v in verdict_results:
            status = f"{GREEN}PASS{NC}" if v.passed else f"{RED}FAIL{NC}"
            log.info(
                "  [%s] %s: expected=%s, observed=%s",
                status, v.description, v.expected, v.observed,
            )
            if v.details:
                log.info("         %s", v.details)

        overall = f"{GREEN}PASS{NC}" if all_passed else f"{RED}FAIL{NC}"
        log.info(
            "  Result: [%s] %s (%.1fs)\n",
            overall, scenario_name, duration,
        )

        return ScenarioResult(
            scenario_id=scenario_id,
            scenario_name=scenario_name,
            description=description,
            verifies=verifies,
            aspice=aspice,
            passed=all_passed,
            duration_sec=duration,
            verdicts=verdict_results,
        )

    # -- Step execution ---

    def _execute_step(
        self,
        step: dict[str, Any],
        scenario_timeout: int,
    ) -> None:
        """Execute a single scenario step.

        Supported actions:
            reset              — POST /api/fault/reset
            inject_scenario    — POST /api/fault/scenario/{name}
            wait               — sleep for N seconds
            wait_state         — wait for vehicle to reach a state
            docker_stop        — docker stop <container>
            docker_start       — docker start <container>
            verify_heartbeat   — confirm a heartbeat CAN ID is present
        """
        action = step.get("action", "")
        log.debug("  Step: %s %s", action, step)

        if action == "reset":
            resp = requests.post(
                f"{self._fault_api_url}/api/fault/reset",
                timeout=10,
            )
            resp.raise_for_status()
            log.info("  [STEP] Reset: %s", resp.json())

        elif action == "inject_scenario":
            name = step["name"]
            resp = requests.post(
                f"{self._fault_api_url}/api/fault/scenario/{name}",
                timeout=30,
            )
            resp.raise_for_status()
            log.info("  [STEP] Injected scenario: %s", name)

        elif action == "wait":
            seconds = float(step.get("seconds", 1))
            log.info("  [STEP] Waiting %.1fs...", seconds)
            time.sleep(seconds)

        elif action == "wait_state":
            state_name = step.get("state", "RUN")
            target = VEHICLE_STATE_NAMES.get(state_name, VehicleState.RUN)
            timeout = float(step.get("timeout", DEFAULT_STATE_WAIT_TIMEOUT_SEC))
            log.info(
                "  [STEP] Waiting for vehicle state %s (timeout: %.0fs)...",
                state_name, timeout,
            )
            reached = self._can.wait_for_state(target, timeout_sec=timeout)
            if not reached:
                raise TimeoutError(
                    f"Vehicle did not reach state {state_name} "
                    f"within {timeout}s (current: "
                    f"{_state_name(self._can.vehicle_state)})"
                )
            log.info("  [STEP] Vehicle reached state %s", state_name)

        elif action == "docker_stop":
            container = step["container"]
            log.info("  [STEP] Stopping container: %s", container)
            subprocess.run(
                ["docker", "stop", container],
                timeout=30,
                check=True,
                capture_output=True,
            )

        elif action == "docker_start":
            container = step["container"]
            log.info("  [STEP] Starting container: %s", container)
            subprocess.run(
                ["docker", "start", container],
                timeout=30,
                check=True,
                capture_output=True,
            )

        elif action == "verify_heartbeat":
            can_id = _parse_int(step.get("can_id", 0))
            ecu = step.get("ecu", "unknown")
            log.info(
                "  [STEP] Verifying heartbeat for %s (0x%03X)...",
                ecu, can_id,
            )
            result = self._can.wait_for_can_message(can_id, timeout_sec=3.0)
            if result is None:
                raise TimeoutError(
                    f"Heartbeat 0x{can_id:03X} ({ecu}) not detected "
                    f"within 3s"
                )
            log.info("  [STEP] Heartbeat confirmed for %s", ecu)

        else:
            raise ValueError(f"Unknown step action: {action}")

    # -- Verdict evaluation ---

    def _evaluate_verdict(
        self,
        vdef: dict[str, Any],
        observation_start: float,
        timeout_sec: int,
    ) -> VerdictEvidence:
        """Evaluate a single verdict definition against observed data.

        Supported verdict types:
            vehicle_state       — check current vehicle state
            can_message         — check for a CAN message (presence + field checks)
            can_message_absent  — check that a CAN ID was NOT received
            motor_shutdown      — check that motor RPM drops to 0
            mqtt_message        — check for an MQTT message field value
            dtc_broadcast       — check for a specific DTC on 0x500
            heartbeat_loss      — verify a heartbeat CAN ID stopped arriving
            motor_rpm_unchanged — verify motor RPM did not change (rejected cmd)
            motor_tracking      — verify motor RPM is tracking (non-zero)
            e2e_error_count     — verify E2E errors were counted (MQTT)
            steering_rate_limit — verify steering rate was limited
            no_active_faults    — verify no active DTCs on bus
            dtc_preserved       — verify a DTC from earlier is still in history
            fault_priority      — verify highest-severity fault is active state
            power_derating      — verify motor power was reduced (derating)
        """
        vtype = vdef.get("type", "")
        description = vdef.get("description", vtype)
        within_ms = float(vdef.get("within_ms", 5000))

        if vtype == "vehicle_state":
            return self._verdict_vehicle_state(vdef, description, within_ms)

        elif vtype == "can_message":
            return self._verdict_can_message(vdef, description, within_ms)

        elif vtype == "can_message_absent":
            return self._verdict_can_message_absent(
                vdef, description, within_ms
            )

        elif vtype == "motor_shutdown":
            return self._verdict_motor_shutdown(vdef, description, within_ms)

        elif vtype == "mqtt_message":
            return self._verdict_mqtt_message(vdef, description, within_ms)

        elif vtype == "dtc_broadcast":
            return self._verdict_dtc_broadcast(vdef, description, within_ms)

        elif vtype == "heartbeat_loss":
            return self._verdict_heartbeat_loss(vdef, description, within_ms)

        elif vtype == "motor_rpm_unchanged":
            return self._verdict_motor_rpm_unchanged(
                vdef, description, within_ms
            )

        elif vtype == "motor_tracking":
            return self._verdict_motor_tracking(vdef, description, within_ms)

        elif vtype == "e2e_error_count":
            return self._verdict_e2e_error_count(
                vdef, description, within_ms
            )

        elif vtype == "steering_rate_limit":
            return self._verdict_steering_rate_limit(
                vdef, description, within_ms
            )

        elif vtype == "no_active_faults":
            return self._verdict_no_active_faults(
                vdef, description, within_ms
            )

        elif vtype == "dtc_preserved":
            return self._verdict_dtc_preserved(vdef, description, within_ms)

        elif vtype == "fault_priority":
            return self._verdict_fault_priority(vdef, description, within_ms)

        elif vtype == "power_derating":
            return self._verdict_power_derating(vdef, description, within_ms)

        elif vtype == "alive_counter_wrap":
            return self._verdict_alive_counter_wrap(
                vdef, description, within_ms
            )

        elif vtype == "all_heartbeats_active":
            return self._verdict_all_heartbeats_active(
                vdef, description, within_ms
            )

        elif vtype == "battery_soc_monotonic":
            return self._verdict_battery_soc_monotonic(
                vdef, description, within_ms
            )

        elif vtype == "can_timing_jitter":
            return self._verdict_can_timing_jitter(
                vdef, description, within_ms
            )

        elif vtype == "motor_temp_stable":
            return self._verdict_motor_temp_stable(
                vdef, description, within_ms
            )

        elif vtype == "no_stuck_signals":
            return self._verdict_no_stuck_signals(
                vdef, description, within_ms
            )

        else:
            return VerdictEvidence(
                description=f"Unknown verdict type: {vtype}",
                expected="N/A",
                observed="N/A",
                passed=False,
                timestamp=time.monotonic(),
                details=f"Verdict type '{vtype}' is not supported",
            )

    def _verdict_vehicle_state(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify the vehicle reached an expected state.

        Checks:
            1. Whether the expected state was observed in the state
               transition history.
            2. Whether the current state matches.
        """
        expected_name = vdef.get("expected", "RUN")
        expected_val = VEHICLE_STATE_NAMES.get(expected_name, -1)
        wait_sec = within_ms / 1000.0

        # Give the system time to reach the expected state
        reached = self._can.wait_for_state(expected_val, timeout_sec=wait_sec)
        current_state = self._can.vehicle_state
        current_name = _state_name(current_state)

        # Also check transition history
        transitions = self._can.state_transitions
        state_seen = any(s == expected_val for _, s in transitions)

        passed = reached or state_seen or current_state == expected_val

        return VerdictEvidence(
            description=description or f"Vehicle state = {expected_name}",
            expected=expected_name,
            observed=current_name,
            passed=passed,
            timestamp=time.monotonic(),
            details=(
                f"Transitions observed: "
                f"{[_state_name(s) for _, s in transitions]}"
            ),
        )

    def _verdict_can_message(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify a specific CAN message was received with expected fields.

        Checks:
            1. A CAN message with the specified ID was received.
            2. Field checks (byte/mask/expected) all pass.
        """
        can_id = _parse_int(vdef.get("can_id", 0))
        field_checks = vdef.get("field_checks", [])
        wait_sec = within_ms / 1000.0

        # Wait for the message to appear
        result = self._can.wait_for_can_message(can_id, timeout_sec=wait_sec)

        if result is None:
            return VerdictEvidence(
                description=description
                or f"CAN message 0x{can_id:03X} received",
                expected=f"CAN 0x{can_id:03X} within {within_ms:.0f}ms",
                observed="No message received",
                passed=False,
                timestamp=time.monotonic(),
            )

        ts, msg = result
        data = msg.data

        # Evaluate field checks
        all_checks_pass = True
        check_details: list[str] = []

        for fc in field_checks:
            byte_idx = fc.get("byte", 0)
            mask = _parse_int(fc.get("mask", 0xFF))
            expected = _parse_int(fc.get("expected", 0))

            if byte_idx >= len(data):
                all_checks_pass = False
                check_details.append(
                    f"byte[{byte_idx}]: out of range "
                    f"(message has {len(data)} bytes)"
                )
                continue

            actual = data[byte_idx] & mask
            if actual != expected:
                all_checks_pass = False
                check_details.append(
                    f"byte[{byte_idx}] & 0x{mask:02X}: "
                    f"expected=0x{expected:02X}, got=0x{actual:02X}"
                )
            else:
                check_details.append(
                    f"byte[{byte_idx}] & 0x{mask:02X}: "
                    f"0x{actual:02X} OK"
                )

        return VerdictEvidence(
            description=description
            or f"CAN message 0x{can_id:03X} field checks",
            expected=f"CAN 0x{can_id:03X} with matching fields",
            observed=f"CAN 0x{can_id:03X} data={data.hex()}",
            passed=all_checks_pass,
            timestamp=time.monotonic(),
            details="; ".join(check_details),
        )

    def _verdict_motor_shutdown(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify that motor RPM drops to 0."""
        wait_sec = within_ms / 1000.0
        reached_zero = self._can.wait_for_motor_rpm_zero(
            timeout_sec=wait_sec
        )
        current_rpm = self._can.motor_rpm

        return VerdictEvidence(
            description=description or "Motor RPM = 0",
            expected="RPM = 0",
            observed=f"RPM = {current_rpm}",
            passed=reached_zero,
            timestamp=time.monotonic(),
            details=f"Waited up to {within_ms:.0f}ms for motor shutdown",
        )

    def _verdict_mqtt_message(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify an MQTT message was received with an expected field value."""
        topic = vdef.get("topic", "")
        field_name = vdef.get("field", "")
        expected_raw = vdef.get("expected")
        wait_sec = within_ms / 1000.0

        msg = self._mqtt.wait_for_message(topic, timeout_sec=wait_sec)

        if msg is None:
            return VerdictEvidence(
                description=description
                or f"MQTT {topic}.{field_name}",
                expected=f"{field_name}={expected_raw}",
                observed="No MQTT message received",
                passed=False,
                timestamp=time.monotonic(),
            )

        # Navigate nested fields (e.g., "motor.overcurrent")
        actual = msg
        for part in field_name.split("."):
            if isinstance(actual, dict):
                actual = actual.get(part)
            else:
                actual = None
                break

        # Type-aware comparison
        passed = False
        if isinstance(expected_raw, bool):
            passed = actual is True if expected_raw else actual is False
        elif isinstance(expected_raw, (int, float)):
            passed = actual == expected_raw
        elif isinstance(expected_raw, str):
            passed = str(actual) == expected_raw
        else:
            passed = actual == expected_raw

        return VerdictEvidence(
            description=description
            or f"MQTT {topic}.{field_name} = {expected_raw}",
            expected=f"{field_name}={expected_raw}",
            observed=f"{field_name}={actual}",
            passed=passed,
            timestamp=time.monotonic(),
            details=f"Topic: {topic}, Full message keys: {list(msg.keys()) if isinstance(msg, dict) else 'N/A'}",
        )

    def _verdict_dtc_broadcast(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify a specific DTC was broadcast on CAN 0x500.

        DTC_Broadcast frame layout (from scenarios.py):
            [0..1] DTC_Number     (16-bit LE)
            [2]    DTC_Status      (0x01 = active)
            [3]    ECU_Source       (1=CVC, 2=FZC, 3=RZC, 4=SC)
            [4]    OccurrenceCount
            [5..7] FreezeFrame0-2
        """
        can_id = _parse_int(vdef.get("can_id", CAN_DTC_BROADCAST))
        expected_dtc = _parse_int(vdef.get("dtc_code", 0))
        expected_source = vdef.get("ecu_source")
        wait_sec = within_ms / 1000.0

        # Check all DTC messages received during the scenario
        history = self._can.get_message_history(can_id)

        # Also wait for new ones if none yet
        if not history:
            result = self._can.wait_for_can_message(
                can_id, timeout_sec=wait_sec
            )
            if result is not None:
                history = self._can.get_message_history(can_id)

        # Search history for matching DTC
        for ts, msg in history:
            if len(msg.data) < 4:
                continue
            dtc_num = msg.data[0] | (msg.data[1] << 8)
            source = msg.data[3]
            if dtc_num == expected_dtc:
                source_match = (
                    expected_source is None or source == expected_source
                )
                if source_match:
                    return VerdictEvidence(
                        description=description
                        or f"DTC 0x{expected_dtc:04X} broadcast",
                        expected=(
                            f"DTC=0x{expected_dtc:04X}"
                            + (
                                f", source={expected_source}"
                                if expected_source is not None
                                else ""
                            )
                        ),
                        observed=(
                            f"DTC=0x{dtc_num:04X}, "
                            f"status=0x{msg.data[2]:02X}, "
                            f"source={source}"
                        ),
                        passed=True,
                        timestamp=time.monotonic(),
                        details=f"Found in {len(history)} DTC messages",
                    )

        # Not found
        dtcs_seen = []
        for _, msg in history:
            if len(msg.data) >= 4:
                dtc_num = msg.data[0] | (msg.data[1] << 8)
                dtcs_seen.append(f"0x{dtc_num:04X}")

        return VerdictEvidence(
            description=description
            or f"DTC 0x{expected_dtc:04X} broadcast",
            expected=f"DTC=0x{expected_dtc:04X}",
            observed=f"DTCs seen: {dtcs_seen}" if dtcs_seen else "No DTC messages",
            passed=False,
            timestamp=time.monotonic(),
            details=f"Searched {len(history)} DTC frames on 0x{can_id:03X}",
        )

    def _verdict_heartbeat_loss(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify that a heartbeat CAN ID stopped arriving.

        A heartbeat loss is confirmed if no new messages arrive on the
        specified CAN ID within the given time window. This means the
        ECU has stopped broadcasting (container stopped, bus-off, etc.).
        """
        can_id = _parse_int(vdef.get("can_id", 0))
        wait_sec = within_ms / 1000.0

        # The heartbeat should NOT appear — we wait and check for absence
        # We already captured initial messages; now wait to see if new ones
        # arrive. If none arrive in the window, the heartbeat has been lost.
        initial_history = self._can.get_message_history(can_id)
        initial_count = len(initial_history)

        # Wait the specified period
        time.sleep(wait_sec)

        # Check if any NEW messages arrived
        current_history = self._can.get_message_history(can_id)
        new_count = len(current_history) - initial_count

        # Heartbeat loss = no new messages in the observation window
        passed = new_count == 0

        return VerdictEvidence(
            description=description
            or f"Heartbeat loss on 0x{can_id:03X}",
            expected=f"0 new messages on 0x{can_id:03X} in {within_ms:.0f}ms",
            observed=f"{new_count} new messages on 0x{can_id:03X}",
            passed=passed,
            timestamp=time.monotonic(),
            details=(
                f"Initial count={initial_count}, "
                f"after wait={len(current_history)}"
            ),
        )

    def _verdict_motor_rpm_unchanged(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify motor RPM did not change (corrupted command was rejected).

        Captures RPM before and after the observation window and checks
        that the values are identical or within a small tolerance.
        """
        rpm_before = self._can.motor_rpm
        wait_sec = within_ms / 1000.0
        time.sleep(wait_sec)
        rpm_after = self._can.motor_rpm

        # Allow small jitter (1 RPM) from physics simulation noise
        tolerance = int(vdef.get("tolerance", 1))
        passed = abs(rpm_after - rpm_before) <= tolerance

        return VerdictEvidence(
            description=description or "Motor RPM unchanged",
            expected=f"RPM change <= {tolerance}",
            observed=f"RPM: {rpm_before} -> {rpm_after} (delta={abs(rpm_after - rpm_before)})",
            passed=passed,
            timestamp=time.monotonic(),
        )

    def _verdict_motor_tracking(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify motor RPM is non-zero (motor is tracking commands).

        Checks that Motor_Status (0x300) shows RPM > 0, indicating the
        motor is responding to torque requests.
        """
        wait_sec = within_ms / 1000.0
        deadline = time.monotonic() + wait_sec

        while time.monotonic() < deadline:
            rpm = self._can.motor_rpm
            if rpm > 0:
                return VerdictEvidence(
                    description=description or "Motor RPM tracking",
                    expected="RPM > 0",
                    observed=f"RPM = {rpm}",
                    passed=True,
                    timestamp=time.monotonic(),
                )
            time.sleep(0.05)

        return VerdictEvidence(
            description=description or "Motor RPM tracking",
            expected="RPM > 0",
            observed=f"RPM = {self._can.motor_rpm}",
            passed=False,
            timestamp=time.monotonic(),
            details=f"Motor did not show non-zero RPM within {within_ms:.0f}ms",
        )

    def _verdict_e2e_error_count(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify E2E error count increased (via MQTT telemetry).

        Checks the MQTT topic taktflow/telemetry/e2e (or configured topic)
        for an error_count field that is greater than 0.
        """
        topic = vdef.get("topic", "taktflow/telemetry/e2e")
        field_name = vdef.get("field", "error_count")
        min_errors = int(vdef.get("min_errors", 1))
        wait_sec = within_ms / 1000.0

        msg = self._mqtt.wait_for_message(topic, timeout_sec=wait_sec)

        if msg is None:
            return VerdictEvidence(
                description=description or "E2E error count",
                expected=f"{field_name} >= {min_errors}",
                observed="No MQTT message received",
                passed=False,
                timestamp=time.monotonic(),
                details=f"Topic: {topic}",
            )

        actual = msg.get(field_name, 0)
        passed = isinstance(actual, (int, float)) and actual >= min_errors

        return VerdictEvidence(
            description=description or "E2E error count",
            expected=f"{field_name} >= {min_errors}",
            observed=f"{field_name} = {actual}",
            passed=passed,
            timestamp=time.monotonic(),
            details=f"Topic: {topic}",
        )

    def _verdict_steering_rate_limit(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify steering rate was limited (fault detected).

        Checks Steering_Status (0x200) for a fault flag indicating
        the steering rate limit was violated.
        """
        wait_sec = within_ms / 1000.0
        result = self._can.wait_for_can_message(
            CAN_STEERING_STATUS, timeout_sec=wait_sec
        )

        if result is None:
            return VerdictEvidence(
                description=description or "Steering rate limit active",
                expected="Steering fault flag set",
                observed="No Steering_Status message",
                passed=False,
                timestamp=time.monotonic(),
            )

        _, msg = result
        # Check for fault bit in steering status
        # Convention: byte 4 or byte 5 contains fault flags
        fault_byte = msg.data[4] if len(msg.data) > 4 else 0
        has_fault = fault_byte != 0

        return VerdictEvidence(
            description=description or "Steering rate limit active",
            expected="Steering fault flag != 0",
            observed=f"Fault byte = 0x{fault_byte:02X}",
            passed=has_fault,
            timestamp=time.monotonic(),
            details=f"Steering_Status data: {msg.data.hex()}",
        )

    def _verdict_no_active_faults(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify no active DTCs are on the bus.

        Checks DTC_Broadcast (0x500) history for any message with
        DTC_Status == 0x01 (active). If none found, the verdict passes.
        """
        wait_sec = within_ms / 1000.0
        time.sleep(min(wait_sec, 1.0))  # Brief observation window

        history = self._can.get_message_history(CAN_DTC_BROADCAST)
        active_dtcs: list[str] = []

        for _, msg in history:
            if len(msg.data) >= 3 and msg.data[2] == 0x01:
                dtc_num = msg.data[0] | (msg.data[1] << 8)
                active_dtcs.append(f"0x{dtc_num:04X}")

        passed = len(active_dtcs) == 0

        return VerdictEvidence(
            description=description or "No active faults",
            expected="0 active DTCs",
            observed=f"{len(active_dtcs)} active DTCs: {active_dtcs}",
            passed=passed,
            timestamp=time.monotonic(),
            details=f"Checked {len(history)} DTC frames",
        )

    def _verdict_dtc_preserved(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify a DTC from a previous phase is still in the DTC history.

        This checks that fault memory is preserved across state transitions
        (e.g., the DTC from the fault phase is still accessible after
        recovery to RUN state).
        """
        expected_dtc = _parse_int(vdef.get("dtc_code", 0))
        wait_sec = within_ms / 1000.0

        # Wait for any DTC messages
        time.sleep(min(wait_sec, 2.0))
        history = self._can.get_message_history(CAN_DTC_BROADCAST)

        for _, msg in history:
            if len(msg.data) >= 2:
                dtc_num = msg.data[0] | (msg.data[1] << 8)
                if dtc_num == expected_dtc:
                    return VerdictEvidence(
                        description=description
                        or f"DTC 0x{expected_dtc:04X} preserved",
                        expected=f"DTC 0x{expected_dtc:04X} in history",
                        observed=f"Found DTC 0x{dtc_num:04X}",
                        passed=True,
                        timestamp=time.monotonic(),
                    )

        return VerdictEvidence(
            description=description
            or f"DTC 0x{expected_dtc:04X} preserved",
            expected=f"DTC 0x{expected_dtc:04X} in history",
            observed=f"Not found in {len(history)} DTC messages",
            passed=False,
            timestamp=time.monotonic(),
        )

    def _verdict_fault_priority(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify the highest-severity fault determines the vehicle state.

        When multiple faults are active, the vehicle state should reflect
        the most critical one. SAFE_STOP > LIMP > DEGRADED > RUN.
        """
        expected_name = vdef.get("expected_state", "SAFE_STOP")
        expected_val = VEHICLE_STATE_NAMES.get(expected_name, -1)
        wait_sec = within_ms / 1000.0

        reached = self._can.wait_for_state(expected_val, timeout_sec=wait_sec)
        current_state = self._can.vehicle_state
        current_name = _state_name(current_state)

        # For fault priority, the state must be at least as severe
        # Severity order: RUN(1) < DEGRADED(2) < LIMP(3) < SAFE_STOP(4)
        at_least_as_severe = current_state >= expected_val
        passed = reached or at_least_as_severe

        return VerdictEvidence(
            description=description
            or f"Fault priority -> {expected_name}",
            expected=f"State >= {expected_name} ({expected_val})",
            observed=f"State = {current_name} ({current_state})",
            passed=passed,
            timestamp=time.monotonic(),
            details=(
                f"Transitions: "
                f"{[_state_name(s) for _, s in self._can.state_transitions]}"
            ),
        )

    def _verdict_power_derating(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify motor power was reduced (derating due to overtemp/fault).

        Checks that motor RPM decreased from a previous level, indicating
        the power output was limited. Compares the current RPM against the
        maximum observed RPM in the history.
        """
        wait_sec = within_ms / 1000.0
        time.sleep(min(wait_sec, 2.0))

        history = self._can.get_message_history(CAN_MOTOR_STATUS)
        if not history:
            return VerdictEvidence(
                description=description or "Power derating",
                expected="RPM decreased (derating)",
                observed="No Motor_Status messages",
                passed=False,
                timestamp=time.monotonic(),
            )

        # Extract RPM values from history
        rpms: list[int] = []
        for _, msg in history:
            if len(msg.data) >= 4:
                rpm = (
                    msg.data[MOTOR_RPM_BYTE_LO]
                    | (msg.data[MOTOR_RPM_BYTE_HI] << 8)
                )
                rpms.append(rpm)

        if not rpms:
            return VerdictEvidence(
                description=description or "Power derating",
                expected="RPM decreased (derating)",
                observed="No valid RPM data",
                passed=False,
                timestamp=time.monotonic(),
            )

        max_rpm = max(rpms)
        current_rpm = rpms[-1]
        # Derating means the latest RPM is lower than the peak
        passed = max_rpm > 0 and current_rpm < max_rpm

        return VerdictEvidence(
            description=description or "Power derating",
            expected=f"Current RPM < peak RPM (derating)",
            observed=f"Peak RPM={max_rpm}, current RPM={current_rpm}",
            passed=passed,
            timestamp=time.monotonic(),
            details=f"RPM samples: {len(rpms)}, range: {min(rpms)}-{max_rpm}",
        )

    def _verdict_can_message_absent(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify that a specific CAN ID was NOT received.

        Used to confirm that a corrupted/invalid command was rejected
        and not forwarded onto the bus.
        """
        can_id = _parse_int(vdef.get("can_id", 0))
        wait_sec = within_ms / 1000.0

        # Wait the observation period
        time.sleep(wait_sec)

        # Check if the message was seen
        seen = self._can.has_seen_can_id(can_id)

        return VerdictEvidence(
            description=description
            or f"CAN 0x{can_id:03X} absent",
            expected=f"CAN 0x{can_id:03X} NOT received",
            observed=(
                f"CAN 0x{can_id:03X} WAS received"
                if seen
                else f"CAN 0x{can_id:03X} correctly absent"
            ),
            passed=not seen,
            timestamp=time.monotonic(),
        )

    def _verdict_alive_counter_wrap(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify E2E alive counters wrap correctly over the observation period.

        Checks that the 4-bit alive counter in byte 0 (upper nibble) of
        E2E-protected messages wraps properly and shows the expected
        minimum number of wrap-arounds.
        """
        can_ids = [_parse_int(x) for x in vdef.get("can_ids", [])]
        counter_bits = int(vdef.get("counter_bits", 4))
        counter_max = (1 << counter_bits) - 1
        expected_wraps_min = int(vdef.get("expected_wraps_min", 1))

        results: list[str] = []
        all_passed = True

        for can_id in can_ids:
            history = self._can.get_message_history(can_id)
            if not history:
                results.append(
                    f"0x{can_id:03X}: no messages"
                )
                all_passed = False
                continue

            # Extract alive counter values (upper nibble of byte 0)
            counters: list[int] = []
            for _, msg in history:
                if len(msg.data) >= 1:
                    counters.append((msg.data[0] >> 4) & counter_max)

            # Count wraps (counter goes from max back to 0)
            wraps = 0
            for i in range(1, len(counters)):
                if counters[i] < counters[i - 1]:
                    wraps += 1

            ok = wraps >= expected_wraps_min
            if not ok:
                all_passed = False
            results.append(
                f"0x{can_id:03X}: {wraps} wraps "
                f"({'OK' if ok else 'LOW'})"
            )

        return VerdictEvidence(
            description=description or "Alive counter wraps",
            expected=f">= {expected_wraps_min} wraps per CAN ID",
            observed="; ".join(results),
            passed=all_passed,
            timestamp=time.monotonic(),
        )

    def _verdict_all_heartbeats_active(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify all specified heartbeat CAN IDs are actively broadcasting.

        Checks that each listed CAN ID has been received recently.
        """
        can_ids = [_parse_int(x) for x in vdef.get("can_ids", [])]
        interval_ms = float(vdef.get("interval_ms", 50))
        wait_sec = within_ms / 1000.0

        # Wait briefly to collect messages
        time.sleep(min(wait_sec, 1.0))

        results: list[str] = []
        all_active = True

        for can_id in can_ids:
            result = self._can.get_latest_message(can_id)
            if result is None:
                results.append(f"0x{can_id:03X}: NOT received")
                all_active = False
            else:
                ts, msg = result
                results.append(f"0x{can_id:03X}: active")

        return VerdictEvidence(
            description=description or "All heartbeats active",
            expected=f"All {len(can_ids)} heartbeats present",
            observed="; ".join(results),
            passed=all_active,
            timestamp=time.monotonic(),
        )

    def _verdict_battery_soc_monotonic(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify battery SOC changes monotonically (decreasing during drive).

        Checks Battery_Status (0x303) byte 2 (SOC percentage) values
        are monotonically decreasing (or non-increasing).
        """
        can_id = _parse_int(vdef.get("can_id", CAN_BATTERY_STATUS))
        direction = vdef.get("direction", "decreasing")

        history = self._can.get_message_history(can_id)
        if not history:
            return VerdictEvidence(
                description=description or "Battery SOC monotonic",
                expected=f"SOC monotonically {direction}",
                observed="No Battery_Status messages",
                passed=False,
                timestamp=time.monotonic(),
            )

        # Extract SOC values (byte 2 of Battery_Status)
        soc_values: list[int] = []
        for _, msg in history:
            if len(msg.data) >= 3:
                soc_values.append(msg.data[2])

        if len(soc_values) < 2:
            return VerdictEvidence(
                description=description or "Battery SOC monotonic",
                expected=f"SOC monotonically {direction}",
                observed=f"Only {len(soc_values)} SOC sample(s)",
                passed=False,
                timestamp=time.monotonic(),
            )

        # Check monotonicity
        violations = 0
        for i in range(1, len(soc_values)):
            if direction == "decreasing":
                if soc_values[i] > soc_values[i - 1]:
                    violations += 1
            else:  # increasing
                if soc_values[i] < soc_values[i - 1]:
                    violations += 1

        passed = violations == 0

        return VerdictEvidence(
            description=description or "Battery SOC monotonic",
            expected=f"SOC monotonically {direction} (0 violations)",
            observed=(
                f"SOC range: {soc_values[0]}% -> {soc_values[-1]}%, "
                f"{violations} violations in {len(soc_values)} samples"
            ),
            passed=passed,
            timestamp=time.monotonic(),
        )

    def _verdict_can_timing_jitter(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify CAN message timing jitter is within bounds.

        Checks that the interval between consecutive messages on each
        CAN ID does not deviate more than max_jitter_ms from the
        nominal interval.
        """
        can_ids = [_parse_int(x) for x in vdef.get("can_ids", [])]
        nominal_ms = float(vdef.get("nominal_interval_ms", 50))
        max_jitter_ms = float(vdef.get("max_jitter_ms", 10))

        results: list[str] = []
        all_passed = True

        for can_id in can_ids:
            history = self._can.get_message_history(can_id)
            if len(history) < 2:
                results.append(f"0x{can_id:03X}: insufficient samples")
                all_passed = False
                continue

            # Calculate intervals
            intervals_ms: list[float] = []
            for i in range(1, len(history)):
                dt = (history[i][0] - history[i - 1][0]) * 1000.0
                intervals_ms.append(dt)

            max_deviation = max(
                abs(dt - nominal_ms) for dt in intervals_ms
            )
            ok = max_deviation <= max_jitter_ms
            if not ok:
                all_passed = False

            avg_interval = sum(intervals_ms) / len(intervals_ms)
            results.append(
                f"0x{can_id:03X}: avg={avg_interval:.1f}ms, "
                f"max_dev={max_deviation:.1f}ms "
                f"({'OK' if ok else 'JITTER'})"
            )

        return VerdictEvidence(
            description=description or "CAN timing jitter",
            expected=f"Jitter <= {max_jitter_ms}ms from {nominal_ms}ms nominal",
            observed="; ".join(results),
            passed=all_passed,
            timestamp=time.monotonic(),
        )

    def _verdict_motor_temp_stable(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify motor temperature stays below a threshold.

        Checks Motor_Temp (0x302) for temperature values. The temperature
        encoding is platform-specific; we read the raw 16-bit value from
        bytes 2-3 and apply the DBC scale.
        """
        can_id = _parse_int(vdef.get("can_id", CAN_MOTOR_TEMP))
        max_temp_c = float(vdef.get("max_temp_c", 90))

        history = self._can.get_message_history(can_id)
        if not history:
            return VerdictEvidence(
                description=description or "Motor temp stable",
                expected=f"Temp < {max_temp_c}C",
                observed="No Motor_Temp messages",
                passed=False,
                timestamp=time.monotonic(),
            )

        # Extract temperature values (bytes 2-3, 16-bit LE, scale 0.1C)
        temps: list[float] = []
        for _, msg in history:
            if len(msg.data) >= 4:
                raw = msg.data[2] | (msg.data[3] << 8)
                temp_c = raw * 0.1
                temps.append(temp_c)

        if not temps:
            return VerdictEvidence(
                description=description or "Motor temp stable",
                expected=f"Temp < {max_temp_c}C",
                observed="No valid temperature data",
                passed=False,
                timestamp=time.monotonic(),
            )

        peak_temp = max(temps)
        passed = peak_temp < max_temp_c

        return VerdictEvidence(
            description=description or "Motor temp stable",
            expected=f"Temp < {max_temp_c}C",
            observed=f"Peak={peak_temp:.1f}C, latest={temps[-1]:.1f}C",
            passed=passed,
            timestamp=time.monotonic(),
            details=f"{len(temps)} samples, range: {min(temps):.1f}-{peak_temp:.1f}C",
        )

    def _verdict_no_stuck_signals(
        self,
        vdef: dict[str, Any],
        description: str,
        within_ms: float,
    ) -> VerdictEvidence:
        """Verify no CAN signal is stuck (repeated identical frames).

        Checks that no CAN ID produces more than max_identical_frames
        consecutive identical data payloads, which would indicate a
        stuck sensor or frozen output.
        """
        can_ids = [_parse_int(x) for x in vdef.get("can_ids", [])]
        max_identical = int(vdef.get("max_identical_frames", 50))

        results: list[str] = []
        all_passed = True

        for can_id in can_ids:
            history = self._can.get_message_history(can_id)
            if len(history) < 2:
                results.append(f"0x{can_id:03X}: insufficient samples")
                continue

            # Count max consecutive identical frames
            max_run = 1
            current_run = 1
            for i in range(1, len(history)):
                if history[i][1].data == history[i - 1][1].data:
                    current_run += 1
                    max_run = max(max_run, current_run)
                else:
                    current_run = 1

            ok = max_run <= max_identical
            if not ok:
                all_passed = False

            results.append(
                f"0x{can_id:03X}: max_run={max_run} "
                f"({'OK' if ok else 'STUCK'})"
            )

        return VerdictEvidence(
            description=description or "No stuck signals",
            expected=f"Max consecutive identical <= {max_identical}",
            observed="; ".join(results),
            passed=all_passed,
            timestamp=time.monotonic(),
        )


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------

def generate_junit_xml(
    results: list[ScenarioResult],
    output_path: Path,
) -> None:
    """Generate a JUnit XML report from scenario results.

    Args:
        results: List of ScenarioResult objects.
        output_path: Path to write the XML file.
    """
    test_cases: list[TestCase] = []

    for result in results:
        tc = TestCase(
            name=f"{result.scenario_id}: {result.scenario_name}",
            classname="sil.verdict_checker",
            elapsed_sec=result.duration_sec,
        )

        if result.error:
            tc.add_error_info(
                message=result.error,
                output=result.error,
            )
        elif not result.passed:
            # Collect failure details from verdicts
            failures = [
                f"  [{v.description}] expected={v.expected}, "
                f"observed={v.observed}"
                + (f" ({v.details})" if v.details else "")
                for v in result.verdicts
                if not v.passed
            ]
            failure_msg = "\n".join(failures)
            tc.add_failure_info(
                message=f"Scenario {result.scenario_id} failed",
                output=failure_msg,
            )

        # Add stdout with full evidence for traceability
        stdout_lines = [
            f"Scenario: {result.scenario_name} ({result.scenario_id})",
            f"Description: {result.description}",
            f"Verifies: {', '.join(result.verifies)}",
            f"ASPICE: {result.aspice}",
            f"Duration: {result.duration_sec:.2f}s",
            f"Result: {'PASS' if result.passed else 'FAIL'}",
            "",
            "Verdicts:",
        ]
        for v in result.verdicts:
            status = "PASS" if v.passed else "FAIL"
            stdout_lines.append(
                f"  [{status}] {v.description}: "
                f"expected={v.expected}, observed={v.observed}"
            )
            if v.details:
                stdout_lines.append(f"         {v.details}")

        tc.stdout = "\n".join(stdout_lines)

        test_cases.append(tc)

    suite = TestSuite(
        name="Taktflow SIL Tests",
        test_cases=test_cases,
        timestamp=time.strftime("%Y-%m-%dT%H:%M:%S"),
    )

    with open(output_path, "w", encoding="utf-8") as fh:
        TestSuite.to_file(fh, [suite], prettyprint=True)

    log.info("JUnit XML report written to %s", output_path)


def generate_summary(
    results: list[ScenarioResult],
    output_path: Path,
) -> None:
    """Generate a human-readable summary text file.

    Args:
        results: List of ScenarioResult objects.
        output_path: Path to write the summary file.
    """
    total = len(results)
    passed = sum(1 for r in results if r.passed)
    failed = total - passed

    lines = [
        "=" * 60,
        "  Taktflow SIL Test Summary",
        f"  Date: {time.strftime('%Y-%m-%d %H:%M:%S')}",
        "=" * 60,
        "",
        f"  Total scenarios:  {total}",
        f"  Passed:           {passed}",
        f"  Failed:           {failed}",
        f"  Pass rate:        {(passed/total*100) if total else 0:.1f}%",
        "",
        "-" * 60,
    ]

    for r in results:
        status = "PASS" if r.passed else "FAIL"
        lines.append(
            f"  [{status}] {r.scenario_id}: {r.scenario_name} "
            f"({r.duration_sec:.1f}s)"
        )
        if r.error:
            lines.append(f"         ERROR: {r.error}")
        for v in r.verdicts:
            v_status = "PASS" if v.passed else "FAIL"
            lines.append(
                f"    [{v_status}] {v.description}: "
                f"expected={v.expected}, observed={v.observed}"
            )

    lines.extend([
        "",
        "-" * 60,
        f"  Requirement coverage: "
        f"{', '.join(sorted(set(r for res in results for r in res.verifies)))}",
        "=" * 60,
    ])

    text = "\n".join(lines) + "\n"

    with open(output_path, "w", encoding="utf-8") as fh:
        fh.write(text)

    # Also print to stdout
    print(text)
    log.info("Summary written to %s", output_path)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _state_name(state_val: int) -> str:
    """Convert a numeric vehicle state to its name."""
    for name, val in VEHICLE_STATE_NAMES.items():
        if val == state_val:
            return name
    return f"UNKNOWN({state_val})"


def _parse_int(value: Any) -> int:
    """Parse an integer from a YAML value (supports hex strings like 0x500)."""
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        return int(value, 0)  # auto-detect base (0x prefix -> hex)
    return int(value)


# ---------------------------------------------------------------------------
# Main entry point
# ---------------------------------------------------------------------------

def main() -> int:
    """Main entry point for the SIL verdict checker.

    Returns:
        Exit code: 0 if all pass, 1 if any fail.
    """
    parser = argparse.ArgumentParser(
        description="Taktflow SIL Verdict Checker — executes scenario YAML "
        "files and evaluates pass/fail verdicts against the SIL platform.",
    )
    parser.add_argument(
        "--scenario",
        action="append",
        required=True,
        help="Path to a scenario YAML file (can be repeated)",
    )
    parser.add_argument(
        "--results-dir",
        type=str,
        default="./results",
        help="Directory to write result files (default: ./results)",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=DEFAULT_SCENARIO_TIMEOUT_SEC,
        help=f"Default per-scenario timeout in seconds "
        f"(default: {DEFAULT_SCENARIO_TIMEOUT_SEC})",
    )
    parser.add_argument(
        "--fault-api-url",
        type=str,
        default="http://localhost:8091",
        help="Fault injection API base URL (default: http://localhost:8091)",
    )
    parser.add_argument(
        "--mqtt-host",
        type=str,
        default="localhost",
        help="MQTT broker host (default: localhost)",
    )
    parser.add_argument(
        "--mqtt-port",
        type=int,
        default=1883,
        help="MQTT broker port (default: 1883)",
    )
    parser.add_argument(
        "--can-channel",
        type=str,
        default=DEFAULT_CAN_CHANNEL,
        help=f"CAN bus channel (default: {DEFAULT_CAN_CHANNEL})",
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Enable verbose (DEBUG) logging",
    )
    args = parser.parse_args()

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    results_dir = Path(args.results_dir)
    results_dir.mkdir(parents=True, exist_ok=True)

    # Validate scenario files exist
    scenario_paths: list[Path] = []
    for sp in args.scenario:
        p = Path(sp)
        if not p.exists():
            log.error("Scenario file not found: %s", p)
            return 1
        scenario_paths.append(p)

    log.info(
        "%s=== Taktflow SIL Verdict Checker ===%s", BOLD, NC,
    )
    log.info("Scenarios: %d", len(scenario_paths))
    log.info("Results dir: %s", results_dir)
    log.info("Fault API: %s", args.fault_api_url)
    log.info("MQTT: %s:%d", args.mqtt_host, args.mqtt_port)
    log.info("CAN channel: %s", args.can_channel)
    log.info("Default timeout: %ds", args.timeout)
    log.info("")

    # Initialize monitors
    can_monitor = CANBusMonitor(channel=args.can_channel)
    mqtt_monitor = MQTTMonitor(
        host=args.mqtt_host,
        port=args.mqtt_port,
    )

    try:
        can_monitor.start()
    except Exception as exc:
        log.error(
            "Cannot start CAN monitor on %s: %s", args.can_channel, exc,
        )
        log.error(
            "Ensure vcan0 is up: "
            "sudo modprobe vcan && sudo ip link add vcan0 type vcan "
            "&& sudo ip link set vcan0 up"
        )
        return 1

    mqtt_monitor.start()

    # Brief pause for monitors to connect
    time.sleep(1.0)

    executor = ScenarioExecutor(
        can_monitor=can_monitor,
        mqtt_monitor=mqtt_monitor,
        fault_api_url=args.fault_api_url,
        default_timeout_sec=args.timeout,
    )

    # Execute all scenarios
    results: list[ScenarioResult] = []
    for scenario_path in scenario_paths:
        try:
            result = executor.execute_scenario(scenario_path)
            results.append(result)
        except Exception as exc:
            log.error(
                "Unhandled exception in scenario %s: %s",
                scenario_path.name, exc,
            )
            results.append(
                ScenarioResult(
                    scenario_id=scenario_path.stem,
                    scenario_name=scenario_path.stem,
                    description="",
                    verifies=[],
                    aspice="SWE.5",
                    passed=False,
                    duration_sec=0.0,
                    error=f"Unhandled exception: {exc}",
                )
            )

    # Stop monitors
    can_monitor.stop()
    mqtt_monitor.stop()

    # Generate reports
    junit_path = results_dir / "sil_results.xml"
    summary_path = results_dir / "summary.txt"

    generate_junit_xml(results, junit_path)
    generate_summary(results, summary_path)

    # Return exit code
    all_passed = all(r.passed for r in results)
    return 0 if all_passed else 1


if __name__ == "__main__":
    sys.exit(main())
