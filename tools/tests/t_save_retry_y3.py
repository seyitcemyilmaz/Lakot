import sys, os, re, json, time, glob, subprocess, threading, socket, traceback
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lakotnet import *

generate_bindings()

DB_NAME = "lakot_y3_test"
os.environ["LAKOT_DB_NAME"] = DB_NAME
PSQL = r"C:\Program Files\PostgreSQL\18\bin\psql.exe"
POTION = 4
ENTER_OK, ENTER_ERROR = 0, 2
S = {"all": []}

TRANSIENT_TRIGGER = ("CREATE OR REPLACE FUNCTION y3_fail() RETURNS trigger AS $$ BEGIN "
                     "RAISE EXCEPTION 'y3 injected' USING ERRCODE = '{code}'; END $$ LANGUAGE plpgsql; "
                     "DROP TRIGGER IF EXISTS y3_block ON item_instances; "
                     "CREATE TRIGGER y3_block BEFORE INSERT ON item_instances FOR EACH ROW EXECUTE FUNCTION y3_fail();")


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


def poll(pFn, pTimeout=8.0, pStep=0.2):
    tEnd = time.time() + pTimeout
    while time.time() < tEnd:
        v = pFn()
        if v:
            return v
        time.sleep(pStep)
    return pFn()


def psql(pSql, pDb=DB_NAME):
    o = subprocess.run([PSQL, "-h", "localhost", "-U", os.environ.get("LAKOT_DB_USER", "postgres"), "-d", pDb, "-tA", "-c", pSql],
                       env={**os.environ, "PGPASSWORD": os.environ["LAKOT_DB_PASSWORD"]}, capture_output=True, text=True, timeout=30)
    if o.returncode != 0:
        return None
    return o.stdout.strip()


def potions_db(pCid):
    r = psql(f"SELECT coalesce(sum(count),0) FROM item_instances WHERE character_id={pCid} AND template_id={POTION}")
    return int(r) if r not in (None, "") else None


def install_trigger(pCode):
    check(psql(TRANSIENT_TRIGGER.format(code=pCode)) is not None, "could not install failure trigger")


def remove_trigger():
    psql("DROP TRIGGER IF EXISTS y3_block ON item_instances")


def block_database():
    check(psql(f"ALTER DATABASE {DB_NAME} ALLOW_CONNECTIONS false", "postgres") is not None, "block failed")
    terminate_backends()


def unblock_database():
    check(psql(f"ALTER DATABASE {DB_NAME} ALLOW_CONNECTIONS true", "postgres") is not None, "unblock failed")


def terminate_backends():
    psql(f"SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname='{DB_NAME}' AND pid <> pg_backend_pid()", "postgres")


def items_of(c):
    with c.cv:
        tLog = list(c.log)
    for _, m in reversed(tLog):
        if m.response.HasField("inventory_update"):
            return list(m.response.inventory_update.items)
    return []


def potions_live(c):
    return sum(it.count for it in items_of(c) if it.template_id == POTION)


def discard_potions(c, pCount):
    tBefore = potions_live(c)
    tSlot = next(it.slot for it in items_of(c) if it.template_id == POTION and it.location == 0)
    check(c.discard(tSlot, POTION, pCount, destroy=True) == 0, "discard failed")
    check(poll(lambda: potions_live(c) == tBefore - pCount, 5), f"live potions {potions_live(c)} expected {tBefore - pCount}")


def logout(c):
    c.send(lambda q: q.logout_request.SetInParent())
    time.sleep(1.0)


def new_account():
    c = Client()
    S["all"].append(c)
    u = rand_name("y3")
    r = c.register(u)
    check(r is not None and r.register_response.register_status == 0, f"register failed {r}")
    r = c.login(u)
    check(r is not None and r.login_response.token, f"login failed {r}")
    c.user = u
    return c


def relogin(pUser):
    c = Client()
    S["all"].append(c)
    r = c.login(pUser)
    check(r is not None and r.login_response.token, f"relogin failed {r}")
    c.user = pUser
    return c


def prepared():
    c = new_account()
    r = c.create_char(rand_name("c"), 1, 1)
    check(r is not None and r.character_create_response.status == 0, f"create failed {r}")
    c.cid = r.character_create_response.character.character_id
    r = c.enter(c.cid)
    check(r is not None and r.enter_world_response.status == ENTER_OK, f"enter failed {r}")
    check(poll(lambda: potions_live(c) == 5, 8), f"no starter kit {potions_live(c)}")
    time.sleep(1.0)
    discard_potions(c, 3)
    check(poll(lambda: potions_db(c.cid) == 2, 10), f"baseline not persisted: db {potions_db(c.cid)}")
    return c


def enter_again(pUser, pCid, pTimeout=14.0):
    c = relogin(pUser)
    c.cid = pCid
    r = c.call(lambda q: setattr(q.enter_world_request, "character_id", pCid), pTimeout)
    return c, (r.enter_world_response.status if r is not None and r.HasField("enter_world_response") else None)


