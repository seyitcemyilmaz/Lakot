"""Splits the hair triangles of a character GLB into their own primitive with a material named "Hair", so the client can skip them under a helmet. Vertices, bones, weights and poses are untouched. Run: python split_hair.py in.glb out.glb"""

import datetime
import io
import json
import os
import shutil
import struct
import sys

import numpy as np
from PIL import Image

ARCHIVE_ROOT = r"C:\Development\LakotArchive"
PROTECTED_ROOT = r"C:\Development\Lakot\data\models"
USAGE = "Kullanım: python split_hair.py in.glb out.glb"
HAIR_MATERIAL = "Hair"
HEAD_MIN_Y = 1.47
HAIR_TOP_Y = 1.54
HAIR_BACK_Z = 0.03
HAIR_MAX_LUMINANCE = 0.12
SCALP_MIN_Y = 1.665
HAIRLINE_MIN_Y = 1.585
GROW_MIN_Y = 1.645
GROW_RINGS = 2
WELD_STEP = 0.002
HEAD_HALF_WIDTH = 0.12
COMPONENT_DTYPES = {5126: "f4", 5123: "u2", 5121: "u1", 5125: "u4"}
COMPONENT_COUNTS = {"VEC3": 3, "VEC2": 2, "VEC4": 4, "SCALAR": 1}


def read_glb(path):
    data = open(path, "rb").read()
    json_length = struct.unpack("<I", data[12:16])[0]
    document = json.loads(data[20:20 + json_length])
    binary_start = 20 + json_length + 8
    binary_length = struct.unpack("<I", data[binary_start - 8:binary_start - 4])[0]
    return document, bytearray(data[binary_start:binary_start + binary_length])


def read_accessor(document, binary, index):
    accessor = document["accessors"][index]
    view = document["bufferViews"][accessor["bufferView"]]
    count = accessor["count"] * COMPONENT_COUNTS[accessor["type"]]
    offset = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    return np.frombuffer(binary, COMPONENT_DTYPES[accessor["componentType"]], count, offset).reshape(accessor["count"], -1)


def read_texture(document, binary):
    view = document["bufferViews"][document["images"][0]["bufferView"]]
    start = view.get("byteOffset", 0)
    return np.array(Image.open(io.BytesIO(bytes(binary[start:start + view["byteLength"]]))).convert("RGB")) / 255.0


def classify_hair(positions, uvs, texture, indices):
    size = texture.shape[0]
    colors = texture[(uvs[:, 1] % 1 * (size - 1)).astype(int), (uvs[:, 0] % 1 * (size - 1)).astype(int)]
    luminance = colors @ np.array([0.299, 0.587, 0.114])
    triangles = indices.reshape(-1, 3)
    centers = positions[triangles].mean(axis=1)
    dark = luminance[triangles].mean(axis=1) < HAIR_MAX_LUMINANCE
    in_head = (centers[:, 1] > HEAD_MIN_Y) & (np.abs(centers[:, 0]) < HEAD_HALF_WIDTH)
    scalp = centers[:, 1] > SCALP_MIN_Y
    nape = (centers[:, 1] > HAIR_TOP_Y) & (centers[:, 2] < HAIR_BACK_Z)
    hairline = dark & (centers[:, 1] > HAIRLINE_MIN_Y)
    hair = in_head & (scalp | nape | hairline | (dark & (centers[:, 1] > HAIR_TOP_Y)))
    keys = np.round(positions[triangles] / WELD_STEP).astype(np.int64)
    vertex_ids = {}
    ids = np.array([[vertex_ids.setdefault(tuple(k), len(vertex_ids)) for k in row] for row in keys])
    for _ in range(GROW_RINGS):
        touched = np.zeros(len(vertex_ids), dtype=bool)
        touched[ids[hair].reshape(-1)] = True
        hair = hair | (in_head & (centers[:, 1] > GROW_MIN_Y) & touched[ids].any(axis=1))
    return hair


