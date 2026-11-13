import sys, os, time, json, shutil, subprocess, threading, socket
from lakotnet import *
import t_regen_g25 as base
from t_regen_g25 import C, check, gm, world_player, poll, stats_of, items_of, run_tests, vitals_now
from t_classes_g12 import equip, bag_slot

generate_bindings()

RUN = type(HERE)(os.environ.get("LAKOT_TEST_RUN_DIR") or HERE / ".run_g11")
base.RUN = RUN
base.GM_ACCOUNT = "gm_g11"
base.GM_CHAR = "GmG11Hero"

OK, INVALID, OUT_OF_RANGE, NOT_ENOUGH_GOLD, BAG_FULL, NOT_SOLD_HERE, NOT_SELLABLE, DEAD, TOO_FAST, GOLD_LIMIT = range(10)
NAMES = ["OK", "INVALID", "OUT_OF_RANGE", "NOT_ENOUGH_GOLD", "BAG_FULL", "NOT_SOLD_HERE", "NOT_SELLABLE", "DEAD", "TOO_FAST", "GOLD_LIMIT"]
POTION, ORE, RELIC, MANA, SWORD, ARMOR = 4, 5, 6, 7, 1001, 2001
NEAR_WEAPON = (-29.0, 25.0)
NEAR_ARMOR = (29.0, 25.0)
FAR = (0.0, 24.0)
PORTAL = (0.0, 60.0)
DATA = HERE.parent.parent / "data"
MAP = json.load(open(DATA / "maps" / "101_altunay_village" / "map.json", encoding="utf-8"))
ITEMS = json.load(open(DATA / "items.json", encoding="utf-8"))
RATIO = ITEMS["sellRatio"]
TPL = {i["id"]: i for i in ITEMS["items"]}
STOCK = {n["name"]: n["shop"] for n in MAP["npcs"] if n.get("shop")}


def price(pId):
    return TPL[pId]["price"]


def sell_price(pId):
    return 0 if TPL[pId]["type"] == 4 else int(price(pId) * RATIO)


def name(pStatus):
    return NAMES[pStatus] if pStatus is not None and 0 <= pStatus < len(NAMES) else str(pStatus)


class G11Server(Server):
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
        tMaps = RUN / "data" / "maps"
        shutil.copytree(tMaps / "101_altunay_village", tMaps / "102_g11_copy", dirs_exist_ok=True)
        for tDir, tId, tTarget in ((tMaps / "101_altunay_village", 101, 102), (tMaps / "102_g11_copy", 102, 101)):
            tPath = tDir / "map.json"
            tMeta = json.load(open(tPath, encoding="utf-8"))
            tMeta["id"] = tId
            tMeta["name"] = f"G11 map {tId}"
            tMeta["spawn"] = {"x": FAR[0], "z": FAR[1], "yaw": 0.0}
            tMeta["safeZones"] = []
            tMeta["monsterSpawns"] = []
            if tId == 101:
                tMeta["portals"] = [{"x": PORTAL[0], "z": PORTAL[1], "radius": 3.0, "targetMap": tTarget,
                                     "targetX": FAR[0], "targetZ": FAR[1], "targetYaw": 0.0}]
            else:
                tMeta["portals"] = []
                tMeta["npcs"] = []
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


def find_npcs(pClient):
    C["weapon"] = poll(lambda: pClient.find_npc_id("Weaponsmith"), 4)
    C["armor"] = poll(lambda: pClient.find_npc_id("Armorsmith"), 4)


