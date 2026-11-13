import sys, os, re, time, math, json, shutil, subprocess, threading, socket, traceback
from lakotnet import *
from t_monsters_g21 import tpl, open_site, View, engage, ORIGIN
from mapnav import nav

generate_bindings()

RUN = type(HERE)(os.environ.get("LAKOT_TEST_RUN_DIR") or HERE / ".run_g28")
GM_ACCOUNT = "gm_g28"
GM_CHAR = "GmG28Hero"
XP_MON = 801
KILLER = 802
AUTO_SECONDS = 12
CHOOSE_SECONDS = 5.0
C = {}

HERE_CHOICE, CITY_CHOICE, UNSPECIFIED = 1, 2, 0
R_OK, R_NOT_DEAD, R_TOO_EARLY, R_INVALID = 0, 1, 2, 3


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


def death_loss(pLevel, pExperience):
    return 0 if pLevel <= 5 else pExperience * 5 // 100


class G28Server(Server):
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
        json.dump({"monsters": [tpl(XP_MON, maxHealth=1, attack=1, experience=1000, respawnSeconds=600),
                                tpl(KILLER, maxHealth=100000000, attack=100000, aggressive=True, aggroRange=25,
                                    attackInterval=1000.0, moveSpeed=6.0, attackRange=2.5, respawnSeconds=600)]},
                  open(RUN / "data" / "monsters.json", "w", encoding="utf-8"), indent=1)
        tMaps = RUN / "data" / "maps"
        shutil.copytree(tMaps / "101_altunay_village", tMaps / "102_g28_copy", dirs_exist_ok=True)
        tPortal = C["portal"]
        for tDir, tId, tTarget in ((tMaps / "101_altunay_village", 101, 102), (tMaps / "102_g28_copy", 102, 101)):
            tPath = tDir / "map.json"
            tMeta = json.load(open(tPath, encoding="utf-8"))
            tMeta["id"] = tId
            tMeta["name"] = f"G28 map {tId}"
            tMeta["spawn"] = {"x": ORIGIN[0], "z": ORIGIN[1], "yaw": 0.0}
            tMeta["safeZones"] = []
            tMeta["monsterSpawns"] = []
            tMeta["portals"] = [{"x": tPortal[0], "z": tPortal[1], "radius": 3.0, "targetMap": tTarget,
                                 "targetX": ORIGIN[0], "targetZ": ORIGIN[1], "targetYaw": 0.0}]
            json.dump(tMeta, open(tPath, "w", encoding="utf-8"))
        tEnv = dict(os.environ)
        tEnv["LAKOT_GM_ACCOUNT"] = GM_ACCOUNT
        tEnv["LAKOT_SERVER_PORT"] = str(PORT)
        tEnv["LAKOT_AUTO_RESPAWN_SECONDS"] = str(AUTO_SECONDS)
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


def poll(pFn, pTimeout=5.0, pStep=0.1):
    tEnd = time.time() + pTimeout
    while time.time() < tEnd:
        v = pFn()
        if v:
            return v
        time.sleep(pStep)
    return pFn()


def info(pClient):
    with pClient.cv:
        tLog = list(pClient.log)
    for tTime, m in reversed(tLog):
        r = m.response
        if r.HasField("inventory_update"):
            s = r.inventory_update.stats
            return {"hp": s.health, "max": s.max_health, "level": s.level, "exp": s.experience, "t": tTime}
        if r.HasField("world_snapshot") and r.world_snapshot.HasField("self_vitals"):
            s = r.world_snapshot.self_vitals
            return {"hp": s.health, "max": s.max_health, "level": s.level, "exp": s.experience, "t": tTime,
                    "respawn": s.respawn_seconds, "auto": s.auto_respawn_seconds}
    return None


def death_time(pClient, pMark, pTimeout=12.0):
    def scan():
        with pClient.cv:
            tLog = list(pClient.log[pMark:])
        for tTime, m in tLog:
            r = m.response
            if r.HasField("world_snapshot") and r.world_snapshot.HasField("self_vitals") and r.world_snapshot.self_vitals.health <= 0:
                return tTime
            if r.HasField("inventory_update") and r.inventory_update.stats.health <= 0:
                return tTime
        return None
    return poll(scan, pTimeout)


