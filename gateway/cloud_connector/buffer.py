"""Offline message buffer — queues messages when AWS IoT Core is unreachable.

Uses a bounded deque so memory stays capped even during extended outages.
Messages are drained FIFO on reconnect.
"""

from __future__ import annotations

import logging
from collections import deque
from dataclasses import dataclass

logger = logging.getLogger(__name__)

MAX_BUFFER_SIZE = 100


@dataclass
class BufferedMessage:
    topic: str
    payload: bytes
    qos: int = 1


class OfflineBuffer:
    """Thread-safe(ish) bounded FIFO for offline messages."""

    def __init__(self, maxlen: int = MAX_BUFFER_SIZE) -> None:
        self._queue: deque[BufferedMessage] = deque(maxlen=maxlen)

    def push(self, topic: str, payload: bytes, qos: int = 1) -> None:
        """Add a message to the buffer. Oldest dropped if full."""
        was_full = len(self._queue) == self._queue.maxlen
        self._queue.append(BufferedMessage(topic, payload, qos))
        if was_full:
            logger.warning("Buffer full (%d) — oldest message dropped", self._queue.maxlen)

    def drain(self) -> list[BufferedMessage]:
        """Return all buffered messages and clear the queue."""
        msgs = list(self._queue)
        self._queue.clear()
        if msgs:
            logger.info("Draining %d buffered messages", len(msgs))
        return msgs

    @property
    def count(self) -> int:
        return len(self._queue)

    @property
    def is_empty(self) -> bool:
        return len(self._queue) == 0
