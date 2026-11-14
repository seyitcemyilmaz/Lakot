import sys, os, time, math, json, shutil, subprocess, threading, socket, traceback
from lakotnet import *
from t_monsters_g21 import tpl, open_site, View, walk_to, engage, hit_on, ORIGIN
from t_classes_g12 import poll, stats_of, items_of, equip, bag_slot

generate_bindings()

RUN = type(HERE)(os.environ.get("LAKOT_TEST_RUN_DIR") or HERE / ".run_g25")
GM_ACCOUNT = "gm_g25"
GM_CHAR = "GmG25Hero"
TOUGH = 501
C = {}
STATE = {}
WINDOW = 7.0


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


class G25Server(Server):
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
        self.log_lines = []
        json.dump({"monsters": [tpl(TOUGH, maxHealth=100000000, attack=1, aggressive=False)]},
                  open(RUN / "data" / "monsters.json", "w", encoding="utf-8"), indent=1)
        tMaps = RUN / "data" / "maps"
        shutil.rmtree(tMaps / "102_altunay_steppe", ignore_errors=True)
        shutil.copytree(tMaps / "101_altunay_village", tMaps / "102_g25_copy", dirs_exist_ok=True)
        tSite, tPortal = C["site"], C["portal"]
        for tDir, tId, tTarget in ((tMaps / "101_altunay_village", 101, 102), (tMaps / "102_g25_copy", 102, 101)):
            tPath = tDir / "map.json"
            tMeta = json.load(open(tPath, encoding="utf-8"))
            tMeta["id"] = tId
            tMeta["name"] = f"G25 map {tId}"
            tMeta["spawn"] = {"x": ORIGIN[0], "z": ORIGIN[1], "yaw": 0.0}
            tMeta["safeZones"] = []
            tMeta["monsterSpawns"] = ([{"monster": TOUGH, "x": tSite[0], "z": tSite[1], "radius": 2.0, "count": 4}]
                                      if tId == 101 else [])
            tMeta["portals"] = [{"x": tPortal[0], "z": tPortal[1], "radius": 3.0, "targetMap": tTarget,
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


def gm(pText):
    time.sleep(0.4)
    r = C["G"].call(lambda q: setattr(q.command_request, "text", pText), 6.0)
    check(r is not None and r.HasField("command_response") and r.command_response.status == 0,
          f"GM command '{pText}' failed: {r}")


def setup():
    C["site"] = open_site(ORIGIN, 14)
    C["portal"] = open_site(ORIGIN, 24, [math.radians(200)], 4.0)
    srv = G25Server()
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


def world_player(pClassId=1):
    c = make_player(1, "p", pClassId)
    enter_world(c)
    C.setdefault("all", []).append(c)
    check(poll(lambda: stats_of(c) is not None, 5), "no InventoryUpdate after entering world")
    return c


def vitals_now(pClient):
    with pClient.cv:
        tLog = list(pClient.log)
    tHp = tMaxHp = tMana = tMaxMana = None
    for _, m in reversed(tLog):
        r = m.response
        if tHp is None:
            if r.HasField("inventory_update"):
                tHp, tMaxHp = r.inventory_update.stats.health, r.inventory_update.stats.max_health
            elif r.HasField("world_snapshot") and r.world_snapshot.HasField("self_vitals"):
                tHp, tMaxHp = r.world_snapshot.self_vitals.health, r.world_snapshot.self_vitals.max_health
        if tMana is None and r.HasField("inventory_update"):
            tMana, tMaxMana = r.inventory_update.stats.mana, r.inventory_update.stats.max_mana
        if tHp is not None and tMana is not None:
            break
    return tHp, tMaxHp, tMana, tMaxMana


def hp_rate(pMax, pInCombat):
    return max(1, pMax * (5 if pInCombat else 20) // 1000)


def mp_rate(pMax, pInCombat):
    return max(1, pMax * (5 if pInCombat else 50) // 1000)


def prepare_wounded(pClient):
    gm(f"/level 30 {pClient.name}")
    check(poll(lambda: stats_of(pClient) is not None and stats_of(pClient).level == 30, 5), "level 30 not reached")
    time.sleep(1.0)
    gm(f"/heal {pClient.name}")
    time.sleep(1.0)
    tHp, tMaxHp, tMana, tMaxMana = vitals_now(pClient)
    check(tMaxHp and tMaxHp >= 200 and tMaxMana and tMaxMana >= 50, f"max vitals too small to measure regen: {tMaxHp}/{tMaxMana}")
    return tMaxHp, tMaxMana


def wound(pClient, pMaxHp, pMaxMana):
    tHp = pMaxHp // 3
    gm(f"/sethp {tHp} {pClient.name}")
    gm(f"/setmp 0 {pClient.name}")
    tHpLimit = tHp + hp_rate(pMaxHp, False) * 2
    tMpLimit = mp_rate(pMaxMana, False) * 2
    check(poll(lambda: (lambda v: v[0] is not None and v[0] <= tHpLimit and v[2] is not None and v[2] <= tMpLimit)(vitals_now(pClient)), 4),
          f"wound not applied: {vitals_now(pClient)}")


def t_out_of_combat_regen():
    c = world_player()
    tMaxHp, tMaxMana = prepare_wounded(c)
    wound(c, tMaxHp, tMaxMana)
    tHp0, _, tMp0, _ = vitals_now(c)
    t0 = time.time()
    time.sleep(WINDOW)
    tHp1, _, tMp1, _ = vitals_now(c)
    tDt = time.time() - t0
    tHpGain, tMpGain = tHp1 - tHp0, tMp1 - tMp0
    tHpRate, tMpRate = hp_rate(tMaxHp, False), mp_rate(tMaxMana, False)
    print(f"      out of combat over {tDt:.1f}s: hp {tHp0}->{tHp1} (rate {tHpRate}/s), mana {tMp0}->{tMp1} (rate {tMpRate}/s)", flush=True)
    check(tHpGain >= tHpRate * 4, f"hp gain {tHpGain} < 4 ticks of {tHpRate}")
    check(tHpGain <= tHpRate * (int(tDt) + 3), f"hp gain {tHpGain} too fast (> {tHpRate * (int(tDt) + 3)})")
    check(tMpGain >= tMpRate * 4, f"mana gain {tMpGain} < 4 ticks of {tMpRate}")
    check(tMpGain <= tMpRate * (int(tDt) + 3), f"mana gain {tMpGain} too fast (> {tMpRate * (int(tDt) + 3)})")
    STATE["ooc_hp_per_s"], STATE["ooc_mp_per_s"] = tHpGain / tDt, tMpGain / tDt


def t_in_combat_regen_slower():
    c = world_player()
    v = View(c)
    tMaxHp, tMaxMana = prepare_wounded(c)
    walk_to(c, v, *C["site"], 2.0)
    v.ingest()
    tMons = v.of(TOUGH)
    check(tMons, "no tough monster in view")
    tId = min(tMons, key=lambda i: math.hypot(tMons[i]["x"] - c.pos[0], tMons[i]["z"] - c.pos[2]))
    check(engage(c, v, tId, lambda: hit_on(v, tId), 40), "never landed a hit on the tough monster")
    wound(c, tMaxHp, tMaxMana)
    tHp0, _, tMp0, _ = vitals_now(c)
    t0 = time.time()
    while time.time() - t0 < WINDOW:
        engage(c, v, tId, lambda: True, 2)
        time.sleep(0.4)
    tHp1, _, tMp1, _ = vitals_now(c)
    tDt = time.time() - t0
    tHpRate, tMpRate = hp_rate(tMaxHp, True), mp_rate(tMaxMana, True)
    print(f"      in combat over {tDt:.1f}s: hp {tHp0}->{tHp1} (rate {tHpRate}/s), mana {tMp0}->{tMp1} (rate {tMpRate}/s)", flush=True)
    check(tHp1 - tHp0 <= tHpRate * (int(tDt) + 3), f"in-combat hp gain {tHp1 - tHp0} exceeds {tHpRate * (int(tDt) + 3)}")
    check(tMp1 - tMp0 >= 1, f"mana did not regenerate at all in combat ({tMp0}->{tMp1})")
    check(tMp1 - tMp0 <= tMpRate * (int(tDt) + 3), f"in-combat mana gain {tMp1 - tMp0} exceeds {tMpRate * (int(tDt) + 3)}")
    if "ooc_hp_per_s" in STATE:
        check((tHp1 - tHp0) / tDt < STATE["ooc_hp_per_s"] * 0.7, f"hp in combat {(tHp1 - tHp0) / tDt:.1f}/s not slower than out of combat {STATE['ooc_hp_per_s']:.1f}/s")
        check((tMp1 - tMp0) / tDt < STATE["ooc_mp_per_s"] * 0.7, f"mana in combat {(tMp1 - tMp0) / tDt:.1f}/s not slower than out of combat {STATE['ooc_mp_per_s']:.1f}/s")


def t_dead_no_regen():
    c = world_player()
    prepare_wounded(c)
    gm(f"/kill {c.name}")
    check(poll(lambda: vitals_now(c)[0] is not None and vitals_now(c)[0] <= 0, 4), f"not dead: {vitals_now(c)}")
    tMark = c.mark()
    time.sleep(4.0)
    check(vitals_now(c)[0] <= 0, f"dead character hp rose to {vitals_now(c)[0]}")
    with c.cv:
        tNew = [m.response for _, m in c.log[tMark:]]
    for r in tNew:
        if r.HasField("inventory_update"):
            check(r.inventory_update.stats.health <= 0, f"dead character got InventoryUpdate with hp {r.inventory_update.stats.health}")
        if r.HasField("world_snapshot") and r.world_snapshot.HasField("self_vitals"):
            check(r.world_snapshot.self_vitals.health <= 0, "dead character got self_vitals with hp > 0")
    gm(f"/revive {c.name}")


def t_full_health_no_change():
    c = world_player()
    time.sleep(2.5)
    tHp, tMaxHp, tMp, tMaxMana = vitals_now(c)
    check(tHp == tMaxHp and tMp == tMaxMana, f"fresh character not full: {tHp}/{tMaxHp} {tMp}/{tMaxMana}")
    tMark = c.mark()
    time.sleep(3.5)
    with c.cv:
        tNew = [m.response for _, m in c.log[tMark:]]
    tUpdates = [r for r in tNew if r.HasField("inventory_update")]
    check(not tUpdates, f"{len(tUpdates)} InventoryUpdate(s) at full hp/mana with no activity")
    for r in tNew:
        if r.HasField("world_snapshot") and r.world_snapshot.HasField("self_vitals"):
            check(r.world_snapshot.self_vitals.health == tMaxHp, f"hp changed at full health: {r.world_snapshot.self_vitals.health}")
    check(vitals_now(c) == (tHp, tMaxHp, tMp, tMaxMana), f"vitals changed at full: {vitals_now(c)}")


def t_server_survived():
    srv = C["srv"]
    check(srv.proc.poll() is None, "server process exited\n" + "\n".join(srv.log_lines[-15:]))
    check(not any("Assertion" in l or "terminate" in l.lower() or "exception" in l.lower() for l in srv.log_lines),
          "exception/assert line in server log")


TESTS = [t_out_of_combat_regen, t_in_combat_regen_slower, t_dead_no_regen, t_full_health_no_change, t_server_survived]


def run_tests(pTests):
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
        for t in pTests:
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
    run_tests(TESTS)
