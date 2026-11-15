"""Procedural undergrowth generator (grass tufts, bushes, ferns, small stones). Run: blender -b -P undergrowth_gen.py -- [out_dir] [kind ...]  (default out: data/models/nature, all kinds, 3 variants each). Same seed gives the same model."""

import math
import random
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))

from tree_gen import OUT_DEFAULT, UP, fbm, make_image, make_material, make_object, patch_glb, perpendicular, stamp

VARIANTS = 3
TEX = 256

KINDS = {
    "grass": {"index": 0, "shape": "tuft", "colors": [(0.30, 0.52, 0.16), (0.38, 0.60, 0.20), (0.26, 0.46, 0.14)], "height": (0.65, 0.85), "blades": 46},
    "drygrass": {"index": 1, "shape": "tuft", "colors": [(0.62, 0.52, 0.26), (0.70, 0.60, 0.32), (0.54, 0.44, 0.22)], "height": (0.55, 0.8), "blades": 38},
    "bush": {"index": 2, "shape": "bush", "colors": [(0.20, 0.40, 0.13), (0.28, 0.48, 0.16), (0.16, 0.34, 0.12)], "height": (0.9, 1.2), "leaves": 40},
    "drybush": {"index": 3, "shape": "bush", "colors": [(0.45, 0.36, 0.18), (0.52, 0.42, 0.22), (0.38, 0.30, 0.16)], "height": (0.7, 1.0), "leaves": 16},
    "fern": {"index": 4, "shape": "fern", "colors": [(0.14, 0.38, 0.18), (0.20, 0.46, 0.20), (0.10, 0.32, 0.16)], "height": (0.7, 0.95), "fronds": 8},
    "stone": {"index": 5, "shape": "stone", "colors": [(0.50, 0.49, 0.46)], "height": (0.28, 0.42)},
    "sandstone": {"index": 6, "shape": "stone", "colors": [(0.66, 0.55, 0.40)], "height": (0.24, 0.38)},
}


def blank():
    tImg = np.zeros((TEX, TEX, 4))
    tImg[:, :, :3] = 0.5
    return tImg


def paint_disc(pImg, pX, pY, pRadius, pColor):
    tR = int(pRadius) + 2
    tY0, tY1 = max(0, int(pY) - tR), min(TEX, int(pY) + tR + 1)
    tX0, tX1 = max(0, int(pX) - tR), min(TEX, int(pX) + tR + 1)
    if tY1 <= tY0 or tX1 <= tX0:
        return
    tYs, tXs = np.mgrid[tY0:tY1, tX0:tX1]
    tAlpha = np.clip(pRadius - np.hypot(tXs - pX, tYs - pY) + 0.5, 0.0, 1.0)
    tReg = pImg[tY0:tY1, tX0:tX1]
    tW = tAlpha * (tAlpha > tReg[:, :, 3] * 0.5)
    tReg[:, :, :3] = tReg[:, :, :3] * (1 - tW[:, :, None]) + np.array(pColor)[None, None, :] * tW[:, :, None]
    tReg[:, :, 3] = np.maximum(tReg[:, :, 3], tAlpha)


def make_tuft_texture(pSpec, pRng):
    tImg = blank()
    for _ in range(pSpec["blades"]):
        tBx = TEX / 2 + pRng.uniform(-0.3, 0.3) * TEX
        tBy = TEX - 2
        tHeight = TEX * pRng.uniform(0.45, 0.96)
        tLean = pRng.uniform(-0.25, 0.25) + (tBx - TEX / 2) / TEX * 0.9
        tWidth = pRng.uniform(2.2, 4.0)
        tColor = pRng.choice(pSpec["colors"])
        for tStep in range(110):
            tT = tStep / 109.0
            tX = tBx + (tLean * tT * tT + pRng.uniform(-0.002, 0.002)) * tHeight
            tY = tBy - tT * tHeight
            paint_disc(tImg, tX, tY, max(0.7, tWidth * (1.0 - tT) ** 0.75), tuple(min(1.0, v * (0.7 + 0.45 * tT)) for v in tColor))
    return tImg


def make_leaf_patch_texture(pSpec, pRng):
    tImg = blank()
    tColors = pSpec["colors"]
    for _ in range(pSpec["leaves"]):
        tAng = pRng.uniform(0, math.tau)
        tRad = pRng.uniform(0, 0.36) * TEX
        tPx = TEX / 2 + math.cos(tAng) * tRad
        tPy = TEX / 2 + math.sin(tAng) * tRad
        tDir = math.atan2(tPy - TEX / 2, tPx - TEX / 2) + pRng.uniform(-0.8, 0.8)
        tTint = tuple(min(1.0, v * pRng.uniform(0.84, 1.16)) for v in pRng.choice(tColors))
        stamp(tImg, tPx, tPy, tDir, pRng.uniform(0.09, 0.13) * TEX, pRng.uniform(0.045, 0.065) * TEX, tTint, True)
    if pSpec["leaves"] < 24:
        for _ in range(9):
            tAng = pRng.uniform(0, math.tau)
            tLen = pRng.uniform(0.25, 0.45) * TEX
            tTint = tuple(v * 0.7 for v in tColors[0])
            stamp(tImg, TEX / 2 + math.cos(tAng) * tLen * 0.5, TEX / 2 + math.sin(tAng) * tLen * 0.5, tAng, tLen * 0.5, 1.4, tTint, False)
    return tImg


