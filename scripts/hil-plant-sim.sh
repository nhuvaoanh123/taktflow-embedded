#!/usr/bin/env bash
# =============================================================================
# hil-plant-sim.sh — Run plant simulator on physical CAN bus for HIL
#
# Usage:
#   ./scripts/hil-plant-sim.sh              # foreground (Ctrl+C to stop)
#   ./scripts/hil-plant-sim.sh --daemon     # background with PID file
#   ./scripts/hil-plant-sim.sh --stop       # stop background instance
#
# Prerequisites:
#   - can0 interface up at 500 kbps
#   - pip3 install python-can cantools paho-mqtt
#
# The plant simulator provides closed-loop physics (motor, battery, steering,
# brake, lidar) over CAN — same models used in SIL Docker environment.
# Sends 0x400/0x401 (virtual sensors) consumed by ECU sensor feeders.
# Reads 0x101/0x102/0x103 (actuator commands) from CVC.
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PID_FILE="/tmp/taktflow-plant-sim.pid"

# CAN channel — physical bus for HIL
export CAN_CHANNEL="${CAN_CHANNEL:-can0}"

# DBC path
export DBC_PATH="${DBC_PATH:-$REPO_ROOT/gateway/taktflow.dbc}"

# MQTT — optional, fault injection works only if mosquitto is running
export MQTT_HOST="${MQTT_HOST:-localhost}"
export MQTT_PORT="${MQTT_PORT:-1883}"

stop_daemon() {
    if [ -f "$PID_FILE" ]; then
        pid=$(cat "$PID_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            echo "Stopping plant-sim (PID $pid)..."
            kill "$pid"
            rm -f "$PID_FILE"
            echo "Stopped."
        else
            echo "PID $pid not running. Removing stale PID file."
            rm -f "$PID_FILE"
        fi
    else
        echo "No PID file found. Plant-sim not running as daemon."
    fi
}

case "${1:-}" in
    --stop)
        stop_daemon
        exit 0
        ;;
    --daemon)
        # Check for existing instance
        if [ -f "$PID_FILE" ] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
            echo "Plant-sim already running (PID $(cat "$PID_FILE")). Stop first with --stop."
            exit 1
        fi

        echo "=== Taktflow Plant Simulator (HIL daemon) ==="
        echo "  CAN:  $CAN_CHANNEL"
        echo "  DBC:  $DBC_PATH"
        echo "  MQTT: $MQTT_HOST:$MQTT_PORT"

        cd "$REPO_ROOT/gateway"
        nohup python3 -m plant_sim.simulator > /tmp/taktflow-plant-sim.log 2>&1 &
        echo $! > "$PID_FILE"

        echo "  PID:  $(cat "$PID_FILE")"
        echo "  Log:  /tmp/taktflow-plant-sim.log"
        echo ""
        echo "Stop with: $0 --stop"
        ;;
    "")
        echo "=== Taktflow Plant Simulator (HIL foreground) ==="
        echo "  CAN:  $CAN_CHANNEL"
        echo "  DBC:  $DBC_PATH"
        echo "  MQTT: $MQTT_HOST:$MQTT_PORT"
        echo "  Press Ctrl+C to stop."
        echo ""

        cd "$REPO_ROOT/gateway"
        exec python3 -m plant_sim.simulator
        ;;
    *)
        echo "Usage: $0 [--daemon|--stop]"
        exit 1
        ;;
esac
