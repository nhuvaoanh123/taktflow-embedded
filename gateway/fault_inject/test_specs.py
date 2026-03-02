"""Dashboard E2E test specifications — verdict definitions for 8 testable scenarios.

Each spec defines the scenario ID, safety goal reference, observe window,
and verdict checks (expected state transitions, DTCs, fault flags).

These specs are consumed by test_runner.py to drive automated E2E testing
from the dashboard with real-time pass/fail verdicts.
"""

from dataclasses import dataclass, field


@dataclass
class VerdictCheck:
    """Single verdict check within a scenario."""
    description: str
    check_type: str          # "vehicle_state", "dtc", "fault_flag", "motor_stop"
    expected: str            # human-readable expected value
    field: str = ""          # MQTT field to monitor
    value: object = None     # expected value (int, str, etc.)
    timeout_ms: int = 5000   # max time to wait for this verdict


@dataclass
class TestSpec:
    """Full specification for one testable scenario."""
    id: str
    label: str
    scenario: str            # fault_inject scenario name to trigger
    sg: str                  # Safety Goal ID
    asil: str                # ASIL level
    he: str                  # Hazardous Event ID
    description: str = ""    # What the test proves (HARA context)
    injection: str = ""      # What CAN/fault is injected
    prep: str = "normal_drive"  # scenario to run before injection
    observe_sec: float = 5.0
    verdicts: list[VerdictCheck] = field(default_factory=list)


# ---------------------------------------------------------------------------
# 8 testable scenarios (injectable via CAN, clear MQTT-observable verdicts)
# ---------------------------------------------------------------------------

