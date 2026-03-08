#!/usr/bin/env bash
# =============================================================================
# hil-stop.sh — Stop HIL platform Docker services
#
# Usage:  sudo ./scripts/hil-stop.sh
# =============================================================================

set -euo pipefail

echo "=== Taktflow HIL — Stopping Platform ==="

cd "$(dirname "$0")/../docker"
docker compose -f docker-compose.yml -f docker-compose.hil.yml down

echo ""
echo "=== Docker services stopped. Physical ECUs still running on CAN bus. ==="
echo "  To bring down CAN interface:  sudo ip link set can0 down"
