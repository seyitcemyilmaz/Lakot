import sys, os, time, math, json, shutil, subprocess, threading, socket, traceback
from lakotnet import *
from t_monsters_g21 import tpl, open_site, View, walk_to, engage, ORIGIN

generate_bindings()

from services import progression_pb2 as prog

RUN = HERE / ".run_g12"
GM_ACCOUNT = "gm_g12"
GM_CHAR = "GmG12Hero"
SWORD = 1001
XP_MON = 401
STR, DEX, INT, VIT = 1, 2, 3, 4
A_OK, A_INVALID, A_NO_POINTS, A_AT_CAP = 0, 1, 2, 3
P_OK, P_INVALID, P_LOW, P_ALREADY = 0, 1, 2, 3
C = {}
PORTAL = None
LAND = ORIGIN


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


class G12Server(Server):
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
        self.log_lines = []
        json.dump({"monsters": [tpl(XP_MON, maxHealth=1, attack=1, experience=100, respawnSeconds=2)]},
                  open(RUN / "data" / "monsters.json", "w", encoding="utf-8"), indent=1)
        tMaps = RUN / "data" / "maps"
        shutil.copytree(tMaps / "101_altunay_village", tMaps / "102_g12_copy")
        tSite = C["site"]
        for tDir, tId, tTarget in ((tMaps / "101_altunay_village", 101, 102), (tMaps / "102_g12_copy", 102, 101)):
            tPath = tDir / "map.json"
            tMeta = json.load(open(tPath, encoding="utf-8"))
            tMeta["id"] = tId
            tMeta["name"] = f"G12 map {tId}"
            tMeta["spawn"] = {"x": ORIGIN[0], "z": ORIGIN[1], "yaw": 0.0}
            tMeta["safeZones"] = []
            tMeta["monsterSpawns"] = ([{"monster": XP_MON, "x": tSite[0], "z": tSite[1], "radius": 3.0, "count": 6}]
                                      if tId == 101 else [])
            tMeta["portals"] = [{"x": C["portal"][0], "z": C["portal"][1], "radius": 3.0, "targetMap": tTarget,
                                 "targetX": ORIGIN[0], "targetZ": ORIGIN[1], "targetYaw": 0.0}] if tId == 101 else []
            json.dump(tMeta, open(tPath, "w", encoding="utf-8"))
        tEnv = dict(os.environ)
        tEnv["LAKOT_GM_ACCOUNT"] = GM_ACCOUNT
        tEnv["LAKOT_SERVER_PORT"] = str(PORT)
        self.proc = subprocess.Popen([os.path.join(self.run_dir, "LakotServer.exe")], cwd=self.run_dir, env=tEnv,
                                     stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                     text=True, errors="replace")
        threading.Thread(target=self._pump, daemon=True).start()
        tEnd = time.time() + timeout
        while time.time() < tEnd:
            if self.proc.poll() is not None:
                raise RuntimeError("server exited early:\n" + "\n".join(self.log_lines[-20:]))
            try:
                socket.create_connection(("127.0.0.1", PORT), timeout=0.5).close()
                return
            except OSError:
                time.sleep(0.3)
        raise RuntimeError("server did not open port\n" + "\n".join(self.log_lines[-20:]))


def poll(pFn, pTimeout=5.0):
    tEnd = time.time() + pTimeout
    while time.time() < tEnd:
        v = pFn()
        if v:
            return v
        time.sleep(0.1)
    return pFn()


def stats_of(pClient):
    with pClient.cv:
        tLog = list(pClient.log)
    for _, m in reversed(tLog):
        if m.response.HasField("inventory_update"):
            return m.response.inventory_update.stats
    return None


def items_of(pClient):
    with pClient.cv:
        tLog = list(pClient.log)
    for _, m in reversed(tLog):
        if m.response.HasField("inventory_update"):
            return list(m.response.inventory_update.items)
    return []


def stats_where(pClient, pPred, pTimeout=5.0):
    return poll(lambda: (lambda s: s if s is not None and pPred(s) else None)(stats_of(pClient)), pTimeout)


def gm(pText):
    time.sleep(0.4)
    r = C["G"].call(lambda q: setattr(q.command_request, "text", pText), 6.0)
    check(r is not None and r.HasField("command_response") and r.command_response.status == 0,
          f"GM command '{pText}' failed: {r}")


def allocate(pClient, pStat, pAmount):
    def f(q):
        q.allocate_stat_request.stat = pStat
        q.allocate_stat_request.amount = pAmount
    r = pClient.call(f, 6.0)
    check(r is not None and r.HasField("allocate_stat_response"), f"no AllocateStatResponse for stat {pStat} x{pAmount}")
    return r.allocate_stat_response.status


