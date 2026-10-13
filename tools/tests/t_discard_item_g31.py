import sys, os, time, math, json, shutil, subprocess, threading, socket
from lakotnet import *
from t_monsters_g21 import tpl, open_site, step_toward, walk_to, engage, ORIGIN, KIND_GROUND
import t_regen_g25 as base
from t_regen_g25 import C, check, gm, world_player, poll, stats_of, items_of, equip, bag_slot, run_tests, vitals_now
from t_monsters_g21 import View

generate_bindings()

RUN = HERE / ".run_g31"
base.RUN = RUN
base.GM_ACCOUNT = "gm_g31"
base.GM_CHAR = "GmG31Hero"

ORE, MANA, SWORD, ARMOR = 5, 7, 1001, 2001
LOOT = 301
OK, FAILED = 0, 1
PK_OK, PK_NOT_FOUND, PK_NOT_OWNER = 0, 1, 2
STATE = {}


class G31Server(Server):
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
        json.dump({"monsters": [tpl(LOOT, maxHealth=1, attack=1, aggressive=False, gold={"min": 0, "max": 0, "chance": 0.0}, drops=[
            {"itemId": 4, "chance": 0.0, "min": 2, "max": 2, "guaranteed": True},
            {"itemId": MANA, "chance": 0.0, "min": 3, "max": 3, "guaranteed": True}])]},
            open(RUN / "data" / "monsters.json", "w", encoding="utf-8"), indent=1)
        tPath = RUN / "data" / "maps" / "101_altunay_village" / "map.json"
        tMeta = json.load(open(tPath, encoding="utf-8"))
        tMeta["spawn"] = {"x": ORIGIN[0], "z": ORIGIN[1], "yaw": 0.0}
        tMeta["safeZones"] = []
        tMeta["portals"] = []
        tSite = C["site"]
        tMeta["monsterSpawns"] = [{"monster": LOOT, "x": tSite[0], "z": tSite[1], "radius": 1.0, "count": 1}]
        json.dump(tMeta, open(tPath, "w", encoding="utf-8"))
        tEnv = dict(os.environ)
        tEnv["LAKOT_GM_ACCOUNT"] = base.GM_ACCOUNT
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


base.G25Server = G31Server


class Ground:
    def __init__(self, pClient):
        self.c = pClient
        self.i = 0
        self.items = {}
        self.ever = {}
        self.mons = {}

    def ingest(self):
        with self.c.cv:
            tNew = self.c.log[self.i:]
            self.i = len(self.c.log)
        for _, m in tNew:
            if not m.response.HasField("world_snapshot"):
                continue
            s = m.response.world_snapshot
            for e in s.entered:
                if e.kind == KIND_GROUND:
                    d = {"tpl": e.template_id, "count": e.count, "x": e.position.x, "z": e.position.z}
                    self.items[e.entity_id] = d
                    self.ever[e.entity_id] = d
                elif e.kind == 1:
                    self.mons[e.entity_id] = {"x": e.position.x, "z": e.position.z}
            for i in s.left:
                self.items.pop(i, None)

    def new_since(self, pKnown, pTpl):
        self.ingest()
        return {i: d for i, d in self.items.items() if d["tpl"] == pTpl and i not in pKnown}


def total(pClient, pTpl, pLocation=0):
    return sum(i.count for i in items_of(pClient) if i.template_id == pTpl and i.location == pLocation)


def stacks(pClient, pTpl):
    return [i for i in items_of(pClient) if i.template_id == pTpl and i.location == 0]


def give(pClient, pTpl, pCount):
    before = total(pClient, pTpl)
    gm(f"/item {pTpl} {pCount} {pClient.name}")
    check(poll(lambda: total(pClient, pTpl) == before + pCount, 5), f"/item {pTpl} x{pCount} not delivered: {total(pClient, pTpl)} vs {before + pCount}")


def player_with_ore(pCount):
    c = world_player()
    time.sleep(1.0)
    give(c, ORE, pCount)
    g = Ground(c)
    time.sleep(0.5)
    g.ingest()
    return c, g


def discard_ore(pClient, pCount, pDestroy=False):
    tSlot = bag_slot(pClient, ORE)
    check(tSlot is not None, "no ore in bag")
    return pClient.discard(tSlot, ORE, pCount, pDestroy)


