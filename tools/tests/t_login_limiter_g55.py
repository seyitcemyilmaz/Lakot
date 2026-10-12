import sys, os, time, shutil, subprocess, threading, socket, traceback
from lakotnet import *

generate_bindings()

RUN = HERE / ".run_g55"
C = {"clients": []}
PW = "pw12345"
BAD = "wrong-pass-9"
INVALID = "Invalid username or password."


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


class G55Server(Server):
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


def has_token(pR):
    return pR is not None and bool(pR.login_response.token)


def msg(pR):
    return "<none>" if pR is None else pR.header.status.message


def throttled(pR):
    return pR is not None and not has_token(pR) and "Too many" in msg(pR)


def invalid(pR):
    return pR is not None and not has_token(pR) and msg(pR) == INVALID


def new_user(pPrefix="g55"):
    c = track(Client())
    u = rand_name(pPrefix)
    r = c.register(u)
    check(r is not None and r.register_response.register_status == 0, f"register failed {r}")
    return u


def ghost():
    return rand_name("nouser")


def wrong(pUser, pPw=BAD):
    c = track(Client())
    return c.login(pUser, pPw)


def db_failures(pUser=None):
    tTag = "Giris basarisiz: " + (pUser or "")
    return sum(1 for l in C["srv"].log_lines if tTag in l)


def t_ip_throttle_distinct_usernames():
    u = new_user()
    tRs = [wrong(ghost()) for _ in range(8)]
    check(all(invalid(r) for r in tRs), f"first 8 failures not plain rejections: {[msg(r) for r in tRs]}")
    r = wrong(ghost())
    check(throttled(r), f"9th failure with a new username not throttled: {msg(r)}")
    time.sleep(0.5)
    check(db_failures() == 8, f"DB-checked failures {db_failures()}, expected 8 (throttled one must not reach DB)")
    r = track(Client()).login(u)
    check(throttled(r), f"documented behaviour: correct login from the throttled IP is rejected, got {msg(r)} token={has_token(r)}")


def t_pair_limit_single_user():
    u = new_user()
    other = new_user()
    tRs = [wrong(u) for _ in range(5)]
    check(all(invalid(r) for r in tRs), f"first 5 failures not plain rejections: {[msg(r) for r in tRs]}")
    r = wrong(u)
    check(throttled(r), f"6th failure for same user not throttled: {msg(r)}")
    r = track(Client()).login(u)
    check(throttled(r), f"correct password for the locked pair accepted/not throttled: {msg(r)} token={has_token(r)}")
    time.sleep(0.5)
    check(db_failures(u) == 5, f"DB-checked failures for user {db_failures(u)}, expected 5")
    r = track(Client()).login(other)
    check(has_token(r), f"other account from same IP (5 < 8 failures) blocked by pair limit: {msg(r)}")
    r = wrong(ghost())
    check(invalid(r), f"new username from same IP rejected as throttled although IP has only 5 failures: {msg(r)}")


def t_success_keeps_ip_throttle():
    mine = new_user()
    tRs = [wrong(ghost()) for _ in range(7)]
    check(all(invalid(r) for r in tRs), f"setup failures not plain: {[msg(r) for r in tRs]}")
    r = track(Client()).login(mine)
    check(has_token(r), f"successful login at 7 IP failures was blocked: {msg(r)}")
    r = wrong(ghost())
    check(invalid(r), f"8th failure after success should still be a credential rejection: {msg(r)}")
    r = wrong(ghost())
    check(throttled(r), f"success login reset the IP counter (9th failure not throttled): {msg(r)}")
    r = track(Client()).login(mine)
    check(throttled(r), f"IP throttle lifted by earlier success: {msg(r)}")


def t_success_resets_only_pair():
    u = new_user()
    for _ in range(4):
        check(invalid(wrong(u)), "setup failure not plain")
    r = track(Client()).login(u)
    check(has_token(r), f"login at 4 pair failures blocked: {msg(r)}")
    for i in range(3):
        r = wrong(u)
        check(invalid(r), f"pair counter not reset by success: failure {i + 1} after success -> {msg(r)}")
    r = wrong(ghost())
    check(invalid(r), f"IP at 7 failures should still accept an 8th: {msg(r)}")
    r = wrong(ghost())
    check(throttled(r), f"IP counter was reset or not counted across success: 9th failure not throttled: {msg(r)}")


def parallel_wrong(pUsers):
    tOut = [None] * len(pUsers)
    tClients = [track(Client()) for _ in pUsers]
    tBarrier = threading.Barrier(len(pUsers))

    def run(i):
        tBarrier.wait()
        tOut[i] = tClients[i].login(pUsers[i], BAD)

    tThreads = [threading.Thread(target=run, args=(i,)) for i in range(len(pUsers))]
    for t in tThreads:
        t.start()
    for t in tThreads:
        t.join(30)
    return tOut


def t_parallel_ip_limit():
    tRs = parallel_wrong([ghost() for _ in range(32)])
    check(all(r is not None for r in tRs), "some parallel logins got no response")
    time.sleep(1.0)
    tPlain = sum(1 for r in tRs if invalid(r))
    tThr = sum(1 for r in tRs if throttled(r))
    check(tPlain + tThr == len(tRs), f"unexpected responses: {set(msg(r) for r in tRs)}")
    check(tPlain <= 8 and db_failures() <= 8, f"{tPlain} responses / {db_failures()} DB checks for 32 parallel failures, cap is 8")
    check(tThr >= 24, f"only {tThr} of 32 parallel attempts were throttled")


def t_parallel_pair_limit():
    u = new_user()
    tRs = parallel_wrong([u] * 20)
    check(all(r is not None for r in tRs), "some parallel logins got no response")
    time.sleep(1.0)
    tPlain = sum(1 for r in tRs if invalid(r))
    check(tPlain <= 5 and db_failures(u) <= 5, f"{tPlain} responses / {db_failures(u)} DB checks for one user in parallel, cap is 5")
    check(sum(1 for r in tRs if throttled(r)) >= 15, "too few parallel attempts throttled")


def t_malformed_counts_for_ip():
    u = new_user()
    for i in range(8):
        r = wrong(rand_name("m"), "")
        check(invalid(r), f"malformed attempt {i} not a plain rejection: {msg(r)}")
    r = track(Client()).login(u)
    check(throttled(r), f"8 malformed attempts did not count toward the IP limit: {msg(r)}")
    time.sleep(0.3)
    check(db_failures() == 0, "malformed attempts reached the DB")


def t_server_survived():
    srv = C["srv"]
    check(srv.proc.poll() is None, "server process exited\n" + "\n".join(srv.log_lines[-15:]))
    check(not any("Assertion" in l or "terminate" in l.lower() or "exception" in l.lower() for l in srv.log_lines),
          "exception/assert line in server log")
    u = new_user()
    r = track(Client()).login(u)
    check(has_token(r), f"fresh server login failed: {msg(r)}")


TESTS = [t_ip_throttle_distinct_usernames, t_pair_limit_single_user, t_success_keeps_ip_throttle,
         t_success_resets_only_pair, t_parallel_ip_limit, t_parallel_pair_limit, t_malformed_counts_for_ip,
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
    srv = G55Server()
    C["srv"] = srv
    try:
        for t in TESTS:
            if tOnly and t.__name__ not in tOnly:
                continue
            t0 = time.time()
            try:
                srv.stop()
                srv.start()
            except Exception as e:
                print("SETUP FAILED:", e, flush=True)
                sys.exit(2)
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