def setup():
    srv = G11Server()
    srv.start()
    C["srv"] = srv
    c = Client()
    r = c.login(base.GM_ACCOUNT)
    if not (r is not None and r.login_response.token):
        r = c.register(base.GM_ACCOUNT)
        check(r is not None and r.register_response.register_status in (0, 2), f"gm register failed {r}")
        c.close()
        srv.stop()
        srv.start()
        c = Client()
        r = c.login(base.GM_ACCOUNT)
        check(r is not None and r.login_response.token, f"gm login failed {r}")
    r = c.char_list()
    tChars = list(r.character_list_response.characters)
    if tChars:
        c.cid, c.name = tChars[0].character_id, tChars[0].name
    else:
        r = c.create_char(base.GM_CHAR, 1, 1)
        check(r is not None and r.character_create_response.status == 0, f"gm char create failed {r}")
        c.cid, c.name = r.character_create_response.character.character_id, base.GM_CHAR
    c.user = base.GM_ACCOUNT
    enter_world(c)
    C["G"] = c
    C.setdefault("all", []).append(c)
    time.sleep(1.5)
    gm("/help")
    find_npcs(c)
    if not C["weapon"]:
        gm("/tp -29 25")
        time.sleep(1.0)
        C["weapon"] = poll(lambda: c.find_npc_id("Weaponsmith"), 6)
    if not C["armor"]:
        gm("/tp 29 25")
        time.sleep(1.0)
        C["armor"] = poll(lambda: c.find_npc_id("Armorsmith"), 6)
    check(C["weapon"], "Weaponsmith not found in snapshots")
    check(C["armor"], "Armorsmith not found in snapshots")


base.setup = setup


def gold(pClient):
    s = stats_of(pClient)
    return s.gold if s is not None else None


def count_of(pClient, pTpl, pLocation=0):
    return sum(i.count for i in items_of(pClient) if i.template_id == pTpl and i.location == pLocation)


def tp(pPos):
    gm(f"/tp {int(pPos[0])} {int(pPos[1])}")
    time.sleep(0.6)


def set_gold(pAmount):
    G = C["G"]
    time.sleep(0.4)
    check(poll(lambda: gold(G) is not None, 4), "no stats")
    tNow = gold(G)
    if tNow != pAmount:
        gm(f"/gold {pAmount - tNow}")
        check(poll(lambda: gold(G) == pAmount, 4), f"gold {gold(G)}, expected {pAmount}")


def paced(pFn, *pArgs):
    time.sleep(0.3)
    return pFn(*pArgs)


def buy(pNpc, pTpl, pQty):
    r = paced(C["G"].buy_item, pNpc, pTpl, pQty)
    return r.status if r is not None else None


def sell(pNpc, pSlot, pTpl, pCount=0):
    r = paced(C["G"].sell_item, pNpc, pSlot, pTpl, pCount)
    return (r.status, r.gold_gained) if r is not None else (None, None)


def settle():
    time.sleep(0.7)


def sell_all(pTpl):
    G = C["G"]
    for _ in range(200):
        tSlot = bag_slot(G, pTpl)
        if tSlot is None:
            return
        s, _ = sell(C["weapon"], tSlot, pTpl, 0)
        check(s in (OK, NOT_SELLABLE), f"cleanup sell of {pTpl} gave {name(s)}")
        if s != OK:
            return
        time.sleep(0.4)


def start_clean():
    G = C["G"]
    tHp = vitals_now(G)[0]
    if tHp is not None and tHp <= 0:
        gm(f"/revive {G.name}")
        time.sleep(1.0)
    tp(NEAR_WEAPON)
    sell_all(POTION)
    sell_all(MANA)
    settle()


def t_open_range_from_spawn():
    c = world_player()
    time.sleep(1.0)
    check(gold(c) == 100, f"fresh character gold {gold(c)}, expected 100 (starter)")
    r = c.open_shop(C["weapon"])
    check(r is not None and r.status == OUT_OF_RANGE, f"open from spawn: {name(r.status if r else None)}")
    time.sleep(0.3)
    r = c.open_shop(C["armor"])
    check(r is not None and r.status == OUT_OF_RANGE, f"open armorsmith from spawn: {name(r.status if r else None)}")


