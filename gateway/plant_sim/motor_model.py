"""First-order DC motor model with thermal dynamics.

Physics:
  dw/dt = (torque - friction * w) / inertia
  current = stall_current * (duty / 100) * (1 - w / no_load_rpm)
  dT/dt = (I^2 * R_thermal - (T - T_ambient) / R_cool) / C_thermal
"""

import time


class MotorModel:
    # Motor parameters (775 DC motor, 12V, demo-quality)
    NO_LOAD_RPM = 4000.0
    STALL_CURRENT_MA = 25000.0  # 25A stall
    FRICTION = 0.005            # friction coefficient (Nm/RPM)
    INERTIA = 0.002             # kg*m^2
    R_THERMAL = 0.008           # degC per mA^2 (heating factor)
    R_COOL = 200.0              # thermal resistance degC/W
    C_THERMAL = 50.0            # thermal capacitance J/degC
    T_AMBIENT = 25.0            # ambient temperature degC

    def __init__(self):
        self.rpm = 0.0
        self.current_ma = 0.0
        self.temp_c = self.T_AMBIENT
        self.direction = 0       # 0=stop, 1=fwd, 2=rev
        self.enabled = False
        self.duty_pct = 0.0
        self.overcurrent = False
        self.overtemp = False
        self.stall_fault = False
        self._last_time = time.monotonic()

    def update(self, duty_pct: float, direction: int, dt: float = None):
        """Advance motor physics by dt seconds."""
        now = time.monotonic()
        if dt is None:
            dt = now - self._last_time
        self._last_time = now

        if dt <= 0 or dt > 1.0:
            dt = 0.01

        self.duty_pct = max(0.0, min(100.0, duty_pct))
        self.direction = direction

        if direction == 0 or self.duty_pct < 1.0:
            self.enabled = False
        else:
            self.enabled = True

        # RPM dynamics
        if self.enabled and not self.stall_fault:
            target_rpm = self.NO_LOAD_RPM * (self.duty_pct / 100.0)
            # First-order approach to target
            tau = 0.3  # time constant in seconds
            self.rpm += (target_rpm - self.rpm) * (dt / tau)
        else:
            # Decelerate
            tau = 0.5
            self.rpm *= max(0.0, 1.0 - dt / tau)
            if self.rpm < 1.0:
                self.rpm = 0.0

        # Current model: proportional to torque load
        if self.enabled:
            load_factor = 1.0 - (self.rpm / self.NO_LOAD_RPM)
            load_factor = max(0.0, min(1.0, load_factor))
            self.current_ma = self.STALL_CURRENT_MA * (self.duty_pct / 100.0) * load_factor
        else:
            tau = 0.1
            self.current_ma *= max(0.0, 1.0 - dt / tau)

        # Thermal model
        heat_input = (self.current_ma / 1000.0) ** 2 * self.R_THERMAL
        heat_loss = (self.temp_c - self.T_AMBIENT) / self.R_COOL
        self.temp_c += (heat_input - heat_loss) * dt * 10.0  # speed up for demo

        # Fault detection
        self.overcurrent = self.current_ma > 20000.0
        self.overtemp = self.temp_c > 100.0

    def inject_overcurrent(self, current_ma: float = 28000.0):
        """Inject an overcurrent fault for demo."""
        self.current_ma = current_ma
        self.overcurrent = True

    def inject_stall(self):
        """Inject a stall fault for demo."""
        self.stall_fault = True
        self.rpm = 0.0

    def reset_faults(self):
        """Clear injected faults."""
        self.stall_fault = False
        self.overcurrent = False
        self.overtemp = False

    @property
    def temp_c_int(self) -> int:
        return int(max(-40, min(215, self.temp_c)))

    @property
    def rpm_int(self) -> int:
        return int(max(0, min(10000, self.rpm)))

    @property
    def current_ma_int(self) -> int:
        return int(max(0, min(30000, self.current_ma)))
