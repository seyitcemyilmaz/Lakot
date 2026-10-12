"""Lakot load and latency test (G57).

Bots (spread over several worker processes) register, log in, enter the world and then play
a mixed session: 10 Hz movement, attacks, nearby chat, /pos, potion use and loot pickup.
Measured: request RTT per kind (header id matching), login/enter time, snapshot interval and
size, drops, throttles, server CPU/RAM and host CPU.

Usage:
  python load_test.py --bots 100 --layout spread --duration 60
  python load_test.py --ladder 100,250,500,1000 --layout spread,stacked --duration 60
Exit code is non-zero when drops, request errors or setup failures exceed --max-error-pct.
"""
import sys, os, time, math, random, threading, struct, ctypes, json, argparse, pathlib, subprocess, datetime, traceback
import multiprocessing as mp
from concurrent.futures import ThreadPoolExecutor

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from lakotnet import *
HERE = pathlib.Path(__file__).resolve().parent
os.environ.setdefault("LAKOT_REGISTER_PER_MINUTE", "100000")

KIND_PLAYER, KIND_MONSTER, KIND_NPC, KIND_ITEM = 0, 1, 2, 3
POTION = 4
PICKUP_RANGE = 2.5
STEP = 0.4
RTT_TIMEOUT_S = 10.0
KINDS = ("chat", "pos", "pickup", "use")
HOME_RADIUS = {"spread": (25.0, 30.0), "stacked": (12.0, 16.0)}


def pct(v, p):
    if len(v) == 0:
        return 0.0
    import numpy as np
    return float(np.percentile(np.asarray(v), 100 * p))


class Bot(Client):
    def __init__(self, idx):
        self.idx = idx
        self.live = False
        self.measuring = False
        self.stopping = False
        self.pending = {}
        self.tracked = {}
        self.pick_tried = set()
        self.snap_n = 0
        self.last_snap = None
        self.iv = []
        self.sizes = []
        self.bytes_rx = 0
        self.rtt = {k: [] for k in KINDS}
        self.sent = {k: 0 for k in KINDS}
        self.ok = {k: 0 for k in KINDS}
        self.throttled = {k: 0 for k in KINDS}
        self.errors = {k: 0 for k in KINDS}
        self.timeouts = {k: 0 for k in KINDS}
        self.results = {"pickup": {}, "use": {}}
        self.err_reasons = {}
        self.attacks_sent = 0
        self.attack_evt = []
        self.attack_t = None
        self.corrections = 0
        self.deaths = 0
        self.dead = False
        self.chat_rx = 0
        self.snap_total = 0
        self.dropped = False
        self.cid = 0
        self.pos = (0.0, 0.0, 0.0)
        self.vis_p = set()
        self.vis_m = set()
        self.last_tick = None
        self.gaps = []
        super().__init__()

    def _read(self):
        try:
            while True:
                n = struct.unpack(">I", self._recv(4))[0]
                raw = self._recv(n)
                tr = time.perf_counter()
                m = self.Message()
                m.ParseFromString(raw)
                r = m.response
                if r.HasField("world_snapshot"):
                    self._on_snapshot(r.world_snapshot, tr, n + 4)
                    continue
                if self.live:
                    self._on_reply(r, tr)
                else:
                    with self.cv:
                        self.log.append((time.time(), m))
                        self.cv.notify_all()
        except Exception:
            pass
        if not self.stopping:
            self.dropped = True
        with self.cv:
            self.closed = True
            self.cv.notify_all()

    def _on_reply(self, r, tr):
        which = r.WhichOneof("payload")
        if which == "chat_message_received":
            self.chat_rx += 1
            return
        p = self.pending.pop(r.header.reply_to, None)
        if p is None:
            return
        kind, t0 = p
        code = r.header.status.code
        outcome = "ok"
        if kind == "chat":
            if code != 0:
                msg = r.header.status.message
                outcome = "throttled" if ("quickly" in msg or "another world" in msg) else "error"
        elif kind == "pos":
            st = r.command_response.status
            outcome = "ok" if st == 0 else ("throttled" if st == 4 else "error")
        elif kind == "pickup":
            if which != "pickup_item_response":
                outcome = "error"
            elif self.measuring:
                k = r.pickup_item_response.result
                self.results["pickup"][k] = self.results["pickup"].get(k, 0) + 1
        elif kind == "use":
            if which != "use_item_response":
                outcome = "error"
            elif self.measuring:
                k = r.use_item_response.status
                self.results["use"][k] = self.results["use"].get(k, 0) + 1
        if not self.measuring:
            return
        if outcome == "error":
            key = f"{kind}:code{code}:{r.header.status.message[:40]}:dead={self.dead}"
            self.err_reasons[key] = self.err_reasons.get(key, 0) + 1
        if outcome == "ok":
            self.ok[kind] += 1
            self.rtt[kind].append(tr - t0)
        elif outcome == "throttled":
            self.throttled[kind] += 1
            self.rtt[kind].append(tr - t0)
        else:
            self.errors[kind] += 1

    def _on_snapshot(self, s, tr, size):
        if self.measuring:
            if self.last_snap is not None:
                self.iv.append(tr - self.last_snap)
                if tr - self.last_snap >= 0.5 and self.last_tick is not None:
                    self.gaps.append((tr - self.last_snap, (s.tick - self.last_tick) & 0xFFFFFFFF,
                                      len(self.vis_p), len(self.vis_m), self.dead))
            self.sizes.append(size)
            self.bytes_rx += size
            self.snap_total += 1
        self.last_snap = tr
        self.last_tick = s.tick
        self.snap_n += 1
        for e in s.entered:
            if e.kind == KIND_PLAYER:
                self.vis_p.add(e.entity_id)
            elif e.kind == KIND_MONSTER:
                self.vis_m.add(e.entity_id)
        for i in s.left:
            self.vis_p.discard(i)
            self.vis_m.discard(i)
        tracked = self.tracked
        for e in s.entered:
            k = e.kind
            if k == KIND_MONSTER:
                if not e.is_dead:
                    tracked[e.entity_id] = [k, e.position.x, e.position.z]
            elif k == KIND_ITEM:
                tracked[e.entity_id] = [k, e.position.x, e.position.z]
        for i in s.left:
            tracked.pop(i, None)
        for v in s.vitals:
            if v.is_dead:
                t = tracked.get(v.entity_id)
                if t is not None and t[0] == KIND_MONSTER:
                    del tracked[v.entity_id]
        if tracked and self.snap_n % 10 == 0:
            for e in s.moved:
                t = tracked.get(e.entity_id)
                if t is not None:
                    t[1], t[2] = e.position.x, e.position.z
        if s.HasField("self_correction"):
            c = s.self_correction
            self.pos = (c.position.x, c.position.y, c.position.z)
            self.corrections += self.measuring
        if s.HasField("self_vitals"):
            dead = s.self_vitals.health <= 0
            if dead and not self.dead and self.measuring:
                self.deaths += 1
            self.dead = dead
        if self.attack_t is not None and s.combat:
            for ev in s.combat:
                if ev.attacker_id == self.cid:
                    if self.measuring:
                        self.attack_evt.append(tr - self.attack_t)
                    self.attack_t = None
                    break

    def request(self, kind, fill):
        self.pending[self.next_id] = (kind, time.perf_counter())
        if self.measuring:
            self.sent[kind] += 1
        self.send(fill)

    def sweep(self, now):
        stale = [k for k, (kind, t0) in self.pending.items() if now - t0 > RTT_TIMEOUT_S]
        for k in stale:
            kind, _ = self.pending.pop(k)
            if self.measuring:
                self.timeouts[kind] += 1

    def pending_timeouts(self, now):
        for kind, t0 in list(self.pending.values()):
            if now - t0 > RTT_TIMEOUT_S:
                self.timeouts[kind] += 1