TEST_SPECS: list[TestSpec] = [
    TestSpec(
        id="overcurrent",
        label="Overcurrent",
        scenario="overcurrent",
        sg="SG-001", asil="D", he="HE-001",
        description="Verifies motor overcurrent triggers DEGRADED mode and DTC. "
                    "Safety Goal SG-001: prevent unintended vehicle acceleration from motor fault.",
        injection="CAN frame Motor_Current (0x201) with Current_A=120 (>100A threshold)",
        observe_sec=5.0,
        verdicts=[
            VerdictCheck(
                description="Vehicle enters DEGRADED",
                check_type="vehicle_state",
                expected="DEGRADED",
                value=2,       # DEGRADED enum
                timeout_ms=5000,
            ),
            VerdictCheck(
                description="DTC 0xE301 broadcast",
                check_type="dtc",
                expected="DTC 0xE301 received",
                value=0xE301,
                timeout_ms=5000,
            ),
        ],
    ),
    TestSpec(
        id="estop",
        label="E-Stop",
        scenario="estop",
        sg="SG-008", asil="C", he="HE-011/013",
        description="Verifies emergency stop brings vehicle to SAFE_STOP within 200ms. "
                    "Safety Goal SG-008: immediate halt on E-Stop activation.",
        injection="CAN frame EStop_Command (0x010) with EStop_Active=1",
        observe_sec=3.0,
        verdicts=[
            VerdictCheck(
                description="Vehicle enters SAFE_STOP within 200ms",
                check_type="vehicle_state",
                expected="SAFE_STOP",
                value=4,       # SAFE_STOP enum
                timeout_ms=3000,
            ),
        ],
    ),
    TestSpec(
        id="steer_fault",
        label="Steer Fault",
        scenario="steer_fault",
        sg="SG-003", asil="D", he="HE-003/004",
        description="Verifies steering actuator fault is detected and flagged. "
                    "Safety Goal SG-003: prevent unintended steering from actuator malfunction.",
        injection="CAN frame Steer_Command (0x301) with SteerAngle beyond plausibility limit",
        observe_sec=3.0,
        verdicts=[
            VerdictCheck(
                description="SteerFaultStatus = 1",
                check_type="fault_flag",
                expected="Steering fault active",
                field="steer_fault",
                value=1,
                timeout_ms=3000,
            ),
        ],
    ),
    TestSpec(
        id="brake_fault",
        label="Brake Fault",
        scenario="brake_fault",
        sg="SG-004", asil="D", he="HE-005",
        description="Verifies brake actuator fault is detected and flagged. "
                    "Safety Goal SG-004: prevent loss of braking from actuator malfunction.",
        injection="CAN frame Brake_Command (0x300) with BrakeForce beyond plausibility limit",
        observe_sec=3.0,
        verdicts=[
            VerdictCheck(
                description="BrakeFaultStatus = 1",
                check_type="fault_flag",
                expected="Brake fault active",
                field="brake_fault",
                value=1,
                timeout_ms=3000,
            ),
        ],
    ),
    TestSpec(
        id="battery_low",
        label="Battery Drain",
        scenario="battery_low",
        sg="SG-006", asil="A", he="HE-015",
        description="Verifies low battery voltage triggers DEGRADED/LIMP and DTC. "
                    "Safety Goal SG-006: prevent loss of vehicle control from power loss.",
        injection="CAN frame Battery_Status (0x400) with BatteryVoltage=9V (<10V threshold)",
        observe_sec=8.0,
        verdicts=[
            VerdictCheck(
                description="DTC 0xE401 broadcast",
                check_type="dtc",
                expected="DTC 0xE401 received",
                value=0xE401,
                timeout_ms=8000,
            ),
            VerdictCheck(
                description="Vehicle enters DEGRADED or LIMP",
                check_type="vehicle_state",
                expected="DEGRADED or LIMP",
                value=[2, 3],  # DEGRADED=2 or LIMP=3
                timeout_ms=8000,
            ),
        ],
    ),
    TestSpec(
        id="motor_reversal",
        label="Motor Reversal",
        scenario="motor_reversal",
        sg="SG-010", asil="C", he="HE-014",
        description="Verifies reverse torque request triggers SAFE_STOP. "
                    "Safety Goal SG-010: prevent unintended reversal at speed.",
        injection="CAN frame Torque_Request (0x101) with negative torque while speed > 0",
        observe_sec=4.0,
        verdicts=[
            VerdictCheck(
                description="Vehicle enters SAFE_STOP",
                check_type="vehicle_state",
                expected="SAFE_STOP",
                value=4,
                timeout_ms=4000,
            ),
        ],
    ),
    TestSpec(
        id="runaway_accel",
        label="Runaway Acceleration",
        scenario="runaway_accel",
        sg="SG-011", asil="C", he="HE-016",
        description="Verifies excessive torque request triggers DEGRADED with torque limiting. "
                    "Safety Goal SG-011: prevent runaway acceleration from sensor/software fault.",
        injection="CAN frame Torque_Request (0x101) with TorquePercent=100 (max) suddenly",
        observe_sec=4.0,
        verdicts=[
            VerdictCheck(
                description="Vehicle enters DEGRADED (torque limited)",
                check_type="vehicle_state",
                expected="DEGRADED",
                value=2,
                timeout_ms=4000,
            ),
        ],
    ),
    TestSpec(
        id="creep_from_stop",
        label="Creep from Stop",
        scenario="creep_from_stop",
        sg="SG-012", asil="D", he="HE-017",
        description="Verifies unintended creep from standstill triggers SAFE_STOP. "
                    "Safety Goal SG-012: prevent vehicle movement without driver intent.",
        injection="CAN frame Torque_Request (0x101) with low torque while vehicle speed=0 and brake released",
        observe_sec=4.0,
        verdicts=[
            VerdictCheck(
                description="Vehicle enters SAFE_STOP",
                check_type="vehicle_state",
                expected="SAFE_STOP",
                value=4,
                timeout_ms=4000,
            ),
        ],
    ),
]

# Lookup by ID
TEST_SPECS_BY_ID: dict[str, TestSpec] = {s.id: s for s in TEST_SPECS}
