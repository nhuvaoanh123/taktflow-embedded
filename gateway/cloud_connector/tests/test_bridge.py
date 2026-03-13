"""Tests for CloudBridge topic filtering and message forwarding."""

from unittest.mock import MagicMock, patch, PropertyMock
import paho.mqtt.client as mqtt

from cloud_connector.bridge import CloudBridge, FORWARDED_TOPICS


def _make_bridge(**kwargs):
    """Create a CloudBridge with mocked MQTT clients."""
    defaults = {
        "local_host": "localhost",
        "local_port": 1883,
        "aws_endpoint": "",
        "aws_port": 8883,
        "cert_dir": "/tmp/certs",
        "device_id": "test-device",
    }
    defaults.update(kwargs)
    return CloudBridge(**defaults)


def test_forwarded_topics_are_budget_safe():
    """Only low-rate aggregate topics should be forwarded."""
    topics = [t for t, _ in FORWARDED_TOPICS]
    assert "vehicle/telemetry" in topics
    assert "vehicle/dtc/new" in topics
    assert "vehicle/alerts" in topics
    # High-rate per-signal topics must NOT be forwarded
    assert "taktflow/can/Motor_Current/Current_mA" not in topics


def test_on_local_message_buffers_when_aws_offline():
    bridge = _make_bridge()
    bridge._aws_connected = False

    # Simulate a local message
    msg = MagicMock()
    msg.topic = "vehicle/telemetry"
    msg.payload = b'{"rpm":1000}'
    msg.qos = 1

    bridge._on_local_message(None, None, msg)

    assert bridge._buffer.count == 1


def test_on_local_message_forwards_when_aws_online():
    bridge = _make_bridge(aws_endpoint="test.iot.amazonaws.com")
    bridge._aws_connected = True

    # Mock the AWS client publish
    mock_result = MagicMock()
    mock_result.rc = mqtt.MQTT_ERR_SUCCESS
    bridge._aws.publish = MagicMock(return_value=mock_result)

    msg = MagicMock()
    msg.topic = "vehicle/telemetry"
    msg.payload = b'{"rpm":1000}'
    msg.qos = 1

    bridge._on_local_message(None, None, msg)

    bridge._aws.publish.assert_called_once_with("vehicle/telemetry", b'{"rpm":1000}', qos=1)
    assert bridge._buffer.count == 0


def test_on_aws_connect_drains_buffer():
    bridge = _make_bridge(aws_endpoint="test.iot.amazonaws.com")

    # Pre-fill buffer
    bridge._buffer.push("vehicle/telemetry", b'msg1', qos=1)
    bridge._buffer.push("vehicle/dtc/new", b'msg2', qos=1)
    assert bridge._buffer.count == 2

    # Mock AWS client publish
    bridge._aws.publish = MagicMock()

    # Simulate AWS connect callback
    bridge._on_aws_connect(bridge._aws, None, None, 0, None)

    assert bridge._aws_connected is True
    assert bridge._buffer.count == 0
    assert bridge._aws.publish.call_count == 2


def test_on_aws_disconnect_sets_offline():
    bridge = _make_bridge(aws_endpoint="test.iot.amazonaws.com")
    bridge._aws_connected = True

    bridge._on_aws_disconnect(bridge._aws, None, None, 1, None)

    assert bridge._aws_connected is False
