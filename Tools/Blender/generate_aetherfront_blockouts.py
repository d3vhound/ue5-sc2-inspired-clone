"""Generate original Aetherfront blockout assets and export Unreal-ready FBX files.

Run with Blender 4.3+:
    blender --background --python Tools/Blender/generate_aetherfront_blockouts.py

The generated geometry is an original gray-box production aid. It contains no
StarCraft assets, extracted meshes, or copied silhouettes.
"""

from __future__ import annotations

import math
from pathlib import Path

import bpy
from mathutils import Vector


SCRIPT_DIR = Path(__file__).resolve().parent
OUTPUT_DIR = SCRIPT_DIR / "output"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def reset_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.armatures, bpy.data.materials):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)

    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.length_unit = "METERS"
    scene.unit_settings.scale_length = 1.0
    scene.render.fps = 30


def make_material(name: str, color: tuple[float, float, float, float], metallic: float, roughness: float):
    material = bpy.data.materials.new(name)
    material.diffuse_color = color
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    if principled:
        principled.inputs["Base Color"].default_value = color
        principled.inputs["Metallic"].default_value = metallic
        principled.inputs["Roughness"].default_value = roughness
    return material


ASSEMBLY_DARK = None
ASSEMBLY_TEAL = None
ASSEMBLY_LIGHT = None
RESOURCE_GLOW = None


def smooth_and_bevel(obj: bpy.types.Object, width: float = 0.05, segments: int = 2) -> None:
    if obj.type != "MESH":
        return
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    bevel = obj.modifiers.new("AF_Bevel", "BEVEL")
    bevel.width = width
    bevel.segments = segments
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel.name)


def add_cube(name: str, location, scale, material, bevel: float = 0.05):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(material)
    smooth_and_bevel(obj, bevel)
    return obj


def add_uv_sphere(name: str, location, scale, material):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=1.0, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(material)
    smooth_and_bevel(obj, 0.025)
    return obj


def add_cylinder(name: str, location, radius: float, depth: float, material, vertices: int = 12):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(material)
    smooth_and_bevel(obj, min(radius, depth) * 0.05)
    return obj


def add_cone(name: str, location, radius1: float, radius2: float, depth: float, material, vertices: int = 10):
    bpy.ops.mesh.primitive_cone_add(
        vertices=vertices,
        radius1=radius1,
        radius2=radius2,
        depth=depth,
        location=location,
    )
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(material)
    smooth_and_bevel(obj, min(radius1, depth) * 0.04)
    return obj


def join_objects(objects: list[bpy.types.Object], name: str) -> bpy.types.Object:
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    joined = bpy.context.object
    joined.name = name
    return joined


def export_selected(filename: str, objects: list[bpy.types.Object], skeletal: bool = False) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]

    bpy.ops.export_scene.fbx(
        filepath=str(OUTPUT_DIR / filename),
        use_selection=True,
        object_types={"ARMATURE", "MESH"} if skeletal else {"MESH"},
        axis_forward="-Y",
        axis_up="Z",
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_ALL",
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        add_leaf_bones=False,
        bake_anim=skeletal,
        bake_anim_use_all_actions=skeletal,
        bake_anim_use_nla_strips=False,
        bake_anim_force_startend_keying=True,
        bake_anim_simplify_factor=0.0,
    )


def add_bone(armature, name: str, head, tail, parent=None):
    bone = armature.edit_bones.new(name)
    bone.head = head
    bone.tail = tail
    bone.parent = parent
    return bone


def bind_part_to_bone(part: bpy.types.Object, bone_name: str) -> None:
    group = part.vertex_groups.new(name=bone_name)
    group.add(list(range(len(part.data.vertices))), 1.0, "REPLACE")


