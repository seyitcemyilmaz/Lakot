"""Procedural tree generator. Run: blender -b -P tree_gen.py -- [out_dir] [species ...]  (default out: data/models/nature, all species, 3 variants + _lod1 each). Same seed gives the same model."""

import math
import random
import struct
import json
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector

ROOT = Path(__file__).resolve().parent.parent.parent
OUT_DEFAULT = ROOT / "data" / "models" / "nature"
VARIANTS = 3
FULL_BUDGET = 2900
LOD_BUDGET = 580
BARK_SIZE = 1024
LEAF_SCALE = 4
EDGE_RAMP = 7.0
UP = Vector((0.0, 0.0, 1.0))


def spindle(pT):
    return max(0.22, 1.0 - ((pT - 0.55) / 0.55) ** 2)


def cone(pT):
    return max(0.1, (1.0 - pT) / 0.78)


def dome(pT):
    return max(0.45, 1.0 - ((pT - 0.7) / 0.6) ** 2)


def flat(pT):
    return 1.0


SPECIES = {
    "poplar": {
        "index": 0, "height": (12.0, 16.0), "radius": 0.021, "bark": (0.42, 0.38, 0.32), "birch": False,
        "leaf": "broad", "leafColors": [(0.30, 0.52, 0.16), (0.38, 0.60, 0.20), (0.24, 0.44, 0.14)], "cardSize": 1.2, "cardsPerPoint": 6, "spread": 0.5, "oriented": False,
        "levels": [
            {"segs": 6, "taper": 0.3, "jitter": 0.03, "up": 0.02, "kids": 24, "start": 0.2, "ang": (16, 34), "kidLen": 0.3, "kidRad": 0.2, "env": spindle, "sides": 7},
            {"segs": 3, "taper": 0.45, "jitter": 0.1, "up": 0.2, "kids": 2, "start": 0.35, "ang": (22, 45), "kidLen": 0.5, "kidRad": 0.5, "env": flat, "sides": 4},
            {"segs": 2, "taper": 0.5, "jitter": 0.12, "up": 0.12, "kids": 0, "sides": 3},
        ],
    },
    "birch": {
        "index": 1, "height": (8.0, 11.0), "radius": 0.016, "bark": (0.9, 0.88, 0.84), "birch": True,
        "leaf": "broad", "leafColors": [(0.52, 0.66, 0.20), (0.44, 0.60, 0.18), (0.62, 0.72, 0.26)], "cardSize": 1.0, "cardsPerPoint": 6, "spread": 0.4, "oriented": False,
        "levels": [
            {"segs": 8, "taper": 0.18, "jitter": 0.11, "up": 0.03, "kids": 13, "start": 0.3, "ang": (28, 52), "kidLen": 0.34, "kidRad": 0.2, "env": spindle, "sides": 6, "irregular": 0.09},
            {"segs": 3, "taper": 0.45, "jitter": 0.1, "up": -0.04, "kids": 3, "start": 0.3, "ang": (20, 45), "kidLen": 0.5, "kidRad": 0.5, "env": flat, "sides": 4},
            {"segs": 2, "taper": 0.5, "jitter": 0.12, "up": -0.14, "kids": 0, "sides": 3},
        ],
    },
    "oak": {
        "index": 2, "height": (8.0, 10.0), "radius": 0.042, "bark": (0.30, 0.24, 0.18), "birch": False,
        "leaf": "broad", "leafColors": [(0.22, 0.40, 0.12), (0.30, 0.48, 0.14), (0.18, 0.34, 0.10)], "cardSize": 1.6, "cardsPerPoint": 10, "spread": 1.0, "oriented": False,
        "levels": [
            {"segs": 4, "taper": 0.6, "jitter": 0.1, "up": 0.04, "kids": 6, "start": 0.55, "ang": (36, 62), "kidLen": 1.0, "kidRad": 0.6, "env": flat, "sides": 8, "lengthRatio": 0.5},
            {"segs": 3, "taper": 0.4, "jitter": 0.16, "up": 0.1, "kids": 3, "start": 0.35, "ang": (28, 55), "kidLen": 0.55, "kidRad": 0.5, "env": flat, "sides": 5},
            {"segs": 2, "taper": 0.5, "jitter": 0.16, "up": 0.04, "kids": 0, "sides": 3},
        ],
    },
    "pine": {
        "index": 3, "height": (9.0, 13.0), "radius": 0.02, "bark": (0.36, 0.26, 0.19), "birch": False,
        "leaf": "needle", "leafColors": [(0.10, 0.28, 0.14), (0.14, 0.34, 0.16), (0.08, 0.22, 0.12)], "cardSize": 1.4, "cardsPerPoint": 5, "spread": 0.15, "oriented": True,
        "levels": [
            {"segs": 8, "taper": 0.12, "jitter": 0.025, "up": 0.0, "kids": 54, "whorl": 6, "start": 0.24, "ang": (74, 100), "kidLen": 0.25, "kidRad": 0.2, "env": cone, "sides": 7},
            {"segs": 3, "taper": 0.5, "jitter": 0.08, "up": -0.02, "kids": 0, "sides": 3},
        ],
    },
    "juniper": {
        "index": 4, "height": (2.6, 3.6), "radius": 0.03, "bark": (0.40, 0.28, 0.20), "birch": False,
        "leaf": "scale", "leafColors": [(0.16, 0.30, 0.22), (0.20, 0.36, 0.24), (0.12, 0.26, 0.20)], "cardSize": 1.0, "cardsPerPoint": 10, "spread": 0.3, "oriented": False,
        "levels": [
            {"segs": 1, "taper": 0.8, "jitter": 0.02, "up": 0.0, "kids": 5, "start": 0.7, "ang": (14, 38), "kidLen": 5.5, "kidRad": 0.6, "env": flat, "sides": 6, "lengthRatio": 0.14},
            {"segs": 4, "taper": 0.3, "jitter": 0.15, "up": 0.05, "kids": 6, "start": 0.25, "ang": (35, 60), "kidLen": 0.35, "kidRad": 0.45, "env": flat, "sides": 4},
            {"segs": 2, "taper": 0.5, "jitter": 0.15, "up": 0.03, "kids": 0, "sides": 3},
        ],
    },
}


