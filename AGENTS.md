# AGENTS.md

## Project mission

Build the MVP for a 3D tactical bullet-hell game in Unreal Engine 5.8+.

The MVP is a **core-experience prototype**, not a vertical slice. Its purpose is to validate whether the following loop is compelling:

1. Enter **TACTICAL MODE** and stop simulation time.
2. Inspect a visually structured 3D bullet pattern from alternate viewpoints.
3. Choose a useful movement frame / destination with the anchor system.
4. Resume **ACTION MODE** and execute a short burst of 2-axis evasive flight.
5. After the encounter, watch the pause-free result as an aesthetically satisfying replay.

Optimize work for learning whether this loop is fun. Do not expand the project merely to make it resemble a complete game.

If present in the repository, treat these documents as product requirements and read the relevant sections before changing gameplay behavior:

- `MVP設計書_3D戦術弾幕シューティング_v0.1.md`
- `3D戦術弾幕シューティング ゲーム企画書.md`

The MVP design document has priority for MVP scope. The broader game proposal describes future intent, not permission to implement post-MVP systems.

---

## Non-negotiable architecture

### GameCore is the source of truth

Authoritative gameplay state must live in an Unreal-independent C++ **GameCore**.

GameCore owns:

- fixed-step simulation time
- player simulation state
- boss simulation state
- coordinate frames
- Follow / Converge / Fixed anchor state
- bullet state and bullet-pattern generation
- gameplay collision
- danger estimation
- ACTION / TACTICAL mode state
- replay state and gameplay events

Unreal owns presentation and platform-facing responsibilities:

- rendering
- camera
- keyboard / mouse input collection
- UI
- audio
- Niagara / VFX
- asset loading
- editor tooling and debug visualization
- replay camera direction

The dependency direction is always:

```text
Unreal Host -> GameCore
```

Never introduce a dependency from GameCore back to Unreal Engine.

### Unreal types are forbidden in GameCore

Do not use Unreal types or APIs inside GameCore, including but not limited to:

- `FVector`, `FQuat`, `FTransform`
- `TArray`, `TMap`, `TSet`
- `FName`, `FString`
- `UObject`, `AActor`, `UActorComponent`
- `UWorld`
- Chaos APIs
- Niagara APIs

Use engine-neutral equivalents such as GameCore-owned `Vec3`, `Quat`, IDs, containers, and plain data structures.

Conversions such as `game::Vec3 <-> FVector` belong in the Unreal bridge layer.

### Unreal objects are render / interaction proxies, not gameplay truth

Do not drive authoritative simulation by reading transforms back from presentation actors every frame.

Preferred data flow:

```text
Input / editor interaction
        -> command converted at bridge boundary
        -> GameCore fixed-step simulation
        -> RenderSnapshot
        -> Unreal presentation
```

A presentation actor may temporarily visualize a state, but the corresponding GameCore state remains authoritative.

### Keep GameCore buildable without Unreal

The GameCore must remain independently buildable and testable without launching Unreal Editor.

When changing GameCore, add or update GameCore-side tests where the behavior can be tested without rendering.

Do not require Unreal headers, UnrealBuildTool, an Editor process, or a `.uproject` launch to run core unit tests.

---

## Unreal MCP workflow

This project assumes **Unreal Engine 5.8+ official Unreal MCP** (`ModelContextProtocol`) with the relevant Toolset Registry toolsets enabled.

Use Unreal MCP whenever the task depends on live Editor state or modifies Unreal assets / levels.

### Tool discovery

Unreal MCP normally runs in tool-search mode. Do not guess unavailable tool names.

Use the discovery flow:

1. `list_toolsets`
2. `describe_toolset` for the relevant toolset
3. `call_tool` with the exact discovered tool and schema

Only discover the toolsets needed for the current task; do not dump every tool schema into context.

### Never parallelize Unreal MCP calls

Treat Unreal MCP Editor calls as serialized operations.

Do **not** issue overlapping MCP calls against the same Editor process, especially mutations. Execute them sequentially and inspect each result before continuing.

Parallel filesystem searches or independent CLI operations are fine when they do not touch the live Unreal Editor state.