def timed(c, fill, timeout=30.0):
    t0 = time.perf_counter()
    r = c.call(fill, timeout)
    return r, time.perf_counter() - t0


def setup_bot(idx, delay, st):
    time.sleep(max(0.0, delay - time.time()))
    c = None
    phase = "connect"
    try:
        t0 = time.perf_counter()
        c = Bot(idx)
        st["connect"].append(time.perf_counter() - t0)
        user = rand_name("ldv")
        phase = "register"
        r, dt = timed(c, lambda q: (setattr(q.register_request, "username", user),
                                    setattr(q.register_request, "password", "pw12345"),
                                    setattr(q.register_request, "email", user + "@test.local")))
        assert r is not None and r.register_response.register_status == 0, f"{r}"
        st["register"].append(dt)
        phase = "login"
        r, dt = timed(c, lambda q: (setattr(q.login_request, "username", user),
                                    setattr(q.login_request, "password", "pw12345")))
        assert r is not None and r.header.status.code == 0, f"{r}"
        st["login"].append(dt)
        phase = "create"

        def mk(q):
            q.character_create_request.name = rand_name("c")
            q.character_create_request.kingdom = 1
        r, dt = timed(c, mk)
        assert r is not None and r.character_create_response.status == 0, f"{r}"
        c.cid = r.character_create_response.character.character_id
        st["create"].append(dt)
        return c
    except Exception as e:
        st["errors"].append((phase, str(e)[:100]))
        if c is not None:
            c.stopping = True
            c.close()
        return None


