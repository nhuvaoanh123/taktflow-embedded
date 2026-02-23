"""Lidar distance model — configurable for scenario injection."""


class LidarModel:
    def __init__(self):
        self.distance_cm = 500  # default 5 meters
        self.signal_strength = 8000
        self.fault = False

    def update(self, dt: float):
        """Steady state — only changes via inject methods."""
        pass

    def inject_distance(self, distance_cm: int):
        self.distance_cm = max(0, min(1200, distance_cm))

    def inject_fault(self):
        self.fault = True
        self.signal_strength = 0

    def reset(self):
        self.distance_cm = 500
        self.signal_strength = 8000
        self.fault = False

    @property
    def obstacle_zone(self) -> int:
        """0=emergency(<30cm), 1=braking(<100cm), 2=warning(<300cm), 3=clear."""
        if self.distance_cm < 30:
            return 0
        elif self.distance_cm < 100:
            return 1
        elif self.distance_cm < 300:
            return 2
        return 3
