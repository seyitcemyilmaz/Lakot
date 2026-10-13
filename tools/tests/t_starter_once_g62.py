import sys, time
from lakotnet import *
import t_regen_g25 as base
from t_regen_g25 import C, check, gm, world_player, poll, stats_of, items_of, equip, bag_slot, View, walk_to, run_tests

SWORD, ARMOR, POTION = 1001, 2001, 4
USE_OK = 0
STATE = {}


def use_potion(pClient):
    r = pClient.call(lambda q: setattr(q.use_item_request, "template_id", POTION), 6.0)
    check(r is not None and r.HasField("use_item_response"), "no UseItemResponse")
    return r.use_item_response.status


def totals(pClient):
    tBag, tEquipped = {}, {}
    for it in items_of(pClient):
        tTarget = tBag if it.location == 0 else tEquipped
        tTarget[it.template_id] = tTarget.get(it.template_id, 0) + it.count
    return tBag, tEquipped


def settled(pClient, pPred, pTimeout=6.0):
    return poll(lambda: pPred(*totals(pClient)), pTimeout)


def snapshot_state(pClient):
    tBag, tEq = totals(pClient)
    s = stats_of(pClient)
    return {"sword": tBag.get(SWORD, 0) + tEq.get(SWORD, 0), "armor": tBag.get(ARMOR, 0) + tEq.get(ARMOR, 0),
            "potion": tBag.get(POTION, 0) + tEq.get(POTION, 0), "gold": None if s is None else s.gold}


def relog(pClient, pLabel, pHold=3):
    tUser, tCid, tName = pClient.user, pClient.cid, pClient.name
    pClient.close()
    time.sleep(pHold)
    d = Client()
    C["all"].append(d)
    r = d.login(tUser)
    check(r is not None and r.login_response.token, f"{pLabel}: login failed")
    r = d.enter(tCid)
    check(r is not None and r.enter_world_response.status == 0, f"{pLabel}: enter failed {r}")
    d.user, d.cid, d.name = tUser, tCid, tName
    d.pos = (r.enter_world_response.pos_x, r.enter_world_response.pos_y, r.enter_world_response.pos_z)
    check(poll(lambda: stats_of(d) is not None, 6), f"{pLabel}: no InventoryUpdate")
    time.sleep(2.5)
    return d


def t_new_character_gets_kit_once():
    c = world_player(1)
    check(settled(c, lambda b, e: b.get(POTION, 0) == 5 and b.get(SWORD, 0) == 1 and b.get(ARMOR, 0) == 1), f"kit not received: {totals(c)}")
    time.sleep(1.0)
    tFirst = snapshot_state(c)
    print(f"      kit state {tFirst}", flush=True)
    check(tFirst["gold"] == 100, f"gold {tFirst['gold']}, expected 100 (starting gold is 0 on a new character)")
    check(tFirst["potion"] == 5 and tFirst["sword"] == 1 and tFirst["armor"] == 1, f"kit mismatch {tFirst}")
    for _ in range(5):
        gm(f"/sethp 1 {c.name}")
        check(poll(lambda: base.vitals_now(c)[0] is not None and base.vitals_now(c)[0] <= 3, 4), "could not wound the player")
        r = use_potion(c)
        check(r == USE_OK, f"drink status {r}")
        time.sleep(1.2)
    check(settled(c, lambda b, e: b.get(POTION, 0) == 0), f"potions not all drunk: {totals(c)}")
    tSlot = bag_slot(c, SWORD)
    check(tSlot is not None, "sword not in bag")
    check(equip(c, tSlot) == 0, "equip sword failed")
    tSlot = bag_slot(c, ARMOR)
    check(tSlot is not None, "armor not in bag")
    check(equip(c, tSlot) == 0, "equip armor failed")
    check(settled(c, lambda b, e: b.get(SWORD, 0) == 0 and b.get(ARMOR, 0) == 0 and e.get(SWORD, 0) == 1 and e.get(ARMOR, 0) == 1),
          f"equip not reflected: {totals(c)}")
    STATE["after_use"] = snapshot_state(c)
    STATE["client"] = c
    check(STATE["after_use"]["potion"] == 0 and STATE["after_use"]["gold"] == 100, f"unexpected state {STATE['after_use']}")


def _assert_no_second_kit(pClient, pLabel):
    tNow = snapshot_state(pClient)
    print(f"      {pLabel}: {tNow}", flush=True)
    check(tNow["potion"] == 0, f"{pLabel}: potions given again ({tNow['potion']})")
    check(tNow["sword"] == 1, f"{pLabel}: sword count {tNow['sword']}, expected 1")
    check(tNow["armor"] == 1, f"{pLabel}: armor count {tNow['armor']}, expected 1")
    check(tNow["gold"] == 100, f"{pLabel}: gold {tNow['gold']}, expected 100 (second +100 or loss)")


def t_quick_relog_no_second_kit():
    check("client" in STATE, "depends on t_new_character_gets_kit_once")
    d = relog(STATE["client"], "quick relog", 3)
    STATE["client"] = d
    _assert_no_second_kit(d, "quick relog")


def t_portal_no_second_kit():
    check("client" in STATE, "depends on t_new_character_gets_kit_once")
    d = STATE["client"]
    v = View(d)
    walk_to(d, v, C["portal"][0], C["portal"][1], 1.0)
    r = poll(lambda: (lambda x: x if x and x.HasField("command_response") else None)(
        d.call(lambda q: setattr(q.command_request, "text", "/pos"), 4.0)), 4)
    tText = " ".join(r.command_response.lines) if r else ""
    check("Map 102" in tText, f"portal did not move the player to map 102: [{tText}]")
    time.sleep(3.0)
    _assert_no_second_kit(d, "after portal")


def t_relog_after_hold_expiry_no_second_kit():
    check("client" in STATE, "depends on t_new_character_gets_kit_once")
    d = relog(STATE["client"], "relog after hold expiry", 50)
    STATE["client"] = d
    _assert_no_second_kit(d, "relog after hold expiry")


TESTS = [t_new_character_gets_kit_once, t_quick_relog_no_second_kit, t_portal_no_second_kit,
         t_relog_after_hold_expiry_no_second_kit, base.t_server_survived]

if __name__ == "__main__":
    run_tests(TESTS)