def choose_path(pClient, pPath):
    r = pClient.call(lambda q: setattr(q.choose_path_request, "path_id", pPath), 6.0)
    check(r is not None and r.HasField("choose_path_response"), f"no ChoosePathResponse for path {pPath}")
    return r.choose_path_response.status


def equip(pClient, pSlot):
    r = pClient.call(lambda q: setattr(q.equip_item_request, "bag_slot", pSlot), 6.0)
    check(r is not None and r.HasField("inventory_action_response"), "no InventoryActionResponse")
    return r.inventory_action_response.status


def bag_slot(pClient, pTemplate):
    for it in items_of(pClient):
        if it.template_id == pTemplate and it.location == 0:
            return it.slot
    return None


def world_player(pClassId=1):
    c = make_player(1, "p", pClassId)
    enter_world(c)
    C.setdefault("all", []).append(c)
    check(poll(lambda: stats_of(c) is not None, 5), "no InventoryUpdate after entering world")
    return c


def setup():
    tSite = open_site(ORIGIN, 14)
    tPortal = open_site(ORIGIN, 24, [math.radians(200)], 4.0)
    C["site"], C["portal"] = tSite, tPortal
    srv = G12Server()
    srv.start()
    C["srv"] = srv
    c = Client()
    r = c.login(GM_ACCOUNT)
    if not (r is not None and r.login_response.token):
        r = c.register(GM_ACCOUNT)
        check(r is not None and r.register_response.register_status in (0, 2), f"gm register failed {r}")
        c.close()
        srv.stop()
        srv.start()
        c = Client()
        r = c.login(GM_ACCOUNT)
        check(r is not None and r.login_response.token, f"gm login failed {r}")
    r = c.char_list()
    tChars = list(r.character_list_response.characters)
    if tChars:
        c.cid, c.name = tChars[0].character_id, tChars[0].name
    else:
        r = c.create_char(GM_CHAR, 1, 1)
        check(r is not None and r.character_create_response.status == 0, f"gm char create failed {r}")
        c.cid, c.name = r.character_create_response.character.character_id, GM_CHAR
    enter_world(c)
    C["G"] = c
    C.setdefault("all", []).append(c)
    time.sleep(1.5)
    gm("/help")


def t_create_class_validation():
    c = Client()
    C.setdefault("all", []).append(c)
    user = rand_name("u")
    check(c.register(user).register_response.register_status == 0, "register failed")
    check(c.login(user).login_response.token, "login failed")
    for tBad in (0, 5, 255, 4294967295):
        r = c.create_char(rand_name("c"), 1, tBad)
        check(r is not None and r.character_create_response.status == 6, f"class {tBad}: got {r.character_create_response.status if r else None}, want INVALID_CLASS(6)")
    check(len(c.char_list().character_list_response.characters) == 0, "invalid-class attempts created characters")
    name = rand_name("c")
    r = c.create_char(name, 1, 2)
    check(r is not None and r.character_create_response.status == 0, f"class 2 create failed {r}")
    tChars = list(c.char_list().character_list_response.characters)
    check(len(tChars) == 1, f"expected 1 character, got {len(tChars)}")
    check(tChars[0].class_id == 2 and tChars[0].level == 1, f"listed class {tChars[0].class_id} level {tChars[0].level}, want 2 / 1")
    check(r.character_create_response.character.class_id == 2, "create response lacks class_id 2")


def t_start_stats_kurtcu():
    c = world_player(2)
    s = stats_of(c)
    got = (s.strength, s.dexterity, s.intelligence, s.vitality)
    check(got == (8, 16, 6, 10), f"Kurtcu stats {got}, want (8,16,6,10)")
    check(s.class_id == 2 and s.path_id == 0 and s.stat_points == 0, f"class/path/points {s.class_id}/{s.path_id}/{s.stat_points}")
    a = world_player(1)
    s = stats_of(a)
    got = (s.strength, s.dexterity, s.intelligence, s.vitality)
    check(got == (14, 8, 6, 12), f"Alp stats {got}, want (14,8,6,12)")
    check(s.class_id == 1, f"Alp class_id {s.class_id}")


def t_equip_class_restriction():
    a = world_player(1)
    check(poll(lambda: bag_slot(a, SWORD) is not None, 5), "Alp did not receive the starter sword")
    check(equip(a, bag_slot(a, SWORD)) == 0, "Alp could not equip sword 1001")
    check(poll(lambda: any(i.template_id == SWORD and i.location == 1 for i in items_of(a)), 4), "sword not shown equipped")
    k = world_player(2)
    time.sleep(1.0)
    check(bag_slot(k, SWORD) is None, "Kurtcu started with a sword")
    gm(f"/item {SWORD} 1 {k.name}")
    check(poll(lambda: bag_slot(k, SWORD) is not None, 5), "/item did not deliver the sword to the Kurtcu")
    check(equip(k, bag_slot(k, SWORD)) == 1, "Kurtcu equipped a class-restricted sword")
    check(not any(i.template_id == SWORD and i.location == 1 for i in items_of(k)), "sword is equipped on Kurtcu")
    b = world_player(3)
    gm(f"/item {SWORD} 1 {b.name}")
    check(poll(lambda: bag_slot(b, SWORD) is not None, 5), "Bahadir has no sword")
    check(equip(b, bag_slot(b, SWORD)) == 0, "Bahadir (class 3) could not equip the sword")


