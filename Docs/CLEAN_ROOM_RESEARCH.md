# Clean-room RTS research boundary

## Permitted research inputs

- Blizzard's MIT-licensed `s2client-api` C++ wrapper
- Blizzard's public protobuf API definitions and stable IDs
- Blizzard's public replay protocol implementation
- official headless runtime and replay/map packs used only under the accepted AI/ML license
- ordinary observation of controls, camera response, command feedback, timing, and match telemetry
- original experiments and measurements produced through the official API

## Excluded from Aetherfront

- StarCraft II executable or data archives
- decompiled or disassembled Blizzard implementation code
- extracted M3 models, rigs, animations, textures, materials, audio, VFX, maps, UI art, fonts, or strings
- copied unit/faction names, lore, silhouettes, ability sets, balance tables, map layouts, or trade dress
- leaked or unofficial proprietary source

The licensed research runtime remains in temporary research storage and is never committed, packaged, or redistributed with this repository.

## Research output contract

Research produces neutral behavioral specifications such as:

- command-to-acknowledgement latency distribution
- selection and control-group state transitions
- camera acceleration, damping, zoom, and edge-pan response curves
- formation convergence and local-avoidance outcomes
- build/economy pacing expressed as dimensionless ratios
- readable feedback requirements and error taxonomy

Implementation consumes only those neutral specifications. Every game identity, content archetype, mesh, material, animation, sound, map system, progression rule, and authored value is original to Aetherfront.