def create_fabricator() -> tuple[bpy.types.Object, bpy.types.Object]:
    """Create a six-legged fabrication drone with a dorsal multi-tool."""
    bpy.ops.object.armature_add(enter_editmode=True, location=(0.0, 0.0, 0.0))
    rig = bpy.context.object
    rig.name = "SK_AF_Fabricator_Rig"
    armature = rig.data
    armature.name = "SK_AF_Fabricator_Skeleton"
    armature.edit_bones.remove(armature.edit_bones[0])

    root = add_bone(armature, "root", (0.0, 0.0, 0.0), (0.0, 0.0, 0.35))
    body = add_bone(armature, "body", (0.0, 0.0, 0.35), (0.0, 0.0, 1.35), root)
    tool = add_bone(armature, "tool", (0.0, 0.15, 1.15), (0.75, 0.15, 1.25), body)

    leg_bones = []
    leg_specs = [
        ("leg_fl", (0.48, 0.48, 0.55), (0.90, 0.75, 0.12)),
        ("leg_fr", (0.48, -0.48, 0.55), (0.90, -0.75, 0.12)),
        ("leg_ml", (0.0, 0.58, 0.48), (0.0, 1.02, 0.10)),
        ("leg_mr", (0.0, -0.58, 0.48), (0.0, -1.02, 0.10)),
        ("leg_bl", (-0.48, 0.48, 0.55), (-0.90, 0.75, 0.12)),
        ("leg_br", (-0.48, -0.48, 0.55), (-0.90, -0.75, 0.12)),
    ]
    for name, head, tail in leg_specs:
        leg_bones.append(add_bone(armature, name, head, tail, body))

    bpy.ops.object.mode_set(mode="OBJECT")

    parts: list[bpy.types.Object] = []
    hull = add_uv_sphere("Fabricator_Hull", (0.0, 0.0, 0.78), (0.72, 0.58, 0.42), ASSEMBLY_DARK)
    bind_part_to_bone(hull, "body")
    parts.append(hull)

    core = add_cylinder("Fabricator_Core", (0.0, 0.0, 1.12), 0.30, 0.18, ASSEMBLY_TEAL, 10)
    bind_part_to_bone(core, "body")
    parts.append(core)

    tool_mesh = add_cone("Fabricator_Tool", (0.72, 0.15, 1.22), 0.13, 0.04, 0.72, ASSEMBLY_LIGHT, 8)
    tool_mesh.rotation_euler[1] = math.radians(90.0)
    bpy.context.view_layer.objects.active = tool_mesh
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    bind_part_to_bone(tool_mesh, "tool")
    parts.append(tool_mesh)

    for bone_name, head, tail in leg_specs:
        head_v = Vector(head)
        tail_v = Vector(tail)
        midpoint = (head_v + tail_v) * 0.5
        length = (tail_v - head_v).length
        leg = add_cylinder(f"Fabricator_{bone_name}", midpoint, 0.085, length, ASSEMBLY_DARK, 8)
        direction = tail_v - head_v
        leg.rotation_mode = "QUATERNION"
        leg.rotation_quaternion = direction.to_track_quat("Z", "Y")
        bpy.context.view_layer.objects.active = leg
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
        bind_part_to_bone(leg, bone_name)
        parts.append(leg)

    mesh = join_objects(parts, "SK_AF_Fabricator")
    mesh.parent = rig
    armature_modifier = mesh.modifiers.new("AF_Armature", "ARMATURE")
    armature_modifier.object = rig

    def make_action(name: str, keyframes: list[tuple[int, dict[str, tuple[float, float, float]]]]):
        action = bpy.data.actions.new(name)
        rig.animation_data_create()
        rig.animation_data.action = action
        for frame, transforms in keyframes:
            for bone_name, rotation in transforms.items():
                pose_bone = rig.pose.bones[bone_name]
                pose_bone.rotation_mode = "XYZ"
                pose_bone.rotation_euler = rotation
                pose_bone.keyframe_insert("rotation_euler", frame=frame, group=bone_name)
        action.frame_start = min(frame for frame, _ in keyframes)
        action.frame_end = max(frame for frame, _ in keyframes)
        rig.animation_data.action = None

    make_action(
        "AF_Fabricator_Idle",
        [
            (1, {"body": (0.0, 0.0, math.radians(-1.5))}),
            (20, {"body": (0.0, 0.0, math.radians(1.5))}),
            (40, {"body": (0.0, 0.0, math.radians(-1.5))}),
        ],
    )
    make_action(
        "AF_Fabricator_Move",
        [
            (1, {name: (math.radians(18 if index % 2 == 0 else -18), 0.0, 0.0) for index, (name, _, _) in enumerate(leg_specs)}),
            (11, {name: (math.radians(-18 if index % 2 == 0 else 18), 0.0, 0.0) for index, (name, _, _) in enumerate(leg_specs)}),
            (21, {name: (math.radians(18 if index % 2 == 0 else -18), 0.0, 0.0) for index, (name, _, _) in enumerate(leg_specs)}),
        ],
    )
    make_action(
        "AF_Fabricator_Work",
        [
            (1, {"tool": (0.0, math.radians(-24.0), 0.0)}),
            (12, {"tool": (0.0, math.radians(18.0), 0.0)}),
            (24, {"tool": (0.0, math.radians(-24.0), 0.0)}),
        ],
    )

    return rig, mesh


