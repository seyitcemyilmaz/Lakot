import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import play_test as pt

STEPS = [
    ("inventory", "i", "i"),
    ("character", "c", "c"),
    ("skills", "k", "k"),
    ("worldmap", "m", "m"),
    ("systemmenu", "esc", "esc"),
]


def click_at(pHwnd, pX, pY):
    tX, tY, tW, tH = pt.client_rect(pHwnd)
    pt.focus(pHwnd)
    pt.click(tX + pX * tW, tY + pY * tH, "left")


def shot(pHwnd, pOutDir, pName):
    pt.focus(pHwnd)
    time.sleep(0.6)
    pt.screenshot(pHwnd, pOutDir / f"{pName}.jpg")
    print(f"shot {pName}", flush=True)


def main():
    tOutDir = Path(os.environ.get("LAKOT_VISION_OUT") or tempfile.mkdtemp(prefix="lakot_ui_"))
    tOutDir.mkdir(parents=True, exist_ok=True)
    tBinDir = Path(pt.lakotnet.resolve_bin())
    tRunRoot = Path(tempfile.mkdtemp(prefix="lakot_ui_run_"))
    tServer = pt.start_server(tBinDir, tRunRoot)
    tGame = None
    try:
        tUser, tPassword, _ = pt.make_account()
        tGameDir = tRunRoot / "game"
        shutil.copytree(tBinDir, tGameDir)
        tGame = subprocess.Popen([str(tGameDir / "LakotGame.exe")], cwd=tGameDir,
                                 env={**os.environ, "LAKOT_SERVER_HOST": "127.0.0.1", "LAKOT_SERVER_PORT": str(pt.PORT)},
                                 stdout=open(tRunRoot / "game.log", "w"), stderr=subprocess.STDOUT)
        tHwnd = None
        tEnd = time.time() + 60
        while time.time() < tEnd and not tHwnd:
            time.sleep(1.0)
            tHwnd = pt.find_window(tGame.pid)
        if not tHwnd:
            raise RuntimeError("game window did not appear")
        time.sleep(4.0)
        shot(tHwnd, tOutDir, "01_login")
        click_at(tHwnd, 0.5, 0.35)
        pt.type_text(tUser)
        click_at(tHwnd, 0.5, 0.44)
        pt.type_text(tPassword)
        click_at(tHwnd, 0.5, 0.525)
        time.sleep(2.5)
        shot(tHwnd, tOutDir, "02_character_select")
        click_at(tHwnd, 0.5, 0.535)
        time.sleep(6.0)
        shot(tHwnd, tOutDir, "03_world")
        for tIndex, (tName, tOpenKey, tCloseKey) in enumerate(STEPS, start=4):
            pt.focus(tHwnd)
            pt.press(tOpenKey, 0.1)
            time.sleep(1.0)
            shot(tHwnd, tOutDir, f"{tIndex:02d}_{tName}")
            pt.press(tCloseKey, 0.1)
            time.sleep(0.5)
        for tIndex, tKey in enumerate(("d", "w", "a", "s"), start=20):
            pt.focus(tHwnd)
            pt.press(tKey, 1.2)
            time.sleep(0.3)
            shot(tHwnd, tOutDir, f"{tIndex:02d}_walk_{tKey}")
        pt.focus(tHwnd)
        pt.press("enter", 0.1)
        time.sleep(0.5)
        pt.type_text("ui snapshot")
        shot(tHwnd, tOutDir, f"{len(STEPS) + 4:02d}_chat")
        pt.press("esc", 0.1)
        print(f"snapshots: {tOutDir}")
    finally:
        if tGame and tGame.poll() is None:
            tGame.kill()
        if tServer.poll() is None:
            tServer.kill()
        for tLog in ("server.log", "game.log"):
            if (tRunRoot / tLog).exists():
                shutil.copy(tRunRoot / tLog, tOutDir / tLog)
        shutil.rmtree(tRunRoot, ignore_errors=True)


if __name__ == "__main__":
    main()
