import sys, os, time, json, shutil, subprocess, threading, socket, traceback, random
from lakotnet import *

generate_bindings()

RUN = HERE / ".run_g34"
C = {"clients": []}
ENTER_OK = 0
POTION = 4
PORTAL = (0.0, 60.0)
LAND = (0.0, 24.0)
GM_COUNT = 6
SUFFIX = rand_name("")[:6]
GM_ACCOUNTS = [f"gmg34{SUFFIX}{i}" for i in range(GM_COUNT)]
NEXT_ACCOUNT = [0]


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


class G34Server(Server):
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
        tMaps = RUN / "data" / "maps"
        tSrc = tMaps / "101_altunay_village"
        tDst = tMaps / "102_g34_copy"
        shutil.copytree(tSrc, tDst)
        for tDir, tId, tTarget in ((tSrc, 101, 102), (tDst, 102, 101)):
            tPath = tDir / "map.json"
            tMeta = json.load(open(tPath, encoding="utf-8"))
            tMeta["id"] = tId
            tMeta["name"] = f"G34 map {tId}"
            tMeta["monsterSpawns"] = []
            tMeta["safeZones"] = []
            tMeta["portals"] = [{"x": PORTAL[0], "z": PORTAL[1], "radius": 3.0, "targetMap": tTarget,
                                 "targetX": LAND[0], "targetZ": LAND[1], "targetYaw": 0.0}]
            json.dump(tMeta, open(tPath, "w", encoding="utf-8"))
        tEnv = dict(os.environ)
        tEnv["LAKOT_GM_ACCOUNT"] = ",".join(GM_ACCOUNTS)
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


def track(pClient):
    C["clients"].append(pClient)
    return pClient


def logout(pClient):
    pClient.send(lambda q: q.logout_request.SetInParent())


def send_enter(pClient, pCid):
    return pClient.send(lambda q: setattr(q.enter_world_request, "character_id", pCid))


def login_retry(pUser, pLimit=2.0):
    tEnd = time.time() + pLimit
    while True:
        c = track(Client())
        r = c.login(pUser)
        if r is not None and r.login_response.token:
            c.token = r.login_response.token
            return c
        c.close()
        check(time.time() < tEnd, f"login of {pUser} still rejected after {pLimit}s: {r}")
        time.sleep(0.05)


def cmd(pClient, pText, pPace=0.3):
    time.sleep(pPace)
    r = pClient.call(lambda q: setattr(q.command_request, "text", pText), 6.0)
    check(r is not None and r.HasField("command_response"), f"no CommandResponse for {pText}")
    return r.command_response


def cmd_ok(pClient, pText):
    r = cmd(pClient, pText)
    check(r.status == 0, f"{pText}: status {r.status} {list(r.lines)}")
    return r


def inv_state(pClient, pSince=0):
    with pClient.cv:
        tLog = [m.response for _, m in pClient.log[pSince:]]
    for r in reversed(tLog):
        if r.HasField("inventory_update"):
            return r.inventory_update
    return None


def enter_get_inv(pClient, pCid):
    m = pClient.mark()
    rid = send_enter(pClient, pCid)
    r = pClient.wait(lambda x: x.header.reply_to == rid and x.HasField("enter_world_response"), 8, m)
    check(r is not None and r.enter_world_response.status == ENTER_OK, f"enter failed: {r}")
    u = pClient.wait(lambda x: x.HasField("inventory_update"), 4, m)
    check(u is not None, "no InventoryUpdate after enter")
    time.sleep(0.15)
    return inv_state(pClient, m)


def bag_counts(pUpdate):
    tCount = {}
    for it in pUpdate.items:
        if it.location == 0:
            tCount[it.template_id] = tCount.get(it.template_id, 0) + it.count
    return tCount


def item_set(pUpdate):
    return sorted((it.template_id, it.count, it.location, it.slot) for it in pUpdate.items)


def gm_victim():
    tUser = GM_ACCOUNTS[NEXT_ACCOUNT[0]]
    NEXT_ACCOUNT[0] += 1
    c = login_retry(tUser)
    r = c.char_list()
    tChars = list(r.character_list_response.characters)
    if tChars:
        c.cid, c.name = tChars[0].character_id, tChars[0].name
    else:
        c.name = rand_name("g")
        r = c.create_char(c.name, 1)
        check(r is not None and r.character_create_response.status == 0, f"char create failed {r}")
        c.cid = r.character_create_response.character.character_id
    c.user = tUser
    return c


def relog(pClient):
    logout(pClient)
    n = login_retry(pClient.user)
    n.user, n.name, n.cid = pClient.user, pClient.name, pClient.cid
    return n


def visible(pObs):
    tIds = {}
    with pObs.cv:
        tSnaps = [m.response.world_snapshot for _, m in pObs.log if m.response.HasField("world_snapshot")]
    for s in tSnaps:
        for e in s.entered:
            if e.kind == 0:
                tIds[e.entity_id] = e.name
        for i in s.left:
            tIds.pop(i, None)
    return list(tIds.values())


def view_count(pName):
    return visible(C["O1"]).count(pName) + visible(C["O2"]).count(pName)


def online(pName):
    for _ in range(4):
        r = C["O1"].whisper(pName, "g34-probe")
        if r is not None and r.HasField("direct_message_response"):
            tCode = r.direct_message_response.error_code
            if tCode == 0:
                return True
            if tCode == 2:
                return False
        time.sleep(1.0)
    raise AssertionError(f"cannot determine presence of {pName}")


def poll(pFn, pTimeout=4.0, pStep=0.15):
    tEnd = time.time() + pTimeout
    while time.time() < tEnd:
        v = pFn()
        if v:
            return v
        time.sleep(pStep)
    return pFn()


