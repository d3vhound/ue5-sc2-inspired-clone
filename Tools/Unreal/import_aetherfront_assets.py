"""Import Blender-generated Aetherfront FBX assets into Unreal Engine 5.6.

Run from Unreal Editor: Tools > Execute Python Script.
"""

from __future__ import annotations

from pathlib import Path

import unreal


PROJECT_DIR = Path(unreal.Paths.project_dir()).resolve()
SOURCE_DIR = PROJECT_DIR / "Tools" / "Blender" / "output"
DESTINATION = "/Game/Aetherfront/Generated"


def import_fbx(path: Path, skeletal: bool) -> None:
    if not path.exists():
        raise FileNotFoundError(f"Missing generated FBX: {path}")

    task = unreal.AssetImportTask()
    task.filename = str(path)
    task.destination_path = DESTINATION
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = skeletal
    options.import_animations = skeletal
    options.import_materials = True
    options.import_textures = False
    options.create_physics_asset = skeletal
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else unreal.FBXImportType.FBXIT_STATIC_MESH
    if skeletal:
        options.skeletal_mesh_import_data.import_morph_targets = False
        options.anim_sequence_import_data.import_bone_tracks = True
        options.anim_sequence_import_data.animation_length = unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME
    else:
        options.static_mesh_import_data.combine_meshes = True
        options.static_mesh_import_data.generate_lightmap_u_vs = True
        options.static_mesh_import_data.auto_generate_collision = True

    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError(f"Unreal imported no assets from {path.name}")
    unreal.log(f"Imported {path.name}: {task.imported_object_paths}")


def main() -> None:
    import_fbx(SOURCE_DIR / "SK_AF_Fabricator.fbx", skeletal=True)
    import_fbx(SOURCE_DIR / "SM_AF_Citadel.fbx", skeletal=False)
    import_fbx(SOURCE_DIR / "SM_AF_Relay.fbx", skeletal=False)
    import_fbx(SOURCE_DIR / "SM_AF_AlloyBloom.fbx", skeletal=False)
    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
    unreal.log("Aetherfront generated asset import complete.")


if __name__ == "__main__":
    main()