def make_fern_texture(pSpec, pRng):
    tImg = blank()
    tColor = pRng.choice(pSpec["colors"])
    tRibX = TEX / 2
    for tI in range(26):
        tT = tI / 25.0
        tY = TEX - 6 - tT * (TEX - 14)
        tReach = TEX * 0.46 * (1.0 - tT) ** 0.8 + 6
        for tSide in (-1, 1):
            tAng = -math.pi / 2 + tSide * math.radians(62 - 22 * tT)
            tTint = tuple(min(1.0, v * (0.8 + 0.4 * tT)) for v in tColor)
            stamp(tImg, tRibX + math.cos(tAng) * tReach * 0.5, tY + math.sin(tAng) * tReach * 0.5 * 0.35, tAng, tReach * 0.5, 3.0, tTint, False)
    for tStep in range(60):
        tT = tStep / 59.0
        paint_disc(tImg, tRibX, TEX - 4 - tT * (TEX - 10), 2.2 * (1.0 - tT) + 0.8, tuple(v * 0.75 for v in tColor))
    return tImg


def make_stone_texture(pSpec, pRng):
    tNp = np.random.default_rng(pRng.randint(0, 1 << 30))
    tBase = np.array(pSpec["colors"][0])
    tShade = 0.55 + 0.6 * fbm(TEX, tNp, 6, 6, 4)
    tCrack = np.clip((fbm(TEX, tNp, 5, 5, 3) - 0.62) * 7.0, 0.0, 1.0)
    tShade = tShade * (1.0 - 0.45 * tCrack) * (0.9 + 0.2 * fbm(TEX, tNp, 40, 40, 2))
    tPix = np.clip(tBase[None, None, :] * tShade[:, :, None], 0.0, 1.0)
    return np.concatenate([tPix, np.ones((TEX, TEX, 1))], axis=2)


def tuft_mesh(pSpec, pRng):
    tHeight = pRng.uniform(*pSpec["height"])
    tWidth = tHeight * pRng.uniform(1.25, 1.5)
    tVerts, tFaces, tUvs, tNormals = [], [], [], []
    tCards = 4
    for tI in range(tCards):
        tAng = math.pi * tI / tCards + pRng.uniform(-0.15, 0.15)
        tAxis = Vector((math.cos(tAng), math.sin(tAng), 0.0))
        tBase = len(tVerts)
        tFlare = pRng.uniform(0.9, 1.1)
        for tX, tZ, tU, tV in ((-1, 0.0, 0.0, 0.0), (1, 0.0, 1.0, 0.0), (1, 1.0, 1.0, 1.0), (-1, 1.0, 0.0, 1.0)):
            tSpread = 1.0 if tZ == 0.0 else tFlare
            tVerts.append(tAxis * tX * tWidth * 0.5 * tSpread + Vector((0.0, 0.0, tZ * tHeight)))
            tNormals.append(UP)
            tUvs.append((tU, tV))
        tFaces.append((tBase, tBase + 1, tBase + 2, tBase + 3))
    tFaceUvs = [tuple(tUvs[tI * 4 + tK] for tK in range(4)) for tI in range(tCards)]
    return tVerts, tFaces, tFaceUvs, tNormals


def bush_mesh(pSpec, pRng):
    tHeight = pRng.uniform(*pSpec["height"])
    tVerts, tFaces, tFaceUvs, tNormals = [], [], [], []
    tCards = 12
    for tI in range(tCards):
        tAz = tI * 2.399963 + pRng.uniform(-0.3, 0.3)
        tLift = 0.18 + 0.82 * (tI / (tCards - 1))
        tCenter = Vector((math.cos(tAz) * 0.28 * (1.2 - tLift), math.sin(tAz) * 0.28 * (1.2 - tLift), tHeight * (0.12 + 0.6 * tLift)))
        tNorm = (Vector((math.cos(tAz), math.sin(tAz), 0.0)) * 0.8 + UP * pRng.uniform(0.4, 1.0)).normalized()
        tAxisU = perpendicular(tNorm)
        tAxisV = tNorm.cross(tAxisU)
        tSize = tHeight * pRng.uniform(0.78, 1.05)
        tBase = len(tVerts)
        for tSx, tSy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
            tV = tCenter + tAxisU * tSx * tSize * 0.5 + tAxisV * tSy * tSize * 0.5
            tV.z = max(tV.z, 0.0)
            tVerts.append(tV)
            tNormals.append(((tV - Vector((0.0, 0.0, tHeight * 0.3))).normalized() * 0.7 + UP * 0.4).normalized())
        tFaces.append((tBase, tBase + 1, tBase + 2, tBase + 3))
        tFaceUvs.append(((0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)))
    return tVerts, tFaces, tFaceUvs, tNormals