class Y3Server(Server):
    def start(self, timeout=60):
        super().start(timeout)
        tEnd = time.time() + timeout
        while time.time() < tEnd and not any("Sinif verisi yuklendi" in l for l in self.log_lines):
            time.sleep(0.2)
        time.sleep(1.5)

    def request_exit(self):
        try:
            self.proc.stdin.write("exit\n")
            self.proc.stdin.flush()
            self.proc.stdin.close()
        except Exception:
            pass

    def restart_in_place(self, timeout=40):
        self.log_lines = []
        self.proc = subprocess.Popen([os.path.join(self.run_dir, "LakotServer.exe")], cwd=self.run_dir,
                                     env={**os.environ, "LAKOT_SERVER_PORT": str(PORT), "LAKOT_SERVER_BIND": "127.0.0.1", "LAKOT_MAX_CONNECTIONS_PER_IP": "0"},
                                     stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
        threading.Thread(target=self._pump, daemon=True).start()
        tEnd = time.time() + timeout
        while time.time() < tEnd:
            check(self.proc.poll() is None, "server exited early:\n" + "\n".join(self.log_lines[-20:]))
            try:
                socket.create_connection(("127.0.0.1", PORT), timeout=0.5).close()
                return
            except OSError:
                time.sleep(0.3)
        raise AssertionError("server did not open port")


def close_clients():
    for c in S["all"]:
        try:
            c.close()
        except Exception:
            pass
    S["all"] = []


def t_restart_first_task_not_lost():
    c = prepared()
    terminate_backends()
    time.sleep(0.5)
    discard_potions(c, 1)
    check(poll(lambda: potions_db(c.cid) == 1, 15), f"save after a database restart lost: db {potions_db(c.cid)}")
    logout(c)
    time.sleep(1.5)
    check(potions_db(c.cid) == 1, f"db after logout {potions_db(c.cid)}")


def t_outage_logout_then_recover():
    c = prepared()
    block_database()
    try:
        discard_potions(c, 1)
        logout(c)
        time.sleep(2.0)
    finally:
        unblock_database()
    check(poll(lambda: potions_db(c.cid) == 1, 20), f"snapshot not written after the outage: db {potions_db(c.cid)}")
    w, st = enter_again(c.user, c.cid)
    check(st == ENTER_OK, f"enter after recovery status {st}")
    check(poll(lambda: potions_live(w) == 1, 8), f"entered with {potions_live(w)} potions, expected 1")


def t_failed_save_blocks_login_with_old_data():
    c = prepared()
    install_trigger("40001")
    try:
        discard_potions(c, 1)
        logout(c)
        time.sleep(1.0)
        w, st = enter_again(c.user, c.cid)
        check(st == ENTER_ERROR, f"enter while the save is failing expected ENTER_WORLD_ERROR(2), got {st}")
        check(potions_db(c.cid) == 2, f"db changed while blocked: {potions_db(c.cid)}")
    finally:
        remove_trigger()
    check(poll(lambda: potions_db(c.cid) == 1, 15), f"retry did not land after recovery: db {potions_db(c.cid)}")
    w2, st = enter_again(c.user, c.cid)
    check(st == ENTER_OK, f"enter after recovery status {st}")
    check(poll(lambda: potions_live(w2) == 1, 8), f"entered with {potions_live(w2)} potions, expected 1")


def t_newest_snapshot_wins():
    c = prepared()
    install_trigger("40001")
    try:
        for _ in range(2):
            discard_potions(c, 1)
            time.sleep(0.4)
        time.sleep(1.0)
    finally:
        remove_trigger()
    check(potions_live(c) == 0, f"live potions {potions_live(c)}")
    check(poll(lambda: potions_db(c.cid) == 0, 15), f"db {potions_db(c.cid)} expected 0")
    time.sleep(3.0)
    check(potions_db(c.cid) == 0, f"older snapshot overwrote the newest: db {potions_db(c.cid)}")


def t_permanent_error_gives_up():
    c = prepared()
    install_trigger("23514")
    try:
        discard_potions(c, 1)
        logout(c)
        time.sleep(0.5)
        w, st = enter_again(c.user, c.cid)
        check(st == ENTER_OK, f"after repeated permanent failures login must be allowed, got {st}")
        check(poll(lambda: potions_live(w) == 2, 8), f"expected the old db data (2 potions), got {potions_live(w)}")
    finally:
        remove_trigger()
    log = "\n".join(S["srv"].log_lines)
    check("kalici olarak basarisiz" in log, "no permanent-failure ERROR line in server log")


def t_log_has_no_unrecovered_failures():
    log = S["srv"].log_lines
    tFailed, tClosed = {}, {}
    for l in log:
        m = re.search(r"Karakter kaydi basarisiz, tekrar denenecek \(id: (\d+)", l)
        if m:
            tFailed[m.group(1)] = tFailed.get(m.group(1), 0) + 1
        m = re.search(r"toparlandi \(id: (\d+)|vazgecildi \(id: (\d+)", l)
        if m:
            k = m.group(1) or m.group(2)
            tClosed[k] = tClosed.get(k, 0) + 1
    check(tFailed, "no save failure was logged although failures were injected")
    for k, n in tFailed.items():
        check(tClosed.get(k, 0) >= n, f"character {k}: {n} failure lines but {tClosed.get(k, 0)} recovery lines")
        check(n <= 2, f"character {k}: {n} failure lines, expected one per outage")
    check(not any("Karakter kaydi hatasi (baglanti)" in l for l in log), "per-attempt connection error spam in log")


def stop_with_pending(pSrv, pUnblockAfter):
    c = prepared()
    install_trigger("40001")
    discard_potions(c, 1)
    tStart = time.time()
    pSrv.request_exit()
    if pUnblockAfter is not None:
        time.sleep(pUnblockAfter)
        remove_trigger()
    try:
        pSrv.proc.wait(timeout=30)
    except subprocess.TimeoutExpired:
        pSrv.proc.kill()
        raise AssertionError("server did not exit within 30s")
    finally:
        remove_trigger()
    return c, time.time() - tStart


def t_shutdown_flush_waits_for_database():
    srv = Y3Server()
    srv.start()
    try:
        c, tElapsed = stop_with_pending(srv, 3.0)
        print(f"      server exit after {tElapsed:.1f}s", flush=True)
        check(tElapsed < 20, f"exit took {tElapsed:.1f}s")
        check(potions_db(c.cid) == 1, f"shutdown flush lost the snapshot: db {potions_db(c.cid)}")
        check(not glob.glob(os.path.join(srv.run_dir, "unsaved", "*.json")), "unsaved files written although the flush succeeded")
    finally:
        close_clients()
        if srv.proc.poll() is None:
            srv.proc.kill()


def t_shutdown_timeout_writes_unsaved():
    srv = Y3Server()
    srv.start()
    try:
        c, tElapsed = stop_with_pending(srv, None)
        print(f"      server exit after {tElapsed:.1f}s", flush=True)
        check(tElapsed < 25, f"exit took {tElapsed:.1f}s")
        log = "\n".join(srv.log_lines)
        check(f"Kaydedilemedi (id: {c.cid}" in log, "no unsaved ERROR line for the character")
        tFiles = glob.glob(os.path.join(srv.run_dir, "unsaved", f"{c.cid}-*.json"))
        check(len(tFiles) == 1, f"expected one unsaved file, got {tFiles}")
        tData = json.load(open(tFiles[0], encoding="utf-8"))
        check(tData["characterId"] == c.cid, f"dump characterId {tData['characterId']}")
        check(sum(i["count"] for i in tData["items"] if i["templateId"] == POTION) == 1, f"dump items {tData['items']}")
        close_clients()
        srv.restart_in_place()
        time.sleep(1.0)
        check(any("unsaved/" in l and "UYARI" in l for l in srv.log_lines), "no startup warning about unsaved files")
    finally:
        close_clients()
        srv.request_exit()
        try:
            srv.proc.wait(timeout=30)
        except Exception:
            srv.proc.kill()


MAIN_TESTS = [t_restart_first_task_not_lost, t_outage_logout_then_recover, t_failed_save_blocks_login_with_old_data,
              t_newest_snapshot_wins, t_permanent_error_gives_up, t_log_has_no_unrecovered_failures]
SHUTDOWN_TESTS = [t_shutdown_flush_waits_for_database, t_shutdown_timeout_writes_unsaved]


def run_one(pTest, pResults, pSrv=None):
    t0 = time.time()
    try:
        pTest()
        pResults.append((pTest.__name__, True, ""))
    except Exception as e:
        m = str(e) or e.__class__.__name__
        if not isinstance(e, AssertionError):
            m += " | " + traceback.format_exc().strip().splitlines()[-3].strip()
        pResults.append((pTest.__name__, False, m[:400]))
        if pSrv is not None:
            print("      server log tail:", *pSrv.log_lines[-8:], sep="\n        ", flush=True)
    print(f"{'PASS' if pResults[-1][1] else 'FAIL'}  {pTest.__name__}  ({time.time() - t0:.1f}s) {pResults[-1][2]}", flush=True)


def main():
    if not os.path.exists(PSQL):
        print("psql not found, cannot run the Y3 database fault tests")
        sys.exit(1)
    only = sys.argv[1:]
    results = []
    srv = Y3Server()
    srv.start()
    S["srv"] = srv
    try:
        remove_trigger()
        unblock_database()
        for t in MAIN_TESTS:
            if not only or t.__name__ in only:
                run_one(t, results, srv)
    finally:
        close_clients()
        remove_trigger()
        psql(f"ALTER DATABASE {DB_NAME} ALLOW_CONNECTIONS true", "postgres")
        srv.stop()
    for t in SHUTDOWN_TESTS:
        if not only or t.__name__ in only:
            run_one(t, results)
    remove_trigger()
    p = sum(1 for r in results if r[1])
    print(f"\n{p}/{len(results)} passed")
    sys.exit(0 if p == len(results) else 1)


main()
