"""Tests for OfflineBuffer."""

import pytest
from cloud_connector.buffer import OfflineBuffer, BufferedMessage


def test_push_and_drain():
    buf = OfflineBuffer(maxlen=10)
    buf.push("vehicle/telemetry", b'{"rpm":1000}', qos=1)
    buf.push("vehicle/dtc/new", b'{"dtc":"0xE301"}', qos=1)

    assert buf.count == 2
    assert not buf.is_empty

    msgs = buf.drain()
    assert len(msgs) == 2
    assert msgs[0].topic == "vehicle/telemetry"
    assert msgs[1].topic == "vehicle/dtc/new"
    assert buf.is_empty


def test_drain_empty():
    buf = OfflineBuffer()
    msgs = buf.drain()
    assert msgs == []
    assert buf.is_empty


def test_maxlen_drops_oldest():
    buf = OfflineBuffer(maxlen=3)
    for i in range(5):
        buf.push(f"topic/{i}", f"msg-{i}".encode(), qos=0)

    assert buf.count == 3
    msgs = buf.drain()
    # Oldest (0, 1) should have been dropped
    assert msgs[0].topic == "topic/2"
    assert msgs[1].topic == "topic/3"
    assert msgs[2].topic == "topic/4"


def test_buffered_message_fields():
    msg = BufferedMessage(topic="test/topic", payload=b"hello", qos=1)
    assert msg.topic == "test/topic"
    assert msg.payload == b"hello"
    assert msg.qos == 1