### Inspect before mutating

Before editing a level, Blueprint, material, Niagara system, widget, project setting, or actor:

1. inspect the relevant object / selection / level state;
2. identify the smallest required mutation;
3. make the change;
4. save the changed asset or level;
5. inspect again to verify the resulting state.

For visual changes, capture or inspect the viewport when a relevant MCP tool is available.

### Prefer structured tools over UI imitation

Prefer direct Unreal MCP tools that manipulate actors, objects, assets, materials, Niagara, UMG, Sequencer, tests, or project settings.

Do not simulate menu clicking when a structured tool exists.

Do not create project-specific MCP tools unless the current toolsets are genuinely insufficient and the new capability will be reused.

### When extending Unreal MCP

If a reusable project-specific Editor operation is repeatedly needed, a custom Toolset Registry tool may be appropriate.

For custom MCP toolsets:

- keep each tool small and single-purpose;
- use structured parameters and structured returns;
- prefer Python toolsets unless engine functionality is unavailable to Python or reflected C++ types/performance require C++;
- after toolset changes, refresh the MCP tool registry;
- adding a new reflected `UFUNCTION` tool requires an Editor restart rather than relying only on Live Coding.

MCP tooling is development infrastructure, not runtime gameplay architecture. GameCore must not depend on MCP.

### If Unreal MCP is unavailable

Do not pretend an Editor-side change was verified.

Continue with source-only or headless work where valid, and clearly report that live Editor verification is blocked.

Do not replace missing MCP access by hand-editing binary Unreal assets.

---

## Filesystem vs Unreal MCP

Use ordinary source/file tools for:

- `.cpp`, `.h`, `.cs`, `.py`
- CMake / build scripts
- text configuration
- tests
- Markdown documentation
- engine-neutral pattern data

Use Unreal MCP / Unreal Editor for:

- `.uasset`
- `.umap`
- Blueprint graph edits
- material / material-instance edits
- Niagara asset edits
- UMG assets
- level actor changes
- Sequencer assets
- live PIE state

Never directly rewrite `.uasset` or `.umap` files as binary blobs.

Avoid intentional edits to generated folders such as:

- `Binaries/`
- `DerivedDataCache/`
- `Intermediate/`
- `Saved/`

unless a task specifically requires cleanup or diagnosis.

---

## MVP scope

### In scope

The MVP contains:

- one boss encounter
- approximately 60-90 seconds of **simulation/game time**
- ACTION MODE
- TACTICAL MODE
- 2-axis ACTION movement
- Follow anchor
- Converge anchor
- automatic Converge -> Fixed transition
- Fixed -> Follow return
- Player Orbit tactical camera
- Boss Orbit tactical camera
- minimal tactical pan if necessary
- three 3D bullet patterns
- gameplay collision and non-terminal hit feedback
- simple danger highlighting
- minimum intentional VFX / audio pass
- in-memory replay
- replay with TACTICAL thinking time removed
- player trajectory visualization
- at least two replay camera modes
- playtest instrumentation

### Explicitly out of scope unless the user asks to change MVP scope

Do not implement:

- route / stage sections before or between bosses
- Straight anchor
- direct user placement of a standalone Fixed anchor
- shooting lock-on
- player shooting systems
- boss HP or damage loop
- distance-based attack-power bonus
- bombs
- score systems
- graze systems
- lives / conventional HP game rules
- multiple regular enemies
- additional bosses
- multiple player craft
- equipment / build systems
- roguelite systems
- online rankings
- story systems
- save / load progression
- production menus
- full gamepad support
- unrestricted noclip tactical camera
- full pathfinding / safe-route prediction
- replay editor
- replay file sharing
- bullet-pattern GUI editor
- custom renderer
- custom game engine

Do not create abstractions primarily for these future features during the MVP.

---

## Gameplay invariants

### ACTION MODE

- Primary player control is 2-axis movement in the current movement plane.
- There is no direct forward/backward depth input.
- Depth behavior comes from the active anchor state.
- ACTION must remain simple enough that player attention can stay on evasion and execution.

### TACTICAL MODE

