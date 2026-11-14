import sys, os, time, math, json, shutil, subprocess, threading, socket, traceback
import numpy as np
from PIL import Image
from lakotnet import *
from mapnav import nav

generate_bindings()

RUN = type(HERE)(os.environ.get("LAKOT_TEST_RUN_DIR") or HERE / ".run_o4")
SITE = (0.0, 0.0)
WALL = (0.0, 0.0, 0.0, 0.0)
MONSTER_OFFSET = 16.0


def check(cond, msg):
    if not cond:
        raise AssertionError(msg)


def find_site():
    n = nav()
    for cx in range(-300, 301, 20):
        for cz in range(-300, 301, 20):
            if all(n.walkable(cx + dx, cz + dz) for dx in range(-30, 31, 3) for dz in range(-8, 25, 3)):
                return float(cx), float(cz)
    raise AssertionError("no open site for the wall scenario")


def template():
    return {"id": 201, "name": "T201", "model": "models/monsters/steppe_bandit.glb", "level": 1, "maxHealth": 1000000,
            "attack": 1, "defense": 0, "experience": 1, "moveSpeed": 4.0, "aggroRange": 30, "attackRange": 1.8,
            "attackInterval": 1.0, "leashRange": 150, "respawnSeconds": 600, "aggressive": True,
            "gold": {"min": 0, "max": 0, "chance": 0.0}, "drops": []}


class OverlayServer(Server):
    def __init__(self, pTemplates, pSpawns):
        super().__init__()
        self.templates, self.spawns = pTemplates, pSpawns

    def start(self, timeout=40):
        if not os.environ.get("LAKOT_DB_PASSWORD"):
            raise RuntimeError("LAKOT_DB_PASSWORD is not set")
        for _ in range(30):
            shutil.rmtree(RUN, ignore_errors=True)
            if not RUN.exists():
                break
            time.sleep(1)
        shutil.copytree(self.bin_dir, RUN, dirs_exist_ok=True)
        self.run_dir = str(RUN)
        json.dump({"monsters": self.templates}, open(RUN / "data" / "monsters.json", "w", encoding="utf-8"), indent=1)
        mp = RUN / "data" / "maps" / "101_altunay_village" / "map.json"
        meta = json.load(open(mp, encoding="utf-8"))
        meta["spawn"] = {"x": SITE[0], "z": SITE[1], "yaw": 0.0}
        meta["safeZones"] = []
        meta["monsterSpawns"] = self.spawns
        json.dump(meta, open(mp, "w", encoding="utf-8"))
        block = RUN / "data" / "maps" / "101_altunay_village" / "block.png"
        img = Image.open(block).convert("L")
        px = np.array(img)
        cs = float(meta["worldSize"]) / px.shape[0]
        half = float(meta["worldSize"]) / 2
        x0, x1 = int(math.floor((WALL[0] + half) / cs)), int(math.ceil((WALL[1] + half) / cs))
        z0, z1 = int(math.floor((WALL[2] + half) / cs)), int(math.ceil((WALL[3] + half) / cs))
        px[z0:z1, x0:x1] = 255
        Image.fromarray(px).save(block)
        self.proc = subprocess.Popen([os.path.join(self.run_dir, "LakotServer.exe")], cwd=self.run_dir,
                                     env={**os.environ, "LAKOT_SERVER_PORT": str(PORT)},
                                     stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                     text=True, errors="replace")
        threading.Thread(target=self._pump, daemon=True).start()
        end = time.time() + timeout
        while time.time() < end:
            if self.proc.poll() is not None:
                raise RuntimeError("server exited early:\n" + "\n".join(self.log_lines[-20:]))
            try:
                socket.create_connection(("127.0.0.1", PORT), timeout=0.5).close()
                return
            except OSError:
                time.sleep(0.3)
        raise RuntimeError("server did not open port\n" + "\n".join(self.log_lines[-20:]))



KIND_MONSTER = 1


def in_wall(x, z):
    return WALL[0] <= x <= WALL[1] and WALL[2] <= z <= WALL[3]


def t_o4_monster_walks_around_wall():
    global SITE, WALL
    SITE = find_site()
    WALL = (SITE[0] - 20.0, SITE[0] + 20.0, SITE[1] + 5.0, SITE[1] + 9.0)
    spawn = [{"monster": 201, "x": SITE[0], "z": SITE[1] + MONSTER_OFFSET, "radius": 0.5, "count": 1}]
    srv = OverlayServer([template()], spawn)
    srv.start()
    c = None
    try:
        c = make_player(1, "o")
        enter_world(c)
        px, pz = c.pos[0], c.pos[2]
        print(f"      site={SITE} wall={WALL} pos={c.pos}", flush=True)
        mons = {}
        index = 0
        crossed = False
        reached_at = None
        start = time.time()
        while time.time() - start < 60:
            time.sleep(0.2)
            with c.cv:
                new = c.log[index:]
                index = len(c.log)
            for t, m in new:
                if not m.response.HasField("world_snapshot"):
                    continue
                s = m.response.world_snapshot
                for e in s.entered:
                    if e.kind == KIND_MONSTER:
                        mons[e.entity_id] = (e.position.x, e.position.z)
                for mv in s.moved:
                    if mv.entity_id in mons:
                        mons[mv.entity_id] = (mv.position.x, mv.position.z)
            for x, z in mons.values():
                crossed = crossed or in_wall(x, z)
                if math.hypot(x - px, z - pz) <= 4.0 and reached_at is None:
                    reached_at = time.time() - start
            if reached_at is not None and time.time() - start > reached_at + 1.0:
                break
        print(f"      player=({px:.1f},{pz:.1f}) monsters={mons} reached_at={reached_at}", flush=True)
        check(len(mons) == 1, f"expected one monster in view, saw {len(mons)}")
        check(not crossed, "monster stood inside the wall")
        check(reached_at is not None, "monster behind the wall never reached the player within 60s")
    except Exception:
        print("      server log tail:", *srv.log_lines[-8:], sep="\n        ", flush=True)
        raise
    finally:
        if c:
            c.close()
        srv.stop()


TESTS = [t_o4_monster_walks_around_wall]


def main():
    only = sys.argv[1:]
    results = []
    for t in TESTS:
        if only and t.__name__ not in only:
            continue
        t0 = time.time()
        try:
            t()
            results.append((t.__name__, True, ""))
        except Exception as e:
            msg = str(e) or e.__class__.__name__
            if not isinstance(e, AssertionError):
                msg += " | " + traceback.format_exc().strip().splitlines()[-3].strip()
            results.append((t.__name__, False, msg[:300]))
        print(f"{'PASS' if results[-1][1] else 'FAIL'}  {t.__name__}  ({time.time() - t0:.1f}s) {results[-1][2]}", flush=True)
    p = sum(1 for r in results if r[1])
    print(f"\n{p}/{len(results)} passed")
    sys.exit(0 if p == len(results) else 1)


if __name__ == "__main__":
    main()
