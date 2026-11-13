import sys, os, time, math, traceback
from lakotnet import *
from t_monsters_g21 import tpl, open_site, View, step_toward, run_scenario, ORIGIN, KIND_GROUND, ITEM_POTION

generate_bindings()

MON = 901
MAX_HP = 1000
THRESHOLD = MAX_HP * 0.05
XP = 1000
PICK_OK, PICK_NOT_FOUND, PICK_NOT_OWNER = 0, 1, 2


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


def total_xp(pClient):
    with pClient.cv:
        tLog = list(pClient.log)
    for _, m in reversed(tLog):
        r = m.response
        if r.HasField("inventory_update"):
            s = r.inventory_update.stats
            return sum(l * l * 100 for l in range(1, s.level)) + s.experience
        if r.HasField("world_snapshot") and r.world_snapshot.HasField("self_vitals"):
            s = r.world_snapshot.self_vitals
            return sum(l * l * 100 for l in range(1, s.level)) + s.experience
    return None


def dealt(pView, pMon, pFrom):
    tSum = 0
    for _, ev, _ in pView.events[pFrom:]:
        if ev.attacker_id == pView.c.cid:
            for h in ev.hits:
                if h.target_id == pMon and h.damage > 0:
                    tSum += h.damage
    return tSum


def approach(pClient, pView, pMon):
    tEnd = time.time() + 30
    while time.time() < tEnd:
        pView.ingest()
        m = pView.mons.get(pMon)
        if m is None:
            time.sleep(0.5)
            continue
        x, _, z = pClient.pos
        d = math.hypot(m["x"] - x, m["z"] - z)
        if d <= 2.0:
            return
        step_toward(pClient, m["x"] - (m["x"] - x) / d * 1.3, m["z"] - (m["z"] - z) / d * 1.3, min(3.0, d - 1.3))
        time.sleep(0.7)
    raise AssertionError("could not reach the monster")


def hit_until(pClient, pView, pMon, pDamage, pStopOnKill=False, pTimeout=150):
    tFrom = len(pView.events)
    tEnd = time.time() + pTimeout
    while time.time() < tEnd:
        pView.ingest()
        if pMon in pView.dead and pStopOnKill:
            break
        if dealt(pView, pMon, tFrom) >= pDamage and not pStopOnKill:
            break
        m = pView.mons.get(pMon)
        x, _, z = pClient.pos
        pClient.attack(math.atan2(m["x"] - x, m["z"] - z))
        time.sleep(0.7)
    pView.ingest()
    return dealt(pView, pMon, tFrom)


def find_mon(pView, pKnown):
    tEnd = time.time() + 15
    while time.time() < tEnd:
        pView.ingest()
        for i, d in pView.of(MON).items():
            if d["alive"] and i not in pKnown:
                return i
        time.sleep(0.4)
    return None