def alive_time(pClient, pMark, pTimeout=12.0):
    def scan():
        with pClient.cv:
            tLog = list(pClient.log[pMark:])
        for tTime, m in tLog:
            r = m.response
            if r.HasField("world_snapshot") and r.world_snapshot.HasField("self_vitals") and r.world_snapshot.self_vitals.health > 0:
                return tTime
        return None
    return poll(scan, pTimeout)


def gm(pText, pOk=True):
    time.sleep(0.4)
    r = C["G"].call(lambda q: setattr(q.command_request, "text", pText), 6.0)
    check(r is not None and r.HasField("command_response"), f"no CommandResponse for '{pText}'")
    if pOk:
        check(r.command_response.status == 0, f"GM command '{pText}' failed: {r.command_response}")
    return r.command_response


def get_pos():
    r = gm("/pos")
    m = re.search(r"Map (\d+), x (-?\d+\.\d), z (-?\d+\.\d)", " ".join(r.lines))
    check(m, f"unparsable /pos: {r}")
    return int(m.group(1)), float(m.group(2)), float(m.group(3))


def tp(pX, pZ):
    gm(f"/tp {int(pX)} {int(pZ)}")
    G = C["G"]
    G.pos = (pX, nav().y_at(pX, pZ), pZ)
    time.sleep(0.4)


def fresh():
    G = C["G"]
    tInfo = info(G)
    if tInfo and tInfo["hp"] <= 0:
        gm("/revive")
        check(poll(lambda: info(G)["hp"] > 0, 4), "GM /revive did not revive")
    tp(*C["home"])
    gm("/heal")
    time.sleep(0.5)


def new_view():
    C["v"] = View(C["G"])
    return C["v"]


def spawn(pTemplate):
    gm(f"/spawn {pTemplate} 1")


def earn_exp(pLevel):
    fresh()
    G = C["G"]
    gm(f"/level {pLevel}")
    check(poll(lambda: info(G)["level"] == pLevel, 4), f"level {pLevel} not reached: {info(G)}")
    v = C.get("v") or new_view()
    tKnown = set(v.of(XP_MON))
    spawn(XP_MON)

    def found():
        v.ingest()
        return [i for i, d in v.of(XP_MON).items() if i not in tKnown and d["alive"]]
    tIds = poll(found, 5)
    check(tIds, "spawned XP monster never entered view")
    tId = tIds[0]
    check(engage(G, v, tId, lambda: tId in v.dead, 40), "could not kill the XP monster")
    tInfo = poll(lambda: (lambda i: i if i and i["exp"] > 0 else None)(info(G)), 5)
    check(tInfo is not None and tInfo["exp"] >= 20, f"no usable experience after kill: {info(G)}")
    gm("/heal")
    time.sleep(0.5)
    return tInfo["exp"]


def die():
    G = C["G"]
    tMark = G.mark()
    spawn(KILLER)
    tTime = death_time(G, tMark, 12)
    check(tTime is not None, f"killer monster never killed the GM: {info(G)}\n" + "\n".join(C["srv"].log_lines[-6:]))
    return tTime, tMark


def wait_choosable(pDeathTime):
    tLeft = pDeathTime + CHOOSE_SECONDS + 0.4 - time.time()
    if tLeft > 0:
        time.sleep(tLeft)


def settle_after_death():
    time.sleep(0.6)
    return info(C["G"])


def relog():
    G = C["G"]
    tCid, tName = G.cid, G.name
    G.close()
    time.sleep(3)
    d = Client()
    r = d.login(GM_ACCOUNT)
    check(r is not None and r.login_response.token, "relog login failed")
    r = d.enter(tCid)
    check(r is not None and r.enter_world_response.status == 0, f"relog enter failed {r}")
    d.cid, d.name = tCid, tName
    d.pos = (r.enter_world_response.pos_x, r.enter_world_response.pos_y, r.enter_world_response.pos_z)
    C["G"] = d
    C["all"].append(d)
    C.pop("v", None)
    time.sleep(1.5)
    return d


