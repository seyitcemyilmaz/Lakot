import sys, os, time, shutil, subprocess, threading, socket, traceback, struct
from lakotnet import *

generate_bindings()

RUN = HERE / ".run_g54"
C = {"clients": []}
PW = "pw12345"
BAD = "wrong-pass-9"


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


class G54Server(Server):
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


def burst(pClient, pFills):
    tFrames = b""
    tIds = []
    for f in pFills:
        m = pClient.Message()
        q = m.request
        q.header.id = pClient.next_id
        tIds.append(q.header.id)
        pClient.next_id += 1
        f(q)
        d = m.SerializeToString()
        tFrames += struct.pack(">I", len(d)) + d
    pClient.sock.sendall(tFrames)
    return tIds


def gather(pClient, pIds, pTimeout=12.0):
    tOut = {}
    tEnd = time.time() + pTimeout
    with pClient.cv:
        while True:
            for _, m in pClient.log:
                r = m.response
                if r.header.reply_to in pIds:
                    tOut[r.header.reply_to] = r
            if len(tOut) == len(pIds):
                return [tOut[i] for i in pIds]
            tLeft = tEnd - time.time()
            if tLeft <= 0 or pClient.closed:
                missing = [i for i in pIds if i not in tOut]
                raise AssertionError(f"no response for {len(missing)}/{len(pIds)} pipelined requests (closed={pClient.closed})")
            pClient.cv.wait(min(tLeft, 0.2))


def login_fill(pUser, pPw):
    return lambda q: (setattr(q.login_request, "username", pUser), setattr(q.login_request, "password", pPw))


def register_fill(pUser):
    return lambda q: (setattr(q.register_request, "username", pUser), setattr(q.register_request, "password", PW),
                      setattr(q.register_request, "email", pUser + "@test.local"))


def has_token(pR):
    return bool(pR.login_response.token)


def sig(pR):
    return (pR.header.status.code, pR.header.status.message)


def new_user(pPrefix="g54"):
    c = track(Client())
    u = rand_name(pPrefix)
    r = c.register(u)
    check(r is not None and r.register_response.register_status == 0, f"register failed {r}")
    return c, u


def db_failures(pUser):
    return sum(1 for l in C["srv"].log_lines if "Giris basarisiz: " + pUser in l)


def t_baseline_login_and_register():
    c, u = new_user()
    r = c.login(u)
    check(r is not None and has_token(r), f"normal login after register on same connection failed {r}")
    r = c.login(u)
    check(r is not None and not has_token(r) and r.header.status.code != 0, f"second login on logged-in connection accepted {r}")
    d = track(Client())
    r = d.login(u, BAD)
    check(r is not None and not has_token(r) and r.header.status.code != 0, f"wrong password accepted {r}")
    r = d.login(u)
    check(r is not None and has_token(r), f"correct login after one failure on same connection failed {r}")


def t_pipeline_all_wrong_single_db_hit():
    c, u = new_user()
    d = track(Client())
    tIds = burst(d, [login_fill(u, BAD)] * 20)
    tRs = gather(d, tIds)
    check(not any(has_token(r) for r in tRs), "wrong password produced a token")
    check(all(r.header.status.code != 0 for r in tRs), "a wrong-password request returned OK status")
    tInvalid = [r for r in tRs if sig(r)[1] == "Invalid username or password."]
    tOther = [r for r in tRs if sig(r)[1] != "Invalid username or password."]
    check(len(tInvalid) >= 1, "no request reached the credential check at all")
    check(len(tInvalid) <= 1, f"{len(tInvalid)} of 20 pipelined logins went through credential check, expected at most 1")
    check(len({sig(r) for r in tOther}) == 1, f"non-first rejections are not uniform: {set(sig(r) for r in tOther)}")
    time.sleep(1.0)
    n = db_failures(u)
    check(n == len(tInvalid) and n <= 1, f"server log shows {n} DB-checked failures, responses show {len(tInvalid)}")
    r = d.login(u)
    check(r is not None and has_token(r), f"correct login right after the burst failed (flag stuck or limiter locked): {r}")
    e = track(Client())
    r = e.login(u)
    check(r is not None and has_token(r), f"correct login from a new connection after the burst failed (limiter locked): {r}")


def t_pipeline_mixed_wrong_first_no_oracle():
    c, u = new_user()
    d = track(Client())
    tFills = []
    for i in range(10):
        tFills.append(login_fill(u, BAD) if i % 2 == 0 else login_fill(u, PW))
    tRs = gather(d, burst(d, tFills))
    check(sig(tRs[0])[1] == "Invalid username or password.", f"first (wrong) request not processed first: {sig(tRs[0])}")
    tRest = tRs[1:]
    check(not any(has_token(r) for r in tRest), "correct password in a pipeline got a token while the wrong one was in flight")
    check(len({sig(r) for r in tRest}) == 1, f"responses for later pipelined requests differ (oracle): {set(sig(r) for r in tRest)}")
    check(tRest[0].header.status.code != 0, "later pipelined request returned OK status without a token")
    r = d.login(u)
    check(r is not None and has_token(r), f"login after mixed pipeline failed {r}")