def t_open_near_and_stock():
    G = C["G"]
    tp(NEAR_WEAPON)
    r = paced(G.open_shop, C["weapon"])
    check(r is not None and r.status == OK, f"open weaponsmith: {name(r.status if r else None)}")
    check(list(r.template_ids) == STOCK["Weaponsmith"], f"stock {list(r.template_ids)} != {STOCK['Weaponsmith']}")
    check(r.npc_id == C["weapon"], "npc_id echo mismatch")
    r = paced(G.open_shop, C["armor"])
    check(r is not None and r.status == OUT_OF_RANGE, f"armorsmith from weapon side: {name(r.status if r else None)}")
    tp(NEAR_ARMOR)
    r = paced(G.open_shop, C["armor"])
    check(r is not None and r.status == OK, f"open armorsmith: {name(r.status if r else None)}")
    check(list(r.template_ids) == STOCK["Armorsmith"], f"stock {list(r.template_ids)} != {STOCK['Armorsmith']}")
    r = paced(G.open_shop, C["weapon"])
    check(r is not None and r.status == OUT_OF_RANGE, f"weaponsmith from armor side: {name(r.status if r else None)}")


def t_buy_and_validation():
    G = C["G"]
    start_clean()
    set_gold(500)
    tNpc = C["weapon"]
    g0, p0 = gold(G), count_of(G, POTION)
    check(buy(tNpc, POTION, 1) == OK, "buy potion x1 refused")
    check(poll(lambda: gold(G) == g0 - price(POTION), 4), f"gold {gold(G)}, expected {g0 - price(POTION)}")
    check(poll(lambda: count_of(G, POTION) == p0 + 1, 4), "bag potion count not +1")
    check(buy(tNpc, POTION, 3) == OK, "buy potion x3 refused")
    check(poll(lambda: count_of(G, POTION) == p0 + 4, 4), f"potions {count_of(G, POTION)}, expected {p0 + 4}")
    check(poll(lambda: gold(G) == g0 - 4 * price(POTION), 4), f"gold after x3 {gold(G)}")
    time.sleep(0.5)
    tStacks = [i for i in items_of(G) if i.template_id == POTION and i.location == 0]
    check(len(tStacks) == 1, f"potions did not share one stack: {[(i.slot, i.count) for i in tStacks]}")
    g1, p1 = gold(G), count_of(G, POTION)
    for tQty in (0, 21, 2 ** 32 - 1):
        s = buy(tNpc, POTION, tQty)
        check(s == INVALID, f"qty {tQty}: {name(s)}, expected INVALID")
    s = buy(tNpc, ARMOR, 1)
    check(s == NOT_SOLD_HERE, f"2001 at weaponsmith: {name(s)}, expected NOT_SOLD_HERE")
    s = buy(tNpc, 999999, 1)
    check(s == INVALID, f"unknown template: {name(s)}")
    for tBad in (0, 1, G.cid, 987654321, 2 ** 63):
        s = buy(tBad, POTION, 1)
        check(s == INVALID, f"npc_id {tBad}: {name(s)}, expected INVALID")
    settle()
    check(gold(G) == g1 and count_of(G, POTION) == p1, "rejected buys changed gold or bag")


def t_not_enough_gold():
    G = C["G"]
    start_clean()
    set_gold(0)
    p0 = count_of(G, POTION)
    s = buy(C["weapon"], POTION, 1)
    check(s == NOT_ENOUGH_GOLD, f"{name(s)}, expected NOT_ENOUGH_GOLD")
    set_gold(price(POTION) - 1)
    s = buy(C["weapon"], POTION, 1)
    check(s == NOT_ENOUGH_GOLD, f"one gold short: {name(s)}")
    settle()
    check(gold(G) == price(POTION) - 1 and count_of(G, POTION) == p0, "failed buy changed gold or bag")
    set_gold(price(POTION))
    check(buy(C["weapon"], POTION, 1) == OK, "exact gold buy refused")
    check(poll(lambda: gold(G) == 0, 4), f"gold after exact buy {gold(G)}")


