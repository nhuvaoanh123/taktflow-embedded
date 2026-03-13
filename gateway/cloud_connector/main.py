"""Cloud Connector entry point — local MQTT → AWS IoT Core bridge.

Reads configuration from environment variables:
    MQTT_HOST         Local broker host (default: localhost)
    MQTT_PORT         Local broker port (default: 1883)
    AWS_IOT_ENDPOINT  AWS IoT Core endpoint (e.g., xxx.iot.eu-central-1.amazonaws.com)
    AWS_IOT_PORT      AWS IoT Core port (default: 8883)
    AWS_CERT_DIR      Directory containing X.509 certs (default: /certs)
    DEVICE_ID         IoT Thing name (default: taktflow-pi-001)

If AWS_IOT_ENDPOINT is empty, runs in local-only mode (health reporting only).
"""

import logging
import os

from .bridge import CloudBridge


def main() -> None:
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(name)s — %(message)s",
    )

    bridge = CloudBridge(
        local_host=os.getenv("MQTT_HOST", "localhost"),
        local_port=int(os.getenv("MQTT_PORT", "1883")),
        aws_endpoint=os.getenv("AWS_IOT_ENDPOINT", ""),
        aws_port=int(os.getenv("AWS_IOT_PORT", "8883")),
        cert_dir=os.getenv("AWS_CERT_DIR", "/certs"),
        device_id=os.getenv("DEVICE_ID", "taktflow-pi-001"),
    )
    bridge.run_forever()


if __name__ == "__main__":
    main()
