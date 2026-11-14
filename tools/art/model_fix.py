"""Applies only the requested fixes to a GLB after backing the input up. Run: blender -b -P model_fix.py -- in.glb out.glb [--recalc-normals] [--decimate RATIO] [--apply-scale] [--origin bottom|center]"""

import argparse
import datetime
import os
import shutil
import sys

import bpy
from mathutils import Vector

ARCHIVE_ROOT = r"C:\Development\LakotArchive"
PROTECTED_ROOT = r"C:\Development\Lakot\data\models"


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("source")
    parser.add_argument("target")
    parser.add_argument("--recalc-normals", action="store_true")
    parser.add_argument("--decimate", type=float)
    parser.add_argument("--apply-scale", action="store_true")
    parser.add_argument("--origin", choices=("bottom", "center"))
    return parser.parse_args(sys.argv[sys.argv.index("--") + 1:])


def back_up(source_path):
    folder = os.path.join(ARCHIVE_ROOT, "assets-" + datetime.date.today().isoformat())
    os.makedirs(folder, exist_ok=True)
    name, extension = os.path.splitext(os.path.basename(source_path))
    target = os.path.join(folder, name + extension)
    count = 0
    while os.path.exists(target):
        count += 1
        target = os.path.join(folder, "%s-%s-%d%s" % (name, datetime.datetime.now().strftime("%H%M%S"), count, extension))
    shutil.copy2(source_path, target)
    return target


def check_target(source_path, target_path):
    source = os.path.normcase(os.path.realpath(source_path))
    target = os.path.normcase(os.path.realpath(target_path))
    protected = os.path.normcase(os.path.realpath(PROTECTED_ROOT))
    if target == source:
        sys.exit("Hedef kaynakla aynı, yazma reddedildi: " + target_path)
    if target == protected or target.startswith(protected + os.sep):
        sys.exit("Hedef data/models altında, yazma reddedildi: " + target_path)


def select_only(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]


def recalc_normals(meshes):
    for mesh in meshes:
        select_only([mesh])
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.mesh.normals_make_consistent(inside=False)
        bpy.ops.object.mode_set(mode="OBJECT")


def decimate(meshes, ratio):
    for mesh in meshes:
        select_only([mesh])
        modifier = mesh.modifiers.new("decimate", "DECIMATE")
        modifier.ratio = ratio
        bpy.ops.object.modifier_apply(modifier=modifier.name)


def apply_scale(meshes):
    select_only(meshes)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)


def move_origin(meshes, mode):
    corners = [m.matrix_world @ Vector(c) for m in meshes for c in m.bound_box]
    low = Vector((min(c[i] for c in corners) for i in range(3)))
    high = Vector((max(c[i] for c in corners) for i in range(3)))
    anchor = (low + high) / 2.0
    if mode == "bottom":
        anchor.z = low.z
    bpy.context.scene.cursor.location = anchor
    select_only(meshes)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    bpy.context.scene.cursor.location = Vector()


def main():
    args = parse_args()
    check_target(args.source, args.target)
    backup = back_up(args.source)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=args.source)
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    if args.apply_scale:
        apply_scale(meshes)
    if args.recalc_normals:
        recalc_normals(meshes)
    if args.decimate is not None:
        decimate(meshes, args.decimate)
    if args.origin:
        move_origin(meshes, args.origin)
    select_only(meshes)
    bpy.ops.export_scene.gltf(filepath=args.target, export_format="GLB", use_selection=True, export_yup=True)
    print("backup", backup, "wrote", args.target)


main()