def t_bag_full_atomic():
    G = C["G"]
    start_clean()
    sell_all(SWORD)
    for _ in range(40):
        before = count_of(G, SWORD)
        time.sleep(0.4)
        G.call(lambda q: setattr(q.command_request, "text", "/item 1001 5"), 6.0)
        time.sleep(0.5)
        if count_of(G, SWORD) - before < 5:
            break
    tSwords = count_of(G, SWORD)
    print(f"      bag filled with {tSwords} swords", flush=True)
    try:
        check(tSwords >= 10, f"could not fill the bag ({tSwords} swords)")
        set_gold(500)
        g0 = gold(G)
        s = buy(C["weapon"], SWORD, 1)
        if s == OK:
            s = buy(C["weapon"], SWORD, 1)
        check(s == BAG_FULL, f"buy into full bag: {name(s)}, expected BAG_FULL")
        settle()
        tBought = count_of(G, SWORD) - tSwords
        check(gold(G) == g0 - tBought * price(SWORD), f"gold {gold(G)} inconsistent with {tBought} swords bought from {g0}")
        s = buy(C["weapon"], POTION, 20)
        if s != OK:
            check(s == BAG_FULL, f"potions into full bag: {name(s)}")
    finally:
        sell_all(SWORD)
        sell_all(POTION)
    check(count_of(G, SWORD) == 0, "could not clear swords")


def t_sell_rules():
    G = C["G"]
    start_clean()
    set_gold(500)
    time.sleep(0.4)
    gm(f"/item {SWORD} 1")
    check(poll(lambda: bag_slot(G, SWORD) is not None, 4), "sword not delivered")
    tSlot = bag_slot(G, SWORD)
    g0 = gold(G)
    s, got = sell(C["weapon"], tSlot, SWORD, 0)
    check(s == OK, f"sell sword: {name(s)}")
    check(got == sell_price(SWORD), f"gold_gained {got}, expected {sell_price(SWORD)} (price {price(SWORD)} x {RATIO})")
    check(poll(lambda: gold(G) == g0 + got, 4), f"gold {gold(G)}, expected {g0 + got}")
    check(poll(lambda: bag_slot(G, SWORD) is None, 4), "sword still in bag")
    check(buy(C["weapon"], POTION, 5) == OK, "buy 5 potions")
    check(poll(lambda: count_of(G, POTION) == 5, 4), "potions missing")
    tSlot = bag_slot(G, POTION)
    g1 = gold(G)
    s, got = sell(C["weapon"], tSlot, POTION, 0)
    check(s == OK and got == sell_price(POTION) * 5, f"stack sell: {name(s)} gained {got}, expected {sell_price(POTION) * 5}")
    check(poll(lambda: count_of(G, POTION) == 0 and gold(G) == g1 + got, 4), "stack not fully sold")
    time.sleep(0.3)
    gm(f"/item {POTION} 6")
    check(poll(lambda: count_of(G, POTION) == 6, 4), "potions not delivered")
    tSlot = bag_slot(G, POTION)
    s, got = sell(C["weapon"], tSlot, POTION, 2)
    check(s == OK and got == sell_price(POTION) * 2, f"partial sell: {name(s)} {got}")
    check(poll(lambda: count_of(G, POTION) == 4, 4), f"potions left {count_of(G, POTION)}, expected 4")
    s, got = sell(C["weapon"], tSlot, POTION, 21)
    check(s == INVALID, f"count > maxStack: {name(s)}, expected INVALID")
    s, got = sell(C["weapon"], tSlot, MANA, 1)
    check(s == INVALID, f"wrong template: {name(s)}")
    tEmpty = next(x for x in range(119, 0, -1) if all(i.slot != x for i in items_of(G)))
    s, got = sell(C["weapon"], tEmpty, POTION, 1)
    check(s == INVALID, f"empty slot: {name(s)}")
    settle()
    check(count_of(G, POTION) == 4, f"rejected sells changed potions: {count_of(G, POTION)}")
    sell_all(POTION)
    time.sleep(0.4)
    gm(f"/item {RELIC} 1")
    check(poll(lambda: bag_slot(G, RELIC) is not None, 4), "relic not delivered")
    g2 = gold(G)
    s, got = sell(C["weapon"], bag_slot(G, RELIC), RELIC, 0)
    check(s == NOT_SELLABLE, f"quest item: {name(s)}, expected NOT_SELLABLE")
    settle()
    check(gold(G) == g2 and count_of(G, RELIC) >= 1, "NOT_SELLABLE sell changed state")