def t_partial_drop_and_second_client_pickup():
    a, ga = player_with_ore(30)
    b = world_player()
    gb = Ground(b)
    known = set(ga.items)
    check(discard_ore(a, 10) == OK, "partial discard refused")
    check(poll(lambda: total(a, ORE) == 20, 4), f"bag ore {total(a, ORE)}, expected 20")
    new = poll(lambda: ga.new_since(known, ORE), 4)
    check(len(new) == 1, f"expected one new ground ore entity, got {new}")
    gid, d = next(iter(new.items()))
    check(d["count"] == 10, f"ground stack count {d['count']}, expected 10")
    check(poll(lambda: gid in (gb.ingest() or gb.items), 4), "second client does not see the dropped item")
    check(gb.items[gid]["count"] == 10, "second client sees wrong count")
    check(b.pickup(gid) == PK_OK, "second client could not pick the dropped item up")
    check(poll(lambda: total(b, ORE) == 10, 4), f"second client ore {total(b, ORE)}, expected 10")
    check(a.pickup(gid) == PK_NOT_FOUND, "item still pickable after being taken")
    check(total(a, ORE) == 20, "first client bag changed by foreign pickup")


def t_simultaneous_pickup_single_winner():
    a, ga = player_with_ore(10)
    b, c = world_player(), world_player()
    for tRound in range(3):
        time.sleep(0.2)
        known = set(ga.ever)
        check(discard_ore(a, 1) == OK, f"round {tRound}: discard refused")
        new = poll(lambda: ga.new_since(known, ORE), 4)
        check(len(new) == 1, f"round {tRound}: expected one ground item, got {new}")
        gid = next(iter(new))
        tRes = []
        tGo = threading.Barrier(2)

        def grab(pClient):
            tGo.wait()
            tRes.append(pClient.pickup(gid))
        ts = [threading.Thread(target=grab, args=(x,)) for x in (b, c)]
        for t in ts:
            t.start()
        for t in ts:
            t.join()
        check(sorted(tRes) == [PK_OK, PK_NOT_FOUND], f"round {tRound}: pickup results {tRes}, expected exactly one OK")
    time.sleep(0.8)
    check(total(b, ORE) + total(c, ORE) == 3, f"ore after races: b={total(b, ORE)} c={total(c, ORE)}, expected 3 in total")


def t_destroy_leaves_nothing():
    a, ga = player_with_ore(10)
    known = set(ga.ever)
    check(discard_ore(a, 4, True) == OK, "destroy refused")
    check(poll(lambda: total(a, ORE) == 6, 4), f"bag ore {total(a, ORE)}, expected 6")
    time.sleep(2.0)
    check(not ga.new_since(known, ORE), "destroy created a ground item")


def t_whole_stack_and_excess_count():
    a, ga = player_with_ore(7)
    known = set(ga.ever)
    check(discard_ore(a, 9999) == OK, "oversized count refused")
    check(poll(lambda: total(a, ORE) == 0, 4), f"bag ore {total(a, ORE)}, expected 0 (whole stack)")
    new = poll(lambda: ga.new_since(known, ORE), 4)
    check(len(new) == 1 and next(iter(new.values()))["count"] == 7, f"ground after oversized discard: {new}")
    give(a, ORE, 5)
    time.sleep(0.2)
    known = set(ga.ever)
    check(discard_ore(a, 0) == OK, "count 0 refused")
    new = poll(lambda: ga.new_since(known, ORE), 4)
    check(len(new) == 1 and next(iter(new.values()))["count"] == 5, f"count 0 should drop the whole stack: {new}")


def t_invalid_requests():
    a, ga = player_with_ore(10)
    tSlot = bag_slot(a, ORE)
    known = set(ga.ever)
    check(a.discard(tSlot, ORE + 1, 3) != OK, "wrong template id accepted")
    time.sleep(0.2)
    empty = next(s for s in range(100, 0, -1) if all(i.slot != s for i in items_of(a) if i.location == 0))
    check(a.discard(empty, ORE, 1) != OK, "empty slot accepted")
    time.sleep(0.2)
    check(a.discard(empty, 0, 1) != OK, "empty slot with template 0 accepted")
    time.sleep(0.2)
    tArmorSlot = bag_slot(a, ARMOR)
    check(tArmorSlot is not None, "no starter armor in bag")
    check(equip(a, tArmorSlot) == OK, "equip failed")
    check(poll(lambda: total(a, ARMOR, 1) == 1, 4), "armor not equipped")
    tEq = next(i for i in items_of(a) if i.template_id == ARMOR and i.location != 0)
    time.sleep(0.2)
    check(a.discard(tEq.slot, ARMOR, 1) != OK, "equipped item accepted")
    time.sleep(0.2)
    r = a.call(lambda q: (setattr(q.discard_item_request, "bag_slot", tSlot), setattr(q.discard_item_request, "template_id", ORE),
                          setattr(q.discard_item_request, "count", 1), setattr(q.discard_item_request, "mode", 7)), 4.0)
    check(r is None or not r.HasField("inventory_action_response") or r.inventory_action_response.status != OK, "unknown mode accepted")
    time.sleep(0.5)
    check(total(a, ORE) == 10 and total(a, ARMOR, 1) == 1, f"inventory changed by rejected requests: ore {total(a, ORE)}")
    check(not ga.new_since(known, ORE), "rejected request spawned a ground item")


