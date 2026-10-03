# Unreal CLI and Editor workflow

Read this when a task touches Unreal integration, assets, maps, PIE, or visual
presentation. The root `AGENTS.md` sets the tool priority; this file supplies
commands and the Editor-specific safety rules.

## CLI commands verified in this repository

Run from the repository root. `UE` and `PROJECT` are examples for this Linux
workspace; adjust their paths if the installation or checkout moves.

```sh
UE=/home/kousei/Apps/Linux_Unreal_Engine_5.8.1
PROJECT="$PWD/ProjectOrbitalBloom.uproject"
bash GameCore/test.sh
"$UE/Engine/Build/BatchFiles/Linux/Build.sh" ProjectOrbitalBloomEditor Linux Development "$PROJECT" -WaitMutex
```

The independent GameCore tests and UE Editor C++ build have both succeeded.
Save verbose Unreal output to a log and inspect only relevant results/errors.
Do not print full engine startup logs into the agent conversation.

The PythonScript Commandlet has read the MVP map/material asset registry and
the saved parent of `M_Bullet` without opening the live Editor. Replace the
script path below with a task-specific Unreal Python inspection script:

```sh
"$UE/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT" \
  -run=pythonscript -script=/tmp/task_inspection.py \
  -unattended -nop4 -nullrhi -nosplash -abslog=/tmp/read-assets.log
```

Use Unreal Python APIs inside the script. Read-only inspection was verified;
asset creation/editing/saving through this path is available in Unreal but
must be validated for the particular operation before relying on it. Never
rewrite `.uasset` or `.umap` bytes with ordinary filesystem tools.

DataValidation validated the project's three material instances from CLI:

```sh
"$UE/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT" \
  -run=DataValidation -AssetType=MaterialInstanceConstant \
  -IncludeOnlyOnDiskAssets -unattended -nop4 -nullrhi -nosplash \
  -abslog=/tmp/asset-validation.log
```

A normal Editor launched from CLI ran `OrbitalBloom.Host.InputAndAnchors`
successfully without MCP. It opened the configured map, started PIE, sent
input, and verified Converge -> Fixed -> Follow:

```sh
"$UE/Engine/Binaries/Linux/UnrealEditor" "$PROJECT" \
  /Game/MVP/Maps/L_MovementStudy \
  -ExecCmds='Automation RunTests OrbitalBloom.Host.InputAndAnchors' \
  -testexit='Automation Test Queue Empty' -unattended -nop4 -nosplash \
  -abslog=/tmp/editor-automation.log
```

Check the named test's `Result={Success}` and the number of tests performed in
the log. This launch returned process status 1 even though the named test
reported success, so process status alone is insufficient for this exact
`-testexit` invocation. A CI wrapper must check both the expected test result
and relevant errors. The same test **failed** under `UnrealEditor-Cmd -nullrhi`
because its Converge ghost placement uses viewport mouse coordinates. This is
a test/viewport limitation, not proof that all headless automation fails.

CLI tools available but not yet validated for this project include Cook/UAT
packaging and other commandlets. Treat availability separately from a passed
project check. The Unreal CLI may need access to the user's Zen/UBT cache;
a sandboxed run failed before Python execution when that cache was read-only.

## When live Editor or MCP is needed

Use live Editor inspection for visual judgment, camera/UI behavior, input
interaction, audio listening, and PIE state that logs/automation cannot prove.
Use official Unreal MCP for structured access to that live state. If a safe,
reproducible commandlet can make and verify an asset change, prefer it; use
MCP for operations that need the live Editor or lack a suitable CLI route.

MCP toolsets normally use tool search. Discover only what the task needs:
`list_toolsets` -> `describe_toolset` -> `call_tool` with the discovered schema.
Do not guess tool names or print all schemas. Never overlap MCP calls against
the same Editor process. Prefer structured asset/actor tools over simulated UI
clicking. Do not add custom MCP tools unless a reusable operation truly needs
one; a new reflected `UFUNCTION` tool requires Editor restart.

Before changing a level, Blueprint, material, Niagara system, widget, or
actor: inspect it, make the smallest change, save, and inspect the result.
For visual changes, inspect a viewport image when available. Do not claim a
live result was verified when MCP or Editor access was unavailable. Continue
with independent CLI/source checks and report the remaining uncertainty.

When Editor integration changes, check the expected asset/map, PIE startup,
Core snapshot presentation, mode switching, batched bullets, tactical markers,
replay/cameras, and relevant logs according to the change. Pure GameCore
changes with adequate core tests need no Editor launch.
