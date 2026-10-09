import sys, os, time, math, random, threading, struct, ctypes, statistics, pathlib
from concurrent.futures import ThreadPoolExecutor
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
from lakotnet import *

generate_bindings()
from common_pb2 import STATUS_OK


class Bot(Client):
    def __init__(self):
        super().__init__()
        self.snap_t = []
        self.snap_bytes = []
        self.parse_ms = []

    def _read(self):
        try:
            while True:
                n = struct.unpack(">I", self._recv(4))[0]
                raw = self._recv(n)
                tp = time.perf_counter()
                m = self.Message()
                m.ParseFromString(raw)
                now = time.time()
                tpm = (time.perf_counter() - tp) * 1000
                with self.cv:
                    if m.response.HasField("world_snapshot"):
                        self.snap_t.append(now)
                        self.snap_bytes.append(n + 4)
                        self.parse_ms.append(tpm)
                        if len(self.log) > 400:
                            del self.log[:200]
                    else:
                        self.log.append((now, m))
                    self.cv.notify_all()
        except Exception:
            pass
        with self.cv:
            self.closed = True
            self.cv.notify_all()


def setup_bot(i, reg_times, errors):
    try:
        c = Bot()
        user = rand_name("ld")
        t0 = time.time()
        r = c.register(user)
        reg_times.append(time.time() - t0)
        assert r and r.register_response.register_status == 0, f"register {r}"
        r = c.login(user)
        assert r and r.header.status.code == STATUS_OK, f"login {r}"
        r = c.create_char(rand_name("c"), 1 + i % 3)
        assert r and r.character_create_response.status == 0, f"create {r}"
        c.cid = r.character_create_response.character.character_id
        c.name = ""
        enter_world(c)
        return c
    except Exception as e:
        errors.append(str(e)[:120])
        return None


def drive(c, stop, seed):
    rnd = random.Random(seed)
    x, y, z = c.pos
    ox, oz = x, z
    yaw = rnd.uniform(0, 6.28)
    k = 0
    while not stop.is_set():
        yaw += rnd.uniform(-0.6, 0.6)
        nx, nz = x + math.sin(yaw) * 2.5, z + math.cos(yaw) * 2.5
        if math.hypot(nx - ox, nz - oz) > 25:
            yaw += math.pi
            nx, nz = x + math.sin(yaw) * 2.5, z + math.cos(yaw) * 2.5
        try:
            c.move(nx, y, nz, yaw)
            k += 1
            if k % 4 == 0:
                c.attack(yaw, k % 3)
        except Exception:
            return
        x, z = nx, nz
        stop.wait(0.7 + rnd.uniform(0, 0.1))


class Cpu:
    def __init__(self, pid):
        k = ctypes.windll.kernel32
        k.OpenProcess.restype = ctypes.c_void_p
        self.h = ctypes.c_void_p(k.OpenProcess(0x1000, False, pid))
        self.k = k

    def secs(self):
        c, e, kt, ut = (ctypes.c_ulonglong() for _ in range(4))
        self.k.GetProcessTimes(self.h, ctypes.byref(c), ctypes.byref(e), ctypes.byref(kt), ctypes.byref(ut))
        return (kt.value + ut.value) / 1e7

    def mem_mb(self):
        class PMC(ctypes.Structure):
            _fields_ = [("cb", ctypes.c_ulong), ("pf", ctypes.c_ulong), ("peak", ctypes.c_size_t), ("ws", ctypes.c_size_t)] + \
                       [(f"x{i}", ctypes.c_size_t) for i in range(6)]
        p = PMC()
        p.cb = ctypes.sizeof(p)
        ctypes.windll.psapi.GetProcessMemoryInfo(self.h, ctypes.byref(p), p.cb)
        return p.ws / 1048576


def pct(v, p):
    v = sorted(v)
    return v[min(len(v) - 1, int(len(v) * p))] if v else 0


def run(n, duration, conc, bin_dir=None):
    srv = Server(bin_dir)
    srv.start()
    cpu = Cpu(srv.proc.pid)
    bots, errors, reg_times = [], [], []
    t0 = time.time()
    with ThreadPoolExecutor(conc) as ex:
        res = list(ex.map(lambda i: setup_bot(i, reg_times, errors), range(n)))
    bots = [b for b in res if b]
    setup_s = time.time() - t0
    stop = threading.Event()
    ths = [threading.Thread(target=drive, args=(b, stop, i), daemon=True) for i, b in enumerate(bots)]
    time.sleep(3)
    for b in bots:
        b.snap_t.clear()
        b.snap_bytes.clear()
        b.parse_ms.clear()
    cpu0, w0 = cpu.secs(), time.time()
    for t in ths:
        t.start()
    time.sleep(duration)
    cpu1, w1 = cpu.secs(), time.time()
    mem = cpu.mem_mb()
    stop.set()
    ivals, sizes, rates, gaps = [], [], [], []
    for b in bots:
        t = list(b.snap_t)
        sizes += list(b.snap_bytes)
        ivals += [t[i + 1] - t[i] for i in range(len(t) - 1)]
        if len(t) > 1:
            rates.append((len(t) - 1) / (t[-1] - t[0]))
        gaps.append(max([t[i + 1] - t[i] for i in range(len(t) - 1)] or [0]))
    parse = [x for b in bots for x in b.parse_ms]
    parse_busy = [100 * sum(b.parse_ms) / 1000 / duration for b in bots]
    alive = sum(1 for b in bots if not b.closed)
    logtail = [l for l in srv.log_lines if "tick" in l.lower()][-3:]
    for b in bots:
        b.close()
    srv.stop()
    out = dict(n=n, ok=len(bots), alive=alive, setup_s=setup_s,
               reg_per_s=len(bots) / setup_s, reg_med_ms=1000 * statistics.median(reg_times) if reg_times else 0,
               snap_hz=statistics.mean(rates) if rates else 0,
               iv_p50=1000 * pct(ivals, .5), iv_p95=1000 * pct(ivals, .95), iv_p99=1000 * pct(ivals, .99),
               iv_max=1000 * max(gaps or [0]), size_mean=statistics.mean(sizes) if sizes else 0,
               size_max=max(sizes or [0]), cpu_pct=100 * (cpu1 - cpu0) / (w1 - w0), mem_mb=mem,
               parse_p50_ms=pct(parse, .5), parse_p99_ms=pct(parse, .99), parse_max_ms=max(parse or [0]),
               parse_busy_pct_mean=statistics.mean(parse_busy) if parse_busy else 0,
               parse_busy_pct_total=sum(parse_busy), bin=srv.bin_dir,
               errors=len(errors), err_sample=errors[:2], tick_log=logtail)
    return out


if __name__ == "__main__":
    args = sys.argv[1:]
    bin_arg = None
    if "--bin" in args:
        i = args.index("--bin")
        bin_arg = args[i + 1]
        del args[i:i + 2]
    n = int(args[0]) if len(args) > 0 else 100
    dur = float(args[1]) if len(args) > 1 else 30
    conc = int(args[2]) if len(args) > 2 else 16
    r = run(n, dur, conc, resolve_bin(bin_arg, BIN_RELEASE))
    for k, v in r.items():
        print(f"{k}={v:.1f}" if isinstance(v, float) else f"{k}={v}")
