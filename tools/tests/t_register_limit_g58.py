import sys, os, time, shutil, subprocess, threading, socket, traceback, statistics
from lakotnet import *

generate_bindings()

RUN = HERE / ".run_g58"
C = {"clients": [], "env": {}}
SYSTEM_ERROR = 4
INVALID = "Invalid username or password."


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


class G58Server(Server):
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
        tEnv = {k: v for k, v in os.environ.items() if k != "LAKOT_REGISTER_PER_MINUTE"}
        tEnv.update(C["env"])
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


def track(pClient):
    C["clients"].append(pClient)
    return pClient


def reg(pUser=None):
    u = pUser or rand_name("g58")
    r = track(Client()).register(u)
    return u, r


def status(pR):
    return None if pR is None else pR.register_response.register_status


def has_token(pR):
    return pR is not None and bool(pR.login_response.token)


def msg(pR):
    return "<none>" if pR is None else pR.header.status.message


def t_default_capacity_rejects_burst():
    tOk, tRej, tOther = [], [], []
    t0 = time.time()
    for _ in range(45):
        u, r = reg()
        s = status(r)
        (tOk if s == 0 else tRej if s == SYSTEM_ERROR else tOther).append((u, s))
    tElapsed = time.time() - t0
    check(not tOther, f"unexpected register statuses: {tOther[:3]}")
    check(len(tOk) >= 30, f"only {len(tOk)} registers accepted, capacity is 30")
    check(len(tOk) <= 30 + int(tElapsed * 0.5) + 2, f"{len(tOk)} accepted in {tElapsed:.1f}s, exceeds capacity 30 + refill 0.5/s")
    check(len(tRej) >= 1, f"no register rejected after 45 attempts in {tElapsed:.1f}s (accepted {len(tOk)})")
    u = tOk[0][0]
    r = track(Client()).login(u)
    check(has_token(r), f"login of an existing user broken while register is throttled: {msg(r)}")
    tGhost = tRej[-1][0]
    r = track(Client()).login(tGhost)
    check(not has_token(r) and msg(r) == INVALID, f"rejected registration appears to have created account: {msg(r)}")


def t_low_limit_server():
    C["env"] = {"LAKOT_REGISTER_PER_MINUTE": "3"}
    tSrv = C["srv"]
    tSrv.stop()
    tSrv.start()
    tUsers = []
    for i in range(3):
        u, r = reg()
        check(status(r) == 0, f"register {i + 1} of 3 rejected (status {status(r)}, msg {msg(r)})")
        tUsers.append(u)
    u4, r = reg()
    check(status(r) == SYSTEM_ERROR, f"4th register not SYSTEM_ERROR: status={status(r)} msg={msg(r)}")
    check(not has_token(r), "rejected register returned a token")
    for u in tUsers:
        r = track(Client()).login(u)
        check(has_token(r), f"normal login of {u} affected by register limit: {msg(r)}")
    r = track(Client()).login(u4)
    check(not has_token(r) and msg(r) == INVALID, f"rejected register created an account: {msg(r)}")
    u5, r = reg(tUsers[0])
    check(status(r) == SYSTEM_ERROR, f"duplicate-username register while throttled should be rejected by limiter first (no USERNAME_IN_USE oracle): status={status(r)}")
    time.sleep(21)
    u6, r = reg()
    check(status(r) == 0, f"register still rejected after refill window: status={status(r)}")
    C["env"] = {}


def t_unknown_vs_wrong_password_same_message():
    C["env"] = {"LAKOT_REGISTER_PER_MINUTE": "100000"}
    tSrv = C["srv"]
    tSrv.stop()
    tSrv.start()
    u, r = reg()
    check(status(r) == 0, "setup register failed")
    tKnown, tUnknown = [], []
    tKnownR, tUnknownR = [], []
    for _ in range(3):
        c = track(Client())
        t0 = time.perf_counter()
        r = c.login(u, "wrong-pass-9")
        tKnown.append(time.perf_counter() - t0)
        tKnownR.append(r)
        c = track(Client())
        t0 = time.perf_counter()
        r = c.login(rand_name("nouser"), "wrong-pass-9")
        tUnknown.append(time.perf_counter() - t0)
        tUnknownR.append(r)
    for r in tKnownR + tUnknownR:
        check(r is not None and not has_token(r), "wrong login got no response or a token")
    tKeys = {(r.header.status.code, msg(r)) for r in tKnownR + tUnknownR}
    check(len(tKeys) == 1, f"unknown vs wrong-password responses differ: {tKeys}")
    check(next(iter(tKeys))[1] == INVALID, f"unexpected message {tKeys}")
    tMk, tMu = statistics.median(tKnown), statistics.median(tUnknown)
    print(f"      timing median known-user {tMk * 1000:.1f} ms, unknown-user {tMu * 1000:.1f} ms", flush=True)
    check(tMu >= 0.5 * tMk, f"unknown user answered much faster ({tMu * 1000:.1f} ms vs {tMk * 1000:.1f} ms): timing leak")


def t_server_survived():
    srv = C["srv"]
    check(srv.proc.poll() is None, "server process exited\n" + "\n".join(srv.log_lines[-15:]))
    check(not any("Assertion" in l or "terminate" in l.lower() or "exception" in l.lower() for l in srv.log_lines),
          "exception/assert line in server log")


TESTS = [t_default_capacity_rejects_burst, t_low_limit_server, t_unknown_vs_wrong_password_same_message,
         t_server_survived]


def close_clients():
    for c in C["clients"]:
        try:
            c.close()
        except Exception:
            pass
    C["clients"] = []


def main():
    tOnly = sys.argv[1:]
    tResults = []
    srv = G58Server()
    C["srv"] = srv
    try:
        srv.start()
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
            close_clients()
            print(f"{'PASS' if tResults[-1][1] else 'FAIL'}  {t.__name__}  ({time.time() - t0:.1f}s) {tResults[-1][2]}", flush=True)
    finally:
        close_clients()
        srv.stop()
    tPassed = sum(1 for r in tResults if r[1])
    print(f"\n{tPassed}/{len(tResults)} passed")
    sys.exit(0 if tPassed == len(tResults) else 1)


if __name__ == "__main__":
    main()
