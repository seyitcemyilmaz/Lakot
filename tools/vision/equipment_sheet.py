import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import play_test as pt
import ui_snapshots as ui

ROOT = HERE.parent.parent
ITEMS = ROOT / "data" / "items.json"
SLOT_WEAPON = 1
SLOT_HELMET = 3
CROP = (880, 340, 1680, 1140)
TILE = 400
LABEL = 56
COLUMNS = 4
VIEWS = ("front", "side")
ZOOM = os.environ.get("LAKOT_SHEET_ZOOM", "3")
ORBIT = os.environ.get("LAKOT_SHEET_ORBIT", "450")


def load_items():
    tItems = json.loads(ITEMS.read_text(encoding="utf-8"))["items"]
    tWeapons = [t for t in tItems if t["equipSlot"] == SLOT_WEAPON]
    tHelmets = [t for t in tItems if t["equipSlot"] == SLOT_HELMET]
    return tWeapons, tHelmets


def has_model(pItem):
    tModel = pItem.get("model") or ""
    return bool(tModel) and (ROOT / "data" / tModel).exists()


def equip(pName, pItems):
    tId = int(ui.run_sql(f"SELECT id FROM characters WHERE name='{pName}'"))
    ui.run_sql("DELETE FROM item_instances WHERE character_id=%d" % tId)
    for tItem in pItems:
        ui.run_sql(f"INSERT INTO item_instances (character_id, template_id, count, location, slot) "
                   f"VALUES ({tId}, {tItem['id']}, 1, 1, {tItem['equipSlot']})")


def session_steps(pUser, pPassword):
    return ui.login_steps(pUser, pPassword) + [
        f"wheel {ZOOM}", "wait 1500", "shot front", "wait 300",
        f"orbit {ORBIT} 0", "wait 1200", "shot side",
    ]


def play(pGameDir, pRunRoot, pName, pSteps, pOutDir):
    tScript = pRunRoot / f"{pName}.txt"
    tScript.write_text(os.linesep.join(pSteps) + os.linesep, encoding="utf-8")
    tEnv = {**os.environ, "LAKOT_SERVER_HOST": "127.0.0.1", "LAKOT_SERVER_PORT": str(pt.PORT), "LAKOT_SNAPSHOT_SCRIPT": str(tScript),
            "LAKOT_SNAPSHOT_OUT": str(pOutDir), "LAKOT_SNAPSHOT_SIZE": "2560x1440"}
    with open(pRunRoot / f"{pName}.log", "w") as tLog:
        tGame = subprocess.Popen([str(pGameDir / "LakotGame.exe")], cwd=pGameDir, env=tEnv, stdout=tLog, stderr=subprocess.STDOUT)
        try:
            tGame.wait(timeout=120)
        except subprocess.TimeoutExpired:
            tGame.kill()
            print(f"{pName}: timed out", flush=True)


def font(pSize):
    for tPath in ("C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf"):
        if os.path.exists(tPath):
            return ImageFont.truetype(tPath, pSize)
    return ImageFont.load_default()


def make_tile(pShot, pLabel, pSub):
    tTile = Image.new("RGB", (TILE, TILE + LABEL), (24, 24, 30))
    if pShot is not None and pShot.exists():
        tTile.paste(Image.open(pShot).convert("RGB").crop(CROP).resize((TILE, TILE), Image.LANCZOS), (0, 0))
    tDraw = ImageDraw.Draw(tTile)
    tDraw.text((8, TILE + 4), pLabel, fill=(235, 235, 240), font=font(15))
    tDraw.text((8, TILE + 30), pSub, fill=(217, 166, 64) if "no model" in pSub else (150, 150, 165), font=font(16))
    return tTile


def build_sheet(pTiles, pPath):
    tRows = (len(pTiles) + COLUMNS - 1) // COLUMNS
    tSheet = Image.new("RGB", (COLUMNS * TILE, tRows * (TILE + LABEL)), (12, 12, 16))
    for tIndex, tTile in enumerate(pTiles):
        tSheet.paste(tTile, ((tIndex % COLUMNS) * TILE, (tIndex // COLUMNS) * (TILE + LABEL)))
    tSheet.save(pPath)


def main():
    tOutDir = Path(os.environ.get("LAKOT_VISION_OUT") or (ROOT / "tasks" / "vision" / time.strftime("%Y%m%d-%H%M%S")))
    tOutDir.mkdir(parents=True, exist_ok=True)
    tWeapons, tHelmets = load_items()
    tCount = max(len(tWeapons), len(tHelmets))
    tBinDir = Path(pt.lakotnet.resolve_bin())
    tRunRoot = Path(tempfile.mkdtemp(prefix="lakot_sheet_run_"))
    tServer = pt.start_server(tBinDir, tRunRoot)
    tTiles = []
    try:
        tGameDir = tRunRoot / "game"
        shutil.copytree(tBinDir, tGameDir)
        for tIndex in range(tCount):
            tPair = [t[tIndex] for t in (tWeapons, tHelmets) if tIndex < len(t)]
            tLabel = " + ".join(t["name"] for t in tPair)
            tMissing = [t["name"] for t in tPair if not has_model(t)]
            tSub = "no model: " + ", ".join(tMissing) if tMissing else " / ".join(Path(t["model"]).name for t in tPair)
            tUser, tPassword, tName = pt.make_account()
            equip(tName, tPair)
            tShotDir = tOutDir / f"s{tIndex:02d}"
            play(tGameDir, tRunRoot, f"s{tIndex:02d}", session_steps(tUser, tPassword), tShotDir)
            for tView in VIEWS:
                tTiles.append(make_tile(tShotDir / f"{tView}.png", f"{tLabel} ({tView})", tSub))
            print(f"session {tIndex + 1}/{tCount}: {tLabel}", flush=True)
        build_sheet(tTiles, tOutDir / "equipment_sheet.png")
        print(f"equipment sheet: {tOutDir / 'equipment_sheet.png'}")
    finally:
        if tServer.poll() is None:
            tServer.kill()
        shutil.copy(tRunRoot / "server.log", tOutDir / "server.log")
        shutil.rmtree(tRunRoot, ignore_errors=True)


if __name__ == "__main__":
    main()
