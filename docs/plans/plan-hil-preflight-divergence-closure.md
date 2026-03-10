# Plan â€” HIL Preflight Divergence Closure

**Date**: 2026-03-10  
**Scope**: Close SIL/HIL divergence risks before physical bench demos and recurring HIL execution.

## Objective
Establish a strict preflight gate so hardware runs are blocked when known architecture-divergence risks are present.

## Problem Statement
SIL can remain green while hardware behavior diverges when:
- simulation behavior leaks into MCAL boundaries,
- safety logic differs by platform guards,
- timing constants are Docker-tuned only,
- recovery semantics are inconsistent with relay state contracts.

## Closure Workstreams

### WS1 â€” Safety Path Equivalence
- Remove/contain platform-specific branches in safety-critical logic.
- Enforce policy: no new `PLATFORM_POSIX` guards in safety decision paths.
- Add CI gate for forbidden guards.

### WS2 â€” MCAL Boundary Purity
- Move sensor behavior synthesis and fault injection out of MCAL stubs.
- Keep MCAL adapter role limited to interface and transport abstraction.
- Route fault injection through plant/sensor model APIs.

### WS3 â€” State/Recovery Correctness
- Validate and fix SC relay state interpretation in CVC SAFE_STOP recovery.
- Align comments, DBC semantics, and condition checks.
- Add explicit tests for recovery gate correctness.

### WS4 â€” Actuation Logic Correctness
- Fix brake feedback sequencing to use current-cycle sensor data.
- Rework steering plausibility to compare actuator output vs sensor feedback.
- Add deterministic unit/integration tests for both.

### WS5 â€” Timing and Calibration
- Benchmark heartbeat/grace/timeout constants at 1x hardware timing.
- Create calibration dataset for SC torque-current plausibility LUT.
- Define accepted SIL↔HIL tolerance bands per key signal/verdict.

## Execution Order
1. Blockers first: recovery semantics, E-stop/relay bench proof, safety-path equivalence.
2. Actuation logic fixes and tests.
3. MCAL boundary refactor for simulation/injection split.
4. Timing/LUT calibration and correlation baselines.

## Deliverables
- HIL preflight audit script and nightly workflow.
- Updated tests for recovery/brake/steering logic.
- Documentation updates for state and relay semantics.
- Correlation report template (SIL vs HIL).

## Exit Criteria
- Preflight nightly has no critical findings.
- E-stop bench test demonstrates relay drop and torque collapse within target latency.
- Recovery path tests pass with consistent relay-state semantics.
- Safety logic parity policy is enforced in CI.
- LUT and timeout values are backed by measured hardware data, not placeholders.
