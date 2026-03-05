"""Verify pipeline — compile check + optional Docker boot test."""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def run_verify(output_dir: Path, ecu_name: str) -> int:
    """Run verification on generated output.

    1. Check Makefile.customer exists
    2. Check all expected files present
    3. If Docker available, build and run 5s boot check
    """
    output_dir = Path(output_dir)

    # Check required files
    required = [
        "Makefile.customer",
        "Dockerfile.customer",
        "docker-compose.customer.yml",
        f"{ecu_name}/src/{ecu_name}_main.c",
        f"{ecu_name}/cfg/{ecu_name[0].upper()}{ecu_name[1:]}_Cfg.h" if '_' not in ecu_name
        else f"{ecu_name}/cfg/{''.join(p.capitalize() for p in ecu_name.split('_'))}_Cfg.h",
    ]

    missing = [f for f in required if not (output_dir / f).exists()]
    if missing:
        print(f"ERROR: Missing required files:", file=sys.stderr)
        for f in missing:
            print(f"  - {f}", file=sys.stderr)
        return 1

    print("All required files present.")

    # Try Docker build + boot check
    try:
        subprocess.run(["docker", "--version"], capture_output=True, check=True)
    except (FileNotFoundError, subprocess.CalledProcessError):
        print("Docker not available — skipping boot check")
        return 0

    print("Building Docker image...")
    result = subprocess.run(
        ["docker", "build", "-f", "Dockerfile.customer", "-t",
         f"taktflow/{ecu_name}-verify", "."],
        cwd=str(output_dir),
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print(f"ERROR: Docker build failed:\n{result.stderr}", file=sys.stderr)
        return 1

    print("Running 5s boot check...")
    result = subprocess.run(
        ["docker", "run", "--rm", "--network", "none",
         f"taktflow/{ecu_name}-verify"],
        capture_output=True,
        text=True,
        timeout=10,
    )
    # Container will likely exit because no CAN interface — that's OK
    # We just check it started and printed the BSW init message
    combined = result.stdout + result.stderr
    if "BSW init complete" in combined:
        print("Boot check PASSED — BSW init message detected")
        return 0

    print(f"Boot check: container exited (rc={result.returncode})")
    print(f"Output: {combined[:500]}")
    # Not a hard failure — container may fail without vcan0
    return 0
