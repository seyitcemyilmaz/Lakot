import sys, os, time, math, traceback
from lakotnet import *
from t_monsters_g21 import tpl, open_site, View, walk_to, run_scenario, ORIGIN
from t_kill_share_o10 import approach, hit_until

generate_bindings()

MON = 902


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


def find_mon(pView):
    tEnd = time.time() + 15
    while time.time() < tEnd:
        pView.ingest()
        for i, d in pView.of(MON).items():
            if d["alive"]:
                return i
        time.sleep(0.4)
    raise AssertionError("monster not in view")


def hits_on(pView, pMon, pVictim, pFrom):
    pView.ingest()
    return sum(1 for _, ev, _ in pView.events[pFrom:] if ev.attacker_id == pMon for h in ev.hits if h.target_id == pVictim)


def health_percent(pClient, pMon):
    with pClient.cv:
        tLog = list(pClient.log)
    tPercent = None
    for _, m in tLog:
        if m.response.HasField("world_snapshot"):
            s = m.response.world_snapshot
            for e in s.entered:
                if e.entity_id == pMon:
                    tPercent = e.health_percent
            for v in s.vitals:
                if v.entity_id == pMon:
                    tPercent = v.health_percent
    return tPercent


def killed_victim(pView, pMon, pVictim, pFrom):
    pView.ingest()
    return any(ev.attacker_id == pMon and h.target_id == pVictim and h.killed for _, ev, _ in pView.events[pFrom:] for h in ev.hits)


def t_retarget_second_attacker():
    tSite = open_site(ORIGIN, 14)
    tTemplates = [tpl(MON, maxHealth=100000, attack=40, moveSpeed=3.0, leashRange=150, aggressive=False)]
    tSpawns = [{"monster": MON, "x": tSite[0], "z": tSite[1], "radius": 0.5, "count": 1}]

    def body(c, v, srv):
        a = make_player(1, "a")
        b = make_player(1, "b")
        try:
            enter_world(a)
            enter_world(b)
            va, vb = View(a), View(b)
            time.sleep(3)
            tMon = find_mon(vb)
            find_mon(va)
            approach(a, va, tMon)
            approach(b, vb, tMon)
            tFrom = len(vb.events)
            tEnd = time.time() + 10
            while time.time() < tEnd:
                for p, pv in ((a, va), (b, vb)):
                    x, _, z = p.pos
                    m = pv.mons[tMon]
                    p.attack(math.atan2(m["x"] - x, m["z"] - z))
                time.sleep(0.7)
                va.ingest()
                vb.ingest()
                if hits_on(vb, tMon, a.cid, tFrom) > 0 or any(ev.attacker_id == a.cid for _, ev, _ in va.events):
                    break
            check(any(ev.attacker_id == a.cid and h.damage > 0 for _, ev, _ in va.events for h in ev.hits), "first attacker dealt no damage")
            check(any(ev.attacker_id == b.cid and h.damage > 0 for _, ev, _ in vb.events for h in ev.hits), "second attacker dealt no damage")
            tVictim = tSurvivor = None
            tEnd = time.time() + 15
            while time.time() < tEnd and tVictim is None:
                for p, o in ((a, b), (b, a)):
                    if killed_victim(vb, tMon, p.cid, tFrom):
                        tVictim, tSurvivor = p, o
                time.sleep(0.5)
            check(tVictim is not None, "monster did not kill its first target; scenario inconclusive")
            tMark = len(vb.events)
            tEnd = time.time() + 15
            while time.time() < tEnd and not hits_on(vb, tMon, tSurvivor.cid, tMark):
                time.sleep(0.5)
            tHits = hits_on(vb, tMon, tSurvivor.cid, tMark)
            print(f"      monster hit the second attacker {tHits} times after its first target died", flush=True)
            check(tHits > 0, "monster did not retarget the remaining attacker after its first target died")
        finally:
            for x in (a, b):
                x.close()

    run_scenario(tTemplates, tSpawns, body)


def t_retarget_lone_attacker_returns():
    tSite = open_site(ORIGIN, 14)
    tTemplates = [tpl(MON, maxHealth=3000, attack=1, moveSpeed=3.0, leashRange=8, aggressive=False)]
    tSpawns = [{"monster": MON, "x": tSite[0], "z": tSite[1], "radius": 0.5, "count": 1}]

    def body(c, v, srv):
        time.sleep(3)
        tMon = find_mon(v)
        approach(c, v, tMon)
        tDealt = hit_until(c, v, tMon, 60)
        check(tDealt >= 60, f"attacker dealt only {tDealt}")
        time.sleep(1.5)
        v.ingest()
        tHurt = health_percent(c, tMon)
        check(tHurt is not None and tHurt < 100, f"monster health percent after hits is {tHurt}")
        walk_to(c, v, ORIGIN[0], ORIGIN[1])
        tEnd = time.time() + 40
        tHome = False
        while time.time() < tEnd:
            v.ingest()
            m = v.mons[tMon]
            if math.hypot(m["x"] - tSite[0], m["z"] - tSite[1]) <= 1.5 and health_percent(c, tMon) == 100:
                tHome = True
                break
            time.sleep(0.5)
        m = v.mons[tMon]
        print(f"      monster at {math.hypot(m['x'] - tSite[0], m['z'] - tSite[1]):.1f} m from spawn, health {health_percent(c, tMon)}%", flush=True)
        check(tHome, "monster did not return to its spawn with full health after the lone attacker fled")

    run_scenario(tTemplates, tSpawns, body)


TESTS = [t_retarget_second_attacker, t_retarget_lone_attacker_returns]


def main():
    tPassed = 0
    for t in TESTS:
        t0 = time.time()
        try:
            t()
            tPassed += 1
            print(f"PASS  {t.__name__}  ({time.time() - t0:.1f}s)", flush=True)
        except Exception as e:
            traceback.print_exc()
            print(f"FAIL  {t.__name__}  {e}", flush=True)
    print(f"\n{tPassed}/{len(TESTS)} passed", flush=True)
    sys.exit(0 if tPassed == len(TESTS) else 1)


if __name__ == "__main__":
    main()
