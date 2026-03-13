"""Health reporter — publishes cloud connector status to local MQTT.

Status message published every 30s to taktflow/cloud/status so the
local dashboard and monitoring can see cloud connectivity state.
"""

from __future__ import annotations

import json
import logging
import time

import paho.mqtt.client as mqtt

logger = logging.getLogger(__name__)

HEALTH_TOPIC = "taktflow/cloud/status"
HEALTH_INTERVAL_S = 30.0


class HealthReporter:
    """Publishes periodic health status to local MQTT broker."""

    def __init__(self, local_client: mqtt.Client, device_id: str) -> None:
        self._client = local_client
        self._device_id = device_id
        self._last_publish = 0.0
        self._cloud_connected = False
        self._msgs_forwarded = 0
        self._msgs_buffered = 0
        self._last_forward_ts = 0.0

    def set_cloud_connected(self, connected: bool) -> None:
        self._cloud_connected = connected

    def record_forward(self) -> None:
        self._msgs_forwarded += 1
        self._last_forward_ts = time.time()

    def set_buffered_count(self, count: int) -> None:
        self._msgs_buffered = count

    def maybe_publish(self) -> None:
        """Publish health status if interval has elapsed."""
        now = time.monotonic()
        if (now - self._last_publish) < HEALTH_INTERVAL_S:
            return
        self._last_publish = now

        status = {
            "device_id": self._device_id,
            "cloud_connected": self._cloud_connected,
            "msgs_forwarded": self._msgs_forwarded,
            "msgs_buffered": self._msgs_buffered,
            "last_forward_ts": self._last_forward_ts,
            "uptime_s": now,
            "ts": time.time(),
        }
        self._client.publish(HEALTH_TOPIC, json.dumps(status), qos=0, retain=True)
        logger.debug("Health: cloud=%s fwd=%d buf=%d",
                      self._cloud_connected, self._msgs_forwarded, self._msgs_buffered)
