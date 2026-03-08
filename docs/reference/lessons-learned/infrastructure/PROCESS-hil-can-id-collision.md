# Lessons Learned — HIL CAN ID Collision & BCM Interface Binding

**Date:** 2026-03-08
**Context:** HIL integration audit — preparing Docker ECUs (BCM/ICU/TCU) + plant-sim to share physical CAN bus (can0) with 4 real hardware ECUs (CVC, FZC, RZC, SC)

## Issue 1: CAN ID Collision Between Plant-Sim and Physical ECUs / BCM

**Mistake:** Plant-sim unconditionally transmits on CAN IDs that are also owned by other ECUs:

| CAN ID | Plant-sim TX       | Also TX by         | Impact                          |
|--------|--------------------|---------------------|---------------------------------|
| 0x400  | FZC Virtual Sensors| BCM (Light Status)  | ICU receives corrupted data     |
| 0x401  | RZC Virtual Sensors| BCM (Indicator)     | ICU receives corrupted data     |

Note: Plant-sim also has dead-code TX methods for 0x100 (Vehicle State), 0x200 (Steering Status), 0x201 (Brake Status) that would collide with CVC and FZC if ever re-enabled.

In SIL, plant-sim is the sole source of truth (it simulates the physical world). In HIL, real ECUs and BCM are the real sources — plant-sim's messages collide on the shared CAN bus.

The 0x400/0x401 collision is especially insidious: plant-sim sends virtual sensor injection data (steering angles, motor current) on IDs that BCM uses for body control status (light/indicator state). ICU subscribes to 0x400/0x401 expecting BCM body status but receives interleaved plant-sim sensor data — completely different signal layouts.

**Fix:** Add `HIL_MODE` environment variable to plant-sim. When `HIL_MODE=1`:
- Skip TX of 0x400, 0x401 (not needed — physical ECUs have real sensors, and BCM owns these IDs for body status)
- Keep TX of 0x220 (lidar — no physical lidar sensor) and 0x500 (DTC broadcast — fault injection)
- Plant-sim still runs physics models and monitors bus traffic for dashboard/MQTT

**Principle:** When the same CAN IDs are used for different purposes in SIL vs HIL, add explicit mode gating. Never assume "the receiver will ignore it" — CAN has no sender identification, so any message on a given ID is indistinguishable to the receiver. Design CAN ID allocation with all deployment modes in mind (SIL, HIL, production).

## Issue 2: BCM SocketCAN Hardcoded Interface (No CAN_INTERFACE Support)

**Mistake:** BCM uses its own SocketCAN implementation (`Swc_BcmCan.c`) instead of the shared `Can_Posix.c`. It binds with a zeroed `sockaddr_can` (ifindex=0, meaning "any CAN interface") and does NOT read the `CAN_INTERFACE` environment variable. The `docker-compose.hil.yml` sets `CAN_INTERFACE=can0` for BCM, but BCM ignores it.

In HIL, ifindex=0 might happen to bind to can0 if it's the only CAN interface, but this is fragile — if both vcan0 and can0 exist, the behavior is undefined.

**Fix:** Add `CAN_INTERFACE` environment variable support to `Swc_BcmCan.c`, using `if_nametoindex()` to resolve the interface name to an ifindex, matching what `Can_Posix.c` already does. Fall back to ifindex=0 (any) if the env var is not set.

**Principle:** All CAN-capable modules in Docker ECUs must respect the same `CAN_INTERFACE` environment variable for HIL/SIL switching. Don't have one module use a shared driver with env var support while another hardcodes the interface — it creates a silent deployment mismatch that only manifests in HIL.
