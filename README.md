# Aetherfront

Aetherfront is an original Unreal Engine 5.7 persistent online real-time strategy game prototype. The long-term goal is a seamless generated frontier where commanders can join at any time, establish durable settlements, coordinate or compete, and grow bases beyond conventional match-sized RTS limits.

The project studies the responsiveness and information design of excellent RTS games through documented, black-box AI/ML research. It does **not** contain or redistribute StarCraft II binaries, source, models, textures, animation, audio, maps, names, or other Blizzard content. Aetherfront is not affiliated with or endorsed by Blizzard Entertainment.

## Checkpoint 1

The first checkpoint is a code-only UE foundation that intentionally depends on no third-party art:

- Unreal Engine 5.7 game, editor, and dedicated-server targets
- server-owned game mode and replicated world director
- RTS camera with WASD pan, wheel zoom, and Q/E rotation
- mouse-aware player controller ready for selection and commands
- a 20 km Engine-native blockout surface with Lumen, virtual shadows, atmosphere, skylight, sun, and fog
- PCG, Mass Entity, Python editor scripting, and dedicated-server capabilities enabled for subsequent checkpoints

## Open and run

1. Install Unreal Engine 5.7 with C++ toolchain support.
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
| Left click | Selection trace (full selection ships next) |
| Right click | Command trace (authoritative movement ships next) |

## Architecture

See [Docs/ARCHITECTURE.md](Docs/ARCHITECTURE.md) for the server, simulation, rendering, persistence, procedural-world, and entity strategy. See [Docs/CLEAN_ROOM_RESEARCH.md](Docs/CLEAN_ROOM_RESEARCH.md) for the research boundary.

## Status

This is an engineering foundation, not a claim of AAA completion. AAA visual quality requires a sustained art, animation, VFX, audio, level-design, networking, backend, and QA production. The repository is structured so those disciplines can grow without replacing the core simulation.