def create_citadel() -> bpy.types.Object:
    parts = [
        add_cylinder("Citadel_Base", (0.0, 0.0, 0.35), 2.6, 0.7, ASSEMBLY_DARK, 12),
        add_cylinder("Citadel_Ring", (0.0, 0.0, 1.0), 1.9, 0.42, ASSEMBLY_TEAL, 12),
        add_cone("Citadel_Spire", (0.0, 0.0, 2.35), 1.2, 0.35, 2.7, ASSEMBLY_LIGHT, 10),
    ]
    for index in range(6):
        angle = index / 6.0 * math.tau
        parts.append(
            add_cube(
                f"Citadel_Fin_{index}",
                (math.cos(angle) * 2.15, math.sin(angle) * 2.15, 0.95),
                (0.48, 0.16, 0.78),
                ASSEMBLY_DARK,
                0.08,
            )
        )
        parts[-1].rotation_euler[2] = angle
    return join_objects(parts, "SM_AF_Citadel")


def create_relay() -> bpy.types.Object:
    parts = [
        add_cylinder("Relay_Base", (0.0, 0.0, 0.25), 1.35, 0.5, ASSEMBLY_DARK, 10),
        add_cube("Relay_Frame", (0.0, 0.0, 0.95), (0.72, 0.72, 0.70), ASSEMBLY_TEAL, 0.12),
        add_cylinder("Relay_Dish", (0.0, 0.0, 1.82), 0.72, 0.16, ASSEMBLY_LIGHT, 12),
    ]
    return join_objects(parts, "SM_AF_Relay")


def create_resource() -> bpy.types.Object:
    parts = []
    for index, (x, y, height, radius) in enumerate(
        [(-0.42, 0.0, 1.8, 0.32), (0.22, 0.28, 2.35, 0.38), (0.38, -0.30, 1.45, 0.27)]
    ):
        shard = add_cone(
            f"Resource_Shard_{index}",
            (x, y, height * 0.5),
            radius,
            radius * 0.32,
            height,
            RESOURCE_GLOW,
            7,
        )
        shard.rotation_euler[0] = math.radians((-1) ** index * 7.0)
        parts.append(shard)
    return join_objects(parts, "SM_AF_AlloyBloom")


def main() -> None:
    global ASSEMBLY_DARK, ASSEMBLY_TEAL, ASSEMBLY_LIGHT, RESOURCE_GLOW
    reset_scene()
    ASSEMBLY_DARK = make_material("M_AF_AssemblyDark", (0.035, 0.055, 0.065, 1.0), 0.78, 0.28)
    ASSEMBLY_TEAL = make_material("M_AF_AssemblyTeal", (0.035, 0.62, 0.56, 1.0), 0.55, 0.24)
    ASSEMBLY_LIGHT = make_material("M_AF_AssemblyLight", (0.54, 0.86, 0.82, 1.0), 0.42, 0.20)
    RESOURCE_GLOW = make_material("M_AF_ResourceGlow", (0.04, 0.95, 0.72, 1.0), 0.15, 0.12)

    rig, fabricator = create_fabricator()
    export_selected("SK_AF_Fabricator.fbx", [rig, fabricator], skeletal=True)

    rig.hide_set(True)
    fabricator.hide_set(True)
    citadel = create_citadel()
    export_selected("SM_AF_Citadel.fbx", [citadel])
    citadel.hide_set(True)
    relay = create_relay()
    export_selected("SM_AF_Relay.fbx", [relay])
    relay.hide_set(True)
    resource = create_resource()
    export_selected("SM_AF_AlloyBloom.fbx", [resource])

    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_DIR / "aetherfront_blockouts.blend"))
    print(f"Aetherfront assets written to {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
