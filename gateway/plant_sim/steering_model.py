"""Steering servo model — rate-limited tracking of commanded angle.

Physics:
  angle += clamp(cmd - actual, -rate_limit, +rate_limit) * dt

Fault detection:
  Rapid direction changes (oscillation) within a short window trigger fault.
"""

from collections import deque


class SteeringModel:
    RATE_LIMIT_DEG_S = 30.0   # max steering rate deg/s
    MIN_ANGLE = -45.0
    MAX_ANGLE = 45.0

    # Fault detection: if command changes direction N times in T seconds
    OSCILLATION_WINDOW_S = 0.5  # time window
    OSCILLATION_THRESHOLD = 4   # direction changes to trigger fault

    def __init__(self):
        self.actual_angle = 0.0
        self.commanded_angle = 0.0
        self.servo_current_ma = 0
        self.fault = False

        # Oscillation detection
        self._prev_cmd = 0.0
        self._prev_direction = 0  # +1, -1, or 0
        self._direction_changes: deque[float] = deque()  # timestamps
        self._elapsed = 0.0

    def update(self, commanded_deg: float, dt: float):
        """Advance steering by dt seconds."""
        self._elapsed += dt

        self.commanded_angle = max(self.MIN_ANGLE,
                                   min(self.MAX_ANGLE, commanded_deg))

        # Detect direction changes in command
        cmd_delta = self.commanded_angle - self._prev_cmd
        if abs(cmd_delta) > 1.0:  # ignore tiny jitter
            direction = 1 if cmd_delta > 0 else -1
            if self._prev_direction != 0 and direction != self._prev_direction:
                self._direction_changes.append(self._elapsed)
            self._prev_direction = direction
        self._prev_cmd = self.commanded_angle

        # Expire old direction changes outside window
        cutoff = self._elapsed - self.OSCILLATION_WINDOW_S
        while self._direction_changes and self._direction_changes[0] < cutoff:
            self._direction_changes.popleft()

        # Set fault if too many direction changes in window
        if len(self._direction_changes) >= self.OSCILLATION_THRESHOLD:
            self.fault = True

        # Physics
        error = self.commanded_angle - self.actual_angle
        max_step = self.RATE_LIMIT_DEG_S * dt

        if abs(error) <= max_step:
            self.actual_angle = self.commanded_angle
        elif error > 0:
            self.actual_angle += max_step
        else:
            self.actual_angle -= max_step

        # Servo current proportional to effort
        self.servo_current_ma = int(abs(error) * 20.0)
        self.servo_current_ma = min(2550, self.servo_current_ma)

    def clear_fault(self):
        """Clear fault state (called on reset)."""
        self.fault = False
        self._direction_changes.clear()

    @property
    def actual_raw(self) -> int:
        """Raw 16-bit value for DBC: (angle + 45) / 0.01."""
        return int((self.actual_angle + 45.0) / 0.01)

    @property
    def commanded_raw(self) -> int:
        return int((self.commanded_angle + 45.0) / 0.01)