def fern_mesh(pSpec, pRng):
    tHeight = pRng.uniform(*pSpec["height"])
    tVerts, tFaces, tFaceUvs, tNormals = [], [], [], []
    tFronds = pSpec["fronds"]
    tSegs = 4
    for tI in range(tFronds):
        tAz = tI * math.tau / tFronds + pRng.uniform(-0.25, 0.25)
        tDir = Vector((math.cos(tAz), math.sin(tAz), 0.0))
        tSide = Vector((-tDir.y, tDir.x, 0.0))
        tLen = tHeight * pRng.uniform(1.0, 1.25)
        tHalf = tLen * 0.3
        tRows = []
        for tS in range(tSegs + 1):
            tT = tS / tSegs
            tRadial = tLen * 0.65 * math.sin(tT * 1.2)
            tZ = tHeight * (0.1 + math.sin(tT * 1.2)) - tHeight * 0.35 * tT * tT
            tRows.append(tDir * tRadial + Vector((0.0, 0.0, max(tZ, 0.02))))
        tBase = len(tVerts)
        for tS, tCentre in enumerate(tRows):
            tTaper = 1.0 - 0.75 * (tS / tSegs) ** 1.5
            tVerts.append(tCentre - tSide * tHalf * tTaper)
            tVerts.append(tCentre + tSide * tHalf * tTaper)
            tNormals.append(UP)
            tNormals.append(UP)
        for tS in range(tSegs):
            tA = tBase + tS * 2
            tFaces.append((tA, tA + 1, tA + 3, tA + 2))
            tV0, tV1 = tS / tSegs, (tS + 1) / tSegs
            tFaceUvs.append(((0.0, tV0), (1.0, tV0), (1.0, tV1), (0.0, tV1)))
    return tVerts, tFaces, tFaceUvs, tNormals


def stone_mesh(pSpec, pRng):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=1.0)
    tObj = bpy.context.active_object
    tHeight = pRng.uniform(*pSpec["height"])
    tSx, tSy = pRng.uniform(0.9, 1.5), pRng.uniform(0.8, 1.3)
    tVerts = []
    for tV in tObj.data.vertices:
        tP = Vector(tV.co)
        tP = tP * pRng.uniform(0.78, 1.12)
        tP = Vector((tP.x * tSx, tP.y * tSy, tP.z * 0.62))
        tP.z = max(tP.z, -0.34)
        tVerts.append((tP + Vector((0.0, 0.0, 0.34))) * tHeight)
    tFaces = [tuple(tPoly.vertices) for tPoly in tObj.data.polygons]
    bpy.data.objects.remove(tObj)
    tFaceUvs = []
    for tFace in tFaces:
        tA, tB, tC = (tVerts[i] for i in tFace)
        tNormal = (tB - tA).cross(tC - tA)
        tAxes = (0, 1) if abs(tNormal.z) >= max(abs(tNormal.x), abs(tNormal.y)) else ((1, 2) if abs(tNormal.x) >= abs(tNormal.y) else (0, 2))
        tScale = 0.3 / tHeight
        tFaceUvs.append(tuple((min(max(0.5 + tVerts[i][tAxes[0]] * tScale, 0.0), 1.0), min(max(0.5 + tVerts[i][tAxes[1]] * tScale, 0.0), 1.0)) for i in tFace))
    return tVerts, tFaces, tFaceUvs, None


def generate(pKind, pVariant, pOutDir):
    tSpec = KINDS[pKind]
    tSeed = 6211 * (tSpec["index"] + 1) + 40009 * pVariant
    tRng = random.Random(tSeed)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    tShape = tSpec["shape"]
    tTextures = {"tuft": make_tuft_texture, "bush": make_leaf_patch_texture, "fern": make_fern_texture, "stone": make_stone_texture}
    tMeshes = {"tuft": tuft_mesh, "bush": bush_mesh, "fern": fern_mesh, "stone": stone_mesh}
    tImage = make_image("%s_atlas" % pKind, tTextures[tShape](tSpec, tRng))
    tMat = make_material("%s_leaf" % pKind if tShape != "stone" else "%s_rock" % pKind, tImage, tShape != "stone")
    tVerts, tFaces, tUvs, tNormals = tMeshes[tShape](tSpec, tRng)
    tObj = make_object(pKind, tVerts, tFaces, tUvs, tMat, tNormals)
    bpy.ops.object.select_all(action="DESELECT")
    tObj.select_set(True)
    tPath = Path(pOutDir) / ("%s_%d.glb" % (pKind, pVariant + 1))
    bpy.ops.export_scene.gltf(filepath=str(tPath), export_format="GLB", use_selection=True, export_yup=True, export_apply=True)
    patch_glb(tPath)
    print("UNDERGROWTH %s tris=%d" % (tPath.name, sum(len(f) - 2 for f in tFaces)))


def main():
    tArgs = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    tOut = Path(tArgs[0]) if tArgs else OUT_DEFAULT
    tKinds = tArgs[1:] or list(KINDS)
    tOut.mkdir(parents=True, exist_ok=True)
    for tKind in tKinds:
        for tVariant in range(VARIANTS):
            generate(tKind, tVariant, tOut)


if __name__ == "__main__":
    main()