def setup():
    C["home"] = open_site(ORIGIN, 14)
    C["portal"] = open_site(ORIGIN, 24, [math.radians(200)], 4.0)
    srv = G28Server()
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
    C["all"] = [c]
    time.sleep(1.5)
    gm("/help")


def t_not_dead_when_alive():
    fresh()
    G = C["G"]
    check(G.respawn(HERE_CHOICE) == R_NOT_DEAD, "HERE while alive was not NOT_DEAD")
    check(G.respawn(CITY_CHOICE) == R_NOT_DEAD, "CITY while alive was not NOT_DEAD")
    check(info(G)["hp"] > 0, "alive request changed vitals")


def t_penalty_level_above_five():
    tExp = earn_exp(6)
    G = C["G"]
    tMark = G.mark()
    tTime, tMark = die()
    tInfo = settle_after_death()
    tWant = tExp - death_loss(6, tExp)
    check(tInfo["hp"] <= 0, f"not dead: {tInfo}")
    check(tInfo["level"] == 6, f"level changed on death: {tInfo['level']}")
    check(tInfo["exp"] == tWant, f"exp {tExp} -> {tInfo['exp']}, want {tWant}")
    with G.cv:
        tTexts = [m.response.system_message.text for _, m in G.log[tMark:] if m.response.HasField("system_message")]
    check(any(str(tExp - tWant) in t for t in tTexts), f"no exp-loss system message with {tExp - tWant}: {tTexts}")
    wait_choosable(tTime)
    check(G.respawn(CITY_CHOICE) == R_OK, "CITY after 5 s not OK")
    check(poll(lambda: info(G)["hp"] > 0, 4), "not alive after CITY")
    check(info(G)["exp"] == tWant, f"exp changed by respawn: {info(G)['exp']}")
    time.sleep(1.0)
    tTime2, _ = die()
    tInfo2 = settle_after_death()
    tWant2 = tWant - death_loss(6, tWant)
    check(tInfo2["exp"] == tWant2, f"second death exp {tWant} -> {tInfo2['exp']}, want {tWant2}")
    check(tInfo2["exp"] >= 0 and tInfo2["level"] == 6, f"bad state after second death {tInfo2}")
    C["last_death"] = tTime2


def t_no_penalty_level_five_or_less():
    tExp = earn_exp(5)
    G = C["G"]
    tTime, _ = die()
    tInfo = settle_after_death()
    check(tInfo["hp"] <= 0, "not dead")
    check(tInfo["exp"] == tExp, f"level 5 lost exp: {tExp} -> {tInfo['exp']}")
    check(tInfo["level"] == 5, f"level changed {tInfo['level']}")


def t_gm_kill_no_penalty():
    tExp = earn_exp(10)
    G = C["G"]
    gm("/kill")
    check(poll(lambda: info(G)["hp"] <= 0, 4), "GM /kill did not kill")
    tInfo = settle_after_death()
    check(tInfo["hp"] <= 0, "not dead after /kill")
    check(tInfo["exp"] == tExp, f"GM /kill cost exp: {tExp} -> {tInfo['exp']}")
    check(tInfo["level"] == 10, "level changed after /kill")


def t_early_invalid_and_vitals():
    tExp = earn_exp(8)
    G = C["G"]
    tTime, _ = die()
    tInfo = settle_after_death()
    check(tInfo.get("respawn", 0) > 0, f"respawn_seconds not set on death: {tInfo}")
    check(tInfo.get("auto", 0) > tInfo["respawn"] and tInfo["auto"] <= AUTO_SECONDS + 0.5,
          f"auto_respawn_seconds {tInfo.get('auto')} not within ({tInfo['respawn']}, {AUTO_SECONDS}]")
    tEarly = G.respawn(HERE_CHOICE)
    check(tEarly == R_TOO_EARLY, f"request before 5 s got {tEarly}, want TOO_EARLY")
    check(info(G)["hp"] <= 0, "TOO_EARLY request revived the player")
    wait_choosable(tTime)
    check(G.respawn(UNSPECIFIED) == R_INVALID, "UNSPECIFIED choice not INVALID")
    check(G.respawn(99) == R_INVALID, "choice 99 not INVALID")
    check(info(G)["hp"] <= 0, "invalid choice revived the player")
    check(G.respawn(HERE_CHOICE) == R_OK, "valid choice after invalid ones not OK")
    check(poll(lambda: info(G)["hp"] > 0, 4), "not alive after HERE")
    check(G.respawn(HERE_CHOICE) == R_NOT_DEAD, "repeat request not NOT_DEAD")
    check(G.respawn(CITY_CHOICE) == R_NOT_DEAD, "repeat CITY request not NOT_DEAD")


