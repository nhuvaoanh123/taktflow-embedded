"""Simple battery voltage model: V = V_nom - I*R_int."""


class BatteryModel:
    V_NOMINAL_MV = 12600   # 12.6V fully charged
    R_INTERNAL_MOHM = 50   # 50 milliohm internal resistance

    def __init__(self):
        self.voltage_mv = self.V_NOMINAL_MV
        self.soc = 100

    def update(self, motor_current_ma: float, dt: float):
        # Voltage sag under load
        drop_mv = (motor_current_ma / 1000.0) * self.R_INTERNAL_MOHM
        self.voltage_mv = int(self.V_NOMINAL_MV - drop_mv)
        self.voltage_mv = max(0, min(20000, self.voltage_mv))

        # Simple SOC drain (very slow for demo)
        energy_used = motor_current_ma * dt / 3600000.0  # Ah
        self.soc -= energy_used * 10.0  # exaggerated for demo
        self.soc = max(0, min(100, self.soc))

    @property
    def status(self) -> int:
        """0=critical_UV, 1=UV_warn, 2=normal, 3=OV_warn, 4=critical_OV."""
        if self.voltage_mv < 9000:
            return 0
        elif self.voltage_mv < 10500:
            return 1
        elif self.voltage_mv > 15000:
            return 4
        elif self.voltage_mv > 14000:
            return 3
        return 2
