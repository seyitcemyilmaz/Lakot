import sys, os, time, shutil, subprocess, threading, socket, traceback, random
from lakotnet import *

generate_bindings()

RUN = HERE / ".run_g33"
C = {"clients": []}
ENTER_OK, ENTER_NOT_FOUND, ENTER_ERROR = 0, 1, 2


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


class G33Server(Server):
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


def track(pClient):
    C["clients"].append(pClient)
    return pClient


def logout(pClient):
    pClient.send(lambda q: q.logout_request.SetInParent())


def new_account(pChars=1, pPrefix="g33"):
    c = track(Client())
    tUser = rand_name(pPrefix)
    r = c.register(tUser)
    check(r is not None and r.register_response.register_status == 0, f"register failed {r}")
    r = c.login(tUser)
    check(r is not None and r.login_response.token, f"login failed {r}")
    c.user, c.token = tUser, r.login_response.token
    tChars = []
    for _ in range(pChars):
        tName = rand_name("c")
        r = c.create_char(tName, 1)
        check(r is not None and r.character_create_response.status == 0, f"create failed {r}")
        tChars.append((r.character_create_response.character.character_id, tName))
    c.chars = tChars
    c.cid, c.name = tChars[0]
    return c


def connect(pUser, pCheck=True):
    c = track(Client())
    r = c.login(pUser)
    if pCheck:
        check(r is not None and r.login_response.token, f"login failed {r}")
        c.token = r.login_response.token
    return c, r


def send_enter(pClient, pCid):
    return pClient.send(lambda q: setattr(q.enter_world_request, "character_id", pCid))


def enter_status(pClient, pRid, pSince=0, pTimeout=8.0):
    r = pClient.wait(lambda x: x.header.reply_to == pRid and x.HasField("enter_world_response"), pTimeout, pSince)
    return None if r is None else r.enter_world_response.status


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


def is_visible(pName):
    return pName in visible(C["O"])


def online(pName):
    for _ in range(4):
        r = C["O"].whisper(pName, "g33-probe")
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


def wait_offline(pName, pTimeout):
    tEnd = time.time() + pTimeout
    while time.time() < tEnd:
        if not online(pName) and not is_visible(pName):
            return True
        time.sleep(0.4)
    return not online(pName) and not is_visible(pName)


def wait_online_single(pName, pTimeout=5.0):
    check(poll(lambda: is_visible(pName), pTimeout), f"{pName} never became visible to the observer")
    check(visible(C["O"]).count(pName) == 1, f"{pName} visible {visible(C['O']).count(pName)} times")
    check(online(pName), f"{pName} visible but /whisper says offline")


def setup():
    srv = G33Server()
    srv.start()
    C["srv"] = srv
    o = make_player(1, "obs")
    track(o)
    enter_world(o)
    C["O"] = o
    time.sleep(1.5)


def t_double_enter_world():
    c = new_account(2)
    (tA, nA), (tB, nB) = c.chars
    m = c.mark()
    r1, r2 = send_enter(c, tA), send_enter(c, tB)
    s1, s2 = enter_status(c, r1, m), enter_status(c, r2, m)
    check(sorted([s1, s2]) == [ENTER_OK, ENTER_ERROR], f"A then B pipelined: statuses {s1},{s2}, want one OK and one ERROR")
    tWinner = nA if s1 == ENTER_OK else nB
    tLoser = nB if s1 == ENTER_OK else nA
    wait_online_single(tWinner)
    time.sleep(1.0)
    check(not is_visible(tLoser) and not online(tLoser), f"loser {tLoser} is a ghost in the world")
    logout(c)
    check(wait_offline(tWinner, 3.0), f"{tWinner} still online 3s after logout")
    check(not online(tLoser), f"{tLoser} online after logout")
    c2 = new_account(1)
    m = c2.mark()
    r1, r2 = send_enter(c2, c2.cid), send_enter(c2, c2.cid)
    s1, s2 = enter_status(c2, r1, m), enter_status(c2, r2, m)
    check(sorted([s1, s2]) == [ENTER_OK, ENTER_ERROR], f"same char twice: statuses {s1},{s2}")
    wait_online_single(c2.name)
    logout(c2)
    check(wait_offline(c2.name, 3.0), "same-char double-enter character still online after logout")