def t_equipped_not_sellable():
    G = C["G"]
    start_clean()
    time.sleep(0.4)
    gm(f"/item {SWORD} 1")
    check(poll(lambda: bag_slot(G, SWORD) is not None, 4), "sword not delivered")
    check(equip(G, bag_slot(G, SWORD)) == 0, "equip failed")
    check(poll(lambda: count_of(G, SWORD, 1) >= 1, 4), "sword not equipped")
    tEq = next(i for i in items_of(G) if i.template_id == SWORD and i.location != 0)
    g0 = gold(G)
    s, got = sell(C["weapon"], tEq.slot, SWORD, 0)
    check(s == INVALID, f"selling equipped slot {tEq.slot}: {name(s)}, expected INVALID")
    settle()
    check(count_of(G, SWORD, 1) >= 1 and gold(G) == g0, "equipment or gold changed")


def t_out_of_range_after_tp():
    G = C["G"]
    start_clean()
    set_gold(500)
    time.sleep(0.4)
    gm(f"/item {POTION} 2")
    check(poll(lambda: count_of(G, POTION) == 2, 4), "potions not delivered")
    tSlot = bag_slot(G, POTION)
    tp(FAR)
    g0 = gold(G)
    s = buy(C["weapon"], POTION, 1)
    check(s == OUT_OF_RANGE, f"buy far away: {name(s)}")
    s, got = sell(C["weapon"], tSlot, POTION, 1)
    check(s == OUT_OF_RANGE, f"sell far away: {name(s)}")
    settle()
    check(gold(G) == g0 and count_of(G, POTION) == 2, "far requests changed state")
    tp(NEAR_WEAPON)
    sell_all(POTION)


def t_burst_rate_limit():
    G = C["G"]
    start_clean()
    set_gold(1000)
    g0, p0 = gold(G), count_of(G, POTION)
    time.sleep(0.5)
    tMark = G.mark()

    def fill(q):
        q.buy_item_request.npc_id = C["weapon"]
        q.buy_item_request.item_id = POTION
        q.buy_item_request.quantity = 1
    ids = [G.send(fill) for _ in range(20)]
    res = []
    for i in ids:
        r = G.wait(lambda r, i=i: r.header.reply_to == i, 6.0, tMark)
        res.append(r.buy_item_response.status if r is not None and r.HasField("buy_item_response") else None)
    tOk = res.count(OK)
    print(f"      burst of 20: ok={tOk} too_fast={res.count(TOO_FAST)} other={[name(x) for x in res if x not in (OK, TOO_FAST)]}", flush=True)
    check(res.count(TOO_FAST) > 0, f"no TOO_FAST in a 20 burst: {[name(x) for x in res]}")
    check(all(x in (OK, TOO_FAST) for x in res), f"unexpected statuses {[name(x) for x in res]}")
    check(tOk >= 1, "no buy succeeded")
    check(poll(lambda: gold(G) == g0 - tOk * price(POTION), 5), f"gold diff {g0 - gold(G)}, expected {tOk * price(POTION)}")
    check(poll(lambda: count_of(G, POTION) == p0 + tOk, 5), f"potions {count_of(G, POTION)}, expected {p0 + tOk}")
    sell_all(POTION)