def perpendicular(pDir):
    tRef = Vector((1.0, 0.0, 0.0)) if abs(pDir.x) < 0.9 else Vector((0.0, 1.0, 0.0))
    return pDir.cross(tRef).normalized()


class TreeBuilder:
    def __init__(self, pSpec, pSeed, pLod):
        self.spec = pSpec
        self.rng = random.Random(pSeed)
        self.lod = pLod
        self.levels = [dict(l) for l in pSpec["levels"]]
        self.branches = []
        self.ringOffsets = {}
        self.leafPoints = []
        self.height = self.rng.uniform(*pSpec["height"])

    def build(self):
        tL0 = self.levels[0]
        tLength = self.height * tL0.get("lengthRatio", 1.0) * (1.0 if tL0.get("lengthRatio") else 1.0)
        self.grow(Vector((0.0, 0.0, -0.05)), UP.copy(), tLength, self.height * self.spec["radius"], 0)

    def grow(self, pStart, pDir, pLength, pRadius, pDepth):
        tLevel = self.levels[pDepth]
        tSegs = tLevel["segs"]
        tPoints = [pStart.copy()]
        tRadii = [pRadius * (1.25 if pDepth == 0 else 1.0)]
        tEnd = pRadius * tLevel["taper"]
        tCur = pStart.copy()
        tDir = pDir.copy()
        for tI in range(tSegs):
            tNoise = Vector((self.rng.uniform(-1, 1), self.rng.uniform(-1, 1), self.rng.uniform(-1, 1)))
            tDir = (tDir + tNoise * tLevel["jitter"] + UP * tLevel["up"]).normalized()
            tCur = tCur + tDir * (pLength / tSegs)
            tPoints.append(tCur.copy())
            tRadii.append((pRadius + (tEnd - pRadius) * (tI + 1) / tSegs) * (1.0 + self.rng.uniform(-1, 1) * tLevel.get("irregular", 0.0)))
        self.branches.append((tPoints, tRadii, tLevel["sides"], pDepth))
        if pDepth + 1 < len(self.levels):
            self.spawn(tLevel, tPoints, tRadii, pLength, pDepth)
        else:
            for tI in range(1, len(tPoints)):
                self.leafPoints.append((tPoints[tI], (tPoints[tI] - tPoints[tI - 1]).normalized()))

    def spawn(self, pLevel, pPoints, pRadii, pLength, pDepth):
        tKids = pLevel["kids"]
        tSegs = len(pPoints) - 1
        for tK in range(tKids):
            tT = pLevel["start"] + (1.0 - pLevel["start"]) * (tK + self.rng.random() * 0.5) / tKids
            tF = min(tT * tSegs, tSegs - 1e-6)
            tI = int(tF)
            tFrac = tF - tI
            tPos = pPoints[tI].lerp(pPoints[tI + 1], tFrac)
            tRad = pRadii[tI] + (pRadii[tI + 1] - pRadii[tI]) * tFrac
            tDir = (pPoints[tI + 1] - pPoints[tI]).normalized()
            tU = perpendicular(tDir)
            tV = tDir.cross(tU)
            tWhorl = pLevel.get("whorl", 0)
            if tWhorl:
                tRing = tK // tWhorl
                tOff = self.ringOffsets.setdefault(tRing, self.rng.uniform(0, math.tau))
                tAz = tOff + (tK % tWhorl) * math.tau / tWhorl + self.rng.uniform(-0.45, 0.45)
                tT = pLevel["start"] + (1.0 - pLevel["start"]) * (tRing + self.rng.random() * 0.3) / (tKids / tWhorl)
                tF = min(tT * tSegs, tSegs - 1e-6)
                tI = int(tF)
                tFrac = tF - tI
                tPos = pPoints[tI].lerp(pPoints[tI + 1], tFrac)
                tRad = pRadii[tI] + (pRadii[tI + 1] - pRadii[tI]) * tFrac
            else:
                tAz = tK * 2.399963 + self.rng.uniform(-0.4, 0.4)
            tPol = math.radians(self.rng.uniform(*pLevel["ang"]))
            tAxis = tU * math.cos(tAz) + tV * math.sin(tAz)
            tChild = tDir * math.cos(tPol) + tAxis * math.sin(tPol)
            tLen = pLength * pLevel["kidLen"] * pLevel["env"](tT) * self.rng.uniform(0.85, 1.15)
            self.grow(tPos, tChild.normalized(), tLen, max(0.012, tRad * pLevel["kidRad"]), pDepth + 1)