def t_revive_here():
    tExp = earn_exp(7)
    G = C["G"]
    tMap0, x0, z0 = get_pos()
    tTime, _ = die()
    tInfo = settle_after_death()
    tAfterDeath = tInfo["exp"]
    wait_choosable(tTime)
    check(G.respawn(HERE_CHOICE) == R_OK, "HERE not OK")
    tMark = G.mark()
    check(poll(lambda: info(G)["hp"] > 0, 4), "not alive after HERE")
    time.sleep(0.8)
    tInfo = info(G)
    check(abs(tInfo["hp"] - tInfo["max"] * 0.5) <= max(2, tInfo["max"] * 0.03), f"HERE health {tInfo['hp']}/{tInfo['max']} is not about 50%")
    tMap, x, z = get_pos()
    check(tMap == tMap0 and math.hypot(x - x0, z - z0) < 1.5, f"HERE moved the player: ({x0:.1f},{z0:.1f}) -> ({x:.1f},{z:.1f})")
    check(tInfo["exp"] == tAfterDeath, f"HERE changed exp {tAfterDeath} -> {tInfo['exp']}")


def t_revive_city():
    tExp = earn_exp(7)
    G = C["G"]
    tp(*C["home"])
    tTime, _ = die()
    tInfo = settle_after_death()
    tAfterDeath = tInfo["exp"]
    wait_choosable(tTime)
    check(G.respawn(CITY_CHOICE) == R_OK, "CITY not OK")
    check(poll(lambda: info(G)["hp"] > 0, 4), "not alive after CITY")
    time.sleep(0.8)
    tInfo = info(G)
    check(tInfo["hp"] == tInfo["max"], f"CITY health {tInfo['hp']}/{tInfo['max']} not full")
    tMap, x, z = get_pos()
    check(tMap == 101 and math.hypot(x - ORIGIN[0], z - ORIGIN[1]) < 3.0, f"CITY did not place at map spawn: map {tMap} ({x:.1f},{z:.1f})")
    check(math.hypot(x - C["home"][0], z - C["home"][1]) > 5.0, "CITY position equals death position")
    check(tInfo["exp"] == tAfterDeath, f"CITY changed exp {tAfterDeath} -> {tInfo['exp']}")
    check(G.respawn(CITY_CHOICE) == R_NOT_DEAD, "repeat CITY not NOT_DEAD")


def t_auto_city():
    fresh()
    G = C["G"]
    gm("/level 7")
    tp(*C["home"])
    tTime, tMark = die()
    tRevived = alive_time(G, tMark, AUTO_SECONDS + 6)
    check(tRevived is not None, f"no automatic revive within {AUTO_SECONDS + 6} s")
    tDt = tRevived - tTime
    check(AUTO_SECONDS - 1.5 <= tDt <= AUTO_SECONDS + 3.0, f"auto revive after {tDt:.1f} s, expected about {AUTO_SECONDS}")
    time.sleep(0.8)
    tInfo = info(G)
    check(tInfo["hp"] == tInfo["max"], f"auto revive health {tInfo['hp']}/{tInfo['max']} not full")
    tMap, x, z = get_pos()
    check(math.hypot(x - ORIGIN[0], z - ORIGIN[1]) < 3.0, f"auto revive not at the map spawn: ({x:.1f},{z:.1f})")
    check(G.respawn(HERE_CHOICE) == R_NOT_DEAD, "request after auto revive not NOT_DEAD")


