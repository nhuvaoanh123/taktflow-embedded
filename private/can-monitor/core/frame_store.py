"""Thread-safe CAN frame storage with ring buffer and signal history."""
import threading
import time
from collections import deque
from dataclasses import dataclass, field
from typing import Optional

# E2E signal names excluded from history tracking
_E2E_SIGNALS = {"E2E_DataID", "E2E_AliveCounter", "E2E_CRC8"}


@dataclass
class ParsedFrame:
    timestamp: float
    can_id: int
    dlc: int
    data: bytes
    msg_name: str = ""
    signals: dict = field(default_factory=dict)
    raw_signals: dict = field(default_factory=dict)  # numeric values (no enum strings)
    sender: str = ""
    receivers: str = "-"
    asil: str = "QM"
    cycle_ms: int = 0
    checksum_ok: bool = True


class MessageState:
    __slots__ = ("latest", "count", "first_seen", "last_seen")

    def __init__(self):
        self.latest: Optional[ParsedFrame] = None
        self.count: int = 0
        self.first_seen: float = 0.0
        self.last_seen: float = 0.0

    @property
    def rate(self):
        elapsed = self.last_seen - self.first_seen
        if elapsed > 0 and self.count > 1:
            return (self.count - 1) / elapsed
        return 0.0


TRACE_MAX = 10_000
SIGNAL_HISTORY_MAX = 2_000
MESSAGE_STALE_SEC = 1.0


class FrameStore:
    def __init__(self):
        self._lock = threading.Lock()
        self.trace = deque(maxlen=TRACE_MAX)
        self.messages = {}  # can_id -> MessageState
        self.signal_history = {}  # "MsgName.SignalName" -> deque of (timestamp, value)
        self.total_frames = 0
        self.error_frames = 0
        self.t0 = time.monotonic()

    def _prune_stale_messages(self, now_ts: float):
        stale_ids = [
            cid for cid, ms in self.messages.items()
            if (now_ts - ms.last_seen) > MESSAGE_STALE_SEC
        ]
        for cid in stale_ids:
            del self.messages[cid]

    def add_frame(self, frame: ParsedFrame):
        with self._lock:
            self.trace.append(frame)
            self.total_frames += 1
            if not frame.checksum_ok:
                self.error_frames += 1
            # Update message table
            cid = frame.can_id
            if cid not in self.messages:
                self.messages[cid] = MessageState()
                self.messages[cid].first_seen = frame.timestamp
            ms = self.messages[cid]
            ms.latest = frame
            ms.count += 1
            ms.last_seen = frame.timestamp
            self._prune_stale_messages(frame.timestamp)
            # Update signal history using raw_signals (always numeric, no enum strings)
            plot_signals = frame.raw_signals if frame.raw_signals else frame.signals
            for sig_name, value in plot_signals.items():
                if sig_name in _E2E_SIGNALS:
                    continue
                if not isinstance(value, (int, float)):
                    continue
                key = f"{frame.msg_name}.{sig_name}"
                if key not in self.signal_history:
                    self.signal_history[key] = deque(maxlen=SIGNAL_HISTORY_MAX)
                self.signal_history[key].append((frame.timestamp, float(value)))

    def get_trace_snapshot(self, last_n=500):
        with self._lock:
            return list(self.trace)[-last_n:]

    def get_messages_snapshot(self):
        with self._lock:
            now_ts = time.monotonic() - self.t0
            self._prune_stale_messages(now_ts)
            result = {}
            for cid, ms in self.messages.items():
                result[cid] = {
                    "latest": ms.latest,
                    "count": ms.count,
                    "rate": ms.rate,
                    "first_seen": ms.first_seen,
                    "last_seen": ms.last_seen,
                }
            return result

    def get_signal_data(self, key):
        """Return list of (timestamp, value) for a signal key."""
        with self._lock:
            if key in self.signal_history:
                return list(self.signal_history[key])
            return []

    def get_signal_keys(self):
        with self._lock:
            return sorted(self.signal_history.keys())

    def get_stats(self):
        with self._lock:
            elapsed = time.monotonic() - self.t0
            self._prune_stale_messages(elapsed)
            return {
                "total": self.total_frames,
                "errors": self.error_frames,
                "elapsed": elapsed,
                "fps": self.total_frames / max(elapsed, 0.001),
                "msg_count": len(self.messages),
            }
