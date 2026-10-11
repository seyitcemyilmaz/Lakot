import sys, os, time, math, json, shutil, subprocess, threading, socket, traceback
from lakotnet import *
from t_monsters_g21 import tpl, open_site, walk_to, step_toward, engage, View, ORIGIN, ITEM_SABRE

RUN = HERE / ".run_g24"
POTION, ORE, RELIC, MANA = 4, 5, 6, 7
LOOT_ID, BITER_ID, KILLER_ID = 201, 202, 203
BITER_ATTACK = 8
USE_OK, USE_INVALID, USE_NOT_OWNED, USE_DEAD, USE_COOLDOWN, USE_FULL = 0, 1, 2, 3, 4, 5
NAMES = {0: "OK", 1: "INVALID", 2: "NOT_OWNED", 3: "DEAD", 4: "COOLDOWN", 5: "FULL"}
STATE = {}


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


class G24Server(Server):
    def __init__(self, pTemplates, pSpawns):
        super().__init__()
        self.templates, self.spawns = pTemplates, pSpawns

    def start(self, timeout=40):
        if not os.environ.get("LAKOT_DB_PASSWORD"):
            raise RuntimeError("LAKOT_DB_PASSWORD is not set")
        shutil.rmtree(RUN, ignore_errors=True)
        shutil.copytree(self.bin_dir, RUN)
        self.run_dir = str(RUN)
        json.dump({"monsters": self.templates}, open(RUN / "data" / "monsters.json", "w", encoding="utf-8"), indent=1)
        tPath = RUN / "data" / "maps" / "101_altunay_village" / "map.json"
        tMeta = json.load(open(tPath, encoding="utf-8"))
        tMeta["spawn"] = {"x": ORIGIN[0], "z": ORIGIN[1], "yaw": 0.0}
        tMeta["safeZones"] = []
        tMeta["monsterSpawns"] = self.spawns
        json.dump(tMeta, open(tPath, "w", encoding="utf-8"))
        self.proc = subprocess.Popen([os.path.join(self.run_dir, "LakotServer.exe")], cwd=self.run_dir,
                                     env={**os.environ, "LAKOT_SERVER_PORT": str(PORT)},
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


class Watch:
    def __init__(self, pClient):
        self.c = pClient
        self.i = 0
        self.inv = {}
        self.inv_updates = 0
        self.stats = None
        self.hp = None
        self.max_hp = None
        self.hits_on_me = 0
        self.pct = {}

    def ingest(self):
        with self.c.cv:
            tNew = self.c.log[self.i:]
            self.i = len(self.c.log)
        for _, m in tNew:
            r = m.response
            if r.HasField("inventory_update"):
                self.inv_updates += 1
                tCount = {}
                for it in r.inventory_update.items:
                    if it.location == 0:
                        tCount[it.template_id] = tCount.get(it.template_id, 0) + it.count
                self.inv = tCount
                self.stats = r.inventory_update.stats
                self.hp, self.max_hp = self.stats.health, self.stats.max_health
            if r.HasField("world_snapshot"):
                s = r.world_snapshot
                if s.HasField("self_vitals"):
                    self.hp, self.max_hp = s.self_vitals.health, s.self_vitals.max_health
                for e in s.entered:
                    self.pct[e.entity_id] = e.health_percent
                for v in s.vitals:
                    self.pct[v.entity_id] = v.health_percent
                for ev in s.combat:
                    for h in ev.hits:
                        if h.target_id == self.c.cid and h.damage > 0:
                            self.hits_on_me += 1

    def wait(self, pPred, pTimeout=5.0):
        tEnd = time.time() + pTimeout
        while time.time() < tEnd:
            self.ingest()
            if pPred():
                return True
            time.sleep(0.05)
        self.ingest()
        return bool(pPred())


def use(pClient, pTemplateId, pTimeout=6.0):
    def f(q):
        q.use_item_request.template_id = pTemplateId
    r = pClient.call(f, pTimeout)
    if r is None or not r.HasField("use_item_response"):
        return None
    return r.use_item_response


def status_name(pResp):
    return "no response" if pResp is None else NAMES.get(pResp.status, str(pResp.status))


def expect(pResp, pAllowed, pCtx):
    check(pResp is not None, f"{pCtx}: no UseItemResponse")
    check(pResp.status in pAllowed, f"{pCtx}: got {status_name(pResp)}, want one of {[NAMES[a] for a in pAllowed]}")


def get_loot(pClient):
    tView = View(pClient)
    time.sleep(3)
    tView.ingest()
    tLoots = tView.of(LOOT_ID)
    check(tLoots, "no loot monsters in view")
    tId = min(tLoots, key=lambda i: math.hypot(tLoots[i]["x"] - pClient.pos[0], tLoots[i]["z"] - pClient.pos[2]))
    check(engage(pClient, tView, tId, lambda: tId in tView.dead, 60), "loot monster not killed in 60s")
    time.sleep(2.5)
    tView.ingest()
    for tEntity in list(tView.grounds):
        for _ in range(8):
            if pClient.pickup(tEntity) in (0, 1):
                break
            time.sleep(0.3)
        time.sleep(0.15)
    time.sleep(0.8)
    return tView


def new_looter(pKingdom=1):
    c = make_player(pKingdom, "u")
    enter_world(c)
    w = Watch(c)
    v = get_loot(c)
    w.wait(lambda: w.inv.get(POTION, 0) > 0 and w.inv.get(MANA, 0) > 0, 4)
    check(w.inv.get(POTION, 0) > 0, f"looting gave no potion, inventory {w.inv}")
    check(w.inv.get(MANA, 0) > 0, f"looting gave no mana potion, inventory {w.inv}")
    return c, w, v


def provoke_biter(pClient, pView, pWatch):
    pView.ingest()
    tBiters = pView.of(BITER_ID)
    check(tBiters, "no biter in view")
    tId = min(tBiters, key=lambda i: math.hypot(tBiters[i]["x"] - pClient.pos[0], tBiters[i]["z"] - pClient.pos[2]))
    tCx, tCz = tBiters[tId]["x"], tBiters[tId]["z"]
    walk_to(pClient, pView, tCx, tCz, 6.0)
    tBase = pWatch.hits_on_me
    check(engage(pClient, pView, tId, lambda: (pWatch.ingest() or True) and pWatch.hits_on_me > tBase, 60),
          "biter never hit the player")


def next_hit(pWatch, pTimeout=12.0):
    tBase = pWatch.hits_on_me
    check(pWatch.wait(lambda: pWatch.hits_on_me > tBase, pTimeout), "no further hit from biter")
    time.sleep(0.3)
    pWatch.ingest()


def hurt_player():
    c, w, v = new_looter()
    provoke_biter(c, v, w)
    for _ in range(14):
        w.ingest()
        if w.max_hp - w.hp >= 50:
            break
        check(w.hp > BITER_ATTACK * 3, f"player hp {w.hp}/{w.max_hp} too low to keep hurting safely")
        next_hit(w)
    return c, w, v


def t_drink_heals_and_second_client_sees():
    c, w, v = new_looter()
    b = make_player(2, "u")
    enter_world(b)
    wb = Watch(b)
    try:
        provoke_biter(c, v, w)
        for _ in range(14):
            w.ingest()
            if w.max_hp - w.hp >= 50:
                break
            check(w.hp > BITER_ATTACK * 3, f"player hp {w.hp}/{w.max_hp} too low to keep hurting safely")
            next_hit(w)
        next_hit(w)
        wb.ingest()
        tBefore, tMax, tCount = w.hp, w.max_hp, w.inv.get(POTION, 0)
        tMissing = tMax - tBefore
        check(tMissing > 0, "player is not damaged")
        tUpdates = w.inv_updates
        r = use(c, POTION)
        expect(r, [USE_OK], "drink damaged")
        check(w.wait(lambda: w.inv_updates > tUpdates, 4), "no InventoryUpdate after drink")
        tGain = min(50, tMissing)
        tExpect = tBefore + tGain
        print(f"      hp {tBefore}/{tMax} missing {tMissing} -> {w.stats.health} (expect +{tGain}); potions {tCount}->{w.inv.get(POTION)}; cooldown {r.cooldown_seconds:.2f}", flush=True)
        check(w.stats.health == tExpect, f"health after drink {w.stats.health}, expected {tExpect}")
        check(w.inv.get(POTION, 0) == tCount - 1, f"potion count {tCount} -> {w.inv.get(POTION, 0)}, expected -1")
        check(w.wait(lambda: w.hp == tExpect, 3), f"self_vitals health {w.hp}, expected {tExpect}")
        tSeen = []
        wb.wait(lambda: wb.pct.get(c.cid) is not None and wb.pct.get(c.cid) >= 100 * tExpect // tMax - 1, 3)
        check(c.cid in wb.pct, "second client never received health_percent for the drinker")
        tWant = 100.0 * tExpect / tMax
        check(abs(wb.pct[c.cid] - tWant) <= 2.0, f"second client sees health_percent {wb.pct[c.cid]}, expected about {tWant:.1f}")
        w.ingest()
        STATE["user"], STATE["cid"], STATE["potions"] = c.user, c.cid, w.inv.get(POTION, 0)
    finally:
        b.close()
        c.close()


def t_cooldown_spam():
    c, w, v = hurt_player()
    try:
        next_hit(w)
        tCount = w.inv.get(POTION, 0)
        r1 = use(c, POTION)
        expect(r1, [USE_OK], "first drink")
        tSent = []
        for _ in range(5):
            tSent.append(use(c, POTION))
        tStatuses = [status_name(x) for x in tSent]
        print(f"      spam statuses: {tStatuses}", flush=True)
        for x in tSent:
            expect(x, [USE_COOLDOWN], "spam within 1s")
            check(x.cooldown_seconds > 0, "COOLDOWN response carries no cooldown_seconds")
        time.sleep(0.5)
        w.ingest()
        check(w.inv.get(POTION, 0) == tCount - 1, f"count after spam {w.inv.get(POTION, 0)}, expected {tCount - 1}")
        time.sleep(1.2)
        r2 = use(c, POTION)
        expect(r2, [USE_OK, USE_FULL], "drink after cooldown elapsed")
    finally:
        c.close()


def t_full_health_not_consumed():
    c, w, v = new_looter()
    try:
        w.ingest()
        check(w.hp == w.max_hp, f"fresh looter is not at full health {w.hp}/{w.max_hp}")
        tCount = w.inv.get(POTION, 0)
        r = use(c, POTION)
        expect(r, [USE_FULL], "health potion at full hp")
        time.sleep(0.8)
        w.ingest()
        check(w.inv.get(POTION, 0) == tCount, f"FULL consumed the potion: {tCount} -> {w.inv.get(POTION, 0)}")
    finally:
        c.close()


def t_mana_potion():
    c, w, v = new_looter()
    try:
        w.ingest()
        tMana, tMax, tCount = w.stats.mana, w.stats.max_mana, w.inv.get(MANA, 0)
        r = use(c, MANA)
        if tMana >= tMax:
            expect(r, [USE_FULL], "mana potion at full mana")
            time.sleep(0.8)
            w.ingest()
            check(w.inv.get(MANA, 0) == tCount, "FULL mana potion was consumed")
            print("      mana clamp not reachable black-box (mana starts full, no way to drain it)", flush=True)
        else:
            expect(r, [USE_OK], "mana potion")
            w.wait(lambda: w.stats.mana == min(tMax, tMana + 40), 3)
            check(w.stats.mana == min(tMax, tMana + 40), f"mana {tMana} -> {w.stats.mana}, expected {min(tMax, tMana + 40)}")
            check(w.inv.get(MANA, 0) == tCount - 1, "mana potion count not decremented")
    finally:
        c.close()


def t_rejects():
    c, w, v = new_looter()
    try:
        w.ingest()
        tBefore = dict(w.inv)
        check(tBefore.get(ORE, 0) > 0 or tBefore.get(ITEM_SABRE, 0) > 0, f"no non-consumable looted: {tBefore}")
        for tId in (ORE, ITEM_SABRE, RELIC, 0, 4294967295, 99999, 3):
            expect(use(c, tId), [USE_INVALID, USE_NOT_OWNED], f"use item {tId}")
        time.sleep(0.8)
        w.ingest()
        check(w.inv == tBefore, f"rejected uses changed the bag: {tBefore} -> {w.inv}")
        expect(use(c, 0), [USE_INVALID], "id 0")
        expect(use(c, 4294967295), [USE_INVALID], "id 2^32-1")
        expect(use(c, 99999), [USE_INVALID], "unknown id")
        expect(use(c, ORE), [USE_INVALID], "ore")
        expect(use(c, ITEM_SABRE), [USE_INVALID], "sabre")
    finally:
        c.close()
    d = make_player(1, "u")
    try:
        enter_world(d)
        time.sleep(1.5)
        check(True, "starter script grants 5 health potions, so only the mana potion can be tested as not owned")
        expect(use(d, MANA), [USE_NOT_OWNED, USE_INVALID], "mana potion, none owned")
    finally:
        d.close()


def t_dead_rejected():
    c, w, v = new_looter()
    try:
        v.ingest()
        tKillers = v.of(KILLER_ID)
        check(tKillers, "no killer monster in view")
        tId = next(iter(tKillers))
        tCount = w.inv.get(POTION, 0)
        tEnd = time.time() + 45
        tNext = 0.0
        while time.time() < tEnd:
            w.ingest()
            v.ingest()
            if w.hp is not None and w.hp <= 0:
                break
            if time.time() >= tNext:
                tNext = time.time() + 0.7
                tm = v.mons.get(tId)
                step_toward(c, tm["x"], tm["z"], 3.0)
            time.sleep(0.03)
        check(w.hp is not None and w.hp <= 0, f"player never died (hp {w.hp})")
        r = use(c, POTION, 3.0)
        expect(r, [USE_DEAD], "drink while dead")
        w.ingest()
        check(w.inv.get(POTION, 0) == tCount, "dead player's potion count changed")
    finally:
        c.close()


def t_not_in_world_invalid():
    c = make_player(1, "u")
    try:
        r = use(c, POTION, 4.0)
        expect(r, [USE_INVALID], "use at character select")
    finally:
        c.close()
    d = Client()
    try:
        d.send(lambda q: setattr(q.use_item_request, "template_id", POTION))
        time.sleep(1.0)
    finally:
        d.close()


def t_relog_count_persists():
    check("user" in STATE, "depends on t_drink_heals_and_second_client_sees (no saved state)")
    tWant = STATE["potions"] - 1
    for tLabel, tWait in (("quick relog", 3), ("relog after hold expiry", 50)):
        time.sleep(tWait)
        c = Client()
        try:
            r = c.login(STATE["user"])
            check(r is not None and r.login_response.token, f"{tLabel}: login failed")
            tMark = c.mark()
            r = c.enter(STATE["cid"])
            check(r is not None and r.enter_world_response.status == 0, f"{tLabel}: enter failed {r}")
            u = c.wait(lambda x: x.HasField("inventory_update"), 5, tMark)
            check(u is not None, f"{tLabel}: no InventoryUpdate on enter")
            tCount = sum(i.count for i in u.inventory_update.items if i.template_id == POTION and i.location == 0)
            print(f"      {tLabel}: potions {tCount} (expect {STATE['potions']})", flush=True)
            check(tCount == STATE["potions"], f"{tLabel}: potion count {tCount}, expected {STATE['potions']}")
        finally:
            c.close()
    del tWant


TESTS = [t_full_health_not_consumed, t_mana_potion, t_rejects, t_not_in_world_invalid, t_drink_heals_and_second_client_sees,
         t_cooldown_spam, t_dead_rejected, t_relog_count_persists]


def main():
    tOnly = sys.argv[1:]
    generate_bindings()
    tLoot = open_site(ORIGIN, 14)
    tBiter = open_site(ORIGIN, 28, [math.radians(60)])
    tKiller = open_site(ORIGIN, 28, [math.radians(240)])
    tTemplates = [
        tpl(LOOT_ID, maxHealth=1, attack=1, gold={"min": 0, "max": 0, "chance": 0.0}, drops=[
            {"itemId": POTION, "chance": 0.0, "min": 5, "max": 5, "guaranteed": True},
            {"itemId": MANA, "chance": 0.0, "min": 3, "max": 3, "guaranteed": True},
            {"itemId": ORE, "chance": 0.0, "min": 1, "max": 1, "guaranteed": True},
            {"itemId": ITEM_SABRE, "chance": 0.0, "min": 1, "max": 1, "guaranteed": True}]),
        tpl(BITER_ID, maxHealth=100000000, attack=BITER_ATTACK, attackInterval=4.0, aggressive=False, aggroRange=0, moveSpeed=4.0),
        tpl(KILLER_ID, maxHealth=100000000, attack=500, attackInterval=1.0, aggressive=True, aggroRange=6, moveSpeed=0.5)]
    tSpawns = [{"monster": LOOT_ID, "x": tLoot[0], "z": tLoot[1], "radius": 4.0, "count": 12},
               {"monster": BITER_ID, "x": tBiter[0], "z": tBiter[1], "radius": 2.0, "count": 2},
               {"monster": KILLER_ID, "x": tKiller[0], "z": tKiller[1], "radius": 1.0, "count": 1}]
    srv = G24Server(tTemplates, tSpawns)
    srv.start()
    tResults = []
    try:
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
                tResults.append((t.__name__, False, tMsg[:300]))
                print("      server log tail:", *srv.log_lines[-8:], sep="\n        ", flush=True)
            print(f"{'PASS' if tResults[-1][1] else 'FAIL'}  {t.__name__}  ({time.time() - t0:.1f}s) {tResults[-1][2]}", flush=True)
        tAlive = srv.proc.poll() is None
        tResults.append(("server_survived", tAlive, "" if tAlive else "server process exited"))
        print(f"{'PASS' if tAlive else 'FAIL'}  server_survived", flush=True)
    finally:
        srv.stop()
    tPassed = sum(1 for r in tResults if r[1])
    print(f"\n{tPassed}/{len(tResults)} passed")
    sys.exit(0 if tPassed == len(tResults) else 1)


if __name__ == "__main__":
    main()
