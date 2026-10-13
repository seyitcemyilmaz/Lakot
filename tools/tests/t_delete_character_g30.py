import sys, os, time, shutil, subprocess, threading, socket, traceback
from lakotnet import *

generate_bindings()

RUN = HERE / ".run_g30"
OK, NOT_FOUND, NAME_MISMATCH, IN_WORLD = 0, 1, 2, 3
POTION, SWORD, ARMOR = 4, 1001, 2001
S = {}


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


def poll(pFn, pTimeout=5.0):
    tEnd = time.time() + pTimeout
    while time.time() < tEnd:
        v = pFn()
        if v:
            return v
        time.sleep(0.1)
    return pFn()


class G30Server(Server):
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
        self.proc.kill()
        raise RuntimeError("server did not open port\n" + "\n".join(self.log_lines[-20:]))


def account():
    c = Client()
    S.setdefault("all", []).append(c)
    u = rand_name("acc")
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


def new_char(c, pName=None, pKingdom=1):
    n = pName or rand_name("c")
    r = c.create_char(n, pKingdom, 1)
    check(r is not None and r.character_create_response.status == 0, f"create '{n}' failed {r}")
    return r.character_create_response.character.character_id, n


def listing(c):
    r = c.char_list()
    check(r is not None and r.HasField("character_list_response"), f"no list {r}")
    return {x.character_id: x.name for x in r.character_list_response.characters}, r.character_list_response.max_slots


def items_of(c):
    with c.cv:
        tLog = list(c.log)
    for _, m in reversed(tLog):
        if m.response.HasField("inventory_update"):
            return list(m.response.inventory_update.items), m.response.inventory_update.stats
    return [], None


def totals(c):
    t = {}
    for it in items_of(c)[0]:
        t[it.template_id] = t.get(it.template_id, 0) + it.count
    return t


def t_delete_frees_slot():
    c = account()
    ids = []
    chars, tMax = listing(c)
    check(tMax >= 2, f"max_slots {tMax} < 2, cannot test")
    for _ in range(tMax):
        ids.append(new_char(c))
    r = c.create_char(rand_name("c"), 1, 1)
    check(r is not None and r.character_create_response.status != 0, f"created beyond max_slots={tMax}: {r}")
    chars, _ = listing(c)
    check(len(chars) == tMax, f"list {len(chars)} != {tMax}")
    victim = ids[0]
    check(c.delete_character(victim[0], victim[1]) == OK, "delete failed")
    chars, _ = listing(c)
    check(len(chars) == tMax - 1 and victim[0] not in chars, f"list after delete {chars}")
    check(all(i[0] in chars for i in ids[1:]), "other characters lost")
    new_char(c)
    chars, _ = listing(c)
    check(len(chars) == tMax, "slot not reusable")


def t_wrong_confirm_name():
    c = account()
    cid, n = new_char(c)
    other, n2 = new_char(c)
    for bad in ("", "wrong", n + "x", n[:-1], n2):
        r = c.delete_character(cid, bad)
        check(r == NAME_MISMATCH, f"confirm '{bad}' expected NAME_MISMATCH(2), got {r}")
    chars, _ = listing(c)
    check(cid in chars and other in chars, "character vanished after bad confirm")
    r = c.delete_character(cid, n.upper())
    check(r == OK, f"case-insensitive confirm expected OK, got {r}")


def t_foreign_and_unknown_id():
    a, b = account(), account()
    cid, n = new_char(a)
    r = b.delete_character(cid, n)
    check(r == NOT_FOUND, f"foreign id expected NOT_FOUND(1), got {r}")
    chars, _ = listing(a)
    check(cid in chars, "foreign delete removed the character")
    for v in (0, 0x7FFFFFFFFFFF, cid + 100000):
        r = b.delete_character(v, n)
        check(r == NOT_FOUND, f"unknown id {v} expected NOT_FOUND, got {r}")