def value_noise(pH, pW, pCh, pCw, pRng):
    tGrid = pRng.random((pCh, pCw))
    tYs = np.arange(pH) * pCh / pH
    tXs = np.arange(pW) * pCw / pW
    tY0 = np.floor(tYs).astype(int)
    tX0 = np.floor(tXs).astype(int)
    tFy = tYs - tY0
    tFx = tXs - tX0
    tFy = tFy * tFy * (3 - 2 * tFy)
    tFx = tFx * tFx * (3 - 2 * tFx)
    tY1 = (tY0 + 1) % pCh
    tX1 = (tX0 + 1) % pCw
    tA = tGrid[np.ix_(tY0 % pCh, tX0 % pCw)]
    tB = tGrid[np.ix_(tY0 % pCh, tX1)]
    tC = tGrid[np.ix_(tY1, tX0 % pCw)]
    tD = tGrid[np.ix_(tY1, tX1)]
    tTop = tA + (tB - tA) * tFx[None, :]
    tBot = tC + (tD - tC) * tFx[None, :]
    return tTop + (tBot - tTop) * tFy[:, None]


def fbm(pSize, pRng, pCh, pCw, pOctaves):
    tSum = np.zeros((pSize, pSize))
    tAmp, tNorm = 1.0, 0.0
    for tO in range(pOctaves):
        tSum += tAmp * value_noise(pSize, pSize, pCh * 2 ** tO, pCw * 2 ** tO, pRng)
        tNorm += tAmp
        tAmp *= 0.5
    return tSum / tNorm