def t_dead_character_cannot_discard():
    a, ga = player_with_ore(10)
    gm(f"/kill {a.name}")
    check(poll(lambda: vitals_now(a)[0] is not None and vitals_now(a)[0] <= 0, 4), "player did not die")
    known = set(ga.ever)
    check(discard_ore(a, 3) != OK, "dead character could discard")
    check(total(a, ORE) == 10, "dead character lost items")
    time.sleep(0.5)
    check(not ga.new_since(known, ORE), "dead character spawned a ground item")
    gm(f"/revive {a.name}")


def t_rate_limit_100ms():
    a, ga = player_with_ore(20)
    time.sleep(0.3)
    tSlot = bag_slot(a, ORE)
    tMark = a.mark()

    def fill(q):
        q.discard_item_request.bag_slot = tSlot
        q.discard_item_request.template_id = ORE
        q.discard_item_request.count = 1
    ids = [a.send(fill), a.send(fill), a.send(fill)]
    res = {}
    for i in ids:
        r = a.wait(lambda r, i=i: r.header.reply_to == i, 4.0, tMark)
        res[i] = r.inventory_action_response.status if r is not None and r.HasField("inventory_action_response") else None
    tList = [res[i] for i in ids]
    print(f"      burst of 3 discards: {tList}", flush=True)
    check(tList[0] == OK and FAILED in tList[1:], f"burst not rate limited: {tList}")
    time.sleep(0.4)
    check(a.discard(tSlot, ORE, 1) == OK, "discard after waiting refused")
    time.sleep(0.5)
    tOk = tList.count(OK) + 1
    check(total(a, ORE) == 20 - tOk, f"bag ore {total(a, ORE)}, expected {20 - tOk}")


def t_drop_then_pickup_merges():
    a, ga = player_with_ore(20)
    known = set(ga.ever)
    check(discard_ore(a, 5) == OK, "discard refused")
    new = poll(lambda: ga.new_since(known, ORE), 4)
    check(len(new) == 1, f"expected one ground item {new}")
    gid = next(iter(new))
    check(a.pickup(gid) == PK_OK, "could not pick own dropped item up")
    check(poll(lambda: total(a, ORE) == 20, 4), f"ore {total(a, ORE)}, expected 20")
    time.sleep(0.5)
    tStacks = stacks(a, ORE)
    check(len(tStacks) == 1 and tStacks[0].count == 20, f"stacks did not merge: {[(s.slot, s.count) for s in tStacks]}")


def t_bot_auto_pickup_not_owner_then_next():
    a, ga = player_with_ore(5)
    b = world_player()
    gb = Ground(b)
    time.sleep(1.5)
    gb.ingest()
    check(gb.mons, "no loot monster in view")
    ga.ingest()
    check(ga.mons, "no loot monster in view of the killer")
    tId = next(iter(ga.mons))
    va, vb = View(a), View(b)
    tMon = ga.mons[tId]
    walk_to(b, vb, tMon["x"] - 1.5, tMon["z"], 0.8)
    check(engage(a, va, tId, lambda: tId in va.dead, 40), "loot monster not killed")
    time.sleep(0.4)
    known = set(ga.ever)
    check(discard_ore(a, 2) == OK, "discard refused")
    tFree = poll(lambda: ga.new_since(known, ORE), 4)
    check(len(tFree) == 1, "discarded ore not seen")
    tFreeId = next(iter(tFree))
    time.sleep(0.5)
    gb.ingest()
    check(tFreeId in gb.items, "bot does not see the discarded item")
    tOrder = sorted(gb.items, key=lambda i: math.hypot(gb.items[i]["x"] - b.pos[0], gb.items[i]["z"] - b.pos[2]))
    tLog = []
    for i in tOrder:
        tLog.append((i, gb.items[i]["tpl"], b.pickup(i)))
        time.sleep(0.15)
    print(f"      bot pickups (id, tpl, result): {tLog}", flush=True)
    tOwned = [r for i, t, r in tLog if i != tFreeId]
    check(PK_NOT_OWNER in tOwned, f"no NOT_OWNER on the killer's loot (window elapsed or loot missing): {tLog}")
    check([r for i, t, r in tLog if i == tFreeId] == [PK_OK], f"discarded item not picked after skipping: {tLog}")
    check(all(r != PK_OK for i, t, r in tLog if i != tFreeId), f"bot picked up the killer's loot: {tLog}")
    check(poll(lambda: total(b, ORE) == 2, 4), f"bot ore {total(b, ORE)}")