def t_choose_path():
    c = world_player(1)
    check(choose_path(c, 1) == P_LOW, "choose_path at level 1 should be LEVEL_TOO_LOW")
    gm(f"/level 10 {c.name}")
    s = stats_where(c, lambda s: s.level == 10)
    check(s is not None, "never saw level 10")
    check(s.stat_points == 36, f"stat_points at level 10 = {s.stat_points}, want 36")
    check(choose_path(c, 3) == P_INVALID, "path 3 should be INVALID")
    check(choose_path(c, 0) == P_INVALID, "path 0 should be INVALID")
    check(stats_of(c).path_id == 0, "invalid choices changed path_id")
    check(choose_path(c, 1) == P_OK, "path 1 should be OK")
    s = stats_where(c, lambda s: s.path_id == 1)
    check(s is not None, "no stats update with path_id 1 after choosing")
    check(choose_path(c, 2) == P_ALREADY, "second choice should be ALREADY_CHOSEN")
    check(choose_path(c, 1) == P_ALREADY, "repeat of same path should be ALREADY_CHOSEN")
    check(stats_of(c).path_id == 1, "path_id changed by rejected choice")


def t_allocate_stat():
    c = world_player(1)
    check(allocate(c, STR, 1) == A_NO_POINTS, "level 1 allocate should be NO_POINTS")
    check(allocate(c, 0, 1) == A_INVALID, "stat 0 should be INVALID")
    check(allocate(c, 9, 1) == A_INVALID, "stat 9 should be INVALID")
    check(allocate(c, STR, 0) == A_INVALID, "amount 0 should be INVALID")
    check(allocate(c, STR, 4294967295) in (A_NO_POINTS, A_AT_CAP), "huge amount must be rejected")
    gm(f"/level 10 {c.name}")
    check(stats_where(c, lambda s: s.level == 10) is not None, "never saw level 10")
    tBase = stats_of(c)
    tStr, tAtk = tBase.strength, tBase.attack
    check(allocate(c, STR, 4) == A_OK, "allocate STR 4 should be OK")
    s = stats_where(c, lambda s: s.strength == tStr + 4)
    check(s is not None, "InventoryUpdate never showed STR+4")
    check(s.stat_points == 32, f"stat_points after spending 4 = {s.stat_points}, want 32")
    check(s.attack >= tAtk, f"attack fell after raising STR ({tAtk} -> {s.attack})")
    check(allocate(c, STR, 33) == A_NO_POINTS, "33 > 32 should be NO_POINTS")
    check(stats_of(c).stat_points == 32, "NO_POINTS changed stat_points")
    check(allocate(c, VIT, 2) == A_OK, "VIT 2 OK")
    s = stats_where(c, lambda s: s.stat_points == 30)
    check(s is not None, "stat_points not 30 after VIT 2")
    gm(f"/level 100 {c.name}")
    s = stats_where(c, lambda s: s.level == 100 and s.stat_points > 300)
    check(s is not None, "level 100 did not grant points")
    tPoints = s.stat_points
    tRoom = 200 - s.dexterity
    check(allocate(c, DEX, tRoom + 1) == A_AT_CAP, "amount beyond cap should be AT_CAP")
    check(stats_of(c).dexterity == s.dexterity, "AT_CAP changed the stat")
    check(allocate(c, DEX, tRoom) == A_OK, f"allocating exactly up to cap ({tRoom}) should be OK")
    s = stats_where(c, lambda s: s.dexterity == 200)
    check(s is not None, "dexterity never reached 200")
    check(s.stat_points == tPoints - tRoom, f"points {s.stat_points}, want {tPoints - tRoom}")
    check(allocate(c, DEX, 1) == A_AT_CAP, "+1 at cap should be AT_CAP")
    check(stats_of(c).stat_points == tPoints - tRoom, "AT_CAP consumed points")