def make_bark(pSpec, pSeed):
    tRng = np.random.default_rng(pSeed)
    tSize = BARK_SIZE
    tBase = np.array(pSpec["bark"])
    if pSpec["birch"]:
        tTone = 0.8 + 0.2 * fbm(tSize, tRng, 6, 4, 4)
        tLines = value_noise(tSize, tSize, 90, 3, tRng)
        tLineDark = np.clip((tLines - 0.62) * 14.0, 0.0, 1.0) * np.clip(value_noise(tSize, tSize, 6, 5, tRng) * 1.6 - 0.2, 0.0, 1.0)
        tBlot = 0.75 * np.clip((fbm(tSize, tRng, 28, 8, 3) - 0.7) * 18.0, 0.0, 1.0)
        tGrain = 0.93 + 0.07 * value_noise(tSize, tSize, 256, 32, tRng)
        tShade = tTone * tGrain * (1.0 - 0.85 * np.maximum(tLineDark, tBlot))
    else:
        tStreak = fbm(tSize, tRng, 5, 26, 3)
        tFine = fbm(tSize, tRng, 60, 90, 3)
        tCrack = np.clip((fbm(tSize, tRng, 8, 18, 3) - 0.6) * 8.0, 0.0, 1.0)
        tShade = (0.5 + 0.7 * tStreak) * (0.8 + 0.4 * tFine) * (1.0 - 0.65 * tCrack)
    tPix = np.clip(tBase[None, None, :] * tShade[:, :, None], 0.0, 1.0)
    return np.concatenate([tPix, np.ones((tSize, tSize, 1))], axis=2)


def stamp(pImg, pCx, pCy, pAng, pA, pB, pColor, pVein):
    tR = int(max(pA, pB)) + 3
    tY0, tY1 = max(0, int(pCy) - tR), min(pImg.shape[0], int(pCy) + tR + 1)
    tX0, tX1 = max(0, int(pCx) - tR), min(pImg.shape[1], int(pCx) + tR + 1)
    if tY1 <= tY0 or tX1 <= tX0:
        return
    tYs, tXs = np.mgrid[tY0:tY1, tX0:tX1]
    tDx, tDy = tXs - pCx, tYs - pCy
    tCa, tSa = math.cos(pAng), math.sin(pAng)
    tLx = (tDx * tCa + tDy * tSa) / pA
    tLy = (-tDx * tSa + tDy * tCa) / pB
    tD = np.sqrt(tLx * tLx + tLy * tLy)
    tAlpha = np.clip(0.5 + ((1.0 - tD) * min(pA, pB * 2.5) - EDGE_RAMP * 0.45) / EDGE_RAMP, 0.0, 1.0)
    tColor = np.array(pColor)[None, None, :] * (1.0 - 0.18 * np.clip(tD, 0.0, 1.0))[:, :, None]
    if pVein:
        tLine = np.clip(1.0 - np.abs(tLy) * pB * 0.25 / LEAF_SCALE, 0.0, 1.0) * (np.abs(tLx) < 0.9)
        tColor = tColor * (1.0 + 0.3 * tLine[:, :, None])
    tReg = pImg[tY0:tY1, tX0:tX1]
    tW = tAlpha * (tAlpha > tReg[:, :, 3] * 0.6)
    tReg[:, :, :3] = tReg[:, :, :3] * (1 - tW[:, :, None]) + tColor * tW[:, :, None]
    tReg[:, :, 3] = np.maximum(tReg[:, :, 3], tAlpha)