- GameCore simulation time is fully stopped.
- Pausing has no score penalty.
- Pause duration has no gameplay penalty.
- Do not add a forced minimum ACTION commitment window unless specifically requested after playtesting.
- Repeated pausing is permitted in the MVP; do not silently “fix” it.
- Tactical visualization should reveal structure, not calculate or display the correct route.

### Tactical camera

Start with constrained, legible cameras:

- Player Orbit
- Boss Orbit
- limited pan where required

Do not default to a fully free noclip camera. Add freer motion only if observation proves orbit views inadequate.

### Follow anchor

- Boss is the only Follow target in the MVP.
- Follow generally maintains boss distance.
- 2-axis input maps onto motion tangent to the boss-relative movement frame.
- The intended result is legible orbital movement around the boss with simple controls.

### Converge anchor

- Placement occurs in TACTICAL MODE.
- Converge defines a destination in 3D space.
- During ACTION, depth motion moves toward the destination while 2-axis evasion remains active.
- On arrival, transition automatically to Fixed.

### Fixed state

- Fixed is not independently placeable in the MVP.
- It exists as the arrival state of Converge.
- Depth velocity becomes zero.
- 2-axis motion continues in the stored arrival movement plane.
- Player can return to Follow.

### Hit handling

Hits should not repeatedly terminate playtests.

On hit, prefer:

- hit VFX / sound
- event logging
- hit counter increment
- short invulnerability to avoid duplicate contact spam

Do not add lives, death loops, conventional HP balancing, or restart-on-hit unless specifically requested.

---

## Bullet patterns

Exactly three core pattern families are required for the MVP. They test different spatial-reading skills, not simply increasing difficulty.

### Pattern A: Spherical Shell / Flower

Purpose: validate discovery by changing viewpoint.

Properties:

- shell-like emission around the boss
- one or more intentional holes / thin regions
- openings are difficult to understand from the default view
- openings become legible from a lateral / oblique tactical view
- Converge can be used to approach a viable passage

### Pattern B: Rotating Helix / Ring Tunnel

Purpose: validate reading a moving 3D structure.

Properties:

- rings / helical bands with temporal motion
- passage geometry changes with angle and time
- Follow and Converge should both be potentially useful
- should remain visually coherent and attractive from non-default viewpoints

### Pattern C: Planar Wall / Lattice

Purpose: validate changing the useful movement plane.

Properties:

- a plane / lattice divides or crosses combat space
- staying in the current Follow frame should be meaningfully inconvenient
- changing position / frame through Converge should create a more understandable solution

Do not add more patterns merely for content volume before H1-H4 can be evaluated.

---

## Bullet simulation and rendering

### Never make one gameplay Actor per bullet

Do not implement authoritative bullets as thousands of `AActor`s or individually ticking `UObject`s.

GameCore should keep bullet state in cache-friendly contiguous / data-oriented structures.

Target MVP scale:

- typical: roughly 1,000-2,000 simultaneous bullets
- stress test: roughly 5,000 simultaneous bullets

### Rendering is a presentation problem

Unreal receives a batched render snapshot from GameCore.

Use an appropriate batched representation such as instancing and/or Niagara for visuals.

Niagara may render glow, trails, secondary particles, hit effects, and other visual treatment, but Niagara particle state must **not** become authoritative gameplay bullet state.

Collision, danger estimation, replay state, and future simulation must remain possible without Niagara.

### Beauty is part of MVP validity

Do not leave the final playtest build as debug spheres only.

The MVP must demonstrate a deliberate visual direction with minimal production cost. At minimum preserve:

- low-information dark background
- emissive bullets
- visually readable pattern structure
- motion trails / afterimages where useful
- readable player trajectory
- basic bloom / post-processing
- distinct TACTICAL visual treatment
- an intentional ACTION-resume accent

Prefer simple assets with coherent art direction over many unfinished assets.

---

## Simulation rules

### Fixed step

Simulation must be fixed-step and independent of render frame rate.

Start with 120 Hz as a candidate, while keeping 60 Hz vs 120 Hz easy to compare.

Do not encode gameplay movement directly in render-frame DeltaTime if it bypasses GameCore fixed-step logic.

