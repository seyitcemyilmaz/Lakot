import sys, os, time, math, struct, socket, traceback
from lakotnet import *
from t_monsters_g21 import tpl, open_site, ORIGIN
from t_use_item_g24 import G24Server, check
from mapnav import nav

INF = float("inf")
NAN = float("nan")
MON_ID = 301
SRV = {}


def survive(pCtx):
    check(SRV["srv"].proc.poll() is None, f"{pCtx}: server process exited")
    c = make_player(1, "s")
    try:
        enter_world(c)
    finally:
        c.close()


def snaps_since(pClient, pSince):
    with pClient.cv:
        return [m.response.world_snapshot for _, m in pClient.log[pSince:] if m.response.HasField("world_snapshot")]


def wait_snap(pClient, pSince, pPred, pTimeout=4.0):
    tEnd = time.time() + pTimeout
    while time.time() < tEnd:
        for s in snaps_since(pClient, pSince):
            r = pPred(s)
            if r:
                return r
        time.sleep(0.1)
    return None


def correction(s):
    return s.self_correction if s.HasField("self_correction") else None


def pair():
    a, b = make_player(1, "a"), make_player(2, "b")
    enter_world(a)
    enter_world(b)
    time.sleep(1.5)
    return a, b


def t_bad_position():
    a, b = pair()
    try:
        x, y, z = a.pos
        for name, val in (("nan", NAN), ("+inf", INF), ("-inf", -INF), ("1e30", 1e30)):
            for axis in range(3):
                v = [x, y, z]
                v[axis] = val
                m = a.mark()
                a.move(v[0], v[1], v[2], 0.0)
                c = wait_snap(a, m, correction)
                check(c is not None, f"{name} axis {axis}: no self_correction")
                p = c.position
                check(math.isfinite(p.x) and math.isfinite(p.z) and abs(p.x - x) < 2.5 and abs(p.z - z) < 2.5,
                      f"{name} axis {axis}: correction pos ({p.x},{p.z}) != ({x},{z})")
        mb = b.mark()
        time.sleep(1.5)
        for s in snaps_since(b, mb):
            for e in s.moved:
                check(math.isfinite(e.position.x) and math.isfinite(e.position.z) and abs(e.position.x) < 1e5,
                      f"observer saw bad position {e.position.x},{e.position.z}")
        survive("bad position")
    finally:
        a.close()
        b.close()


def t_bad_yaw():
    a, b = pair()
    try:
        x, y, z = a.pos
        for name, yaw in (("nan", NAN), ("+inf", INF), ("-inf", -INF)):
            m = a.mark()
            a.move(x, y, z, yaw)
            c = wait_snap(a, m, correction)
            check(c is not None, f"yaw {name}: not rejected (no self_correction)")
        nx = x + 0.4
        ny = nav().y_at(nx, z)
        mb = b.mark()
        a.move(nx, ny, z, 1e9)

        def seen(s):
            for e in s.moved:
                if e.entity_id == a.cid:
                    return e
            return None
        e = wait_snap(b, mb, seen, 5)
        check(e is not None, "huge finite yaw: observer saw no movement (rejected?)")
        check(math.isfinite(e.yaw) and -math.pi - 1e-3 <= e.yaw <= math.pi + 1e-3, f"broadcast yaw {e.yaw} outside [-pi, pi]")
        survive("bad yaw")
    finally:
        a.close()
        b.close()


def attack_hits(pClient, pYaw, pSeconds=3.0):
    m = pClient.mark()
    pClient.attack(pYaw)
    time.sleep(pSeconds)
    out = []
    for s in snaps_since(pClient, m):
        for ev in s.combat:
            if ev.attacker_id == pClient.cid:
                out.append(ev)
    return out


def t_attack_bad_yaw():
    a, b = pair()
    try:
        tHit = None
        for yaw in (0.0, math.pi / 2, math.pi, -math.pi / 2, 0.0, math.pi / 2, math.pi, -math.pi / 2):
            ev = attack_hits(a, yaw, 1.2)
            if any(h.damage > 0 for e in ev for h in e.hits):
                tHit = yaw
                break
        check(tHit is not None, "control: valid attack never hit the adjacent monster/player")
        time.sleep(1.5)
        for name, yaw in (("nan", NAN), ("+inf", INF), ("-inf", -INF)):
            ev = attack_hits(a, yaw, 2.0)
            hits = [h for e in ev for h in e.hits]
            check(not hits, f"attack yaw {name} produced {len(hits)} hits")
            time.sleep(1.0)
        survive("attack yaw")
    finally:
        a.close()
        b.close()


def t_unknown_packet_rst():
    from connection_pb2 import Message
    m = Message()
    m.request.header.id = 7
    body = m.SerializeToString()
    m2 = Message()
    m2.response.header.reply_to = 9
    body2 = m2.SerializeToString()
    for i in range(50):
        s = socket.create_connection(("127.0.0.1", PORT), timeout=5)
        s.sendall(struct.pack(">I", len(body)) + body + struct.pack(">I", len(body2)) + body2)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
        s.close()
    time.sleep(1.0)
    survive("rst")


def released_after(pName, pBytes):
    a, b = pair()
    try:
        m = b.mark()
        a.sock.sendall(pBytes)
        tEnd = time.time() + 5
        while time.time() < tEnd and not a.closed:
            time.sleep(0.1)
        check(a.closed, f"{pName}: server did not close the connection within 5s")
        check(SRV["srv"].proc.poll() is None, f"{pName}: server exited")
        r = wait_snap(b, m, lambda s: a.cid in s.left, 70)
        check(r, f"{pName}: character not removed from world for observer within 70s")
        c = Client()
        r = c.login(a.user)
        check(r is not None and r.login_response.token, f"{pName}: relogin failed")
        r = c.enter(a.cid)
        check(r is not None and r.enter_world_response.status == 0,
              f"{pName}: re-enter refused: {r.enter_world_response.status if r else None}")
        c.close()
        survive(pName)
    finally:
        a.close()
        b.close()


def t_oversize_frame():
    released_after("oversize", struct.pack(">I", 20 * 1024 * 1024))


def t_malformed_body():
    released_after("malformed", struct.pack(">I", 32) + b"\xff" * 32)


TESTS = [t_bad_position, t_bad_yaw, t_attack_bad_yaw, t_unknown_packet_rst, t_oversize_frame, t_malformed_body]


def main():
    tOnly = sys.argv[1:]
    tSite = open_site(ORIGIN, 2.5, pRadius=0.5)
    tSpawns = [{"monster": MON_ID, "x": tSite[0], "z": tSite[1], "radius": 0.3, "count": 1}]
    srv = G24Server([tpl(MON_ID, maxHealth=1000000, defense=0)], tSpawns)
    srv.start()
    SRV["srv"] = srv
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
        tResults.append(("server_survived", tAlive, ""))
        print(f"{'PASS' if tAlive else 'FAIL'}  server_survived", flush=True)
    finally:
        srv.stop()
    tPassed = sum(1 for r in tResults if r[1])
    print(f"\n{tPassed}/{len(tResults)} passed")
    sys.exit(0 if tPassed == len(tResults) else 1)


if __name__ == "__main__":
    main()