def t_persistence_relog_and_portal():
    c = world_player(2)
    gm(f"/level 10 {c.name}")
    check(stats_where(c, lambda s: s.level == 10) is not None, "never saw level 10")
    check(choose_path(c, 2) == P_OK, "choose path 2")
    check(allocate(c, DEX, 5) == A_OK, "allocate DEX 5")
    check(allocate(c, INT, 3) == A_OK, "allocate INT 3")
    want = stats_where(c, lambda s: s.dexterity == 21 and s.intelligence == 9 and s.path_id == 2)
    check(want is not None, "local state never reached expected values")
    tWant = (want.class_id, want.path_id, want.stat_points, want.strength, want.dexterity, want.intelligence, want.vitality)
    check(tWant[2] == 28, f"points {tWant[2]}, want 28")

    def relog(pLabel):
        user, cid = c.user, c.cid
        c.close()
        time.sleep(3)
        d = Client()
        C["all"].append(d)
        r = d.login(user)
        check(r is not None and r.login_response.token, f"{pLabel}: login failed")
        tMark = d.mark()
        r = d.enter(cid)
        check(r is not None and r.enter_world_response.status == 0, f"{pLabel}: enter failed {r}")
        u = d.wait(lambda x: x.HasField("inventory_update"), 6, tMark)
        check(u is not None, f"{pLabel}: no InventoryUpdate on enter")
        s = u.inventory_update.stats
        got = (s.class_id, s.path_id, s.stat_points, s.strength, s.dexterity, s.intelligence, s.vitality)
        check(got == tWant, f"{pLabel}: stats {got}, want {tWant}")
        d.user, d.cid, d.name = user, cid, c.name
        d.pos = (r.enter_world_response.pos_x, r.enter_world_response.pos_y, r.enter_world_response.pos_z)
        return d

    d = relog("relog")
    v = View(d)
    tMark = d.mark()
    walk_to(d, v, C["portal"][0], C["portal"][1], 1.0)
    r = poll(lambda: (lambda x: x if x and x.HasField("command_response") else None)(
        d.call(lambda q: setattr(q.command_request, "text", "/pos"), 4.0)), 4)
    tText = " ".join(r.command_response.lines) if r else ""
    check("Map 102" in tText, f"portal did not move the player to map 102: [{tText}]")
    with d.cv:
        tLog = list(d.log[tMark:])
    for _, m in tLog:
        if m.response.HasField("inventory_update"):
            s = m.response.inventory_update.stats
            got = (s.class_id, s.path_id, s.stat_points, s.strength, s.dexterity, s.intelligence, s.vitality)
            check(got == tWant, f"stats message after portal {got}, want {tWant}")
    c = d
    relog("relog after portal")


def t_xp_levelup_grants_points():
    c = world_player(1)
    v = View(c)
    time.sleep(2.5)
    v.ingest()
    tMons = v.of(XP_MON)
    check(tMons, "no XP monsters in view")
    tId = min(tMons, key=lambda i: math.hypot(tMons[i]["x"] - c.pos[0], tMons[i]["z"] - c.pos[2]))
    check(engage(c, v, tId, lambda: tId in v.dead, 70), "XP monster not killed in 70s")
    s = stats_where(c, lambda s: s.level >= 2, 5)
    check(s is not None, "no level-up after killing a 100-XP monster")
    check(s.level == 2, f"level {s.level}, want 2")
    check(s.stat_points == 4, f"stat_points after one level-up = {s.stat_points}, want 4")


TESTS = [t_create_class_validation, t_start_stats_kurtcu, t_equip_class_restriction, t_choose_path, t_allocate_stat,
         t_persistence_relog_and_portal, t_xp_levelup_grants_points]


def main():
    tOnly = sys.argv[1:]
    tResults = []
    try:
        try:
            setup()
        except Exception as e:
            print("SETUP FAILED:", e, flush=True)
            traceback.print_exc()
            if C.get("srv"):
                print("\n".join(C["srv"].log_lines[-20:]))
                C["srv"].stop()
            sys.exit(2)
        srv = C["srv"]
        for t in TESTS:
            if tOnly and t.__name__ not in tOnly:
                continue
            t0 = time.time()
            try:
                t()
                tResults.append((t.__name__, True, ""))
            except Exception as e:
                tMsg = str(e) or e.__class__.__name__
                if not isinstance(e, AssertionError):
                    tMsg += " | " + traceback.format_exc().strip().splitlines()[-3].strip()
                tResults.append((t.__name__, False, tMsg[:400]))
                print("      server log tail:", *srv.log_lines[-8:], sep="\n        ", flush=True)
            print(f"{'PASS' if tResults[-1][1] else 'FAIL'}  {t.__name__}  ({time.time() - t0:.1f}s) {tResults[-1][2]}", flush=True)
        tAlive = srv.proc.poll() is None
        tResults.append(("server_survived", tAlive, ""))
        print(f"{'PASS' if tAlive else 'FAIL'}  server_survived", flush=True)
    finally:
        for c in C.get("all", []):
            try:
                c.close()
            except Exception:
                pass
        if C.get("srv"):
            C["srv"].stop()
    tPassed = sum(1 for r in tResults if r[1])
    print(f"\n{tPassed}/{len(tResults)} passed")
    sys.exit(0 if tPassed == len(tResults) else 1)


if __name__ == "__main__":
    main()
