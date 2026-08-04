# Blender → Unreal asset pipeline

The repository starts with procedurally generated original blockouts so gameplay can be tested before a full art team exists. They are not intended to masquerade as final AAA assets.

## Generate

With Blender 4.3 or newer installed:

```bash
blender --background --python Tools/Blender/generate_aetherfront_blockouts.py
```

The script creates:

- `SK_AF_Fabricator.fbx` — original six-legged fabrication drone, skeleton, and Idle/Move/Work actions
- `SM_AF_Citadel.fbx` — Aster Assembly command structure
- `SM_AF_Relay.fbx` — modular infrastructure relay
- `SM_AF_AlloyBloom.fbx` — generated-world alloy resource formation
- `aetherfront_blockouts.blend` — editable Blender source

Generated outputs stay ignored until they are reviewed. The authored generator script is the source of truth.

## Import

Open the Unreal project, then run `Tools/Unreal/import_aetherfront_assets.py` through **Tools → Execute Python Script**. It imports the files into `/Game/Aetherfront/Generated` with skeletal animation, collision, and lightmap settings.

## Production contract

- Blender uses meters; Unreal imports to centimeters.
- Forward axis is `-Y`; up axis is `Z`.
- Gameplay origin is centered on the ground contact plane.
- Skeletal root bone is named `root` and remains at world origin.
- Static mesh prefixes: `SM_AF_`; skeletal meshes: `SK_AF_`; animations: `AF_<Unit>_<Action>`.
- Final assets require authored simple collision, at least three LODs or suitable Nanite settings, reused material families, texel-density review, animation compression review, and Unreal Insights validation.
- Faction silhouettes, proportions, surface language, animations, VFX, and audio must remain original to Aetherfront.
