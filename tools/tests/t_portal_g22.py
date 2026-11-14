import sys, os, time, math, re, shutil, subprocess, threading, socket, traceback
from lakotnet import *
from mapnav import nav

generate_bindings()

RUN = type(HERE)(os.environ.get("LAKOT_TEST_RUN_DIR") or HERE / ".run_g22")
SUFFIX = rand_name("")[:6]
GM_ACCOUNTS = [f"gmg22a{SUFFIX}", f"gmg22b{SUFFIX}"]
C = {"clients": []}
P101 = (810.0, -32.0)
ARRIVE = {102: (-735.0, 14.0), 101: (760.0, -30.0), 401: (-735.0, 14.0)}
ARRIVE_102_FROM_401 = (735.0, -14.0)
PORTAL_RADIUS = 14.0
ARRIVE_TOL = 8.0


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


class G22Server(Server):
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
        tEnv = dict(os.environ)
        tEnv["LAKOT_GM_ACCOUNT"] = ",".join(GM_ACCOUNTS)
        tEnv["LAKOT_SERVER_PORT"] = str(PORT)
        tEnv["LAKOT_SERVER_BIND"] = "127.0.0.1"
        tEnv["LAKOT_MAX_CONNECTIONS_PER_IP"] = "0"
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


def cmd(pClient, pText, pPace=0.4):
    time.sleep(pPace)
    r = pClient.call(lambda q: setattr(q.command_request, "text", pText), 6.0)
    check(r is not None and r.HasField("command_response"), f"no CommandResponse for {pText}")
    check(r.command_response.status == 0, f"{pText}: status {r.command_response.status} {list(r.command_response.lines)}")
    return list(r.command_response.lines)


def get_pos(pClient):
    tText = " ".join(cmd(pClient, "/pos"))
    m = re.search(r"Map (\d+), x (-?\d+(?:\.\d+)?), z (-?\d+(?:\.\d+)?)", tText)
    check(m, f"/pos unparsable: {tText}")
    return int(m.group(1)), float(m.group(2)), float(m.group(3))


def poll(pFn, pTimeout=5.0, pStep=0.15):
    tEnd = time.time() + pTimeout
    while time.time() < tEnd:
        v = pFn()
        if v:
            return v
        time.sleep(pStep)
    return pFn()


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


def make_gm(pIndex):
    c = Client()
    r = c.login(GM_ACCOUNTS[pIndex])
    check(r is not None and r.login_response.token, f"gm login failed {r}")
    tChars = list(c.char_list().character_list_response.characters)
    if tChars:
        c.cid, c.name = tChars[0].character_id, tChars[0].name
    else:
        tName = rand_name("g22")
        r = c.create_char(tName, 1)
        check(r is not None and r.character_create_response.status == 0, f"char create failed {r}")
        c.cid, c.name = r.character_create_response.character.character_id, tName
    C["clients"].append(c)
    enter_world(c)
    time.sleep(1.0)
    return c


def setup():
    srv = G22Server()
    srv.start()
    for tUser in GM_ACCOUNTS:
        c = Client()
        r = c.register(tUser)
        check(r is not None and r.register_response.register_status == 0, f"gm register failed {r}")
        c.close()
    srv.stop()
    srv.start()
    C["srv"] = srv
    C["A"] = make_gm(0)
    C["B"] = make_gm(1)
    check("/tp" in " ".join(cmd(C["A"], "/help")), "account 0 has no GM rights")
    check("/tp" in " ".join(cmd(C["B"], "/help")), "account 1 has no GM rights")
    check(get_pos(C["A"])[0] == 101, "A not on map 101")


def map_changed_since(pClient, pSince):
    with pClient.cv:
        return [m.response.map_changed for _, m in pClient.log[pSince:] if m.response.HasField("map_changed")]


def expect_transfer(pClient, pMark, pMap, pXZ, pCtx):
    check(poll(lambda: map_changed_since(pClient, pMark), 6.0), f"{pCtx}: no MapChanged message")
    tMc = map_changed_since(pClient, pMark)[-1]
    check(tMc.map_id == pMap, f"{pCtx}: MapChanged map {tMc.map_id}, want {pMap}")
    tDist = math.hypot(tMc.position.x - pXZ[0], tMc.position.z - pXZ[1])
    check(tDist <= ARRIVE_TOL, f"{pCtx}: MapChanged pos ({tMc.position.x:.1f},{tMc.position.z:.1f}) is {tDist:.1f} from {pXZ}")
    time.sleep(0.6)
    tMap, x, z = get_pos(pClient)
    check(tMap == pMap, f"{pCtx}: /pos map {tMap}, want {pMap}")
    check(math.hypot(x - pXZ[0], z - pXZ[1]) <= ARRIVE_TOL, f"{pCtx}: /pos ({x},{z}) not near {pXZ}")
    return tMc


def t_101_to_102_walk_and_observer():
    a, b = C["A"], C["B"]
    cmd(b, f"/tp {int(P101[0]) - 40} {int(P101[1])}")
    cmd(a, f"/tp {int(P101[0]) - 25} {int(P101[1])}")
    check(poll(lambda: a.name in visible(b), 6.0), "observer B never saw A near the 101 portal")
    tMark = a.mark()
    n = nav()
    x = P101[0] - 25.0
    tEnd = time.time() + 20
    while time.time() < tEnd and not map_changed_since(a, tMark):
        x += 1.0
        a.move(x, n.y_at(x, P101[1]), P101[1], 0.0)
        time.sleep(0.25)
        check(x < P101[0] + 40, "walked far past the portal without a transfer")
    print(f"      walked until x={x:.0f} (portal at {P101[0]}, radius {PORTAL_RADIUS})", flush=True)
    check(x >= P101[0] - PORTAL_RADIUS - 2, f"transfer fired too early at x={x}")
    expect_transfer(a, tMark, 102, ARRIVE[102], "101->102")
    check(poll(lambda: a.name not in visible(b), 6.0), "observer B still sees A on 101 after A left")
    time.sleep(1.5)
    check(a.name not in visible(b), "A reappeared in B's view after leaving 101")


def t_102_to_401():
    a = C["A"]
    tMark = a.mark()
    cmd(a, "/tp 795 0")
    expect_transfer(a, tMark, 401, ARRIVE[401], "102->401")


def t_401_to_102():
    a = C["A"]
    tMark = a.mark()
    cmd(a, "/tp -795 0")
    expect_transfer(a, tMark, 102, ARRIVE_102_FROM_401, "401->102")


def t_102_to_101_and_observer_sees_return():
    a, b = C["A"], C["B"]
    tMark = a.mark()
    cmd(a, "/tp -795 0")
    expect_transfer(a, tMark, 101, ARRIVE[101], "102->101")
    check(poll(lambda: a.name in visible(b), 8.0), "observer B did not see A again after A returned to 101")


TESTS = [t_101_to_102_walk_and_observer, t_102_to_401, t_401_to_102, t_102_to_101_and_observer_sees_return]


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
                C["srv"].stop()
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