def make_leaf_atlas(pSpec, pSeed):
    tRng = random.Random(pSeed)
    tS = LEAF_SCALE
    tCell = 128 * tS
    tImg = np.zeros((tCell * 2, tCell * 2, 4))
    tColors = pSpec["leafColors"]
    tAvg = np.mean(np.array(tColors), axis=0)
    for tCy in range(2):
        for tCx in range(2):
            tCell2 = np.zeros((tCell, tCell, 4))
            tCell2[:, :, :3] = tAvg
            tOx, tOy = tCx * tCell, tCy * tCell
            tKind = pSpec["leaf"]
            tTint = lambda c: tuple(min(1.0, v * tRng.uniform(0.82, 1.18)) for v in c)
            if tKind == "broad":
                for _ in range(18):
                    tAng = tRng.uniform(0, math.tau)
                    tRad = tRng.uniform(0, 42) * tS
                    tPx = tCell / 2 + math.cos(tAng) * tRad
                    tPy = tCell / 2 + math.sin(tAng) * tRad
                    tDirA = math.atan2(tPy - tCell / 2, tPx - tCell / 2) + tRng.uniform(-0.7, 0.7)
                    stamp(tCell2, tPx, tPy, tDirA, tRng.uniform(15, 21) * tS, tRng.uniform(7, 10) * tS, tTint(tRng.choice(tColors)), True)
            elif tKind == "needle":
                for _ in range(120):
                    tAng = math.radians(tRng.uniform(-64, 64)) - math.pi / 2
                    tLen = tRng.uniform(34, 60) * tS
                    tBx, tBy = tCell / 2 + tRng.uniform(-6, 6) * tS, tCell - 8 * tS
                    stamp(tCell2, tBx + math.cos(tAng) * tLen, tBy + math.sin(tAng) * tLen, tAng, tLen, tRng.uniform(0.7, 1.3) * tS, tTint(tRng.choice(tColors)), False)
                for _ in range(10):
                    tAng = math.radians(tRng.uniform(-30, 30)) - math.pi / 2
                    tLen = tRng.uniform(50, 62) * tS
                    stamp(tCell2, tCell / 2 + math.cos(tAng) * tLen, tCell - 8 * tS + math.sin(tAng) * tLen, tAng, tLen, 1.6 * tS, tuple(v * 0.7 for v in tColors[0]), False)
            else:
                for _ in range(70):
                    tAng = tRng.uniform(0, math.tau)
                    tRad = tRng.uniform(0, 44) * (0.5 + 0.5 * tRng.random()) * tS
                    stamp(tCell2, tCell / 2 + math.cos(tAng) * tRad, tCell / 2 + math.sin(tAng) * tRad * 0.8, tRng.uniform(0, math.tau), tRng.uniform(7, 11) * tS, tRng.uniform(5, 8) * tS, tTint(tRng.choice(tColors)), False)
            tImg[tOy:tOy + tCell, tOx:tOx + tCell] = tCell2
    return tImg


def make_image(pName, pArray):
    tH, tW = pArray.shape[0], pArray.shape[1]
    tImage = bpy.data.images.new(pName, tW, tH, alpha=True)
    tImage.pixels.foreach_set(np.flipud(pArray).astype(np.float32).flatten())
    tImage.pack()
    return tImage


def make_material(pName, pImage, pAlpha):
    tMat = bpy.data.materials.new(pName)
    tMat.use_nodes = True
    tNodes = tMat.node_tree.nodes
    tLinks = tMat.node_tree.links
    tBsdf = tNodes["Principled BSDF"]
    tTex = tNodes.new("ShaderNodeTexImage")
    tTex.image = pImage
    tTex.interpolation = "Linear"
    tLinks.new(tTex.outputs["Color"], tBsdf.inputs["Base Color"])
    tBsdf.inputs["Roughness"].default_value = 0.92
    if pAlpha:
        tLinks.new(tTex.outputs["Alpha"], tBsdf.inputs["Alpha"])
        try:
            tMat.blend_method = "CLIP"
        except Exception:
            pass
    return tMat


def make_ring_frames(pPoints):
    tFrames = []
    tU = None
    for tI in range(len(pPoints)):
        tA = pPoints[max(tI - 1, 0)]
        tB = pPoints[min(tI + 1, len(pPoints) - 1)]
        tD = (tB - tA).normalized()
        tU = perpendicular(tD) if tU is None else (tU - tD * tU.dot(tD)).normalized()
        tFrames.append((tU, tD.cross(tU)))
    return tFrames


