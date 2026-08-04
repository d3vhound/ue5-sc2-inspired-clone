# Aetherfront

Aetherfront is an original Unreal Engine 5.8 persistent online real-time strategy game prototype. The long-term goal is a seamless generated frontier where commanders can join at any time, establish durable settlements, coordinate or compete, and grow bases beyond conventional match-sized RTS limits.

The project studies the responsiveness and information design of excellent RTS games through documented, black-box AI/ML research. It does **not** contain or redistribute StarCraft II binaries, source, models, textures, animation, audio, maps, names, or other Blizzard content. Aetherfront is not affiliated with or endorsed by Blizzard Entertainment.

## Current checkpoint

The current code-only checkpoint intentionally depends on no third-party art and includes:

- Unreal Engine 5.8 game, editor, and dedicated-server targets
- server-owned game mode and replicated world director
- RTS camera with WASD pan, wheel zoom, and Q/E rotation
- click and drag selection with local ownership feedback
- server-validated formation movement for up to 200 selected units per command
- replicated Alloy/Flux economy and server-validated Relay construction
- Citadel, Fabricator, Warden, Relay, and resource-node blockouts made from Engine-native shapes
- deterministic resource scattering across a 20 km blockout surface
- a pinned SpacetimeDB 2.7.1 Rust module for persistent identity, economy, structures, units, cells, orders, and command receipts
- a transactional 20 Hz SpacetimeDB movement and construction simulation
- an Unreal GameInstance subsystem that reconnects with a saved identity token and subscribes to authoritative shard state
- versioned JSON snapshots retained only as an offline/local fallback
- PCG, Python editor scripting, and dedicated-server capabilities enabled for scale work; Mass stays disabled until its projection checkpoint has real code
- Blender generation and Unreal import scripts for an original rigged Fabricator and static structures

## Open and run

1. Install Unreal Engine 5.8 with C++ toolchain support and the SpacetimeDB 2.7.1 CLI.
2. Clone this repository.
3. Run `./Tools/SpacetimeDB/bootstrap.sh` on macOS/Linux or `./Tools/SpacetimeDB/bootstrap.ps1` in PowerShell. This fetches the official SDK pinned at `v2.7.1`, builds the Rust module, and generates Unreal bindings.
4. In one terminal, run `spacetime start`.
5. In another terminal, publish the local shard:

   ```bash
   spacetime publish aetherfront-dev --server local --module-path ./spacetimedb --yes
   ```

6. Right-click `Aetherfront.uproject` and generate project files.
7. Build the `AetherfrontEditor` Development target.
8. Open `Aetherfront.uproject` and press Play.

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

See [Docs/ARCHITECTURE.md](Docs/ARCHITECTURE.md) for the simulation, rendering, persistence, procedural-world, and entity strategy. The [SpacetimeDB guide](Docs/SPACETIMEDB.md), [asset pipeline](Docs/ASSET_PIPELINE.md), [testing guide](Docs/TESTING.md), and [clean-room research boundary](Docs/CLEAN_ROOM_RESEARCH.md) cover the corresponding workflows.

## Status

This is an engineering foundation, not a claim of AAA completion. AAA visual quality requires a sustained art, animation, VFX, audio, level-design, networking, backend, and QA production. The repository is structured so those disciplines can grow without replacing the core simulation.