def enter_bot(c, delay, st):
    time.sleep(max(0.0, delay - time.time()))
    try:
        r, dt = timed(c, lambda q: setattr(q.enter_world_request, "character_id", c.cid))
        assert r is not None and r.enter_world_response.status == 0, f"{r}"
        e = r.enter_world_response
        c.pos = (e.pos_x, e.pos_y, e.pos_z)
        c.home = (e.pos_x, e.pos_z)
        c.live = True
        st["enter"].append(dt)
        return True
    except Exception as e:
        st["errors"].append(("enter", str(e)[:100]))
        return False


def nearest(tracked, kind, x, z, limit):
    best, bd = None, limit * limit
    for t in tracked.values():
        if t[0] != kind:
            continue
        d = (t[1] - x) ** 2 + (t[2] - z) ** 2
        if d < bd:
            best, bd = t, d
    return best, math.sqrt(bd)


def drive(bots, nav, layout, ev_stop, lag):
    wander = HOME_RADIUS[layout][1]
    rnd = random.Random(os.getpid())
    now = time.perf_counter()
    for b in bots:
        b.yaw = rnd.uniform(-math.pi, math.pi)
        b.goal = None
        b.n_chat = 0
        b.next_attack = now + rnd.uniform(0, 2)
        b.next_chat = now + rnd.uniform(1, 10)
        b.next_pos = now + rnd.uniform(0, 5)
        b.next_use = now + rnd.uniform(3, 20)
        b.tick = rnd.randrange(5)
    tick_s = 0.1
    due = time.perf_counter()
    sweep_at = due + 2.0
    while not ev_stop.is_set():
        start = time.perf_counter()
        if start - due > 0:
            lag.append(start - due) if bots and bots[0].measuring else None
        for b in bots:
            if b.closed:
                continue
            try:
                step_bot(b, nav, rnd, wander, start)
            except Exception:
                b.closed = True
        due += tick_s
        if start > sweep_at:
            for b in bots:
                b.sweep(start)
            sweep_at = start + 2.0
        left = due - time.perf_counter()
        if left > 0:
            ev_stop.wait(left)
        elif left < -1.0:
            due = time.perf_counter()


def step_bot(b, nav, rnd, wander, now):
    x, y, z = b.pos
    hx, hz = b.home
    b.tick += 1
    if b.tick % 5 == 0:
        item, di = nearest(b.tracked, KIND_ITEM, x, z, 25.0)
        if item is not None:
            b.goal = (item, di, KIND_ITEM)
        else:
            mon, dm = nearest(b.tracked, KIND_MONSTER, x, z, 30.0)
            b.goal = (mon, dm, KIND_MONSTER) if mon is not None else None
    yaw = b.yaw
    moving = True
    if b.goal is not None:
        t, _, gk = b.goal
        dx, dz = t[1] - x, t[2] - z
        d = math.hypot(dx, dz)
        yaw = math.atan2(dx, dz)
        stop_at = PICKUP_RANGE - 0.5 if gk == KIND_ITEM else 2.0
        if d < stop_at:
            moving = False
            if gk == KIND_ITEM:
                iid = next((k for k, v in b.tracked.items() if v is t), None)
                if iid is not None and iid not in b.pick_tried:
                    b.pick_tried.add(iid)
                    b.request("pickup", lambda q, i=iid: setattr(q.pickup_item_request, "entity_id", i))
                b.goal = None
    else:
        yaw += rnd.uniform(-0.4, 0.4)
    if math.hypot(x - hx, z - hz) > wander:
        yaw = math.atan2(hx - x, hz - z)
    nx, nz = x, z
    if moving:
        nx, nz = x + math.sin(yaw) * STEP, z + math.cos(yaw) * STEP
        if not nav.walkable(nx, nz):
            yaw += rnd.uniform(1.0, 2.5)
            nx, nz = x, z
    b.yaw = yaw
    ny = nav.y_at(nx, nz)

    def fill(q):
        p = q.player_state_update
        p.position.x, p.position.y, p.position.z, p.yaw = nx, ny, nz, yaw
    b.send(fill)
    b.pos = (nx, ny, nz)
    if now >= b.next_attack:
        b.next_attack = now + rnd.uniform(1.6, 2.4)
        ay = math.atan2(b.goal[0][1] - nx, b.goal[0][2] - nz) if b.goal and b.goal[2] == KIND_MONSTER else yaw

        def fa(q):
            q.attack_request.yaw = ay
            q.attack_request.combo = b.tick % 3
        b.attack_t = time.perf_counter()
        b.send(fa)
        if b.measuring:
            b.attacks_sent += 1
    if now >= b.next_chat:
        b.next_chat = now + rnd.uniform(9, 12)
        b.n_chat += 1
        text = f"hello from bot {b.idx} #{b.n_chat}"
        b.request("chat", lambda q: (setattr(q.chat_message_request, "text", text),
                                     setattr(q.chat_message_request, "is_global", False)))
    if now >= b.next_pos:
        b.next_pos = now + rnd.uniform(4.5, 5.5)
        b.request("pos", lambda q: setattr(q.command_request, "text", "/pos"))
    if now >= b.next_use:
        b.next_use = now + rnd.uniform(12, 18)
        b.request("use", lambda q: setattr(q.use_item_request, "template_id", POTION))


