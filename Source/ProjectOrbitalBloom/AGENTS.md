# Unreal Host instructions

The root `AGENTS.md` and the MVP design document remain authoritative.
This module collects input and presents GameCore state. Keep the bridge narrow:
convert Unreal types to engine-neutral commands at the boundary, and convert
`RenderSnapshot` data to Unreal presentation types on the way out.

- Keep authoritative movement, bullets, hit detection, danger, anchors,
  mode, and replay state in GameCore. Blueprints may handle UI and one-off
  visual wiring, but must not duplicate gameplay rules.
- Do not create one ticking Actor or UObject per bullet. Use a batched
  representation such as instancing; Niagara can add trails, glow, and hit
  effects without becoming gameplay truth.
- Keep tactical cameras constrained and legible: Player Orbit and Boss Orbit,
  plus limited pan if observation requires it. Replay camera direction lives
  here, with at least chase and wide/boss-oriented views.
- Preserve a coherent MVP visual pass: low-information background following
  the current visual guide, emissive/readable bullets, player trajectory,
  basic bloom, distinct TACTICAL treatment, and ACTION-resume accent.
- Inspect the existing responsibility boundary before adding a class.
  Prefer Unreal-native presentation tools when they do not leak into Core.

For build, commandlets, automation, asset inspection, and MCP routing, read
`docs/guidelines/UNREAL_WORKFLOW.md`. Run relevant CLI checks first. Validate live PIE or
viewport results when interaction or visual behavior changed, inspect logs,
and report any presentation state that was not verified.
