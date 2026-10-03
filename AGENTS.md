# AGENTS.md

## Mission and source of truth

Build the Unreal Engine 5.8+ MVP as a core-experience prototype: stop time,
inspect a structured 3D bullet pattern, choose an anchor, execute a short
2-axis evasive flight, then watch a replay without thinking-time pauses.
Optimize for learning whether observation, anchor choice, execution, and replay
are compelling (H1–H4). Do not expand scope merely to resemble a full game.

Use these existing documents as the product requirements, reading only the
sections relevant to the task:

- `docs/specifications/MVP設計書_3D戦術弾幕シューティング_v0.1.md` — MVP scope and gameplay rules; takes priority.
- `docs/specifications/3D戦術弾幕シューティング ゲーム企画書.md` — longer-term intent, not MVP authorization.
- `docs/guidelines/ビジュアル／UI デザインガイドライン.md` — visual and UI direction; use it for visual changes.
- `docs/guidelines/MVPプレイテスト手順.md` — H1–H4 evaluation and observations.

## Non-negotiable architecture

GameCore is the authoritative, Unreal-independent C++ simulation. It owns
fixed-step time, player and boss state, coordinate frames, anchors, bullets and
patterns, gameplay collision and danger, ACTION/TACTICAL state, replay, and
gameplay events. Unreal owns input collection, rendering, camera, UI, audio,
VFX, assets, editor tooling, and replay camera direction.

Dependency direction: `Unreal Host -> GameCore`. Do not put Unreal types or APIs
(`FVector`, `UObject`, `TArray`, Chaos, Niagara, etc.) in GameCore. Convert types
in the bridge. Feed input/commands into GameCore, then present its
`RenderSnapshot`; never read proxy Actor transforms back as gameplay truth.
GameCore must build and test without Unreal.

Read `GameCore/AGENTS.md` when changing GameCore, even when Codex was started
from the repository root. Read `Source/ProjectOrbitalBloom/AGENTS.md` when
changing the Unreal Host or bridge. These nested files may not be loaded
automatically by a session started at the root.

## Tool choice: CLI first

Use ordinary file tools for source, text config, tests, pattern data, and docs.
Run GameCore tests, Unreal builds, commandlets, asset checks, and suitable
automation from the CLI first. CLI can also launch a normal Editor and execute
PIE automation without MCP. Consult `docs/guidelines/UNREAL_WORKFLOW.md` for the verified
commands, limits, and log checks when the task touches Unreal integration.

Use Unreal APIs for `.uasset` and `.umap` creation or modification. A CLI
commandlet or Editor Python script is appropriate when it can safely complete
and verify the operation. Never write binary asset bytes directly. Use Unreal
MCP when live Editor state, interactive asset editing, or visual inspection is
needed and a suitable CLI route does not answer the question. Serialize MCP
calls against one Editor process. Do not claim visual or PIE behavior was
verified from a source build alone.

Avoid intentional edits to `Binaries/`, `DerivedDataCache/`, `Intermediate/`,
and `Saved/` except for a task that specifically needs diagnosis or cleanup.

## MVP boundaries that apply to every task

The MVP is one boss encounter, about 60–90 seconds of simulation time, exactly
three spatial-reading pattern families, Follow/Converge/arrival-Fixed, two-axis
ACTION movement, constrained tactical orbit views, non-terminal hit feedback,
switchable advisory danger highlighting, in-memory pause-free replay, and
playtest instrumentation. Keep a deliberate visual/audio treatment.

TACTICAL stops GameCore time completely. Repeated pausing is allowed without
score or time penalty. ACTION has no direct depth input; anchor state controls
depth. Converge is placed in TACTICAL, moves toward a 3D destination while
allowing evasion, then automatically becomes Fixed. Fixed is not independently
placeable and can return to Follow. Hits do not end playtests. Replay excludes
real-world TACTICAL thinking time and provides at least chase and wide cameras.

The MVP design document's explicit out-of-scope list governs scope. In
particular, do not add shooting, HP/lives/score, Straight anchor, extra stages
or bosses, a free noclip camera, route solving, a replay editor, or a custom
renderer unless the user changes scope. Do not build abstractions primarily
for post-MVP systems.

## Validation and change safety

Choose the narrowest meaningful check for the change. For pure GameCore work,
run `bash GameCore/test.sh` and the relevant core tests. For Unreal code, run
the CLI build and relevant automation. For assets or presentation, inspect the
result in Editor only when a command-line check cannot establish it; use live
PIE and viewport inspection when visual or interaction behavior matters.
Check relevant logs and save changed Unreal assets/levels. Report what ran and
what remains unverified. Do not repeat broad tests after sufficient validation.

Keep gameplay events and playtest metrics for real/game/TACTICAL time, pause
intervals, anchor actions, hits, pattern time, travel, and replay watching.
These explain the experience; pause frequency is not a penalty.

Preserve unrelated user changes. Do not reset branches, force checkout, or
rewrite large asset trees. Report changed `.uasset`/`.umap` files. Do not commit
generated products or add dependencies without a concrete MVP need. Prefer
small, explicit changes within existing responsibility boundaries.
