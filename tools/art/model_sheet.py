"""Photographs and measures game GLBs with Blender in the background. Run: python model_sheet.py [out_dir] [model.glb ...] (default: every weapon, helmet and armor GLB under data/models; out_dir default tasks/vision/<time>)"""

import json
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
MODELS = ROOT / "data" / "models"
GROUPS = ("weapons", "helmets", "armor")
BLENDER = Path(os.environ.get("LAKOT_BLENDER") or r"C:\Program Files\Blender Foundation\Blender 4.5\blender.exe")
VIEWS = ("front", "side", "top")
TILE = 256
LABEL = 40
ROWS_PER_SHEET = 8
LIMITS = {"maxTriangles": 20000, "maxInvertedRatio": 0.5, "minLongestSide": 0.05, "maxLongestSide": 3.0}


def collect_models():
    return sorted(p for g in GROUPS if (MODELS / g).is_dir() for p in (MODELS / g).glob("*.glb"))


def check_entry(pEntry):
    tIssues = []
    if "error" in pEntry:
        return [pEntry["error"]]
    tLongest = max(pEntry["size"])
    if tLongest < LIMITS["minLongestSide"] or tLongest > LIMITS["maxLongestSide"]:
        tIssues.append("size %.3f outside %.2f..%.2f" % (tLongest, LIMITS["minLongestSide"], LIMITS["maxLongestSide"]))
    if pEntry["triangles"] > LIMITS["maxTriangles"]:
        tIssues.append("triangles %d" % pEntry["triangles"])
    if pEntry["invertedNormalRatio"] > LIMITS["maxInvertedRatio"]:
        tIssues.append("inverted normals %.0f%%" % (pEntry["invertedNormalRatio"] * 100))
    if not pEntry["hasTexture"]:
        tIssues.append("no texture")
    if pEntry["bones"]:
        tIssues.append("has rig (%d bones)" % pEntry["bones"])
    if not pEntry["scaleApplied"]:
        tIssues.append("object scale not applied")
    tLow, tHigh = pEntry["boundsMin"], pEntry["boundsMax"]
    if not all(tLow[i] <= 0.0 <= tHigh[i] for i in range(3)):
        tIssues.append("origin outside bounds")
    return tIssues


def to_blender(pGltf):
    return (pGltf[0], -pGltf[2], pGltf[1])


def project(pView, pPoint, pFrame):
    tCenter = to_blender(pFrame["center"])
    tAxes = {"front": (0, 2), "side": (1, 2), "top": (0, 1)}[pView]
    tHalf = pFrame["halfSpan"]
    tU = (pPoint[tAxes[0]] - tCenter[tAxes[0]]) / (2 * tHalf) * TILE + TILE / 2
    tV = TILE / 2 - (pPoint[tAxes[1]] - tCenter[tAxes[1]]) / (2 * tHalf) * TILE
    return tU, tV


def draw_markers(pDraw, pView, pEntry, pFrame):
    tOrigin = project(pView, (0.0, 0.0, 0.0), pFrame)
    pDraw.line([(tOrigin[0] - 8, tOrigin[1]), (tOrigin[0] + 8, tOrigin[1])], fill=(255, 60, 60), width=2)
    pDraw.line([(tOrigin[0], tOrigin[1] - 8), (tOrigin[0], tOrigin[1] + 8)], fill=(255, 60, 60), width=2)
    if pView == "top":
        return
    tLow, tHigh = pEntry["boundsMin"], pEntry["boundsMax"]
    tBottom = project(pView, (0.0, 0.0, tLow[1]), pFrame)[1]
    tTop = project(pView, (0.0, 0.0, tHigh[1]), pFrame)[1]
    pDraw.line([(0, tBottom), (TILE, tBottom)], fill=(60, 200, 255), width=1)
    pDraw.line([(0, tTop), (TILE, tTop)], fill=(255, 220, 60), width=1)
    pDraw.text((4, tBottom - 12), "bottom", fill=(60, 200, 255))
    pDraw.text((4, tTop + 2), "top", fill=(255, 220, 60))
    pDraw.text((TILE - 40, 4), "Y up", fill=(120, 255, 120))
    pDraw.line([(TILE - 14, 30), (TILE - 14, 14)], fill=(120, 255, 120), width=2)


def build_sheet(pReport, pShots, pTarget):
    tNames = list(pReport)
    tFont = ImageFont.load_default()
    tSheet = Image.new("RGB", (TILE * len(VIEWS), (TILE + LABEL) * len(tNames)), (24, 24, 28))
    tDraw = ImageDraw.Draw(tSheet)
    for tRow, tName in enumerate(tNames):
        tEntry = pReport[tName]
        tTop = tRow * (TILE + LABEL)
        tIssues = tEntry.get("issues", [])
        tDraw.text((6, tTop + 6), tName, fill=(255, 255, 255), font=tFont)
        tDraw.text((6, tTop + 22), "; ".join(tIssues) if tIssues else "ok", fill=(255, 120, 120) if tIssues else (120, 255, 120), font=tFont)
        if "frames" not in tEntry:
            continue
        for tColumn, tView in enumerate(VIEWS):
            tTile = Image.open(pShots / ("%s_%s.png" % (tName, tView))).convert("RGB")
            draw_markers(ImageDraw.Draw(tTile), tView, tEntry, tEntry["frames"][tView])
            tSheet.paste(tTile, (tColumn * TILE, tTop + LABEL))
    tSheet.save(pTarget)


def main():
    tArgs = sys.argv[1:]
    tOut = Path(tArgs[0]) if tArgs else ROOT / "tasks" / "vision" / time.strftime("model-%H%M%S")
    tModels = [Path(a).resolve() for a in tArgs[1:]] or collect_models()
    tOut.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tTemp:
        tShots = Path(tTemp)
        if not BLENDER.is_file():
            sys.exit("Blender bulunamadı: %s (LAKOT_BLENDER ile ayarla)" % BLENDER)
        subprocess.run([str(BLENDER), "-b", "-P", str(HERE / "model_probe.py"), "--", str(tShots)] + [str(m) for m in tModels], check=True)
        tReport = json.loads((tShots / "probe.json").read_text(encoding="utf-8"))
        for tEntry in tReport.values():
            tEntry["issues"] = check_entry(tEntry)
        tChecked = {n: {k: v for k, v in e.items() if k != "frames"} for n, e in tReport.items()}
        (tOut / "model_check.json").write_text(json.dumps({"limits": LIMITS, "models": tChecked}, indent=1), encoding="utf-8")
        build_sheet(tReport, tShots, tOut / "model_sheet.png")
    print("wrote", tOut)


main()