def t_in_world_then_after_exit():
    a = account()
    cid, n = new_char(a)
    r = a.enter(cid)
    check(r is not None and r.enter_world_response.status == 0, f"enter failed {r}")
    time.sleep(1.5)
    r = a.delete_character(cid, n)
    check(r == IN_WORLD, f"in-world session expected IN_WORLD(3), got {r}")
    check(cid in listing(a)[0], "character removed while own session in world")
    b = a
    a = relogin(b.user)
    t0 = time.time()
    r = a.delete_character(cid, n)
    print(f"      delete from a fresh session after kick: status {r} in {time.time() - t0:.1f}s", flush=True)
    check(r == OK, f"delete after the world session was kicked expected OK (server waits for release), got {r}")
    check(cid not in listing(a)[0], "still listed after delete")
    r = a.enter(cid)
    check(r is not None and r.enter_world_response.status == 1, f"enter after delete {r}")


def t_unauthenticated_no_reply():
    c = Client()
    S["all"].append(c)
    r = c.call(lambda q: (setattr(q.delete_character_request, "character_id", 1),
                          setattr(q.delete_character_request, "confirm_name", "abc")), 3.0)
    print(f"      unauthenticated reply: {'none' if r is None else str(r).strip()[:80]}", flush=True)
    check(r is None, f"unauthenticated session got a response: {r}")


def t_enter_deleted_id():
    c = account()
    cid, n = new_char(c)
    check(c.delete_character(cid, n) == OK, "delete failed")
    r = c.enter(cid)
    check(r is not None and r.enter_world_response.status == 1, f"enter deleted expected NOT_FOUND, got {r}")
    d = relogin(c.user)
    r = d.enter(cid)
    check(r is not None and r.enter_world_response.status == 1, f"fresh session enter deleted: {r}")
    check(d.delete_character(cid, n) == NOT_FOUND, "second delete of the same id expected NOT_FOUND")


def t_name_reusable():
    a, b = account(), account()
    cid, n = new_char(a)
    r = b.create_char(n, 1, 1)
    check(r.character_create_response.status == 1, "name not taken while character exists")
    check(a.delete_character(cid, n) == OK, "delete failed")
    cid2, _ = new_char(b, n)
    check(cid2 != cid, "same id reused")
    check(n in listing(b)[0].values(), "recreated name missing")


def t_race_enter_and_delete():
    wins = {"enter": 0, "delete": 0}
    for i in range(6):
        a = account()
        cid, n = new_char(a)
        since = a.mark()

        def fe(q):
            q.enter_world_request.character_id = cid

        def fd(q):
            q.delete_character_request.character_id = cid
            q.delete_character_request.confirm_name = n
        order = (fe, fd) if i % 2 == 0 else (fd, fe)
        ids = [a.send(f) for f in order]
        res = {}
        for f, rid in zip(order, ids):
            r = a.wait(lambda r, rid=rid: r.header.reply_to == rid, 6, since)
            res["enter" if f is fe else "delete"] = r
        e, d = res["enter"], res["delete"]
        eok = e is not None and e.HasField("enter_world_response") and e.enter_world_response.status == 0
        dok = d is not None and d.HasField("delete_character_response") and d.delete_character_response.status == OK
        check(eok != dok, f"iter {i}: enter_ok={eok} delete_ok={dok} enter={e} delete={d}")
        if dok:
            check(listing(a)[0].get(cid) is None, f"iter {i}: deleted but still listed")
        wins["enter" if eok else "delete"] += 1
        a.close()
    print(f"      race winners {wins}", flush=True)


def psql_count(pCid):
    tExe = r"C:\Program Files\PostgreSQL\18\bin\psql.exe"
    if not os.path.exists(tExe):
        return None
    try:
        o = subprocess.run([tExe, "-h", "localhost", "-U", os.environ.get("LAKOT_DB_USER", "postgres"), "-d", "lakot_db", "-tA",
                            "-c", f"SELECT count(*) FROM item_instances WHERE character_id={pCid}"],
                           env={**os.environ, "PGPASSWORD": os.environ["LAKOT_DB_PASSWORD"]}, capture_output=True,
                           text=True, timeout=15)
        return int(o.stdout.strip()) if o.returncode == 0 else None
    except Exception:
        return None