def worker_main(wid, cfg, idxs, q, ev_enter, ev_go, ev_stop):
    try:
        run_worker(wid, cfg, idxs, q, ev_enter, ev_go, ev_stop)
    except BaseException:
        q.put(("fatal", wid, traceback.format_exc()))


def run_worker(wid, cfg, idxs, q, ev_enter, ev_go, ev_stop):
    import numpy as np
    from mapnav import MapNav
    nav = MapNav()
    rate = cfg["rate"] / cfg["workers"]
    st = {k: [] for k in ("connect", "register", "login", "create", "enter", "errors")}
    t0 = time.time() + 0.2
    with ThreadPoolExecutor(24) as ex:
        res = list(ex.map(lambda a: setup_bot(a[1], t0 + a[0] / rate, st), enumerate(idxs)))
    bots = [b for b in res if b]
    q.put(("setup", wid, {"cids": {b.idx: b.cid for b in bots}, "st": st}))
    ev_enter.wait()
    t0 = time.time() + 0.2
    with ThreadPoolExecutor(24) as ex:
        okv = list(ex.map(lambda a: enter_bot(a[1], t0 + a[0] / rate, st), enumerate(bots)))
    for b, ok in zip(bots, okv):
        if not ok:
            b.stopping = True
            b.close()
    bots = [b for b, ok in zip(bots, okv) if ok]
    q.put(("entered", wid, {"pos": {b.idx: b.pos for b in bots}, "st": st}))
    ev_go.wait()
    lag = []
    th = threading.Thread(target=drive, args=(bots, nav, cfg["layout"], ev_stop, lag), daemon=True)
    th.start()
    ev_stop_measure = cfg["warmup"]
    time.sleep(ev_stop_measure)
    for b in bots:
        b.measuring = True
    ev_stop.wait()
    for b in bots:
        b.measuring = False
    th.join(timeout=3)
    now = time.perf_counter()
    for b in bots:
        b.pending_timeouts(now)
    out = {"bots": len(bots), "dropped": sum(b.dropped for b in bots)}
    out["iv"] = np.array([x for b in bots for x in b.iv], dtype=np.float32)
    out["maxgap"] = np.array([max(b.iv) if b.iv else 0.0 for b in bots], dtype=np.float32)
    out["gaps"] = [g for b in bots for g in b.gaps]
    out["sizes"] = np.array([x for b in bots for x in b.sizes], dtype=np.int32)
    out["bytes_rx"] = sum(b.bytes_rx for b in bots)
    out["snaps"] = sum(b.snap_total for b in bots)
    out["lag"] = np.array(lag, dtype=np.float32)
    out["attack_evt"] = np.array([x for b in bots for x in b.attack_evt], dtype=np.float32)
    for k in KINDS:
        out["rtt_" + k] = np.array([x for b in bots for x in b.rtt[k]], dtype=np.float32)
        for f in ("sent", "ok", "throttled", "errors", "timeouts"):
            out[f"{f}_{k}"] = sum(getattr(b, f)[k] for b in bots)
    out["results"] = {k: {} for k in ("pickup", "use")}
    for b in bots:
        for k in out["results"]:
            for rk, rv in b.results[k].items():
                out["results"][k][rk] = out["results"][k].get(rk, 0) + rv
    for f in ("attacks_sent", "corrections", "deaths", "chat_rx"):
        out[f] = sum(getattr(b, f) for b in bots)
    out["err_reasons"] = {}
    for b in bots:
        for k, v in b.err_reasons.items():
            out["err_reasons"][k] = out["err_reasons"].get(k, 0) + v
    out["setup_st"] = st
    q.put(("final", wid, out))
    for b in bots:
        b.stopping = True
        b.close()


