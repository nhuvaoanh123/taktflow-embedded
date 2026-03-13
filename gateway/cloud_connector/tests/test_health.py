"""Tests for HealthReporter."""

import json
from unittest.mock import MagicMock

from cloud_connector.health import HealthReporter, HEALTH_TOPIC


def test_health_publishes_status():
    mock_client = MagicMock()
    health = HealthReporter(mock_client, device_id="test-device")
    health._last_publish = 0.0  # force immediate publish

    health.maybe_publish()

    mock_client.publish.assert_called_once()
    call_args = mock_client.publish.call_args
    assert call_args[0][0] == HEALTH_TOPIC

    payload = json.loads(call_args[0][1])
    assert payload["device_id"] == "test-device"
    assert payload["cloud_connected"] is False
    assert payload["msgs_forwarded"] == 0


def test_health_tracks_forwards():
    mock_client = MagicMock()
    health = HealthReporter(mock_client, device_id="test-device")

    health.record_forward()
    health.record_forward()
    health.record_forward()

    assert health._msgs_forwarded == 3
    assert health._last_forward_ts > 0


def test_health_cloud_connected_state():
    mock_client = MagicMock()
    health = HealthReporter(mock_client, device_id="test-device")

    assert health._cloud_connected is False
    health.set_cloud_connected(True)
    assert health._cloud_connected is True


def test_health_respects_interval():
    """Should not publish if interval hasn't elapsed."""
    mock_client = MagicMock()
    health = HealthReporter(mock_client, device_id="test-device")

    # First call: _last_publish defaults to 0.0, so it will publish
    health.maybe_publish()
    assert mock_client.publish.call_count == 1

    # Second call: interval hasn't elapsed
    health.maybe_publish()
    assert mock_client.publish.call_count == 1  # still 1
