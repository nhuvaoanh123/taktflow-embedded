#!/usr/bin/env bash
# =============================================================================
# pil-start.sh — Start PIL platform (3 Docker ECUs + gateway on physical CAN)
#
# Requires:
#   - USB-CAN adapter plugged in (CANable, PCAN-USB, Waveshare HAT, etc.)
#   - Physical ECUs (CVC, FZC, RZC, SC) powered and on the CAN bus
#   - Docker and Docker Compose installed
#
# Usage:  sudo ./scripts/pil-start.sh
# =============================================================================

set -euo pipefail

echo "=== Taktflow PIL — Starting Processor-in-the-Loop Platform ==="
echo ""

# --- Bring up physical CAN interface ---
echo "[1/4] Setting up physical CAN interface (can0)..."

# Detect CAN adapter type
if ip link show can0 > /dev/null 2>&1; then
    echo "  can0 already exists"
elif lsmod | grep -q gs_usb 2>/dev/null || lsmod | grep -q peak_usb 2>/dev/null; then
    echo "  USB-CAN driver loaded, creating can0..."
    ip link set can0 type can bitrate 500000
else
    # Try loading common USB-CAN drivers
    modprobe gs_usb 2>/dev/null || modprobe peak_usb 2>/dev/null || true

    if ! ip link show can0 > /dev/null 2>&1; then
        echo "  ERROR: No CAN adapter detected."
        echo "  Plug in a USB-CAN adapter and try again."
        echo "  Supported: CANable (gs_usb), PCAN-USB (peak_usb), MCP2515 HAT"
        exit 1
    fi

    ip link set can0 type can bitrate 500000
fi

ip link set can0 up
echo "  can0 is UP at 500 kbps"

# --- Quick CAN bus check ---
echo ""
echo "[2/4] Checking CAN bus for physical ECU traffic..."
echo "  Listening for 2 seconds..."

FRAMES=$(timeout 2 candump can0 2>/dev/null | head -20 || true)
if [ -z "$FRAMES" ]; then
    echo "  WARNING: No CAN frames detected. Are physical ECUs powered on?"
    echo "  Continuing anyway (ECUs may still be booting)..."
else
    FRAME_COUNT=$(echo "$FRAMES" | wc -l)
    echo "  Detected $FRAME_COUNT frames in 2s — physical ECUs are transmitting"
fi

# --- Start Docker services ---
echo ""
echo "[3/4] Starting Docker services (BCM, ICU, TCU + gateway)..."
cd "$(dirname "$0")/../docker"
docker compose -f docker-compose.yml -f docker-compose.pil.yml up --build -d

# --- Status report ---
echo ""
echo "[4/4] PIL platform status:"
echo ""
docker compose -f docker-compose.yml -f docker-compose.pil.yml ps --format "table {{.Name}}\t{{.Status}}\t{{.Ports}}"

echo ""
echo "=== PIL Platform Running ==="
echo ""
echo "  Physical ECUs:  CVC, FZC, RZC, SC (on CAN bus)"
echo "  Docker ECUs:    BCM, ICU, TCU (on can0)"
echo "  Gateway:        MQTT + CAN-GW + WS-Bridge + ML + Fault-Inject"
echo ""
echo "  Monitor CAN:    candump can0"
echo "  View logs:      docker compose -f docker-compose.yml -f docker-compose.pil.yml logs -f"
echo "  Plant-sim:      docker compose -f docker-compose.yml -f docker-compose.pil.yml logs -f plant-sim"
echo "  Stop:           sudo ./scripts/pil-stop.sh"
echo ""
