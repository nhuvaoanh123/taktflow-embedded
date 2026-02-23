"""Brake servo model — rate-limited tracking of commanded force."""


class BrakeModel:
    RATE_LIMIT_PCT_S = 200.0  # can go 0-100% in 0.5s

    def __init__(self):
        self.actual_pct = 0.0
        self.commanded_pct = 0.0
        self.servo_current_ma = 0
        self.fault = False

    def update(self, commanded_pct: float, dt: float):
        self.commanded_pct = max(0.0, min(100.0, commanded_pct))

        error = self.commanded_pct - self.actual_pct
        max_step = self.RATE_LIMIT_PCT_S * dt

        if abs(error) <= max_step:
            self.actual_pct = self.commanded_pct
        elif error > 0:
            self.actual_pct += max_step
        else:
            self.actual_pct -= max_step

        self.servo_current_ma = int(abs(error) * 15.0)
        self.servo_current_ma = min(3000, self.servo_current_ma)

    @property
    def position_int(self) -> int:
        return int(max(0, min(100, self.actual_pct)))
