"""Steering servo model — rate-limited tracking of commanded angle.

Physics:
  angle += clamp(cmd - actual, -rate_limit, +rate_limit) * dt
"""


class SteeringModel:
    RATE_LIMIT_DEG_S = 30.0   # max steering rate deg/s
    MIN_ANGLE = -45.0
    MAX_ANGLE = 45.0

    def __init__(self):
        self.actual_angle = 0.0
        self.commanded_angle = 0.0
        self.servo_current_ma = 0
        self.fault = False

    def update(self, commanded_deg: float, dt: float):
        """Advance steering by dt seconds."""
        self.commanded_angle = max(self.MIN_ANGLE,
                                   min(self.MAX_ANGLE, commanded_deg))

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

    @property
    def actual_raw(self) -> int:
        """Raw 16-bit value for DBC: (angle + 45) / 0.01."""
        return int((self.actual_angle + 45.0) / 0.01)

    @property
    def commanded_raw(self) -> int:
        return int((self.commanded_angle + 45.0) / 0.01)