def t_disconnect_during_enter_x20():
    c = new_account(1)
    c.close()
    for i in range(20):
        d, _ = connect(c.user)
        send_enter(d, c.cid)
        time.sleep(random.choice((0, 0.002, 0.01, 0.03, 0.06)))
        d.close()
    e, _ = connect(c.user)
    m = e.mark()
    rid = send_enter(e, c.cid)
    s = enter_status(e, rid, m)
    check(s == ENTER_OK, f"enter after 20 aborted attempts: status {s}")
    wait_online_single(c.name)
    time.sleep(1.0)
    check(visible(C["O"]).count(c.name) == 1, "ghost duplicates after aborted entries")
    e.close()
    tStart = time.time()
    check(wait_offline(c.name, 52.0), "character still online 52s after the connection dropped (ghost)")
    print(f"      offline after {time.time() - tStart:.1f}s", flush=True)
    check(time.time() - tStart <= 48.0, f"offline only after {time.time() - tStart:.1f}s, want <= 47s")


def t_login_while_logged_in():
    c = new_account(1)
    from common_pb2 import STATUS_INVALID_REQUEST
    r = c.login(c.user)
    check(r is not None and r.header.status.code == STATUS_INVALID_REQUEST and "already logged in" in r.header.status.message.lower(),
          f"char select: login -> {r}")
    r = c.char_list()
    check(r is not None and len(r.character_list_response.characters) == 1, f"connection broken after rejected login: {r}")
    enter_world(c)
    wait_online_single(c.name)
    r = c.login(c.user)
    check(r is not None and r.header.status.code == STATUS_INVALID_REQUEST and "already logged in" in r.header.status.message.lower(),
          f"in world: login -> {r}")
    check(c.chat("still-alive") is not None, "connection dead after rejected login in world")
    wait_online_single(c.name)
    logout(c)
    check(wait_offline(c.name, 1.5), "character still present 1.5s after logout")
    r = c.login(c.user)
    check(r is not None and r.login_response.token, f"login on same connection after logout failed {r}")
    m = c.mark()
    rid = send_enter(c, c.cid)
    check(enter_status(c, rid, m) == ENTER_OK, "enter after logout+login on same connection failed")
    wait_online_single(c.name)
    logout(c)
    check(wait_offline(c.name, 1.5), "character still present after final logout")


def t_logout_during_db_load():
    c = new_account(1)
    for i in range(8):
        m = c.mark()
        rid = send_enter(c, c.cid)
        if i % 2:
            time.sleep(0.003 * i)
        logout(c)
        s = enter_status(c, rid, m, 1.5)
        check(wait_offline(c.name, 1.5), f"iter {i}: character online 1.5s after enter+logout (enter status {s})")
        r = c.login(c.user)
        check(r is not None and r.login_response.token, f"iter {i}: login after logout failed {r}")
        c.token = r.login_response.token
        m = c.mark()
        rid = send_enter(c, c.cid)
        check(enter_status(c, rid, m) == ENTER_OK, f"iter {i}: enter after aborted enter failed")
        wait_online_single(c.name)
        logout(c)
        check(wait_offline(c.name, 2.0), f"iter {i}: offline after logout failed")
        r = c.login(c.user)
        check(r is not None and r.login_response.token, f"iter {i}: relogin failed {r}")
    for i in range(6):
        logout(c)
        m = c.mark()
        r = c.login(c.user)
        check(r is not None and r.login_response.token, f"rapid {i}: login failed {r}")
        send_enter(c, c.cid)
        rid2 = send_enter(c, c.cid)
        time.sleep(0.01)
        s = enter_status(c, rid2, m)
        tOks = [x for x in c.collect(lambda y: y.HasField("enter_world_response") and y.enter_world_response.status == 0, 0.8, m)]
        wait_online_single(c.name)
        check(visible(C["O"]).count(c.name) == 1, f"rapid {i}: not a single view")
        logout(c)
        check(wait_offline(c.name, 2.0), f"rapid {i}: offline after logout failed")
        r = c.login(c.user)
        check(r is not None and r.login_response.token, f"rapid {i}: relogin failed {r}")