def t_dead_cannot_trade():
    G = C["G"]
    start_clean()
    set_gold(500)
    time.sleep(0.4)
    gm(f"/item {POTION} 1")
    check(poll(lambda: count_of(G, POTION) == 1, 4), "potion not delivered")
    tSlot = bag_slot(G, POTION)
    gm(f"/kill {G.name}")
    try:
        check(poll(lambda: vitals_now(G)[0] is not None and vitals_now(G)[0] <= 0, 4), "player did not die")
        g0 = gold(G)
        r = paced(G.open_shop, C["weapon"])
        check(r is not None and r.status == DEAD, f"open while dead: {name(r.status if r else None)}")
        s = buy(C["weapon"], POTION, 1)
        check(s == DEAD, f"buy while dead: {name(s)}")
        s, got = sell(C["weapon"], tSlot, POTION, 1)
        check(s == DEAD, f"sell while dead: {name(s)}")
        check(gold(G) == g0 and count_of(G, POTION) == 1, "dead trade changed state")
    finally:
        gm(f"/revive {G.name}")
        time.sleep(1.0)


def t_persist_relog_and_portal():
    G = C["G"]
    start_clean()
    set_gold(500)
    check(buy(C["weapon"], POTION, 3) == OK, "buy refused")
    check(poll(lambda: count_of(G, POTION) == 3, 4), "potions missing")
    g0, p0 = gold(G), count_of(G, POTION)
    time.sleep(1.0)
    tCid, tName = G.cid, G.name
    G.close()
    time.sleep(3)
    d = Client()
    C["all"].append(d)
    r = d.login(base.GM_ACCOUNT)
    check(r is not None and r.login_response.token, "relog login failed")
    r = d.enter(tCid)
    check(r is not None and r.enter_world_response.status == 0, f"relog enter failed {r}")
    d.cid, d.name, d.user = tCid, tName, base.GM_ACCOUNT
    C["G"] = d
    check(poll(lambda: stats_of(d) is not None, 6), "no InventoryUpdate after relog")
    time.sleep(1.0)
    check(gold(d) == g0, f"gold after relog {gold(d)}, expected {g0}")
    check(count_of(d, POTION) == p0, f"potions after relog {count_of(d, POTION)}, expected {p0}")
    tNpc = poll(lambda: d.find_npc_id("Weaponsmith"), 5)
    if tNpc:
        C["weapon"] = tNpc
    tp(NEAR_WEAPON)
    r = paced(d.open_shop, C["weapon"])
    check(r is not None and r.status == OK, f"open after relog: {name(r.status if r else None)}")
    sell_all(POTION)
    tp(PORTAL)
    time.sleep(2.0)
    r = paced(d.open_shop, C["weapon"])
    check(r is not None and r.status == INVALID, f"stale npc_id after portal: {name(r.status if r else None)}, expected INVALID")
    r = paced(d.buy_item, C["weapon"], POTION, 1)
    check(r is not None and r.status == INVALID, f"stale buy: {name(r.status if r else None)}")
    gm("/tp 0 24")


def t_server_survived():
    srv = C["srv"]
    check(srv.proc.poll() is None, "server process exited\n" + "\n".join(srv.log_lines[-15:]))
    check(not any("Assertion" in l or "assert" in l.lower() or "terminate" in l.lower() or "exception" in l.lower() for l in srv.log_lines),
          "exception/assert line in server log")


TESTS = [t_open_range_from_spawn, t_open_near_and_stock, t_buy_and_validation, t_not_enough_gold, t_bag_full_atomic,
         t_sell_rules, t_equipped_not_sellable, t_out_of_range_after_tp, t_burst_rate_limit, t_dead_cannot_trade,
         t_persist_relog_and_portal, t_server_survived]

if __name__ == "__main__":
    run_tests(TESTS)
