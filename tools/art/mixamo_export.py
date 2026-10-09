"""Exports Mixamo downloads as game GLBs, keeping Mixamo's rig and weights. Run: blender -b -P mixamo_export.py -- character.fbx out.glb (a model downloaded "With Skin") or -- animation.fbx out.glb --rest body.glb (an animation, baked onto the exported character's skeleton)"""

import os
import sys

import bpy


def import_fbx(path):
    before = set(bpy.context.scene.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    return [o for o in bpy.context.scene.objects if o not in before]


def export_glb(rig, objects, target_path, animated):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in [rig] + objects:
        obj.select_set(True)
    bpy.ops.export_scene.gltf(filepath=target_path, export_format="GLB", use_selection=True, export_animations=animated,
                              export_animation_mode="NLA_TRACKS", export_skins=True, export_yup=True)
    print("joints", len(rig.data.bones), "exported", target_path)


def export_character(source_path, target_path):
    imported = import_fbx(source_path)
    rig = next(o for o in imported if o.type == "ARMATURE")
    meshes = [o for o in imported if o.type == "MESH"]

    rig.animation_data_clear()
    for action in list(bpy.data.actions):
        bpy.data.actions.remove(action)

    weighted = {g.name for mesh in meshes for g in mesh.vertex_groups
                if any(e.group == g.index and e.weight > 0.0 for v in mesh.data.vertices for e in v.groups)}
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode="EDIT")
    for bone in list(rig.data.edit_bones):
        if not bone.children and bone.name not in weighted:
            rig.data.edit_bones.remove(bone)
    bpy.ops.object.mode_set(mode="OBJECT")

    export_glb(rig, meshes, target_path, False)


def export_motion(source_path, target_path, rest_path):
    bpy.ops.import_scene.gltf(filepath=rest_path)
    rig = next(o for o in bpy.context.scene.objects if o.type == "ARMATURE")
    for obj in list(bpy.context.scene.objects):
        if obj is not rig:
            bpy.data.objects.remove(obj, do_unlink=True)
    rig.animation_data_clear()
    for action in list(bpy.data.actions):
        bpy.data.actions.remove(action)

    imported = import_fbx(source_path)
    source = next(o for o in imported if o.type == "ARMATURE")
    action = source.animation_data.action
    root = next(b.name for b in rig.data.bones if b.parent is None)
    for bone in rig.pose.bones:
        if bone.name not in source.pose.bones:
            continue
        rotation = bone.constraints.new("COPY_ROTATION")
        rotation.target, rotation.subtarget = source, bone.name
        if bone.name == root:
            location = bone.constraints.new("COPY_LOCATION")
            location.target, location.subtarget = source, bone.name

    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode="POSE")
    bpy.ops.pose.select_all(action="SELECT")
    start, end = (int(v) for v in action.frame_range)
    bpy.ops.nla.bake(frame_start=start, frame_end=end, only_selected=True, visual_keying=True,
                     clear_constraints=True, use_current_action=False, bake_types={"POSE"})
    bpy.ops.object.mode_set(mode="OBJECT")

    for obj in imported:
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.data.actions.remove(action)
    baked = rig.animation_data.action
    baked.name = os.path.splitext(os.path.basename(source_path))[0].replace(" ", "_")
    rig.animation_data.action = None
    track = rig.animation_data.nla_tracks.new()
    track.name = baked.name
    track.strips.new(baked.name, start, baked)

    export_glb(rig, [], target_path, True)


def main():
    arguments = sys.argv[sys.argv.index("--") + 1:]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    if "--rest" in arguments:
        export_motion(arguments[0], arguments[1], arguments[arguments.index("--rest") + 1])
    else:
        export_character(arguments[0], arguments[1])


main()
