"""MQTT publisher — publishes decoded CAN signals to MQTT topics.

Topic structure:
    taktflow/can/{MessageName}/{SignalName}  -> value
    taktflow/telemetry/stats/can_msgs_per_sec -> rate
    taktflow/alerts/dtc/{dtc_code}           -> { json }
"""

import json
import logging
import time

import paho.mqtt.client as mqtt

log = logging.getLogger("can_gateway.mqtt")

TOPIC_PREFIX = "taktflow"

# DTC-related signals from DTC_Broadcast (0x500 = 1280)
DTC_MSG_NAME = "DTC_Broadcast"


class MqttPublisher:
    """Publishes decoded CAN signals to MQTT broker."""

    def __init__(self, host: str = "localhost", port: int = 1883):
        self.host = host
        self.port = port
        self._client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id="can-gateway",
        )
        self._connected = False

        # Stats
        self._msg_count = 0
        self._last_stats_time = time.monotonic()
        self._msgs_per_sec = 0.0

        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect

    def _on_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            self._connected = True
            log.info("Connected to MQTT broker at %s:%d", self.host, self.port)
        else:
            log.error("MQTT connect failed: rc=%d", rc)

    def _on_disconnect(self, client, userdata, flags, rc, properties=None):
        self._connected = False
        log.warning("Disconnected from MQTT broker (rc=%d)", rc)

    def connect(self):
        """Connect to the MQTT broker with auto-reconnect."""
        self._client.connect_async(self.host, self.port, keepalive=30)
        self._client.loop_start()

    def stop(self):
        """Disconnect and stop the MQTT loop."""
        self._client.loop_stop()
        self._client.disconnect()

    def publish_signals(self, msg_name: str, signals: dict):
        """Publish each signal as a separate MQTT topic."""
        if not self._connected:
            return

        for signal_name, value in signals.items():
            topic = f"{TOPIC_PREFIX}/can/{msg_name}/{signal_name}"
            # Use str for numeric values to keep them lightweight
            payload = str(value) if not isinstance(value, (dict, list)) else json.dumps(value)
            self._client.publish(topic, payload, qos=0, retain=True)

        self._msg_count += 1

        # Check for DTC broadcast — publish as alert
        if msg_name == DTC_MSG_NAME:
            self._publish_dtc_alert(signals)

    def _publish_dtc_alert(self, signals: dict):
        """Publish a DTC alert with structured JSON."""
        dtc_number = signals.get("DTC_Number", 0)
        if dtc_number == 0:
            return

        alert = {
            "dtc": f"0x{int(dtc_number):04X}",
            "status": int(signals.get("DTC_Status", 0)),
            "ecu_source": int(signals.get("ECU_Source", 0)),
            "occurrence": int(signals.get("OccurrenceCount", 0)),
            "freeze_frame": [
                int(signals.get("FreezeFrame0", 0)),
                int(signals.get("FreezeFrame1", 0)),
                int(signals.get("FreezeFrame2", 0)),
            ],
            "ts": time.time(),
        }
        topic = f"{TOPIC_PREFIX}/alerts/dtc/{alert['dtc']}"
        self._client.publish(topic, json.dumps(alert), qos=1)

    def publish_stats(self):
        """Publish CAN message rate stats. Called periodically."""
        now = time.monotonic()
        elapsed = now - self._last_stats_time
        if elapsed >= 1.0:
            self._msgs_per_sec = self._msg_count / elapsed
            self._msg_count = 0
            self._last_stats_time = now

            if self._connected:
                self._client.publish(
                    f"{TOPIC_PREFIX}/telemetry/stats/can_msgs_per_sec",
                    f"{self._msgs_per_sec:.0f}",
                    qos=0,
                    retain=True,
                )

    @property
    def msgs_per_sec(self) -> float:
        return self._msgs_per_sec