def t_items_deleted_and_fresh_start():
    a = account()
    cid, n = new_char(a)
    w = a
    r = w.enter(cid)
    check(r is not None and r.enter_world_response.status == 0, f"enter {r}")
    check(poll(lambda: totals(w).get(POTION, 0) == 5 and totals(w).get(SWORD, 0) == 1, 8), f"no kit {totals(w)}")
    time.sleep(1.0)
    slot = next(it.slot for it in items_of(w)[0] if it.template_id == POTION and it.location == 0)
    r = w.discard(slot, POTION, 3, destroy=True)
    check(r == 0, f"destroy potions status {r}")
    check(poll(lambda: totals(w).get(POTION, 0) == 2, 5), f"potions {totals(w)}")
    time.sleep(1.0)
    before = psql_count(cid)
    print(f"      item rows before delete: {before}", flush=True)
    w.close()
    a = relogin(w.user)
    t0 = time.time()
    res = None
    while time.time() - t0 < 75:
        res = a.delete_character(cid, n)
        if res == OK:
            break
        time.sleep(2)
    check(res == OK, f"delete not OK (last {res})")
    after = psql_count(cid)
    print(f"      item rows after delete: {after}", flush=True)
    if after is not None:
        check(before and before > 0, f"expected item rows before delete, got {before}")
        check(after == 0, f"{after} item rows remain for deleted character")
    cid2, _ = new_char(a, n)
    w2 = a
    r = w2.enter(cid2)
    check(r is not None and r.enter_world_response.status == 0, f"enter recreated {r}")
    check(poll(lambda: totals(w2).get(POTION, 0) > 0, 8), f"no kit for recreated {totals(w2)}")
    time.sleep(1.5)
    t = totals(w2)
    st = items_of(w2)[1]
    check(t.get(POTION, 0) == 5 and t.get(SWORD, 0) == 1 and t.get(ARMOR, 0) == 1,
          f"recreated char inventory {t} (old state leaked or kit wrong)")
    check(st is not None and st.gold == 100, f"gold {None if st is None else st.gold}")


def t_server_survived():
    srv = S["srv"]
    check(srv.proc.poll() is None, "server exited\n" + "\n".join(srv.log_lines[-15:]))
    bad = [l for l in srv.log_lines if "Assertion" in l or "terminate" in l.lower() or "exception" in l.lower()]
    check(not bad, "exception/assert line in server log: " + (bad[0] if bad else ""))


TESTS = [t_delete_frees_slot, t_wrong_confirm_name, t_foreign_and_unknown_id, t_unauthenticated_no_reply,
         t_enter_deleted_id, t_name_reusable, t_race_enter_and_delete, t_in_world_then_after_exit,
         t_items_deleted_and_fresh_start, t_server_survived]


def main():
    only = sys.argv[1:]
    srv = G30Server()
    srv.start()
    S["srv"] = srv
    res = []
    try:
        for t in TESTS:
            if only and t.__name__ not in only:
                continue
            t0 = time.time()
            try:
                t()
                res.append((t.__name__, True, ""))
            except Exception as e:
                m = str(e) or e.__class__.__name__
                if not isinstance(e, AssertionError):
                    m += " | " + traceback.format_exc().strip().splitlines()[-3].strip()
                res.append((t.__name__, False, m[:400]))
                print("      server log tail:", *srv.log_lines[-8:], sep="\n        ", flush=True)
            print(f"{'PASS' if res[-1][1] else 'FAIL'}  {t.__name__}  ({time.time() - t0:.1f}s) {res[-1][2]}", flush=True)
    finally:
        for c in S.get("all", []):
            try:
                c.close()
            except Exception:
                pass
        srv.stop()
    p = sum(1 for r in res if r[1])
    print(f"\n{p}/{len(res)} passed")
    sys.exit(0 if p == len(res) else 1)


main()