def trunk_mesh(pBuilder, pTriLimit=None):
    tVerts, tFaces, tUvs = [], [], []
    tBranches = pBuilder.branches
    if pBuilder.lod:
        tBranches = []
        tUsed = 0
        for tB in pBuilder.branches:
            if tB[3] > 1:
                continue
            tCost = 2 * (4 if tB[3] == 0 else 3) * (len(tB[0]) - 1)
            if tUsed + tCost > pTriLimit:
                continue
            tUsed += tCost
            tBranches.append((tB[0], tB[1], 4 if tB[3] == 0 else 3, tB[3]))
    for tPoints, tRadii, tSides, tDepth in tBranches:
        tFrames = make_ring_frames(tPoints)
        tBase = len(tVerts)
        tDist = 0.0
        tVs = []
        tTile = max(math.tau * tRadii[0], 0.4)
        for tI, tP in enumerate(tPoints):
            if tI:
                tDist += (tP - tPoints[tI - 1]).length
            tVs.append(tDist)
            tU, tV = tFrames[tI]
            for tS in range(tSides):
                tA = tS * math.tau / tSides
                tVerts.append(tP + (tU * math.cos(tA) + tV * math.sin(tA)) * tRadii[tI])
        for tI in range(len(tPoints) - 1):
            for tS in range(tSides):
                tN = (tS + 1) % tSides
                tA0 = tBase + tI * tSides + tS
                tA1 = tBase + tI * tSides + tN
                tB0 = tBase + (tI + 1) * tSides + tS
                tB1 = tBase + (tI + 1) * tSides + tN
                tFaces.append((tA0, tA1, tB1, tB0))
                tU0, tU1 = 0.005 + 0.49 * tS / tSides, 0.005 + 0.49 * (tS + 1) / tSides
                tV0, tV1 = tVs[tI] / tTile, tVs[tI + 1] / tTile
                tUvs.append(((tU0, tV0), (tU1, tV0), (tU1, tV1), (tU0, tV1)))
    return tVerts, tFaces, tUvs


def leaf_mesh(pBuilder, pCardCap):
    tSpec = pBuilder.spec
    tRng = pBuilder.rng
    tCandidates = [(p, d) for p, d in pBuilder.leafPoints for _ in range(tSpec["cardsPerPoint"])]
    tRng.shuffle(tCandidates)
    tScale = 1.0
    if len(tCandidates) > pCardCap:
        tScale = math.sqrt(len(tCandidates) / max(pCardCap, 1))
        tCandidates = tCandidates[:pCardCap]
    tCenter = sum((p for p, _ in pBuilder.leafPoints), Vector()) / max(len(pBuilder.leafPoints), 1)
    tSize = tSpec["cardSize"] * tScale
    tVerts, tFaces, tUvs, tNormals = [], [], [], []
    for tPoint, tDir in tCandidates:
        tOffset = Vector((tRng.uniform(-1, 1), tRng.uniform(-1, 1), tRng.uniform(-1, 1))) * tSpec["spread"]
        tPos = tPoint + tOffset
        if tSpec["oriented"]:
            tLong = (tDir + Vector((0, 0, -0.25))).normalized()
            tSide = perpendicular(tLong)
            tRoll = tRng.uniform(0, math.pi)
            tSide = tSide * math.cos(tRoll) + tLong.cross(tSide) * math.sin(tRoll)
            tW, tH = tSize * tRng.uniform(0.8, 1.1), tSize * tRng.uniform(1.0, 1.4)
            tPos = tPos + tLong * tH * 0.4
            tAxisU, tAxisV = tSide, tLong
        else:
            tNorm = Vector((tRng.uniform(-1, 1), tRng.uniform(-1, 1), tRng.uniform(-0.6, 1))).normalized()
            tAxisU = perpendicular(tNorm)
            tAxisV = tNorm.cross(tAxisU)
            tW = tH = tSize * tRng.uniform(0.8, 1.2)
        tBase = len(tVerts)
        for tSx, tSy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
            tV = tPos + tAxisU * tSx * tW * 0.5 + tAxisV * tSy * tH * 0.5
            tVerts.append(tV)
            tN = (tV - tCenter).normalized() * 0.75 + UP * 0.35
            tNormals.append(tN.normalized())
        tFaces.append((tBase, tBase + 1, tBase + 2, tBase + 3))
        tCx, tCy = tRng.randint(0, 1), tRng.randint(0, 1)
        tU0, tV0 = 0.5 + tCx * 0.25 + 0.005, tCy * 0.5 + 0.01
        tU1, tV1 = tU0 + 0.24, tV0 + 0.48
        tUvs.append(((tU0, tV0), (tU1, tV0), (tU1, tV1), (tU0, tV1)))
    return tVerts, tFaces, tUvs, tNormals