def t_pipeline_correct_first_single_session():
    c, u = new_user()
    d = track(Client())
    tFills = [login_fill(u, PW) if i % 2 == 0 else login_fill(u, BAD) for i in range(10)]
    tRs = gather(d, burst(d, tFills))
    check(has_token(tRs[0]), f"first correct request did not log in {sig(tRs[0])}")
    tRest = tRs[1:]
    check(not any(has_token(r) for r in tRest), "more than one login succeeded on one connection")
    check(len({sig(r) for r in tRest}) == 1, f"later responses differ between right/wrong passwords (oracle): {set(sig(r) for r in tRest)}")
    time.sleep(0.5)
    check(db_failures(u) == 0, "a wrong password was checked against DB although the first login had succeeded")


def t_retry_after_failure_same_connection():
    c, u = new_user()
    d = track(Client())
    for _ in range(3):
        r = d.login(u, BAD)
        check(r is not None and not has_token(r) and r.header.status.code != 0, f"wrong password not rejected {r}")
    r = d.login(u)
    check(r is not None and has_token(r), f"login after sequential failures on the same connection failed {r}")


def t_register_rejected_when_logged_in():
    c, u = new_user()
    r = c.login(u)
    check(r is not None and has_token(r), f"login failed {r}")
    v = rand_name("g54v")
    r = c.call(register_fill(v))
    check(r is not None, "no response to register on a logged-in connection")
    check(r.header.status.code != 0, f"register on logged-in connection returned OK status {r}")
    d = track(Client())
    r = d.login(v)
    check(r is not None and not has_token(r), f"account {v} was created by a rejected register on a logged-in connection")
    r = d.register(v)
    check(r is not None and r.register_response.register_status == 0, f"fresh connection register of {v} should succeed (name not taken): {r}")
    r = d.login(v)
    check(r is not None and has_token(r), f"register then login on same fresh connection failed {r}")


def t_register_inflight_blocks_login():
    c, u = new_user()
    d = track(Client())
    v = rand_name("g54r")
    tRs = gather(d, burst(d, [register_fill(v), login_fill(u, PW)]))
    check(tRs[0].register_response.register_status == 0 and tRs[0].header.status.code == 0, f"pipelined register failed {tRs[0]}")
    check(not has_token(tRs[1]) and tRs[1].header.status.code != 0, f"login accepted while register was in flight {tRs[1]}")
    r = d.login(u)
    check(r is not None and has_token(r), f"login after register completed failed {r}")
    e = track(Client())
    w = rand_name("g54w")
    tRs = gather(e, burst(e, [login_fill(u, PW), register_fill(w)]))
    check(has_token(tRs[0]), f"first pipelined login failed {tRs[0]}")
    check(tRs[1].header.status.code != 0, f"register accepted in pipeline behind a login {tRs[1]}")
    f = track(Client())
    r = f.login(w)
    check(r is not None and not has_token(r), f"account {w} exists although its register followed a login")


def t_server_survived():
    srv = C["srv"]
    check(srv.proc.poll() is None, "server process exited\n" + "\n".join(srv.log_lines[-15:]))
    check(not any("Assertion" in l or "terminate" in l.lower() or "exception" in l.lower() for l in srv.log_lines),
          "exception/assert line in server log")
    c, u = new_user()
    r = c.login(u)
    check(r is not None and has_token(r), "login after the whole run failed")


TESTS = [t_baseline_login_and_register, t_pipeline_all_wrong_single_db_hit, t_pipeline_mixed_wrong_first_no_oracle,
         t_pipeline_correct_first_single_session, t_retry_after_failure_same_connection,
         t_register_rejected_when_logged_in, t_register_inflight_blocks_login, t_server_survived]


def main():
    tOnly = sys.argv[1:]
    tResults = []
    srv = G54Server()
    try:
        try:
            srv.start()
        except Exception as e:
            print("SETUP FAILED:", e, flush=True)
            sys.exit(2)
        C["srv"] = srv
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
                print("      server log tail:", *srv.log_lines[-6:], sep="\n        ", flush=True)
            print(f"{'PASS' if tResults[-1][1] else 'FAIL'}  {t.__name__}  ({time.time() - t0:.1f}s) {tResults[-1][2]}", flush=True)
    finally:
        for c in C["clients"]:
            try:
                c.close()
            except Exception:
                pass
        srv.stop()
    tPassed = sum(1 for r in tResults if r[1])
    print(f"\n{tPassed}/{len(tResults)} passed")
    sys.exit(0 if tPassed == len(tResults) else 1)


if __name__ == "__main__":
    main()