def setup():
    srv = G34Server()
    srv.start()
    for tUser in GM_ACCOUNTS:
        c = Client()
        r = c.register(tUser)
        check(r is not None and r.register_response.register_status == 0, f"gm register failed {r}")
        c.close()
    srv.stop()
    srv.start()
    C["srv"] = srv
    o1 = make_player(1, "obs")
    track(o1)
    enter_world(o1)
    C["O1"] = o1
    NEXT_ACCOUNT[0] = GM_COUNT - 1
    o2 = gm_victim()
    NEXT_ACCOUNT[0] = 0
    enter_get_inv(o2, o2.cid)
    cmd_ok(o2, f"/tp {int(PORTAL[0])} {int(PORTAL[1])}")
    time.sleep(1.5)
    r = cmd_ok(o2, "/pos")
    check("Map 102" in " ".join(r.lines), f"observer did not reach map 102: {list(r.lines)}")
    C["O2"] = o2
    time.sleep(1.0)


def t_gold_relogin_x30():
    c = gm_victim()
    u = enter_get_inv(c, c.cid)
    tExpected = u.stats.gold
    for i in range(30):
        tAdd = random.randint(1, 50)
        cmd_ok(c, f"/gold {tAdd}")
        tExpected += tAdd
        c = relog(c)
        u = enter_get_inv(c, c.cid)
        check(u.stats.gold == tExpected, f"rep {i}: gold {u.stats.gold} after relogin, want {tExpected}")
    logout(c)


def t_item_potion_relogin_x20():
    c = gm_victim()
    u = enter_get_inv(c, c.cid)
    tBase = bag_counts(u).get(POTION, 0)
    cmd_ok(c, f"/item {POTION} 5")
    tWant = tBase + 5
    for i in range(20):
        c = relog(c)
        u = enter_get_inv(c, c.cid)
        tGot = bag_counts(u).get(POTION, 0)
        check(tGot == tWant, f"rep {i}: potions {tGot} after relogin, want {tWant}")
        if i % 2 == 1:
            cmd_ok(c, "/sethp 1")
            r = c.call(lambda q: setattr(q.use_item_request, "template_id", POTION), 6.0)
            check(r is not None and r.use_item_response.status == 0, f"rep {i}: drink -> {r}")
            tWant -= 1
        else:
            cmd_ok(c, f"/item {POTION} 1")
            tWant += 1
            check(tWant <= 20, "test bug: potion stack overflow")
    logout(c)


def t_portal_disconnect_x20():
    v = gm_victim()
    enter_get_inv(v, v.cid)
    for i in range(20):
        v.send(lambda q: setattr(q.command_request, "text", f"/tp {int(PORTAL[0])} {int(PORTAL[1])}"))
        time.sleep(random.choice((0, 0.002, 0.01, 0.03, 0.06, 0.1)))
        logout(v)
        time.sleep(0.2)
        v.close()
        check(poll(lambda: view_count(v.name) == 0 and not online(v.name), 4.0, 0.3),
              f"rep {i}: {v.name} still visible/online after portal+disconnect (views {view_count(v.name)})")
        time.sleep(1.5)
        check(view_count(v.name) == 0, f"rep {i}: ghost view {view_count(v.name)} 2s after disconnect")
        tName, tUser = v.name, v.user
        v = login_retry(tUser)
        v.user, v.name = tUser, tName
        v.cid = v.char_list().character_list_response.characters[0].character_id
        enter_get_inv(v, v.cid)
        check(poll(lambda: view_count(v.name) == 1, 4.0), f"rep {i}: after relogin visible {view_count(v.name)} times, want 1")
        time.sleep(0.5)
        check(view_count(v.name) == 1, f"rep {i}: duplicate views {view_count(v.name)}")
        check(online(v.name), f"rep {i}: not online after relogin")
    logout(v)


def t_big_inventory_relogin():
    c = gm_victim()
    enter_get_inv(c, c.cid)
    tIds = [1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 2001, 2002, 2003, 2004, 2005, 2006, 3001, 3002, 3003, 3004, 3005, 3006]
    for tId in tIds:
        cmd_ok(c, f"/item {tId} 1")
    for tId, tCount in ((POTION, 17), (5, 33), (7, 9)):
        cmd_ok(c, f"/item {tId} {tCount}")
    u = inv_state(c)
    tSlots = {it.template_id: it.slot for it in u.items if it.location == 0}
    for tId in (1001, 2001, 3001):
        c.send(lambda q, s=tSlots[tId]: setattr(q.equip_item_request, "bag_slot", s))
        time.sleep(0.4)
    time.sleep(0.5)
    tBefore = item_set(inv_state(c))
    check(len(tBefore) >= 23, f"setup produced only {len(tBefore)} stacks")
    check(any(e[2] != 0 for e in tBefore), "nothing got equipped")
    for i in range(3):
        c = relog(c)
        u = enter_get_inv(c, c.cid)
        tAfter = item_set(u)
        check(tAfter == tBefore, f"relogin {i}: items differ: missing {sorted(set(tBefore) - set(tAfter))} extra {sorted(set(tAfter) - set(tBefore))}")
    logout(c)


TESTS = [t_gold_relogin_x30, t_item_potion_relogin_x20, t_big_inventory_relogin, t_portal_disconnect_x20]


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
                print("      server log tail:", *[l[:240] for l in srv.log_lines if "ZoneStats" not in l][-30:], sep="\n        ", flush=True)
            print(f"{'PASS' if tResults[-1][1] else 'FAIL'}  {t.__name__}  ({time.time() - t0:.1f}s) {tResults[-1][2]}", flush=True)
    finally:
        for c in C["clients"]:
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
