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
  metrics include tactical intervals, pattern durations and distance.
- Optional 75-second encounter: shell, helix and lattice, each lasting 25 seconds.
  Batched bullet state, swept relative collision, 0.6-second invulnerability and
  closest-approach danger estimation remain entirely engine-independent.
- `Recording` stores fixed-tick inputs, anchor commands and trajectory. `Replay`
  reconstructs the same-build encounter with no tactical wall-time at 1x or 0.5x.
  Storage is in memory for one run; replay file interchange is not supported.
- Encounter generation is opt-in in `Config`; the Unreal host enables it.
  Density 1 is normal; density 3 is the approximately 5,000-bullet stress case.

Tests cover cadence independence, 60/120 Hz comparison, orbit radius and poles,
orthonormal frames, diagonal speed, pause/resume, repeated pausing, read-only target
validation, convergence during evasion, Fixed depth, Follow return, invalid data,
pattern invariants, swept collision, invulnerability, danger estimation, complete
encounter timing, deterministic input/anchor playback and stress density.