class Proc:
    def __init__(self, pid):
        k = ctypes.windll.kernel32
        k.OpenProcess.restype = ctypes.c_void_p
        k.OpenProcess.argtypes = [ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong]
        k.GetProcessTimes.argtypes = [ctypes.c_void_p] + [ctypes.c_void_p] * 4
        self.k = k
        self.h = k.OpenProcess(0x1000, False, pid)
        if not self.h:
            raise OSError(f"OpenProcess({pid}) failed, GetLastError={ctypes.get_last_error() or k.GetLastError()}")

    def secs(self):
        c, e, kt, ut = (ctypes.c_ulonglong() for _ in range(4))
        if not self.k.GetProcessTimes(self.h, ctypes.byref(c), ctypes.byref(e), ctypes.byref(kt), ctypes.byref(ut)):
            raise OSError("GetProcessTimes failed")
        return (kt.value + ut.value) / 1e7

    def mem_mb(self):
        class PMC(ctypes.Structure):
            _fields_ = [("cb", ctypes.c_ulong), ("pf", ctypes.c_ulong), ("peak", ctypes.c_size_t), ("ws", ctypes.c_size_t)] + \
                       [(f"x{i}", ctypes.c_size_t) for i in range(6)]
        p = PMC()
        p.cb = ctypes.sizeof(p)
        ctypes.windll.psapi.GetProcessMemoryInfo.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_ulong]
        if not ctypes.windll.psapi.GetProcessMemoryInfo(self.h, ctypes.byref(p), p.cb):
            raise OSError("GetProcessMemoryInfo failed")
        return p.ws / 1048576


def system_times():
    i, k, u = ctypes.c_ulonglong(), ctypes.c_ulonglong(), ctypes.c_ulonglong()
    if not ctypes.windll.kernel32.GetSystemTimes(ctypes.byref(i), ctypes.byref(k), ctypes.byref(u)):
        raise OSError("GetSystemTimes failed")
    return i.value, k.value + u.value


def psql(sql, check=True):
    exe = os.environ.get("LAKOT_PSQL", r"C:\Program Files\PostgreSQL\18\bin\psql.exe")
    env = {**os.environ, "PGPASSWORD": os.environ.get("LAKOT_DB_PASSWORD", "")}
    r = subprocess.run([exe, "-q", "-v", "ON_ERROR_STOP=1", "-h", os.environ.get("LAKOT_DB_HOST", "localhost"),
                        "-p", os.environ.get("LAKOT_DB_PORT", "5432"), "-U", os.environ.get("LAKOT_DB_USER", "postgres"),
                        "-d", os.environ.get("LAKOT_DB_NAME", "lakot_db")],
                       input=sql, capture_output=True, text=True, env=env, timeout=120)
    if check and r.returncode != 0:
        raise RuntimeError("psql failed: " + r.stderr[-300:])
    return r


def plan_positions(nav, layout, n, seed):
    rnd = random.Random(seed)
    spawns = nav.meta["monsterSpawns"]
    scatter = HOME_RADIUS[layout][0]
    pos = []
    k = 0
    while len(pos) < n:
        s = spawns[0] if layout == "stacked" else spawns[len(pos) % len(spawns)]
        a, d = rnd.uniform(0, 6.2832), scatter * math.sqrt(rnd.random())
        x, z = s["x"] + math.cos(a) * d, s["z"] + math.sin(a) * d
        k += 1
        if k > n * 200:
            raise RuntimeError("no walkable placement")
        if all(nav.walkable(x + dx, z + dz) for dx, dz in ((0, 0), (2, 0), (-2, 0), (0, 2), (0, -2))):
            pos.append((x, nav.y_at(x, z), z, rnd.uniform(-3, 3)))
    return pos


def summarize(a, p):
    return {"p50": 1000 * pct(a, .5), "p95": 1000 * pct(a, .95), "p99": 1000 * pct(a, .99),
            "max": 1000 * float(max(a)) if len(a) else 0.0, "n": int(len(a))}


