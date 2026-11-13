import sys, os, time, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lakotnet import *
from mapnav import nav

generate_bindings()


def authoritative(c):
    since = c.mark()
    c.move(float("nan"), 0.0, 0.0)
    end = time.time() + 3.0
    while time.time() < end:
        for s in c.snapshots(since):
            if s.HasField("self_correction"):
                p = s.self_correction.position
                return p.x, p.z
        time.sleep(0.05)
    return None


def run(c, n, x0, z0, dx, dz, step, hz, count):
    since = c.mark()
    t0 = time.time()
    for i in range(1, count + 1):
        x, z = x0 + dx * step * i, z0 + dz * step * i
        c.move(x, n.y_at(x, z), z)
        time.sleep(1.0 / hz)
    elapsed = time.time() - t0
    time.sleep(0.3)
    rejected = sum(1 for s in c.snapshots(since) if s.HasField("self_correction"))
    pos = authoritative(c)
    dist = math.hypot(pos[0] - x0, pos[1] - z0) if pos else -1.0
    return dist, elapsed, rejected, pos


WALK_SPEED = 5.0
BUDGET_CAP = WALK_SPEED * 2.0 + 1.5


def main():
    s = Server()
    s.start()
    failures = []
    try:
        c = make_player(prefix="spd")
        enter_world(c)
        n = nav()
        x0, _, z0 = c.pos
        best = (0.0, 0.0, 1.0)
        for k in range(32):
            a = k * math.pi / 16
            dx, dz = math.sin(a), math.cos(a)
            length = 0.0
            while length < 80.0 and n.segment_ok(x0, z0, x0 + dx * (length + 2.0), z0 + dz * (length + 2.0)):
                length += 2.0
            if length > best[0]:
                best = (length, dx, dz)
        length, dx, dz = best
        if length < 10.0:
            raise RuntimeError(f"no free straight run near spawn ({length} m)")
        time.sleep(1.0)
        for label, step, hz, legit in (("legit 0.5 m @10Hz", 0.5, 10, True), ("cheat 1.9 m @10Hz", 1.9, 10, False), ("cheat 1.5 m @30Hz", 1.5, 30, False)):
            sx, sz = authoritative(c)
            count = max(1, int(min(length * 0.45, 36.0) / step))
            direction = -1 if math.hypot(sx - x0, sz - z0) > 1.0 else 1
            dist, elapsed, rejected, end = run(c, n, sx, sz, dx * direction, dz * direction, step, hz, count)
            limit = WALK_SPEED * elapsed + BUDGET_CAP + 0.5
            print(f"{label}: sent {count} steps in {elapsed:.2f}s -> accepted {dist:.1f} m (limit {limit:.1f}), corrections={rejected}")
            if legit and (rejected > 0 or dist < count * step * 0.9):
                failures.append(f"{label}: honest walk was corrected")
            if dist > limit:
                failures.append(f"{label}: accepted {dist:.1f} m > {limit:.1f} m")
            time.sleep(2.5)
        c.close()
    except Exception:
        import traceback
        traceback.print_exc()
        failures.append("exception")
    finally:
        s.stop()
    print("PASS" if not failures else "FAIL: " + "; ".join(failures))
    sys.exit(0 if not failures else 1)


main()
