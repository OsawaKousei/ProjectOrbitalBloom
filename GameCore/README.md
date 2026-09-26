# Orbital Bloom GameCore

Unreal-independent C++17 simulation. Units are metres, seconds and radians;
X is forward, Y right and Z up. The bridge converts positions to Unreal centimetres.
`Simulation` is authoritative. The host submits commands and two-axis input, then
reads the immutable `RenderSnapshot`; render actors never feed positions back.

## Test without Unreal or CMake

From the repository root:

```sh
bash GameCore/test.sh
```

Requires a C++17 compiler (`g++` by default, or set `CXX`). Temporary output goes to
`/tmp` and is removed on exit. No downloaded test framework is required.

If CMake/CTest is installed:

```sh
cmake -S GameCore -B /tmp/orbital-core-build
cmake --build /tmp/orbital-core-build
ctest --test-dir /tmp/orbital-core-build --output-on-failure
```

## Current behavior

- Fixed 120 Hz or 60 Hz; game time is derived from integer ticks.
- Host elapsed time is consumed in full. Host must submit elapsed intervals, not
  wall-clock timestamps. Tactical intervals never accumulate simulation debt.
- Follow captures an 80 m initial radius; returning to Follow captures the current
  radius. Tangent movement parallel-transports the frame, including over poles.
- Tactical-only Converge placement, simultaneous lateral evasion, automatic Fixed
  arrival, and tactical-only Follow return. No independently placeable Fixed state.
- A zero-length or invalid target is rejected. Placement range: 0.5–250 m from
  player, 3–300 m from boss. These are initial prototype tuning limits.
- Arrival stores the actual movement frame. Fixed movement preserves that plane.
- Frame turning is limited to 8 rad/s; movement is 12 m/s, convergence 20 m/s.
- State-change events include tick, real time, position and anchor data; session
  metrics include tactical intervals and distance. This is recording infrastructure,
  **not yet a complete replay recording/playback implementation**.

Tests cover cadence independence, 60/120 Hz comparison, orbit radius and poles,
orthonormal frames, diagonal speed, pause/resume, repeated pausing, read-only target
validation, convergence during evasion, Fixed depth, Follow return and invalid data.