def t_ground_cap_rejects_without_losing_items():
    tN = 5
    cs = [world_player() for _ in range(tN)]
    for c in cs:
        for _ in range(3):
            give(c, ORE, 50)
    tRes = {id(c): [] for c in cs}

    def spam(pClient):
        for _ in range(140):
            tSlot = bag_slot(pClient, ORE)
            if tSlot is None:
                break
            tRes[id(pClient)].append(pClient.discard(tSlot, ORE, 1, False, 6.0))
            time.sleep(0.13)
    ts = [threading.Thread(target=spam, args=(c,)) for c in cs]
    for t in ts:
        t.start()
    for t in ts:
        t.join()
    time.sleep(1.0)
    tOk = sum(r.count(OK) for r in tRes.values())
    tFail = sum(1 for r in tRes.values() for s in r if s != OK)
    print(f"      cap run: ok={tOk} failed={tFail}", flush=True)
    check(tFail > 0, f"700 discards all accepted (ok={tOk}); ground cap of 500 never hit")
    check(tOk <= 520, f"{tOk} items accepted, far above the 500 ground cap")
    for c in cs:
        tOkC = tRes[id(c)].count(OK)
        check(poll(lambda: total(c, ORE) == 150 - tOkC, 4), f"rejected discards lost items: bag {total(c, ORE)}, expected {150 - tOkC}")


def relog(pClient):
    tUser, tCid, tName = pClient.user, pClient.cid, pClient.name
    pClient.close()
    time.sleep(3)
    d = Client()
    C["all"].append(d)
    r = d.login(tUser)
    check(r is not None and r.login_response.token, "relog login failed")
    r = d.enter(tCid)
    check(r is not None and r.enter_world_response.status == 0, f"relog enter failed {r}")
    d.user, d.cid, d.name = tUser, tCid, tName
    check(poll(lambda: stats_of(d) is not None, 6), "no InventoryUpdate after relog")
    time.sleep(1.0)
    return d


def t_discard_persists_across_relog_and_restart():
    a, ga = player_with_ore(7)
    check(discard_ore(a, 3) == OK, "discard refused")
    check(poll(lambda: total(a, ORE) == 4, 4), "bag not reduced")
    time.sleep(1.0)
    d = relog(a)
    check(total(d, ORE) == 4, f"after relog ore {total(d, ORE)}, expected 4")
    time.sleep(0.2)
    check(discard_ore(d, 1) == OK, "second discard refused")
    check(poll(lambda: total(d, ORE) == 3, 4), "bag not reduced after relog")
    time.sleep(1.0)
    tUser, tCid = d.user, d.cid
    d.close()
    time.sleep(1.0)
    srv = C["srv"]
    srv.stop()
    srv.start()
    tGm = C["G"]
    g2 = Client()
    C["all"].append(g2)
    r = g2.login(base.GM_ACCOUNT)
    check(r is not None and r.login_response.token, "gm login after restart failed")
    g2.cid, g2.name = tGm.cid, tGm.name
    enter_world(g2)
    C["G"] = g2
    time.sleep(1.5)
    e = Client()
    C["all"].append(e)
    r = e.login(tUser)
    check(r is not None and r.login_response.token, "login after restart failed")
    r = e.enter(tCid)
    check(r is not None and r.enter_world_response.status == 0, f"enter after restart failed {r}")
    check(poll(lambda: stats_of(e) is not None, 6), "no InventoryUpdate after restart")
    time.sleep(1.0)
    check(total(e, ORE) == 3, f"after restart ore {total(e, ORE)}, expected 3 (dropped items must not return)")
    ge = Ground(e)
    time.sleep(1.0)
    ge.ingest()
    check(not any(d["tpl"] == ORE for d in ge.items.values()), "dropped ore survived the restart on the ground")


def t_server_survived():
    srv = C["srv"]
    check(srv.proc.poll() is None, "server process exited\n" + "\n".join(srv.log_lines[-15:]))
    check(not any("Assertion" in l or "terminate" in l.lower() or "exception" in l.lower() for l in srv.log_lines),
          "exception/assert line in server log")


TESTS = [t_partial_drop_and_second_client_pickup, t_simultaneous_pickup_single_winner, t_destroy_leaves_nothing,
         t_whole_stack_and_excess_count, t_invalid_requests, t_dead_character_cannot_discard, t_rate_limit_100ms,
         t_drop_then_pickup_merges, t_bot_auto_pickup_not_owner_then_next, t_discard_persists_across_relog_and_restart,
         t_ground_cap_rejects_without_losing_items, t_server_survived]

if __name__ == "__main__":
    run_tests(TESTS)
