"""Renders front, side and top views of GLBs and measures them. Run: blender -b -P model_probe.py -- out_dir model.glb [model.glb ...]"""

import json
import math
import os
import sys

import bpy
from mathutils import Matrix, Vector

RESOLUTION = 256
MARGIN = 1.25
VIEWS = {
    "front": (0.0, 1.0, 0.0),
    "side": (-1.0, 0.0, 0.0),
    "top": (0.0, 0.0, -1.0),
}


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    for engine in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE", "BLENDER_WORKBENCH"):
        try:
            scene.render.engine = engine
            break
        except TypeError:
            continue
    scene.render.resolution_x = RESOLUTION
    scene.render.resolution_y = RESOLUTION
    scene.render.film_transparent = False
    world = bpy.data.worlds.new("probe")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (0.18, 0.18, 0.2, 1.0)
    scene.world = world
    for name, direction, energy in (("key", (0.5, -0.6, -0.7), 3.0), ("fill", (-0.6, 0.4, -0.5), 1.5)):
        data = bpy.data.lights.new(name, "SUN")
        data.energy = energy
        light = bpy.data.objects.new(name, data)
        light.rotation_euler = Vector(direction).to_track_quat("-Z", "Y").to_euler()
        scene.collection.objects.link(light)
    return scene


def world_bounds(meshes):
    corners = [m.matrix_world @ Vector(c) for m in meshes for c in m.bound_box]
    low = Vector((min(c[i] for c in corners) for i in range(3)))
    high = Vector((max(c[i] for c in corners) for i in range(3)))
    return low, high


def gltf_axes(vector):
    return [vector.x, vector.z, -vector.y]


def gltf_bounds(low, high):
    return [low.x, low.z, -high.y], [high.x, high.z, -low.y]


def inverted_ratio(meshes):
    total = 0
    inward = 0
    for mesh in meshes:
        data = mesh.data
        data.calc_loop_triangles()
        center = sum((v.co for v in data.vertices), Vector()) / max(len(data.vertices), 1)
        for polygon in data.polygons:
            total += 1
            if polygon.normal.dot(polygon.center - center) < 0.0:
                inward += 1
    return inward / total if total else 0.0


def has_texture(meshes):
    for mesh in meshes:
        for slot in mesh.material_slots:
            if slot.material and slot.material.use_nodes:
                if any(n.type == "TEX_IMAGE" and n.image for n in slot.material.node_tree.nodes):
                    return True
    return False


def measure(meshes, rigs):
    low, high = world_bounds(meshes)
    size = high - low
    triangles = sum(len(p.vertices) - 2 for m in meshes for p in m.data.polygons)
    gltf_low, gltf_high = gltf_bounds(low, high)
    return {
        "size": [round(size.x, 4), round(size.z, 4), round(size.y, 4)],
        "originOffsetFromBottom": round(-low.z, 4),
        "boundsMin": [round(v, 4) for v in gltf_low],
        "boundsMax": [round(v, 4) for v in gltf_high],
        "triangles": triangles,
        "meshes": len(meshes),
        "invertedNormalRatio": round(inverted_ratio(meshes), 4),
        "hasTexture": has_texture(meshes),
        "bones": sum(len(r.data.bones) for r in rigs),
        "scaleApplied": all(all(abs(s - 1.0) < 1e-4 for s in m.scale) for m in meshes),
    }


def render_views(scene, meshes, name, out_dir):
    low, high = world_bounds(meshes)
    center = (low + high) / 2.0
    extent = max(max(high - low) / 2.0, 1e-4)
    camera_data = bpy.data.cameras.new("probe")
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = extent * 2.0 * MARGIN
    camera = bpy.data.objects.new("probe", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    frames = {}
    for view, direction in VIEWS.items():
        forward = Vector(direction)
        camera.location = center - forward * (extent * 4.0)
        up = Vector((0.0, 1.0, 0.0)) if view == "top" else Vector((0.0, 0.0, 1.0))
        right = forward.cross(up).normalized()
        camera.rotation_euler = Matrix((right, right.cross(forward).normalized(), -forward)).transposed().to_euler()
        scene.render.filepath = os.path.join(out_dir, "%s_%s.png" % (name, view))
        bpy.ops.render.render(write_still=True)
        frames[view] = {"center": gltf_axes(center), "halfSpan": extent * MARGIN}
    return frames


def probe(path, out_dir):
    scene = reset_scene()
    bpy.ops.import_scene.gltf(filepath=path)
    meshes = [o for o in scene.objects if o.type == "MESH"]
    rigs = [o for o in scene.objects if o.type == "ARMATURE"]
    name = os.path.basename(os.path.dirname(path)) + "_" + os.path.splitext(os.path.basename(path))[0]
    result = {"file": path.replace("\\", "/")}
    if not meshes:
        result["error"] = "no meshes"
        return name, result
    result.update(measure(meshes, rigs))
    result["frames"] = render_views(scene, meshes, name, out_dir)
    return name, result


def main():
    args = sys.argv[sys.argv.index("--") + 1:]
    out_dir, paths = args[0], args[1:]
    os.makedirs(out_dir, exist_ok=True)
    report = {}
    for path in paths:
        name, result = probe(path, out_dir)
        report[name] = result
    with open(os.path.join(out_dir, "probe.json"), "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=1)


main()
