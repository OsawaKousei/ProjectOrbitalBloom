# GameCore instructions

The root `AGENTS.md` and the MVP design document remain authoritative.
GameCore is an independent C++17 simulation and must build without Unreal
headers, UnrealBuildTool, an Editor process, or a `.uproject` launch.

- Use GameCore-owned math types, IDs, standard containers, and plain data.
  `FVector`, `FQuat`, `TArray`, `UObject`, Chaos, and Niagara belong outside.
- Keep simulation at a fixed step, independent of render cadence. Start with
  120 Hz while retaining an easy 60 Hz comparison.
- Keep player/boss state, movement frames, Follow/Converge/Fixed, bullets,
  collision, danger, mode, replay data, and events authoritative here.
- Treat Unreal actors and Niagara as consumers of snapshots. Never depend on
  their transforms, particle state, or collision for gameplay truth.
- Keep bullet state contiguous and batched, not one object per bullet. The
  MVP target is roughly 1,000–2,000 normally and about 5,000 in stress tests.
- Keep canonical pattern data in C++ or engine-neutral text. Do not make an
  Unreal DataAsset the only source of pattern rules.
- Favor reproducible simulation. Record sufficient input, anchor commands,
  time, trajectory, pattern state, and events for one in-memory replay. Replay
  advances on GameCore time and omits TACTICAL wall time.
- Danger highlighting is switchable advice based on short-horizon motion;
  it must not mark a complete safe route or canonical answer.
- Extend a narrow existing interface before adding a new subsystem. Avoid
  generic frameworks for systems outside the MVP.

When changing behavior, add focused core tests where rendering is unnecessary.
High-value cases are frame orthogonality, Follow distance and tangent motion,
Converge progression, arrival-Fixed and Follow return, fixed-step cadence,
collision/invulnerability, danger approach, pattern geometry, and replay timing.
Run `bash GameCore/test.sh` from the repository root. Pure core math changes do
not need a visual Editor run once the relevant independent tests pass.