def t_relogin_race():
    a = new_account(1)
    tCur = a
    for i in range(5):
        b1 = tCur
        send_enter(b1, a.cid)
        b2, r = connect(a.user)
        check(poll(lambda: b1.closed, 4.0), f"iter {i}: b-1 not closed after relogin")
        m2 = b2.mark()
        rid = send_enter(b2, a.cid)
        s = enter_status(b2, rid, m2)
        check(s == ENTER_OK, f"iter {i}: b-2 enter status {s}")
        wait_online_single(a.name)
        time.sleep(0.5)
        check(visible(C["O"]).count(a.name) == 1, f"iter {i}: ghost view")
        tCur = b2
    logout(tCur)
    check(wait_offline(a.name, 3.0), "character online after final logout")


def t_double_login_fresh_connection():
    c = new_account(1)
    c.close()
    d = track(Client())
    m = d.mark()
    r1 = d.send(lambda q: (setattr(q.login_request, "username", c.user), setattr(q.login_request, "password", "pw12345")))
    r2 = d.send(lambda q: (setattr(q.login_request, "username", c.user), setattr(q.login_request, "password", "pw12345")))
    a = d.wait(lambda x: x.header.reply_to == r1, 8, m)
    b = d.wait(lambda x: x.header.reply_to == r2, 8, m)
    check(a is not None and b is not None, f"missing login replies {a is not None} {b is not None}")
    tOk = [x for x in (a, b) if x.login_response.token]
    tBad = [x for x in (a, b) if not x.login_response.token]
    check(len(tOk) == 1 and len(tBad) == 1, f"want one OK and one rejection, got {len(tOk)} OK / {len(tBad)} rejected")
    check(tBad[0].header.status.code != 0, "rejected login has OK status")
    check("already logged in" in tBad[0].header.status.message.lower(), f"rejection text: {tBad[0].header.status.message}")
    tToken = tOk[0].login_response.token
    rr = d.char_list()
    check(rr is not None and len(rr.character_list_response.characters) == 1, "original connection unusable after double login")
    R = track(Client())
    r = R.call(lambda q: setattr(q.resume_session_request, "token", tToken))
    check(r is not None and r.resume_session_response.status == 0, f"resume with token from double login: {r}")
    rr = R.char_list()
    check(rr is not None and len(rr.character_list_response.characters) == 1, "resumed session cannot list characters")
    check(poll(lambda: d.closed, 3.0), "old connection not closed after resume")


def t_resume_regression():
    c = new_account(1)
    enter_world(c)
    wait_online_single(c.name)
    tTok = c.token
    c.close()
    time.sleep(1.0)
    R = track(Client())
    r = R.call(lambda q: setattr(q.resume_session_request, "token", tTok))
    check(r is not None and r.resume_session_response.status == 0, f"resume within hold failed: {r}")
    check(r.resume_session_response.character_id == c.cid, f"resume character_id {r.resume_session_response.character_id}, want {c.cid}")
    tNew = r.resume_session_response.token
    check(tNew, "resume gave no new token")
    time.sleep(1.0)
    check(visible(C["O"]).count(c.name) == 1 and online(c.name), "character not single/online after resume")
    r = R.chat("resumed-alive")
    check(r is not None and r.chat_message_response.error_code == 0, f"chat after resume failed {r}")
    logout(R)
    check(wait_offline(c.name, 2.0), "character online after logout of resumed session")
    for tT in (tTok, tNew):
        X = track(Client())
        r = X.call(lambda q: setattr(q.resume_session_request, "token", tT))
        check(r is not None and r.resume_session_response.status == 1, f"old token usable after logout: {r}")
    srv = C["srv"]
    check(srv.proc.poll() is None, "server died")


TESTS = [t_double_enter_world, t_disconnect_during_enter_x20, t_login_while_logged_in, t_logout_during_db_load,
         t_relogin_race, t_double_login_fresh_connection, t_resume_regression]


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
                print("      server log tail:", *srv.log_lines[-8:], sep="\n        ", flush=True)
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