def make_object(pName, pVerts, pFaces, pUvs, pMat, pNormals):
    tMesh = bpy.data.meshes.new(pName)
    tMesh.from_pydata([tuple(v) for v in pVerts], [], pFaces)
    tLayer = tMesh.uv_layers.new(name="UVMap")
    tLoop = 0
    for tFaceUv in pUvs:
        for tUv in tFaceUv:
            tLayer.data[tLoop].uv = tUv
            tLoop += 1
    tMesh.polygons.foreach_set("use_smooth", [True] * len(tMesh.polygons))
    tMesh.update()
    if pNormals:
        tMesh.normals_split_custom_set_from_vertices([tuple(n) for n in pNormals])
    tMesh.materials.append(pMat)
    tObj = bpy.data.objects.new(pName, tMesh)
    bpy.context.scene.collection.objects.link(tObj)
    return tObj


def patch_glb(pPath):
    tData = Path(pPath).read_bytes()
    tJsonLen = struct.unpack_from("<I", tData, 12)[0]
    tJson = json.loads(tData[20:20 + tJsonLen].decode("utf-8"))
    for tMat in tJson.get("materials", []):
        if tMat["name"].endswith("_leaf"):
            tMat["alphaMode"] = "MASK"
            tMat["alphaCutoff"] = 0.5
            tMat["doubleSided"] = True
        else:
            tMat["alphaMode"] = "OPAQUE"
            tMat.pop("alphaCutoff", None)
    tNew = json.dumps(tJson, separators=(",", ":")).encode("utf-8")
    tNew += b" " * ((4 - len(tNew) % 4) % 4)
    tRest = tData[20 + tJsonLen:]
    tOut = struct.pack("<4sII", b"glTF", 2, 12 + 8 + len(tNew) + len(tRest)) + struct.pack("<I4s", len(tNew), b"JSON") + tNew + tRest
    Path(pPath).write_bytes(tOut)


def generate(pName, pVariant, pLod, pOutDir):
    tSpec = SPECIES[pName]
    tSeed = 7919 * (tSpec["index"] + 1) + 104729 * pVariant
    bpy.ops.wm.read_factory_settings(use_empty=True)
    tBudget = LOD_BUDGET if pLod else FULL_BUDGET
    tBuilder = None
    for tDrop in range(3):
        tBuilder = TreeBuilder(tSpec, tSeed, pLod)
        if tDrop:
            tBuilder.levels = tBuilder.levels[:-tDrop]
            tBuilder.levels[-1] = dict(tBuilder.levels[-1], kids=0)
        tBuilder.build()
        tTrunk = trunk_mesh(tBuilder, tBudget * 0.3)
        if len(tTrunk[1]) * 2 <= tBudget * 0.6:
            break
    tTrunkTris = len(tTrunk[1]) * 2
    tCap = max((tBudget - tTrunkTris) // 2, 10)
    tLeaf = leaf_mesh(tBuilder, tCap)
    tAtlas = np.concatenate([make_bark(tSpec, tSeed), make_leaf_atlas(tSpec, tSeed)], axis=1)
    tBark = tLeafMat = make_material(pName + "_leaf", make_image(pName + "_atlas", tAtlas), True)
    tObjects = [make_object("trunk", tTrunk[0], tTrunk[1], tTrunk[2], tBark, None), make_object("leaves", tLeaf[0], tLeaf[1], tLeaf[2], tLeafMat, tLeaf[3])]
    bpy.ops.object.select_all(action="DESELECT")
    for tObj in tObjects:
        tObj.select_set(True)
    tSuffix = "_lod1" if pLod else ""
    tPath = Path(pOutDir) / ("%s_%d%s.glb" % (pName, pVariant + 1, tSuffix))
    bpy.ops.export_scene.gltf(filepath=str(tPath), export_format="GLB", use_selection=True, export_yup=True, export_apply=True)
    patch_glb(tPath)
    print("TREE %s tris=%d (trunk %d, leaf %d)" % (tPath.name, tTrunkTris + len(tLeaf[1]) * 2, tTrunkTris, len(tLeaf[1]) * 2))


def main():
    tArgs = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    tOut = Path(tArgs[0]) if tArgs else OUT_DEFAULT
    tNames = tArgs[1:] or list(SPECIES)
    tOut.mkdir(parents=True, exist_ok=True)
    for tName in tNames:
        for tVariant in range(VARIANTS):
            for tLod in (False, True):
                generate(tName, tVariant, tLod, tOut)


if __name__ == "__main__":
    main()