### Determinism and reproducibility

Favor deterministic or reproducible simulation where reasonable because replay and prediction benefit from it.

Do not make determinism a reason to build a complex custom engine during the MVP. Test the parts that matter and record sufficient replay state/events to reproduce the encounter reliably.

### Danger highlighting

Danger highlighting is advisory, not route solving.

MVP danger estimation may use short-horizon closest-approach approximations from player and bullet position/velocity.

It may highlight bullets worth attention, but must not draw a complete safe path or mark a canonical solution.

Keep danger highlighting switchable for playtest comparison.

---

## Replay rules

Replay is an MVP feature, not optional polish.

The player-facing purpose is to transform interrupted planning/execution into a continuous visual flight.

Record enough information to reproduce or reconstruct:

- GameCore game time
- player state / trajectory
- boss state
- bullet or pattern state needed for playback
- anchor changes
- TACTICAL enter / exit events
- hit events

For MVP, in-memory storage for one run is sufficient.

Replay playback must exclude TACTICAL real-world thinking time and advance according to GameCore simulation time.

Provide at least:

- chase camera
- wide / boss-oriented camera

A third side / cinematic camera is desirable when inexpensive.

Replay camera decisions belong to the Unreal presentation layer, not GameCore.

---

## Content data

Do not make Unreal `DataAsset`s the sole canonical representation of bullet-pattern gameplay data.

For the MVP, pattern definitions may be:

- plain C++ data, or
- engine-neutral text data such as JSON / YAML

If an Unreal asset mirrors pattern parameters for editor convenience, keep a clear path to regenerate / translate it from engine-neutral data.

Do not build the full bullet-pattern GUI editor during the MVP.

---

## Blueprint policy

Blueprints are allowed for fast presentation iteration, UI, one-off Editor wiring, and non-authoritative visual behavior.

Do not place authoritative simulation logic in Blueprint when it belongs in GameCore.

Avoid large Blueprint graphs that duplicate C++ gameplay state.

If a Blueprint needs GameCore state, expose a narrow bridge API or presentation snapshot rather than reimplementing the rule in Blueprint.

---

## C++ change policy

Before adding a new class or subsystem:

1. locate the existing responsibility boundary;
2. prefer extending an existing narrow interface when appropriate;
3. avoid “manager” classes with unrelated responsibilities;
4. avoid premature generic frameworks for future non-MVP features.

Favor plain data and explicit transformations in GameCore.

Favor Unreal-native idioms inside the Unreal Host layer when they do not leak into GameCore.

Follow the repository's existing compiler standard, formatting, naming, and module structure. Do not reformat unrelated files.

---

## Suggested ownership boundaries

Use this structure conceptually. Adapt to the repository if it already has a different layout; do not reorganize the whole repository merely to match this example.

```text
/GameCore
    Math/
    Simulation/
    Player/
    Anchor/
    Bullet/
    Pattern/
    Collision/
    Danger/
    Replay/
    Tests/

/UnrealProject
    Source/ or Plugins/
        GameCoreBridge/
        TacticalGame/
        TacticalRendering/
        TacticalUI/
        TacticalReplay/
    Content/
        MVP/
            Maps/
            Art/
            VFX/
            UI/
            Audio/
            Debug/
```

A single bridge layer should perform most Unreal <-> GameCore type conversion.

---

## Testing requirements

### GameCore tests

Add focused automated tests for behavior changed by the task when practical.

High-value GameCore tests include:

- movement-frame orthogonality / normalization
- Follow distance behavior
- stable tangent movement
- Converge progression
- Converge -> Fixed transition
- Fixed depth behavior
- Fixed -> Follow transition
- fixed-step independence from render cadence
- collision primitives
- danger closest-approach calculations
- pattern-generation invariants
- replay timing that excludes TACTICAL thinking time

### Unreal-side validation

For changes involving Unreal integration or presentation, validate through the Editor using Unreal MCP when available.

Depending on the change, validation may include:

- asset exists at intended path
- actor / component configuration is correct
- level saves successfully
- PIE starts without relevant errors
- ACTION <-> TACTICAL transition works
- bridge receives GameCore snapshots
- expected batched bullet representation renders
- tactical markers / movement plane render correctly
- replay starts and camera modes function
- visual result is inspected in the viewport

