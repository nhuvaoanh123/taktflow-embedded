"""CloudBridge — two paho-mqtt clients bridging local MQTT to AWS IoT Core.

Local client subscribes to selected topics. On each message, the AWS client
publishes to the same topic on IoT Core. If the AWS connection is down,
messages are buffered in an OfflineBuffer and drained on reconnect.

No AWS SDK needed — IoT Core is just MQTT 3.1.1 over TLS with X.509 mutual auth.
"""

from __future__ import annotations

import json
import logging
import os
import time
from pathlib import Path

import paho.mqtt.client as mqtt

from .buffer import OfflineBuffer
from .health import HealthReporter

logger = logging.getLogger(__name__)

# Topics forwarded to cloud — minimal set, well within free tier.
# telemetry: 1 msg every 5s = ~520K/month
# dtc: on-event only = ~100/month
FORWARDED_TOPICS = [
    ("vehicle/telemetry", 1),
    ("vehicle/dtc/new", 1),
]


class CloudBridge:
    """Bridges local MQTT broker to AWS IoT Core via TLS+X.509."""

    def __init__(
        self,
        local_host: str = "localhost",
        local_port: int = 1883,
        aws_endpoint: str = "",
        aws_port: int = 8883,
        cert_dir: str = "/certs",
        device_id: str = "taktflow-pi-001",
    ) -> None:
        self._local_host = local_host
        self._local_port = local_port
        self._aws_endpoint = aws_endpoint
        self._aws_port = aws_port
        self._cert_dir = Path(cert_dir)
        self._device_id = device_id

        self._buffer = OfflineBuffer()
        self._aws_connected = False
        self._dtc_last_forwarded: dict[str, float] = {}

        # --- Local MQTT client ---
        self._local = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id="cloud-connector-local",
        )
        self._local.on_connect = self._on_local_connect
        self._local.on_message = self._on_local_message

        # --- AWS IoT Core MQTT client ---
        self._aws = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id=device_id,
        )
        self._aws.on_connect = self._on_aws_connect
        self._aws.on_disconnect = self._on_aws_disconnect

        # --- Health reporter ---
        self._health = HealthReporter(self._local, device_id)

    def _setup_aws_tls(self) -> bool:
        """Configure TLS for AWS IoT Core using X.509 certificates."""
        ca_path = self._cert_dir / "AmazonRootCA1.pem"
        cert_path = self._cert_dir / "certificate.pem.crt"
        key_path = self._cert_dir / "private.pem.key"

        for path in [ca_path, cert_path, key_path]:
            if not path.exists():
                logger.error("Missing cert file: %s", path)
                return False

        self._aws.tls_set(
            ca_certs=str(ca_path),
            certfile=str(cert_path),
            keyfile=str(key_path),
        )
        logger.info("TLS configured: ca=%s cert=%s", ca_path.name, cert_path.name)
        return True

    # --- Local MQTT callbacks ---

    def _on_local_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            logger.info("Connected to local MQTT broker %s:%d",
                        self._local_host, self._local_port)
            for topic, qos in FORWARDED_TOPICS:
                client.subscribe(topic, qos=qos)
                logger.info("  subscribed → %s (qos=%d)", topic, qos)
        else:
            logger.error("Local MQTT connect failed: rc=%s", rc)

    def _should_forward(self, msg: mqtt.MQTTMessage) -> bool:
        """Rate-limit DTCs — same code only forwarded once per hour."""
        if msg.topic == "vehicle/dtc/new":
            try:
                data = json.loads(msg.payload)
                dtc_code = data.get("dtc", "")
                now = time.time()
                last = self._dtc_last_forwarded.get(dtc_code, 0)
                if now - last < 3600:
                    return False
                self._dtc_last_forwarded[dtc_code] = now
            except (json.JSONDecodeError, TypeError):
                pass
        return True

    def _on_local_message(self, client, userdata, msg: mqtt.MQTTMessage):
        """Forward received local message to AWS IoT Core (or buffer)."""
        if not self._should_forward(msg):
            return

        if self._aws_connected:
            result = self._aws.publish(msg.topic, msg.payload, qos=msg.qos)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                self._health.record_forward()
            else:
                logger.warning("AWS publish failed (rc=%d), buffering", result.rc)
                self._buffer.push(msg.topic, msg.payload, msg.qos)
        else:
            self._buffer.push(msg.topic, msg.payload, msg.qos)

        self._health.set_buffered_count(self._buffer.count)

    # --- AWS IoT Core callbacks ---

    def _on_aws_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            self._aws_connected = True
            self._health.set_cloud_connected(True)
            logger.info("Connected to AWS IoT Core at %s:%d",
                        self._aws_endpoint, self._aws_port)
            # Drain offline buffer
            for msg in self._buffer.drain():
                client.publish(msg.topic, msg.payload, qos=msg.qos)
                self._health.record_forward()
            self._health.set_buffered_count(0)
        else:
            logger.error("AWS IoT Core connect failed: rc=%s", rc)

    def _on_aws_disconnect(self, client, userdata, flags, rc, properties=None):
        self._aws_connected = False
        self._health.set_cloud_connected(False)
        logger.warning("Disconnected from AWS IoT Core (rc=%d) — buffering", rc)

    # --- Main run ---

    def start(self) -> None:
        """Connect both MQTT clients and start network loops."""
        # Connect to local broker (retry until available)
        logger.info("Connecting to local MQTT broker %s:%d ...",
                     self._local_host, self._local_port)
        for attempt in range(30):
            try:
                self._local.connect(self._local_host, self._local_port, keepalive=30)
                break
            except (ConnectionRefusedError, OSError) as exc:
                logger.warning("Local MQTT attempt %d: %s", attempt + 1, exc)
                time.sleep(1)
        else:
            logger.error("Local MQTT connection failed after 30 attempts")
            return
        self._local.loop_start()

        # Connect to AWS IoT Core (if endpoint configured)
        if self._aws_endpoint:
            if self._setup_aws_tls():
                self._aws.reconnect_delay_set(min_delay=1, max_delay=60)
                try:
                    self._aws.connect_async(self._aws_endpoint, self._aws_port, keepalive=60)
                    self._aws.loop_start()
                    logger.info("AWS IoT Core connection initiated")
                except Exception as exc:
                    logger.error("AWS IoT Core connect error: %s", exc)
            else:
                logger.warning("AWS TLS setup failed — running in local-only mode")
        else:
            logger.warning("AWS_IOT_ENDPOINT not set — running in local-only mode")

    def run_forever(self) -> None:
        """Block forever, publishing health status periodically."""
        self.start()
        try:
            while True:
                self._health.maybe_publish()
                time.sleep(1)
        except KeyboardInterrupt:
            logger.info("Shutting down cloud connector")
        finally:
            self.stop()

    def stop(self) -> None:
        """Disconnect both clients."""
        self._local.loop_stop()
        self._local.disconnect()
        if self._aws_endpoint:
            self._aws.loop_stop()
            self._aws.disconnect()
        logger.info("Cloud connector stopped")