def run_scenario(layout, n, args, out_dir):
    import numpy as np
    from mapnav import MapNav
    name = f"{layout}-{n}"
    nw = max(1, min(args.max_workers, math.ceil(n / args.bots_per_worker)))
    print(f"=== {name}: {n} bots, {nw} worker processes, {args.rate}/s connect, {args.duration}s window ===", flush=True)
    cfg = {"layout": layout, "rate": float(args.rate), "workers": nw, "warmup": float(args.warmup)}
    nav = MapNav()
    pos = plan_positions(nav, layout, n, 1234 + n)
    srv = Server(args.bin)
    procs, ctx = [], mp.get_context("spawn")
    q = ctx.Queue()
    ev_enter, ev_go, ev_stop = ctx.Event(), ctx.Event(), ctx.Event()
    result = {"scenario": name, "layout": layout, "bots": n, "workers": nw, "duration_s": args.duration,
              "warmup_s": args.warmup, "connect_rate": args.rate, "bin": None, "date": datetime.datetime.now().isoformat(timespec="seconds")}
    fatal = []
    finals, setups, entereds = {}, {}, {}

    def gather(kind, store, timeout):
        end = time.time() + timeout
        while len(store) < nw:
            left = end - time.time()
            if left <= 0:
                raise TimeoutError(f"workers did not report '{kind}' ({len(store)}/{nw})")
            try:
                msg = q.get(timeout=min(2.0, left))
            except Exception:
                if any(not p.is_alive() for p in procs) and q.empty():
                    raise RuntimeError("a worker process died")
                continue
            if msg[0] == "fatal":
                raise RuntimeError("worker fatal:\n" + msg[2])
            {"setup": setups, "entered": entereds, "final": finals}[msg[0]][msg[1]] = msg[2]

    try:
        srv.start()
        result["bin"] = srv.bin_dir
        sp = Proc(srv.proc.pid)
        parts = [list(range(i, n, nw)) for i in range(nw)]
        for w in range(nw):
            p = ctx.Process(target=worker_main, args=(w, cfg, parts[w], q, ev_enter, ev_go, ev_stop), daemon=True)
            p.start()
            procs.append(p)
        wp = [Proc(p.pid) for p in procs]
        t_setup = time.time()
        gather("setup", setups, n / args.rate + 120)
        result["setup_s"] = time.time() - t_setup
        cids = {i: c for s in setups.values() for i, c in s["cids"].items()}
        if cids:
            rows = ",".join(f"({c},{pos[i][0]:.3f},{pos[i][1]:.3f},{pos[i][2]:.3f},{pos[i][3]:.3f})" for i, c in cids.items())
            sql = (f"UPDATE characters c SET map_id=101,pos_x=v.x::real,pos_y=v.y::real,pos_z=v.z::real,yaw=v.w::real "
                   f"FROM (VALUES {rows}) AS v(id,x,y,z,w) WHERE c.id=v.id;\n"
                   f"INSERT INTO item_instances(character_id,template_id,count,location,slot) "
                   f"SELECT id,{POTION},10,0,0 FROM (VALUES {','.join('(%d)' % c for c in cids.values())}) AS t(id);\n")
            psql(sql)
        t_enter = time.time()
        ev_enter.set()
        gather("entered", entereds, n / args.rate + 120)
        result["enter_phase_s"] = time.time() - t_enter
        time.sleep(args.settle)
        reloc = 0
        for e in entereds.values():
            for i, p in e["pos"].items():
                if math.hypot(p[0] - pos[i][0], p[2] - pos[i][2]) > 1.0:
                    reloc += 1
        result["relocated"] = reloc
        ev_go.set()
        time.sleep(args.warmup)
        s0, w0 = sp.secs(), time.time()
        wc0 = [x.secs() for x in wp]
        idle0, tot0 = system_times()
        samples, last = [], (s0, w0)
        end = w0 + args.duration
        while time.time() < end:
            time.sleep(min(1.0, max(0.0, end - time.time())))
            sc, sw = sp.secs(), time.time()
            samples.append((100 * (sc - last[0]) / (sw - last[1]), sp.mem_mb()))
            last = (sc, sw)
        s1, w1 = sp.secs(), time.time()
        wc1 = [x.secs() for x in wp]
        idle1, tot1 = system_times()
        ev_stop.set()
        gather("final", finals, 60)
        cpus = [s[0] for s in samples]
        result["server"] = {"cpu_pct_core_mean": 100 * (s1 - s0) / (w1 - w0), "cpu_pct_core_p95": pct(cpus, .95),
                            "cpu_pct_core_max": max(cpus or [0]), "mem_mb_end": samples[-1][1] if samples else 0,
                            "mem_mb_max": max([s[1] for s in samples] or [0])}
        result["host"] = {"cpu_busy_pct": 100 * (1 - (idle1 - idle0) / max(1, tot1 - tot0)),
                          "workers_cpu_pct_core_total": 100 * sum(b - a for a, b in zip(wc0, wc1)) / (w1 - w0),
                          "workers_cpu_pct_core_max": 100 * max(b - a for a, b in zip(wc0, wc1)) / (w1 - w0)}
        agg = lambda f: np.concatenate([finals[w][f] for w in finals]) if finals else np.array([])
        sm = lambda f: sum(finals[w][f] for w in finals)
        st = {k: [x for w in setups for x in setups[w]["st"][k]] for k in ("connect", "register", "login", "create")}
        st["enter"] = [x for w in entereds for x in entereds[w]["st"]["enter"]]
        errs = [e for w in setups for e in setups[w]["st"]["errors"]] + [e for w in entereds for e in entereds[w]["st"]["errors"]]
        errs = list(dict.fromkeys(errs))
        result["setup"] = {k: summarize(np.array(v), 0) for k, v in st.items()}
        result["setup_errors"] = len(errs)
        result["setup_error_sample"] = errs[:3]
        active = sm("bots")
        iv, sizes, lag = agg("iv"), agg("sizes"), agg("lag")
        result["snapshot"] = {"interval_ms": summarize(iv, 0), "worst_bot_gap_ms_p50": 1000 * pct(agg("maxgap"), .5),
                              "worst_bot_gap_ms_max": 1000 * float(max(agg("maxgap"))) if active else 0,
                              "size_bytes_mean": float(sizes.mean()) if len(sizes) else 0,
                              "size_bytes_p95": pct(sizes, .95), "size_bytes_max": int(sizes.max()) if len(sizes) else 0,
                              "rx_mbit_s_total": 8 * sm("bytes_rx") / args.duration / 1e6,
                              "rate_hz_per_bot": sm("snaps") / max(1, active) / args.duration}
        result["client_drive_lag_ms"] = summarize(lag, 0)
        gl = [g for w in finals for g in finals[w]["gaps"]]
        def gdesc(g):
            return {"gap_ms": round(1000 * g[0]), "tick_delta": g[1], "tick_delta_x50_ms": g[1] * 50,
                    "players_visible": g[2], "monsters_visible": g[3], "dead": bool(g[4])}
        empty = [g for g in gl if g[2] + g[3] == 0 and not g[4]]
        seen = [g for g in gl if g[2] + g[3] > 0 or g[4]]
        result["snapshot_gaps"] = {
            "count_ge_0_5s": len(gl),
            "largest": gdesc(max(gl, key=lambda g: g[0])) if gl else None,
            "empty_view": {"count": len(empty), "count_ge_1s": sum(g[0] >= 1 for g in empty),
                           "largest": gdesc(max(empty, key=lambda g: g[0])) if empty else None},
            "non_empty_view": {"count": len(seen), "count_ge_1s": sum(g[0] >= 1 for g in seen),
                               "largest": gdesc(max(seen, key=lambda g: g[0])) if seen else None,
                               "list_ge_1s": [gdesc(g) for g in sorted(seen, key=lambda g: -g[0]) if g[0] >= 1][:20]},
        }
        req = {}
        tot_sent = tot_err = 0
        for k in KINDS:
            r = summarize(agg("rtt_" + k), 0)
            r.update(sent=sm("sent_" + k), ok=sm("ok_" + k), throttled=sm("throttled_" + k),
                     errors=sm("errors_" + k), timeouts=sm("timeouts_" + k))
            req[k] = r
            tot_sent += r["sent"]
            tot_err += r["errors"] + r["timeouts"]
        att = summarize(agg("attack_evt"), 0)
        att["sent"] = sm("attacks_sent")
        req["attack_to_combat_event"] = att
        result["requests"] = req
        result["results"] = {k: {} for k in ("pickup", "use")}
        for w in finals:
            for k, d in finals[w]["results"].items():
                for rk, rv in d.items():
                    result["results"][k][str(rk)] = result["results"][k].get(str(rk), 0) + rv
        result["error_reasons"] = {}
        for w in finals:
            for k, v in finals[w]["err_reasons"].items():
                result["error_reasons"][k] = result["error_reasons"].get(k, 0) + v
        result["game"] = {"corrections": sm("corrections"), "deaths": sm("deaths"), "chat_received": sm("chat_rx"),
                          "active_bots": active, "dropped": sm("dropped")}
        drop_pct = 100 * (n - active + sm("dropped")) / n
        err_pct = 100 * tot_err / tot_sent if tot_sent else 0.0
        result["error_pct"] = {"drops_and_setup_failures": drop_pct, "request_errors": err_pct}
        fails = []
        if drop_pct > args.max_error_pct:
            fails.append(f"drops/setup failures {drop_pct:.2f}% > {args.max_error_pct}%")
        if err_pct > args.max_error_pct:
            fails.append(f"request errors {err_pct:.2f}% > {args.max_error_pct}%")
        result["failures"] = fails
        result["server_log_tail"] = srv.log_lines[-5:]
        return result
    except BaseException as e:
        result["exception"] = f"{type(e).__name__}: {e}"
        result["failures"] = [result["exception"]]
        result["server_log_tail"] = srv.log_lines[-15:]
        return result
    finally:
        ev_stop.set()
        ev_go.set()
        ev_enter.set()
        for p in procs:
            p.join(timeout=10)
            if p.is_alive():
                p.terminate()
        srv.stop()
        try:
            time.sleep(0.5)
            with open(out_dir / f"{datetime.date.today().isoformat()}-{name}.server.log", "w", encoding="utf-8") as lf:
                lf.write("\n".join(srv.log_lines) + "\n")
        except Exception as e:
            print("server log save failed:", e)
        if not args.keep_db:
            try:
                psql("DELETE FROM accounts WHERE username LIKE 'ldv%';\n", check=False)
            except Exception as e:
                print("db cleanup failed:", e)


