# Aetherfront

Aetherfront is an original Unreal Engine 5.6 persistent online real-time strategy game prototype. The long-term goal is a seamless generated frontier where commanders can join at any time, establish durable settlements, coordinate or compete, and grow bases beyond conventional match-sized RTS limits.

The project studies the responsiveness and information design of excellent RTS games through documented, black-box AI/ML research. It does **not** contain or redistribute StarCraft II binaries, source, models, textures, animation, audio, maps, names, or other Blizzard content. Aetherfront is not affiliated with or endorsed by Blizzard Entertainment.

## Current checkpoint

The current code-only checkpoint intentionally depends on no third-party art and includes:

- Unreal Engine 5.6 game, editor, and dedicated-server targets
- server-owned game mode and replicated world director
- RTS camera with WASD pan, wheel zoom, and Q/E rotation
- click and drag selection with local ownership feedback
- server-validated formation movement for up to 200 selected units per command
- replicated Alloy/Flux economy and server-validated Relay construction
- Citadel, Fabricator, Warden, Relay, and resource-node blockouts made from Engine-native shapes
- deterministic resource scattering across a 20 km blockout surface
- versioned, atomically replaced JSON snapshots for persistent structures
- PCG, Mass Entity, Python editor scripting, and dedicated-server capabilities enabled for scale work
- Blender generation and Unreal import scripts for an original rigged Fabricator and static structures

## Open and run

1. Install Unreal Engine 5.6 with C++ toolchain support.
2. Clone this repository.
3. Right-click `Aetherfront.uproject` and generate project files.
4. Build the `AetherfrontEditor` Development target.
5. Open `Aetherfront.uproject` and press Play.

The project currently uses `/Engine/Maps/Entry` so it can boot without a binary `.umap`. The authoritative game mode creates the presentation world at runtime. A generated World Partition map replaces this in the next content checkpoint.

## Controls

| Input | Action |
| --- | --- |
| W/A/S/D | Pan camera |
| Mouse wheel | Zoom |
| Q/E | Rotate camera |
| Left click / drag | Select an owned unit, building, or group |
| Shift + select | Add to current selection |
| Right click | Issue a server-authoritative formation move |
| Shift + right click | Queue another formation waypoint |
| B, then left click | Place an Assembly Relay for 250 Alloy |

## Architecture

See [Docs/ARCHITECTURE.md](Docs/ARCHITECTURE.md) for the server, simulation, rendering, persistence, procedural-world, and entity strategy. The [asset pipeline](Docs/ASSET_PIPELINE.md), [testing guide](Docs/TESTING.md), and [clean-room research boundary](Docs/CLEAN_ROOM_RESEARCH.md) cover the corresponding workflows.

## Status

This is an engineering foundation, not a claim of AAA completion. AAA visual quality requires a sustained art, animation, VFX, audio, level-design, networking, backend, and QA production. The repository is structured so those disciplines can grow without replacing the core simulation.
