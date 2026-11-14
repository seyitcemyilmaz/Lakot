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


def run_interactive():
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


PSQL = "C:/Program Files/PostgreSQL/18/bin/psql.exe"
EQUIPPED_ITEMS = [(1001, 1, 1), (2001, 1, 2), (3001, 1, 3)]
BAG_ITEMS = [(4, 5, 0), (7, 3, 1), (5, 8, 2), (6, 1, 10)]


def run_sql(pSql):
    return subprocess.run([PSQL, "-h", "localhost", "-U", os.environ.get("LAKOT_DB_USER", "postgres"), "-d", "lakot_db", "-At", "-c", pSql],
                          env={**os.environ, "PGPASSWORD": os.environ["LAKOT_DB_PASSWORD"]}, check=True, capture_output=True, text=True).stdout.strip()


def give_equipment(pName):
    tId = int(run_sql(f"SELECT id FROM characters WHERE name='{pName}'"))
    run_sql("DELETE FROM item_instances WHERE character_id=%d" % tId)
    for tTemplate, tCount, tSlot in EQUIPPED_ITEMS:
        run_sql(f"INSERT INTO item_instances (character_id, template_id, count, location, slot) VALUES ({tId}, {tTemplate}, {tCount}, 1, {tSlot})")
    for tTemplate, tCount, tSlot in BAG_ITEMS:
        run_sql(f"INSERT INTO item_instances (character_id, template_id, count, location, slot) VALUES ({tId}, {tTemplate}, {tCount}, 0, {tSlot})")


def login_steps(pUser, pPassword):
    return [
        "wait 4000", "shot 01_login",
        "click 0.5 0.35", "text " + pUser, "click 0.5 0.44", "text " + pPassword, "click 0.5 0.525",
        "wait 2500", "shot 02_character_select",
        "click 0.5 0.535", "wait 7000", "shot 03_world",
    ]


def panel_steps(pStartIndex, pPanels):
    tSteps = []
    for tIndex, (tName, tKey) in enumerate(pPanels, start=pStartIndex):
        tSteps += [f"key {tKey}", "wait 1200", f"shot {tIndex:02d}_{tName}", f"key {tKey}", "wait 500"]
    return tSteps


EMPTY_PANELS = [("inventory", "i"), ("character", "c"), ("skills", "k"), ("worldmap", "m"), ("systemmenu", "esc")]
EQUIPPED_PANELS = [("inventory_equipped", "i"), ("character_equipped", "c")]


def run_game(pBinDir, pRunRoot, pName, pScript, pOutDir, pSize):
    tScriptPath = pRunRoot / f"{pName}.txt"
    tScriptPath.write_text(os.linesep.join(pScript) + os.linesep, encoding="utf-8")
    tGameDir = pRunRoot / f"game_{pName}"
    shutil.copytree(pBinDir, tGameDir)
    tEnv = {**os.environ, "LAKOT_SERVER_HOST": "127.0.0.1", "LAKOT_SERVER_PORT": str(pt.PORT), "LAKOT_SNAPSHOT_SCRIPT": str(tScriptPath),
            "LAKOT_SNAPSHOT_OUT": str(pOutDir), "LAKOT_SNAPSHOT_SIZE": pSize}
    with open(pRunRoot / f"game_{pName}.log", "w") as tLog:
        tGame = subprocess.Popen([str(tGameDir / "LakotGame.exe")], cwd=tGameDir, env=tEnv, stdout=tLog, stderr=subprocess.STDOUT)
        try:
            tGame.wait(timeout=180)
        except subprocess.TimeoutExpired:
            tGame.kill()
            print(f"{pName}: timed out", flush=True)
    shutil.copy(pRunRoot / f"game_{pName}.log", pOutDir / f"game_{pName}.log")


def run_scripted():
    tSize = os.environ.get("LAKOT_SNAPSHOT_SIZE") or "2560x1440"
    tOutDir = Path(os.environ.get("LAKOT_VISION_OUT") or (HERE.parent.parent / "tasks" / "vision" / time.strftime("%Y%m%d-%H%M%S")))
    tOutDir.mkdir(parents=True, exist_ok=True)
    tBinDir = Path(pt.lakotnet.resolve_bin())
    tRunRoot = Path(tempfile.mkdtemp(prefix="lakot_ui_run_"))
    tServer = pt.start_server(tBinDir, tRunRoot)
    try:
        tUser, tPassword, _ = pt.make_account()
        run_game(tBinDir, tRunRoot, "empty", login_steps(tUser, tPassword) + panel_steps(4, EMPTY_PANELS), tOutDir, tSize)
        tUser2, tPassword2, tName2 = pt.make_account()
        give_equipment(tName2)
        run_game(tBinDir, tRunRoot, "equipped", login_steps(tUser2, tPassword2) + panel_steps(10, EQUIPPED_PANELS), tOutDir, tSize)
        print(f"snapshots: {tOutDir}")
    finally:
        if tServer.poll() is None:
            tServer.kill()
        shutil.copy(tRunRoot / "server.log", tOutDir / "server.log")
        shutil.rmtree(tRunRoot, ignore_errors=True)


def main():
    if "--interactive" in sys.argv:
        run_interactive()
    else:
        run_scripted()


if __name__ == "__main__":
    main()
