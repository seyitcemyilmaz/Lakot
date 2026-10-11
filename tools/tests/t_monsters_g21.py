import sys, os, time, math, json, shutil, subprocess, threading, socket, traceback, pathlib
from lakotnet import *
from mapnav import nav

generate_bindings()

RUN = HERE / ".run_g21"
SRC_MAP = ROOT / "data" / "maps" / "101_altunay_village" / "map.json"
ORIGIN = (-290.0, -200.0)
ITEM_POTION, ITEM_SABRE, ITEM_BLADE = 4, 1005, 1003
KIND_MONSTER, KIND_GROUND = 1, 3


def check(cond, msg):
    if not cond:
        raise AssertionError(msg)


def tpl(pId, **kw):
    t = {"id": pId, "name": f"T{pId}", "model": "models/monsters/steppe_bandit.glb", "level": 1, "maxHealth": 100,
         "attack": 1, "defense": 0, "experience": 1, "moveSpeed": 4.0, "aggroRange": 0, "attackRange": 1.8,
         "attackInterval": 1.0, "leashRange": 150, "respawnSeconds": 600, "aggressive": False,
         "gold": {"min": 0, "max": 0, "chance": 0.0}, "drops": []}
    t.update(kw)
    return t


def open_site(pOrigin, pDist, pAngles=None, pRadius=7.0):
    n = nav()
    for k in range(36):
        a = math.radians(10 * k) if pAngles is None else pAngles[k % len(pAngles)] + math.radians(10 * (k // max(1, len(pAngles or [0]))))
        x, z = pOrigin[0] + math.cos(a) * pDist, pOrigin[1] + math.sin(a) * pDist
        disc = [(x + math.cos(t) * pRadius * f, z + math.sin(t) * pRadius * f) for t in [i * math.pi / 4 for i in range(8)] for f in (0.5, 1.0)]
        if n.walkable(x, z) and all(n.walkable(*p) for p in disc) and n.segment_ok(pOrigin[0], pOrigin[1], x, z, margin=1.0):
            return x, z
    raise AssertionError(f"no open site at distance {pDist} from {pOrigin}")


def far_from(pPoints, pMin):
    n = nav()
    for dist in (30.0, 34.0, 38.0):
        for k in range(36):
            a = math.radians(10 * k)
            x, z = ORIGIN[0] + math.cos(a) * dist, ORIGIN[1] + math.sin(a) * dist
            if all(math.hypot(x - p[0], z - p[1]) >= pMin for p in pPoints):
                try:
                    return open_site(ORIGIN, dist, [a])
                except AssertionError:
                    pass
    raise AssertionError("no site far enough")


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
        shutil.copytree(self.bin_dir, RUN)
        self.run_dir = str(RUN)
        json.dump({"monsters": self.templates}, open(RUN / "data" / "monsters.json", "w", encoding="utf-8"), indent=1)
        mp = RUN / "data" / "maps" / "101_altunay_village" / "map.json"
        meta = json.load(open(mp, encoding="utf-8"))
        meta["spawn"] = {"x": ORIGIN[0], "z": ORIGIN[1], "yaw": 0.0}
        meta["safeZones"] = []
        meta["monsterSpawns"] = self.spawns
        json.dump(meta, open(mp, "w", encoding="utf-8"))
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


class View:
    def __init__(self, pClient):
        self.c = pClient
        self.i = 0
        self.mons = {}
        self.grounds = {}
        self.spawns = []
        self.events = []
        self.dead = set()
        self.hp = None

    def ingest(self):
        with self.c.cv:
            new = self.c.log[self.i:]
            self.i = len(self.c.log)
        for t, m in new:
            if not m.response.HasField("world_snapshot"):
                continue
            s = m.response.world_snapshot
            for e in s.entered:
                self.spawns.append(e)
                if e.kind == KIND_MONSTER:
                    self.mons[e.entity_id] = {"x": e.position.x, "z": e.position.z, "tpl": e.template_id, "level": e.level, "alive": not e.is_dead}
                elif e.kind == KIND_GROUND:
                    self.grounds[e.entity_id] = {"tpl": e.template_id, "count": e.count, "gold": e.gold, "t": t}
            for mv in s.moved:
                if mv.entity_id in self.mons:
                    self.mons[mv.entity_id]["x"], self.mons[mv.entity_id]["z"] = mv.position.x, mv.position.z
            for i in s.left:
                self.grounds.pop(i, None)
            if s.HasField("self_correction"):
                self.c.pos = (s.self_correction.position.x, self.c.pos[1], s.self_correction.position.z)
            for v in s.vitals:
                if v.entity_id in self.mons:
                    self.mons[v.entity_id]["alive"] = not v.is_dead
            if s.HasField("self_vitals"):
                self.hp = s.self_vitals.health
            for ev in s.combat:
                pos = {i: (d["x"], d["z"]) for i, d in self.mons.items()}
                self.events.append((t, ev, pos))
                for h in ev.hits:
                    if h.killed:
                        self.dead.add(h.target_id)

    def of(self, pTemplate):
        return {i: d for i, d in self.mons.items() if d["tpl"] == pTemplate}


def start_player(pView_holder=None):
    c = make_player(1, "g")
    enter_world(c)
    c.pos = (c.pos[0], c.pos[1], c.pos[2])
    return c, View(c)


def step_toward(c, tx, tz, pStep=3.0):
    x, y, z = c.pos
    d = math.hypot(tx - x, tz - z)
    if d < 0.01:
        return
    k = min(pStep, d) / d
    nx, nz = x + (tx - x) * k, z + (tz - z) * k
    ny = nav().y_at(nx, nz)
    c.move(nx, ny, nz, math.atan2(tx - x, tz - z))
    c.pos = (nx, ny, nz)


def walk_to(c, v, tx, tz, pStop=0.5):
    while math.hypot(tx - c.pos[0], tz - c.pos[2]) > pStop:
        step_toward(c, tx, tz)
        time.sleep(0.7)
        v.ingest()


def engage(c, v, pId, pUntil, pTimeout=60):
    t0 = time.time()
    while time.time() - t0 < pTimeout:
        v.ingest()
        m = v.mons.get(pId)
        if m is None:
            return None
        x, y, z = c.pos
        d = math.hypot(m["x"] - x, m["z"] - z)
        yaw = math.atan2(m["x"] - x, m["z"] - z)
        if d > 2.2:
            step_toward(c, m["x"] - (m["x"] - x) / d * 1.5, m["z"] - (m["z"] - z) / d * 1.5, min(3.0, d - 1.5))
        else:
            c.attack(yaw)
        r = pUntil()
        if r:
            return r
        time.sleep(0.7)
    return None


def hit_on(v, pId):
    for t, ev, pos in v.events:
        if ev.attacker_id == v.c.cid:
            for h in ev.hits:
                if h.target_id == pId and h.damage > 0:
                    return t
    return None


def run_scenario(pTemplates, pSpawns, pBody):
    srv = OverlayServer(pTemplates, pSpawns)
    srv.start()
    c = None
    try:
        c, v = start_player()
        pBody(c, v, srv)
    except Exception:
        print("      server log tail:", *srv.log_lines[-8:], sep="\n        ", flush=True)
        raise
    finally:
        if c:
            c.close()
        srv.stop()


def t_g21_level_roll():
    a = open_site(ORIGIN, 30)
    b = open_site(ORIGIN, 30, [math.radians(120)])
    d = open_site(ORIGIN, 30, [math.radians(240)])
    templates = [tpl(101, level=5, minLevel=2, maxLevel=9), tpl(102, level=7), tpl(103, level=3, minLevel=3, maxLevel=3)]
    spawns = [{"monster": 101, "x": a[0], "z": a[1], "radius": 5.0, "count": 12},
              {"monster": 102, "x": b[0], "z": b[1], "radius": 4.0, "count": 3},
              {"monster": 103, "x": d[0], "z": d[1], "radius": 4.0, "count": 5, "minLevel": 12, "maxLevel": 13}]

    def body(c, v, srv):
        time.sleep(4)
        v.ingest()
        r, f, o = v.of(101), v.of(102), v.of(103)
        print(f"      template range levels={sorted(m['level'] for m in r.values())} fixed={sorted(m['level'] for m in f.values())} "
              f"spawn override={sorted(m['level'] for m in o.values())}", flush=True)
        check(len(r) == 12 and len(f) == 3 and len(o) == 5, f"unexpected monster counts {len(r)}/{len(f)}/{len(o)} in view")
        check(all(2 <= m["level"] <= 9 for m in r.values()), "template minLevel/maxLevel range violated")
        check(len({m["level"] for m in r.values()}) >= 3, "12 monsters in a 2..9 range all rolled <3 distinct levels (no roll?)")
        check(all(m["level"] == 7 for m in f.values()), "template without range must keep its fixed level 7")
        check(all(12 <= m["level"] <= 13 for m in o.values()), "spawn minLevel/maxLevel must override the template range")

    run_scenario(templates, spawns, body)


def t_g21_ranged_attack():
    ar = open_site(ORIGIN, 24)
    me = open_site(ORIGIN, 24, [math.radians(180)])
    templates = [tpl(110, maxHealth=100000, attack=3, aggressive=True, aggroRange=32, attackRange=12.0, projectile="arrow"),
                 tpl(111, maxHealth=100000, attack=3, aggressive=True, aggroRange=32, attackRange=1.8)]
    spawns = [{"monster": 110, "x": ar[0], "z": ar[1], "radius": 1.0, "count": 1},
              {"monster": 111, "x": me[0], "z": me[1], "radius": 1.0, "count": 1}]

    def body(c, v, srv):
        t0 = time.time()
        while time.time() - t0 < 40:
            time.sleep(0.5)
            v.ingest()
            ids = {i: d["tpl"] for i, d in v.mons.items()}
            ra = [e for e in v.events if ids.get(e[1].attacker_id) == 110]
            me_ev = [e for e in v.events if ids.get(e[1].attacker_id) == 111]
            if len(ra) >= 3 and len(me_ev) >= 2:
                break
        check(ra, "archer never produced a CombatEvent in 40s")
        check(me_ev, "melee control never produced a CombatEvent in 40s")
        far = 0
        for t, ev, pos in ra:
            check(ev.projectile == "arrow", f"archer event projectile={ev.projectile!r}, expected 'arrow'")
            check(ev.target_id == c.cid, f"archer event target_id={ev.target_id}, expected the player {c.cid}")
            ax, az = pos[ev.attacker_id]
            d = math.hypot(ax - c.pos[0], az - c.pos[2])
            far += d > 5.0
            check(d <= 14.0, f"archer fired from {d:.1f} units, beyond attackRange 12")
        print(f"      archer events={len(ra)} fired from >5 units={far}; melee events={len(me_ev)}", flush=True)
        check(far == len(ra), f"archer shots from <=5 units: {len(ra) - far}/{len(ra)} (not a ranged attack)")
        check(any(h.damage > 0 and h.target_id == c.cid for t, ev, pos in ra for h in ev.hits), "archer shots never damaged the player")
        for t, ev, pos in me_ev:
            check(ev.projectile == "", f"melee event carries projectile {ev.projectile!r}")
            check(ev.target_id == c.cid, "melee event target_id must be the player")
            ax, az = pos[ev.attacker_id]
            check(math.hypot(ax - c.pos[0], az - c.pos[2]) <= 4.0, "melee attacker hit from beyond melee reach")

    run_scenario(templates, spawns, body)


def t_g21_pack_assist():
    ctl = open_site(ORIGIN, 28, [math.radians(60)])
    pack = open_site(ORIGIN, 28, [math.radians(240)])
    check(math.hypot(ctl[0] - pack[0], ctl[1] - pack[1]) > 40, "test sites too close for a valid control")
    base = dict(maxHealth=1000000, attack=1, aggroRange=0, aggressive=False, moveSpeed=4.0)
    templates = [tpl(121, assistRange=0.0, **base), tpl(120, assistRange=25.0, **base)]
    spawns = [{"monster": 121, "x": ctl[0], "z": ctl[1], "radius": 4.0, "count": 4},
              {"monster": 120, "x": pack[0], "z": pack[1], "radius": 4.0, "count": 4}]

    def attackers_after(v, pTemplate, pSince, pTarget):
        ids = {i for i, d in v.mons.items() if d["tpl"] == pTemplate}
        return {ev.attacker_id for t, ev, pos in v.events if t >= pSince and ev.attacker_id in ids and ev.attacker_id != pTarget}

    def body(c, v, srv):
        time.sleep(3)
        v.ingest()
        check(len(v.of(120)) == 4 and len(v.of(121)) == 4, f"expected 4+4 monsters in view, got {len(v.of(120))}+{len(v.of(121))}")
        before = {i: (d["x"], d["z"]) for i, d in v.mons.items()}
        time.sleep(4)
        v.ingest()
        check(all(math.hypot(d["x"] - before[i][0], d["z"] - before[i][1]) < 1.0 for i, d in v.mons.items() if i in before),
              "non-aggressive monsters moved with nobody near them")
        check(not any(ev.attacker_id in v.mons for t, ev, pos in v.events), "monster attacked an uninvolved player")

        for name, tplid, expect_join in (("control(assistRange 0)", 121, False), ("pack(assistRange 25)", 120, True)):
            group = v.of(tplid)
            cx = sum(d["x"] for d in group.values()) / len(group)
            cz = sum(d["z"] for d in group.values()) / len(group)
            walk_to(c, v, cx, cz, 6.0)
            target = min(group, key=lambda i: math.hypot(group[i]["x"] - c.pos[0], group[i]["z"] - c.pos[2]))
            t_hit = engage(c, v, target, lambda: hit_on(v, target))
            check(t_hit is not None, f"{name}: could not land a hit on a monster")
            others = {i for i in group if i != target}
            end = time.time() + 9
            while time.time() < end:
                time.sleep(0.5)
                v.ingest()
                c.attack(math.atan2(v.mons[target]["x"] - c.pos[0], v.mons[target]["z"] - c.pos[2]))
            joined = attackers_after(v, tplid, t_hit, target) & others
            print(f"      {name}: neighbours that attacked the player = {len(joined)}/{len(others)}", flush=True)
            if expect_join:
                check(len(joined) >= 2, f"{name}: only {len(joined)} of {len(others)} neighbours within assistRange joined the fight")
            else:
                check(len(joined) == 0, f"{name}: {len(joined)} neighbours joined although assistRange is 0")

    run_scenario(templates, spawns, body)


def t_g21_boss_loot():
    bp = open_site(ORIGIN, 14)
    templates = [tpl(130, maxHealth=40, boss=True, lootRolls=3, gold={"min": 10, "max": 10, "chance": 1.0},
                     drops=[{"itemId": ITEM_POTION, "chance": 0.0, "min": 2, "max": 2, "guaranteed": True},
                            {"itemId": ITEM_SABRE, "chance": 0.0, "min": 1, "max": 1, "guaranteed": True},
                            {"itemId": ITEM_BLADE, "chance": 1.0, "min": 1, "max": 1}])]
    spawns = [{"monster": 130, "x": bp[0], "z": bp[1], "radius": 0.5, "count": 1}]

    def body(c, v, srv):
        time.sleep(3)
        v.ingest()
        boss = next(iter(v.of(130)), None)
        check(boss is not None, "boss not in view")
        done = engage(c, v, boss, lambda: boss in v.dead, 90)
        check(done, "boss was not killed within 90s")
        n0 = len(v.spawns)
        time.sleep(3)
        v.ingest()
        drops = [e for e in v.spawns[n0 - 1:] if e.kind == KIND_GROUND] if False else [e for e in v.spawns if e.kind == KIND_GROUND]
        tally = {}
        for e in drops:
            key = "gold" if e.template_id == 0 and e.gold > 0 else e.template_id
            tally[key] = tally.get(key, 0) + 1
        print(f"      ground entities by template: {tally}", flush=True)
        check(tally.get(ITEM_POTION) == 1, f"guaranteed potion drops: {tally.get(ITEM_POTION)} (want exactly 1, once regardless of rolls)")
        check(tally.get(ITEM_SABRE) == 1, f"guaranteed sabre drops: {tally.get(ITEM_SABRE)} (want exactly 1)")
        check(tally.get(ITEM_BLADE) == 3, f"chance-1.0 blade drops: {tally.get(ITEM_BLADE)} (want lootRolls=3)")
        check(tally.get("gold") == 1, f"gold drops: {tally.get('gold')} (want 1)")
        pot = next(e for e in drops if e.template_id == ITEM_POTION)
        check(pot.count == 2, f"guaranteed potion count {pot.count}, want 2")

    run_scenario(templates, spawns, body)


TESTS = [t_g21_level_roll, t_g21_ranged_attack, t_g21_pack_assist, t_g21_boss_loot]


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