def t_relog_while_dead():
    tExp = earn_exp(9)
    G = C["G"]
    tTime, _ = die()
    tInfo = settle_after_death()
    tAfterDeath = tInfo["exp"]
    check(tAfterDeath == tExp - death_loss(9, tExp), f"penalty wrong before relog: {tExp} -> {tAfterDeath}")
    d = relog()
    tInfo = poll(lambda: info(d), 5)
    check(tInfo is not None, "no vitals after relog")
    check(tInfo["hp"] <= 0, f"relog revived a dead character: {tInfo}")
    check(tInfo["exp"] == tAfterDeath, f"exp after relog {tInfo['exp']}, want {tAfterDeath}")
    tRelogged = time.time()
    time.sleep(CHOOSE_SECONDS + 0.8)
    r = d.respawn(HERE_CHOICE)
    check(r == R_OK, f"HERE after relog got {r}, want OK")
    check(poll(lambda: info(d)["hp"] > 0, 4), "not alive after relog + HERE")
    time.sleep(0.8)
    check(info(d)["exp"] == tAfterDeath, f"penalty applied again after relog: {tAfterDeath} -> {info(d)['exp']}")
    d2 = relog()
    tInfo = poll(lambda: info(d2), 5)
    check(tInfo is not None and tInfo["hp"] > 0, f"alive character not alive after second relog: {tInfo}")
    check(tInfo["exp"] == tAfterDeath, f"exp after second relog {tInfo['exp']}, want {tAfterDeath}")


def t_dead_player_skips_portal():
    fresh()
    G = C["G"]
    gm("/level 7")
    px, pz = C["portal"]
    ux, uz = ORIGIN[0] - px, ORIGIN[1] - pz
    tLen = math.hypot(ux, uz)
    ux, uz = ux / tLen, uz / tLen
    tEdge = (px + ux * 3.6, pz + uz * 3.6)
    tInside = (px + ux * 1.5, pz + uz * 1.5)
    tp(*tEdge)
    tTime, _ = die()
    settle_after_death()
    G.move(tInside[0], nav().y_at(*tInside), tInside[1], 0.0)
    time.sleep(1.5)
    tMap, x, z = get_pos()
    check(tMap == 101, f"dead player was moved through the portal to map {tMap}")
    check(math.hypot(x - px, z - pz) > 3.0, f"dead player moved onto the portal: ({x:.1f},{z:.1f})")
    wait_choosable(tTime)
    check(G.respawn(CITY_CHOICE) == R_OK, "CITY after portal attempt not OK")
    check(poll(lambda: info(G)["hp"] > 0, 4), "not alive")
    time.sleep(1.0)
    tp(*tEdge)
    G.move(tInside[0], nav().y_at(*tInside), tInside[1], 0.0)
    G.pos = (tInside[0], nav().y_at(*tInside), tInside[1])
    tMapNow = None
    tEnd = time.time() + 4
    while time.time() < tEnd:
        time.sleep(0.5)
        tMapNow, _, _ = get_pos()
        if tMapNow == 102:
            break
    check(tMapNow == 102, f"control: alive player did not use the portal (map {tMapNow}); portal geometry invalid for this test")
    time.sleep(1.5)
    tp(*tEdge)
    G.move(tInside[0], nav().y_at(*tInside), tInside[1], 0.0)
    check(poll(lambda: get_pos()[0] == 101, 5, 0.5), "could not return to map 101 through the 102 portal")


def t_server_survived():
    srv = C["srv"]
    check(srv.proc.poll() is None, "server process exited\n" + "\n".join(srv.log_lines[-15:]))
    check(not any("Assertion" in l or "terminate" in l.lower() or "exception" in l.lower() for l in srv.log_lines),
          "exception/assert line in server log")


TESTS = [t_not_dead_when_alive, t_penalty_level_above_five, t_no_penalty_level_five_or_less, t_gm_kill_no_penalty,
         t_early_invalid_and_vitals, t_revive_here, t_revive_city, t_auto_city, t_relog_while_dead,
         t_dead_player_skips_portal, t_server_survived]


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
                tResults.append((t.__name__, False, tMsg[:500]))
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