def round_(pLabel, pMajor, pMinor, pTiny, pViews, pNames):
    tAll = list(pViews)
    tMon = None
    for v in tAll:
        v.ingest()
    tAlive = [i for i, d in tAll[0].of(MON).items() if d["alive"]]
    check(tAlive, f"{pLabel}: monster not alive in view")
    tMon = tAlive[0]
    for v in tAll:
        approach(v.c, v, tMon)
    tBase = {v.c.cid: total_xp(v.c) for v in tAll}
    tGroundBefore = {e.entity_id for v in tAll for e in v.spawns if e.kind == KIND_GROUND}
    tDeadBefore = tMon in tAll[0].dead
    if pTiny is not None:
        tDmg = hit_until(pTiny.c, pTiny, tMon, 1)
        print(f"      {pLabel}: tiny hitter dealt {tDmg} (threshold {THRESHOLD})", flush=True)
        check(0 < tDmg < THRESHOLD, f"{pLabel}: single hit {tDmg} not below the 5% threshold {THRESHOLD}; scenario inconclusive")
    tMinorDmg = hit_until(pMinor.c, pMinor, tMon, THRESHOLD * 2.0)
    check(tMinorDmg >= THRESHOLD, f"{pLabel}: minor player dealt only {tMinorDmg}")
    hit_until(pMajor.c, pMajor, tMon, 0, pStopOnKill=True)
    tEnd = time.time() + 10
    while time.time() < tEnd and tMon not in pMajor.dead:
        pMajor.ingest()
        time.sleep(0.3)
    check(tMon in pMajor.dead, f"{pLabel}: monster was not killed")
    time.sleep(3)
    for v in tAll:
        v.ingest()
    tMajorDmg = sum(h.damage for _, ev, _ in pMajor.events if ev.attacker_id == pMajor.c.cid for h in ev.hits if h.target_id == tMon and h.damage > 0)
    tGain = {v.c.cid: total_xp(v.c) - tBase[v.c.cid] for v in tAll}
    gM, gm_ = tGain[pMajor.c.cid], tGain[pMinor.c.cid]
    print(f"      {pLabel}: gains major={gM} minor={gm_} tiny={tGain[pTiny.c.cid] if pTiny else None} "
          f"(major dealt {tMajorDmg} total, minor {tMinorDmg})", flush=True)
    check(gM > 0 and gm_ > 0, f"{pLabel}: both eligible players must gain experience: {tGain}")
    check(gM >= 3 * gm_, f"{pLabel}: major hitter gain {gM} not clearly above minor {gm_}")
    check(950 <= gM + gm_ <= 1050, f"{pLabel}: eligible gains sum {gM + gm_} outside 950..1050")
    if pTiny is not None:
        check(tGain[pTiny.c.cid] == 0, f"{pLabel}: sub-5% player gained {tGain[pTiny.c.cid]} experience")
    tNew = {e.entity_id: e for v in tAll for e in v.spawns if e.kind == KIND_GROUND and e.entity_id not in tGroundBefore}
    check(len(tNew) == 1, f"{pLabel}: expected exactly one loot drop, saw {len(tNew)}: {sorted(tNew)}")
    tLoot = next(iter(tNew))
    tResults = {}
    for v in tAll:
        tResults[v.c.cid] = v.c.pickup(tLoot)
    tOk = [cid for cid, r in tResults.items() if r == PICK_OK]
    print(f"      {pLabel}: pickup results {tResults}", flush=True)
    check(len(tOk) == 1, f"{pLabel}: exactly one player must pick the loot up, results {tResults}")
    if pTiny is not None:
        check(tOk[0] != pTiny.c.cid, f"{pLabel}: loot went to the sub-5% player")
    return tOk[0]


def t_kill_share():
    tSite = open_site(ORIGIN, 14)
    tTemplates = [tpl(MON, maxHealth=MAX_HP, attack=1, experience=XP, respawnSeconds=3, moveSpeed=0.0,
                      drops=[{"itemId": ITEM_POTION, "chance": 0.0, "min": 1, "max": 1, "guaranteed": True}])]
    tSpawns = [{"monster": MON, "x": tSite[0], "z": tSite[1], "radius": 0.5, "count": 1}]

    def body(c, v, srv):
        b = make_player(1, "b")
        d = make_player(1, "c")
        tClients = [c, b, d]
        try:
            enter_world(b)
            enter_world(d)
            vb, vd = View(b), View(d)
            time.sleep(3)
            tViews = [v, vb, vd]
            tOwners = [round_("round1", v, vb, vd, tViews, "A,B,C")]
            tMon = next(iter(v.of(MON)))
            time.sleep(5)
            for x in tViews:
                x.ingest()
            for x in tViews:
                x.dead.discard(tMon)
            tAlive = poll_alive(v, tMon)
            check(tAlive, "monster did not respawn within 5 s in view")
            tOwners.append(round_("round2", vb, v, vd, tViews, "B,A,C"))
            print(f"      loot owners: {tOwners}", flush=True)
        finally:
            for x in (b, d):
                x.close()

    run_scenario(tTemplates, tSpawns, body)


def poll_alive(pView, pMon):
    tEnd = time.time() + 6
    while time.time() < tEnd:
        pView.ingest()
        if any(d["alive"] for d in pView.of(MON).values()):
            return True
        time.sleep(0.3)
    return False


if __name__ == "__main__":
    t0 = time.time()
    try:
        t_kill_share()
        print(f"PASS  t_kill_share  ({time.time() - t0:.1f}s)\n\n1/1 passed", flush=True)
        sys.exit(0)
    except Exception as e:
        traceback.print_exc()
        print(f"FAIL  t_kill_share  {e}\n\n0/1 passed", flush=True)
        sys.exit(1)