def split(source_path, target_path):
    document, binary = read_glb(source_path)
    primitive = document["meshes"][0]["primitives"][0]
    positions = read_accessor(document, binary, primitive["attributes"]["POSITION"])
    uvs = read_accessor(document, binary, primitive["attributes"]["TEXCOORD_0"])
    index_accessor = document["accessors"][primitive["indices"]]
    indices = read_accessor(document, binary, primitive["indices"]).reshape(-1)
    is_hair = classify_hair(positions, uvs, read_texture(document, binary), indices)
    triangles = indices.reshape(-1, 3)
    ordered = np.concatenate([triangles[~is_hair], triangles[is_hair]]).reshape(-1).astype(indices.dtype)

    view = document["bufferViews"][index_accessor["bufferView"]]
    start = view.get("byteOffset", 0) + index_accessor.get("byteOffset", 0)
    binary[start:start + ordered.nbytes] = ordered.tobytes()

    body_count = int((~is_hair).sum()) * 3
    hair_count = int(is_hair.sum()) * 3
    body_accessor = dict(index_accessor, count=body_count)
    hair_accessor = dict(index_accessor, count=hair_count, byteOffset=index_accessor.get("byteOffset", 0) + body_count * indices.itemsize)
    for key in ("min", "max"):
        body_accessor.pop(key, None)
        hair_accessor.pop(key, None)
    document["accessors"][primitive["indices"]] = body_accessor
    document["accessors"].append(hair_accessor)
    document["materials"].append(dict(document["materials"][primitive.get("material", 0)], name=HAIR_MATERIAL))
    hair_primitive = dict(primitive, indices=len(document["accessors"]) - 1, material=len(document["materials"]) - 1)
    document["meshes"][0]["primitives"].append(hair_primitive)

    json_bytes = json.dumps(document, separators=(",", ":")).encode()
    json_bytes += b" " * (-len(json_bytes) % 4)
    binary_bytes = bytes(binary) + b"\0" * (-len(binary) % 4)
    total = 12 + 8 + len(json_bytes) + 8 + len(binary_bytes)
    with open(target_path, "wb") as out:
        out.write(struct.pack("<4sII", b"glTF", 2, total))
        out.write(struct.pack("<I4s", len(json_bytes), b"JSON") + json_bytes)
        out.write(struct.pack("<I4s", len(binary_bytes), b"BIN\0") + binary_bytes)
    print("hair triangles", hair_count // 3, "of", len(triangles), "->", target_path)


def back_up(target_path):
    folder = os.path.join(ARCHIVE_ROOT, "assets-" + datetime.date.today().isoformat())
    os.makedirs(folder, exist_ok=True)
    name, extension = os.path.splitext(os.path.basename(target_path))
    backup = os.path.join(folder, name + extension)
    count = 0
    while os.path.exists(backup):
        count += 1
        backup = os.path.join(folder, "%s-%s-%d%s" % (name, datetime.datetime.now().strftime("%H%M%S"), count, extension))
    shutil.copy2(target_path, backup)
    return backup


def check_target(source_path, target_path):
    source = os.path.normcase(os.path.realpath(source_path))
    target = os.path.normcase(os.path.realpath(target_path))
    if target == source:
        sys.exit("Hedef kaynakla aynı, yazma reddedildi: " + target_path)


def is_protected(target_path):
    target = os.path.normcase(os.path.realpath(target_path))
    protected = os.path.normcase(os.path.realpath(PROTECTED_ROOT))
    return target == protected or target.startswith(protected + os.sep)


def main():
    if len(sys.argv) != 3:
        sys.exit(USAGE)
    source_path, target_path = sys.argv[1], sys.argv[2]
    check_target(source_path, target_path)
    if is_protected(target_path) and os.path.exists(target_path):
        print("yedek", back_up(target_path))
    split(source_path, target_path)


if __name__ == "__main__":
    main()