### Build and automation

Prefer unattended command-line build/test/automation for work that does not require live Editor state.

Use live Unreal MCP for Editor-dependent inspection and validation.

A code change is not considered verified merely because files compile locally if it changes Editor integration or gameplay presentation.

Conversely, do not require a visual Editor test for a pure GameCore math/unit-test change when independent tests fully cover it.

---

## Validation loop for Codex

For any non-trivial implementation task, follow this loop:

1. **Read**: inspect relevant code, design requirements, and current Unreal state if relevant.
2. **Plan locally**: identify the smallest change that satisfies the requested behavior.
3. **Implement**: preserve GameCore/Unreal boundaries.
4. **Build/test**: run the narrowest useful automated checks first.
5. **Open/inspect with Unreal MCP** when the change touches assets, levels, Editor integration, rendering, UI, VFX, camera, or PIE behavior.
6. **Run PIE / relevant automation** when useful.
7. **Inspect logs and viewport state** for regressions.
8. **Save changed Unreal assets/levels**.
9. **Summarize** changed files/assets, tests run, and any remaining uncertainty.

Do not claim an Unreal-side result is working if only source code was inspected and the live integration was not verified.

---

## Playtest instrumentation

Instrumentation exists to understand the experience, not to score the player.

Keep or add logging for:

- real play duration
- GameCore progression time
- total TACTICAL time
- number of TACTICAL entries
- duration of each TACTICAL interval
- Converge placements
- Follow returns
- hits
- time spent in each bullet pattern
- player travel distance
- replay watched duration / skip time when available

Do not turn pause frequency into a penalty metric.

---

## Product hypotheses to protect

When choosing between two implementations, prefer the one that preserves the ability to evaluate these hypotheses cleanly:

### H1 — Observation

Changing viewpoint during stopped time reveals meaningful and aesthetically coherent 3D bullet structure.

### H2 — Anchor strategy

The player chooses a different movement frame / destination because it changes how the bullet pattern can be navigated.

### H3 — Execution

Resuming ACTION after planning creates tension, commitment, and release rather than feeling like a passive answer check.

### H4 — Replay beauty

Removing thinking-time pauses produces a flight worth watching on its own and makes the player want to try another trajectory.

If a proposed feature does not help evaluate H1-H4 and is not required infrastructure, defer it.

---

## Git and change safety

- Never discard or overwrite unrelated user changes.
- Do not reset branches, force checkout files, or perform destructive Git operations unless explicitly instructed.
- Keep changes scoped to the request.
- Do not rename/move large asset trees without explicit reason.
- Treat Unreal binary asset changes as important; report which `.uasset` / `.umap` files were changed.
- Do not commit generated build products unless the repository explicitly tracks them.
- Do not introduce a new dependency without a concrete MVP need.

---

## Definition of done for a task

A task is done when all applicable conditions are true:

- requested behavior is implemented;
- GameCore remains Unreal-independent;
- no new post-MVP gameplay scope was accidentally introduced;
- relevant automated tests pass;
- relevant source build succeeds;
- Unreal-side changes were verified through Unreal MCP / Editor when applicable;
- PIE or automation was run when gameplay integration changed;
- changed Unreal assets/levels are saved;
- logs contain no new relevant errors or warnings that are being ignored;
- the final report states what changed, what was tested, and what was not verified.

---

## Decision defaults

When requirements are ambiguous, use these defaults unless the user says otherwise:

- choose the smaller MVP scope;
- prefer clear behavior over extensibility;
- prefer GameCore purity over Unreal convenience for authoritative gameplay;
- prefer Unreal convenience over custom technology for rendering/editor workflows;
- prefer batched data-oriented bullet handling over per-object gameplay architecture;
- prefer orbit-based tactical observation over unrestricted free camera;
- permit frequent pausing rather than adding anti-pause mechanics;
- preserve visual quality sufficient to test the art hypothesis;
- prefer measurable playtest behavior over speculative polish;
- do not build a custom engine or custom renderer during this MVP.
