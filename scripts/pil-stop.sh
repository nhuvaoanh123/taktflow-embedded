#!/usr/bin/env bash
# =============================================================================
# pil-stop.sh — Stop PIL platform Docker services
#
# Usage:  sudo ./scripts/pil-stop.sh
# =============================================================================

set -euo pipefail

echo "=== Taktflow PIL — Stopping Platform ==="

cd "$(dirname "$0")/../docker"
docker compose -f docker-compose.yml -f docker-compose.pil.yml down

echo ""
echo "=== Docker services stopped. Physical ECUs still running on CAN bus. ==="
echo "  To bring down CAN interface:  sudo ip link set can0 down"