def print_summary(r):
    print(f"--- {r['scenario']} ---")
    if "exception" in r:
        print("EXCEPTION:", r["exception"])
        for l in r.get("server_log_tail", [])[-8:]:
            print("  srv|", l)
    if "server" not in r:
        return
    s, h, sn = r["server"], r["host"], r["snapshot"]
    g = r["game"]
    print(f"bots active {g['active_bots']}/{r['bots']} dropped {g['dropped']} setup_errors {r['setup_errors']} relocated {r['relocated']} "
          f"deaths {g['deaths']} corrections {g['corrections']} setup {r['setup_s']:.0f}s")
    print(f"server cpu {s['cpu_pct_core_mean']:.0f}% of one core (p95 {s['cpu_pct_core_p95']:.0f}, max {s['cpu_pct_core_max']:.0f}); "
          f"ram {s['mem_mb_max']:.0f} MB; host busy {h['cpu_busy_pct']:.0f}%; worker procs {h['workers_cpu_pct_core_total']:.0f}% "
          f"(max one {h['workers_cpu_pct_core_max']:.0f}%); client drive lag p99 {r['client_drive_lag_ms']['p99']:.0f} ms")
    iv = sn["interval_ms"]
    print(f"snapshot interval ms p50 {iv['p50']:.0f} p95 {iv['p95']:.0f} p99 {iv['p99']:.0f} max {iv['max']:.0f} | "
          f"size mean {sn['size_bytes_mean']:.0f} p95 {sn['size_bytes_p95']:.0f} max {sn['size_bytes_max']} B | "
          f"{sn['rate_hz_per_bot']:.1f} Hz/bot, rx {sn['rx_mbit_s_total']:.0f} Mbit/s")
    sg = r.get("snapshot_gaps")
    if sg:
        print(f"gaps>=0.5s {sg['count_ge_0_5s']} largest {sg['largest']}")
        print(f"  empty-view gaps {sg['empty_view']['count']} (>=1s {sg['empty_view']['count_ge_1s']}) largest {sg['empty_view']['largest']}")
        print(f"  non-empty-view gaps {sg['non_empty_view']['count']} (>=1s {sg['non_empty_view']['count_ge_1s']}) largest {sg['non_empty_view']['largest']}")
    print(f"{'request':26}{'p50':>8}{'p95':>8}{'p99':>8}{'max':>9}{'sent':>8}{'ok':>8}{'thr':>6}{'err':>6}{'tmo':>6}")
    for k in ("login", "enter", "register", "create"):
        d = r["setup"].get(k)
        if d:
            print(f"{k + ' (setup)':26}{d['p50']:8.1f}{d['p95']:8.1f}{d['p99']:8.1f}{d['max']:9.1f}{d['n']:8d}")
    for k, d in r["requests"].items():
        if k == "attack_to_combat_event":
            print(f"{k:26}{d['p50']:8.1f}{d['p95']:8.1f}{d['p99']:8.1f}{d['max']:9.1f}{d['sent']:8d}{d['n']:8d}  (hits seen)")
        else:
            print(f"{k:26}{d['p50']:8.1f}{d['p95']:8.1f}{d['p99']:8.1f}{d['max']:9.1f}{d['sent']:8d}{d['ok']:8d}{d['throttled']:6d}{d['errors']:6d}{d['timeouts']:6d}")
    print(f"pickup results {r['results']['pickup']}  use results {r['results']['use']}")
    if r["error_reasons"]:
        print("error reasons:", r["error_reasons"])
    print("RESULT:", "FAIL " + "; ".join(r["failures"]) if r["failures"] else "PASS", flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bots", type=int, default=100)
    ap.add_argument("--ladder", help="comma list of bot counts, overrides --bots")
    ap.add_argument("--layout", default="spread", help="spread, stacked or both (comma list ok)")
    ap.add_argument("--duration", type=float, default=60)
    ap.add_argument("--warmup", type=float, default=5)
    ap.add_argument("--settle", type=float, default=3)
    ap.add_argument("--rate", type=float, default=50, help="new connections (and enters) per second")
    ap.add_argument("--bots-per-worker", type=int, default=50)
    ap.add_argument("--max-workers", type=int, default=12)
    ap.add_argument("--max-error-pct", type=float, default=1.0)
    ap.add_argument("--bin", default=None)
    ap.add_argument("--keep-db", action="store_true")
    ap.add_argument("--out", default=str(HERE / "results"))
    args = ap.parse_args()
    args.bin = resolve_bin(args.bin, BIN_RELEASE)
    layouts = ["spread", "stacked"] if args.layout == "both" else args.layout.split(",")
    sizes = [int(x) for x in args.ladder.split(",")] if args.ladder else [args.bots]
    generate_bindings()
    out_dir = pathlib.Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    failed = False
    for layout in layouts:
        for n in sizes:
            r = run_scenario(layout, n, args, out_dir)
            print_summary(r)
            stamp = datetime.date.today().isoformat()
            with open(out_dir / f"{stamp}-{r['scenario']}.json", "w", encoding="utf-8") as f:
                json.dump(r, f, indent=1, default=str)
            failed = failed or bool(r["failures"])
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    mp.freeze_support()
    main()
