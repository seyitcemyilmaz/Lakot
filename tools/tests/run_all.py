import sys, time, math, traceback, threading
from lakotnet import *

generate_bindings()

from common_pb2 import STATUS_OK, STATUS_UNAUTHORIZED, STATUS_INVALID_REQUEST

S = {}
RESULTS = []


def check(cond, msg):
    if not cond:
        raise AssertionError(msg)


def walk_to(c, tx, tz, y, step=3.0, dt=0.7):
    x, z = c.pos[0], c.pos[2]
    while math.hypot(tx - x, tz - z) > 0.01:
        d = math.hypot(tx - x, tz - z)
        k = min(step, d) / d
        nx, nz = x + (tx - x) * k, z + (tz - z) * k
        c.move(nx, y, nz, math.atan2(nx - x, nz - z))
        x, z = nx, nz
        time.sleep(dt)
    c.pos = (x, y, z)


def entities(c, since=0):
    ents = {}
    for s in c.snapshots(since):
        for e in s.entered:
            ents[e.entity_id] = e
    return ents


def t_register_ok():
    c = Client()
    S["u1"] = rand_name("acc")
    S["email1"] = S["u1"] + "@t.local"
    r = c.register(S["u1"], email=S["email1"])
    check(r and r.register_response.register_status == 0, f"got {r}")
    c.close()


def t_register_dup_username():
    c = Client()
    r = c.register(S["u1"], email=rand_name("e") + "@t.local")
    check(r and r.register_response.register_status == 2, f"got {r}")
    c.close()


def t_register_dup_email():
    c = Client()
    r = c.register(rand_name("acc"), email=S["email1"])
    check(r and r.register_response.register_status == 1, f"got {r}")
    c.close()


def t_register_invalid():
    c = Client()
    r = c.register("ab", email="a@t.local")
    check(r and r.header.status.code == STATUS_INVALID_REQUEST, f"short username not rejected: {r}")
    r = c.register(rand_name("acc"), pw="x", email="a@t.local")
    check(r and r.header.status.code == STATUS_INVALID_REQUEST, f"short password not rejected: {r}")
    c.close()


def t_login_ok_and_bad():
    c = Client()
    r = c.login(S["u1"])
    check(r and r.header.status.code == STATUS_OK and r.login_response.token, f"login failed: {r}")
    c.close()
    c = Client()
    r = c.login(S["u1"], "wrongpass")
    check(r and r.header.status.code == STATUS_UNAUTHORIZED and not r.login_response.token, f"bad pw accepted: {r}")
    c.close()


def t_character_flow():
    c = Client()
    c.login(S["u1"])
    r = c.char_list()
    check(r and len(r.character_list_response.characters) == 0 and r.character_list_response.max_slots >= 1, f"{r}")
    name = rand_name("ch")
    r = c.create_char(name, 1)
    check(r and r.character_create_response.status == 0, f"{r}")
    cid = r.character_create_response.character.character_id
    r = c.char_list()
    check([x.character_id for x in r.character_list_response.characters] == [cid], "created char not listed")
    check(r.character_list_response.kingdom == 1, "kingdom not locked")
    S["dupname"], S["dupcid"] = name, cid
    r = c.create_char(name, 1)
    check(r and r.character_create_response.status == 1, f"same-account dup name: {r}")
    r = c.create_char("ab", 1)
    check(r and r.character_create_response.status == 2, f"short name: {r}")
    c.close()


def t_dup_name_other_account():
    c = Client()
    u = rand_name("acc")
    c.register(u)
    c.login(u)
    r = c.create_char(S["dupname"], 1)
    check(r and r.character_create_response.status == 1, f"cross-account dup name: {r}")
    r = c.create_char(rand_name("ch"), 9)
    check(r and r.character_create_response.status == 5, f"invalid kingdom: {r}")
    c.close()


def t_enter_foreign_char():
    c = make_player()
    r = c.enter(S["dupcid"])
    check(r and r.enter_world_response.status == 1 and r.header.status.code == STATUS_UNAUTHORIZED, f"{r}")
    c.close()


def t_enter_world():
    S["A"] = a = make_player(1, "a")
    w = enter_world(a)
    check(w.map_id == 101 and w.stats.max_health > 0, f"{w}")
    s = a.wait(lambda r: r.HasField("world_snapshot"), 5)
    check(s is not None, "no WorldSnapshot after enter")


def t_two_players_see_each_other():
    a = S["A"]
    S["B"] = b = make_player(2, "b")
    enter_world(b)
    ok = set()
    end = time.time() + 8
    while time.time() < end and len(ok) < 2:
        if any(e.name == a.name for e in entities(b).values()):
            ok.add("b_sees_a")
        if any(e.name == b.name for e in entities(a).values()):
            ok.add("a_sees_b")
        time.sleep(0.2)
    check(len(ok) == 2, f"visibility incomplete: {ok}")
    S["idA"] = next(i for i, e in entities(b).items() if e.name == a.name)
    S["idB"] = next(i for i, e in entities(a).items() if e.name == b.name)


def t_whisper():
    a, b = S["A"], S["B"]
    r = a.whisper(b.name, "hello-whisper")
    check(r and r.direct_message_response.error_code == 0, f"{r}")
    got = b.wait(lambda r: r.HasField("direct_message_received") and r.direct_message_received.text == "hello-whisper", 4)
    check(got is not None, "whisper not delivered")
    r = a.whisper("nobody_" + rand_name(), "x")
    check(r and r.direct_message_response.error_code == 2, f"offline target: {r}")


def t_nearby_chat():
    a, b = S["A"], S["B"]
    time.sleep(3)
    r = a.chat("hello-near")
    check(r and r.chat_message_response.error_code == 0, f"{r}")
    got = b.wait(lambda r: r.HasField("chat_message_received") and r.chat_message_received.text == "hello-near", 4)
    check(got is not None and not got.chat_message_received.is_global, "nearby chat not delivered")
    r = a.chat("")
    check(r and r.header.status.code == STATUS_INVALID_REQUEST, f"empty chat accepted: {r}")
    r = a.chat("x" * 300)
    check(r and r.header.status.code == STATUS_INVALID_REQUEST, f"oversize chat accepted: {r}")


def t_chat_rate_limit():
    a = S["A"]
    time.sleep(6)
    res = [a.chat(f"spam{i}") for i in range(12)]
    ok = sum(1 for r in res if r and r.chat_message_response.error_code == 0)
    bad = [r for r in res if r and r.chat_message_response.error_code != 0]
    check(bad and ok <= 7, f"no throttle: ok={ok}")
    check(any("quickly" in r.header.status.message for r in bad), "throttle message missing")


def t_global_cooldown():
    a, b = S["A"], S["B"]
    r1 = b.chat("glob1", True)
    check(r1 and r1.chat_message_response.error_code == 0, f"{r1}")
    got = a.wait(lambda r: r.HasField("chat_message_received") and r.chat_message_received.text == "glob1", 4)
    check(got is not None and got.chat_message_received.is_global, "global not delivered")
    r2 = b.chat("glob2", True)
    check(r2 and r2.chat_message_response.error_code != 0 and "world message" in r2.header.status.message, f"{r2}")


def t_safe_zone_no_damage():
    a, b = S["A"], S["B"]
    x, _, z = a.pos
    bx, _, bz = b.pos
    check(abs(bx - x) < 5 and abs(bz - z) < 5, f"players not together {a.pos} {b.pos}")
    since = a.mark()
    yaw = math.atan2(bx - x, bz - z)
    for _ in range(4):
        a.attack(yaw)
        time.sleep(0.7)
    time.sleep(0.5)
    hits = [h for s in a.snapshots(since) for ev in s.combat for h in ev.hits]
    check(not hits, f"damage dealt inside safe zone: {len(hits)} hits")


def t_pvp_hit_death_respawn():
    a, b = S["A"], S["B"]
    time.sleep(2)
    th = threading.Thread(target=walk_to, args=(b, b.pos[0], 62.0, b.pos[1]))
    th.start()
    walk_to(a, a.pos[0], 62.0, a.pos[1])
    th.join()
    ax, ay, az = a.pos
    b.move(ax, b.pos[1], az + 1.5, 0.0)
    b.pos = (ax, b.pos[1], az + 1.5)
    time.sleep(1.0)
    sa, sb = a.mark(), b.mark()
    hit_seen = False
    killed_at = None
    for i in range(80):
        a.attack(0.0, i % 3)
        time.sleep(0.6)
        mine = [h for s in a.snapshots(sa) for ev in s.combat for h in ev.hits if h.target_id == S["idB"]]
        hit_seen = hit_seen or bool(mine)
        if any(h.killed for h in mine):
            killed_at = time.time()
            break
    check(hit_seen, "no PvP hit registered outside safe zone")
    check(killed_at, "target never died after 80 attacks")
    dead = b.wait(lambda r: r.HasField("world_snapshot") and r.world_snapshot.HasField("self_vitals")
                  and r.world_snapshot.self_vitals.health <= 0, 3, sb)
    check(dead is not None, "victim never received dead self_vitals")
    check(dead.world_snapshot.self_vitals.respawn_seconds > 0, "respawn_seconds not set on death")
    with b.cv:
        death_idx = next(i for i in range(sb, len(b.log)) if b.log[i][1].response is dead) + 1
    rev = b.wait(lambda r: r.HasField("world_snapshot") and r.world_snapshot.HasField("self_vitals")
                 and r.world_snapshot.self_vitals.health > 0, 10, death_idx)
    t = time.time() - killed_at
    check(rev is not None, "victim never revived")
    check(4.0 <= t <= 7.5, f"respawn took {t:.1f}s (expected ~5)")


def t_monster_kill_and_respawn():
    c = make_player(1, "m")
    enter_world(c)
    time.sleep(2)
    mons = {}
    kill = None
    kill_time = None
    respawned = False
    seen = False
    since = c.mark()
    t0 = time.time()
    while time.time() - t0 < 150:
        for s in c.snapshots(since):
            for e in s.entered:
                if e.kind == 1:
                    mons[e.entity_id] = [e.position.x, e.position.z, not e.is_dead, e.template_id]
                    if kill_time and e.entity_id == kill and not e.is_dead and time.time() - kill_time > 3:
                        respawned = True
            if s.HasField("self_correction"):
                c.pos = (s.self_correction.position.x, c.pos[1], s.self_correction.position.z)
            for m in s.moved:
                if m.entity_id in mons:
                    mons[m.entity_id][0], mons[m.entity_id][1] = m.position.x, m.position.z
            for v in s.vitals:
                if v.entity_id in mons:
                    mons[v.entity_id][2] = not v.is_dead
                    if kill_time and v.entity_id == kill and not v.is_dead and time.time() - kill_time > 3:
                        respawned = True
            for ev in s.combat:
                for h in ev.hits:
                    if h.killed and h.target_id in mons and kill is None and ev.attacker_id not in mons:
                        kill, kill_time = h.target_id, time.time()
        since = c.mark()
        if respawned or (kill_time and time.time() - kill_time > 40):
            break
        alive = {i: m for i, m in mons.items() if m[2]}
        x, y, z = c.pos
        if kill is None:
            if alive:
                seen = True
                i, m = min(alive.items(), key=lambda kv: math.hypot(kv[1][0] - x, kv[1][1] - z))
                d = math.hypot(m[0] - x, m[1] - z)
                yaw = math.atan2(m[0] - x, m[1] - z)
                if d > 2.2:
                    st = min(3.0, d - 1.5)
                    nx, nz = x + (m[0] - x) / d * st, z + (m[1] - z) / d * st
                    c.move(nx, y, nz, yaw)
                    c.pos = (nx, y, nz)
                else:
                    c.attack(yaw)
            else:
                nz = min(z + 3.0, 235.0)
                c.move(x, y, nz, 0.0)
                c.pos = (x, y, nz)
        time.sleep(0.7)
    ktpl = mons[kill][3] if kill in mons else None
    srv = S["srv"]
    want_k = f"[Script] monster_killed {c.name} template={ktpl}"
    want_l = f"[Script] level_up {c.name} "
    end = time.time() + 5
    while time.time() < end and not any(want_k in l for l in srv.log_lines):
        time.sleep(0.2)
    got_k = any(want_k in l for l in srv.log_lines)
    S["levelup"] = any(want_l in l for l in srv.log_lines)
    print(f"      script events: monster_killed={got_k} (template={ktpl}) level_up={S['levelup']}", flush=True)
    c.close()
    check(seen, "no monster ever entered view")
    check(kill is not None, "could not kill a monster within 150s")
    check(respawned, "killed monster did not respawn within 40s")
    check(got_k, f"server log lacks: {want_k}")


def _corrections(c, since):
    return [s.self_correction for s in c.snapshots(since) if s.HasField("self_correction")]


def t_vertical_normal_walk_no_correction():
    c = make_player(1, "y")
    enter_world(c)
    time.sleep(1.5)
    x, y, z = c.pos
    since = c.mark()
    for _ in range(6):
        z += 1.0
        c.move(x, y, z, 0.0)
        time.sleep(0.7)
    time.sleep(1.0)
    cor = _corrections(c, since)
    c.close()
    check(not cor, f"{len(cor)} self_correction(s) on a normal walk at server-provided y={y:.2f}")


def t_vertical_offset_rejected():
    c = make_player(1, "y")
    enter_world(c)
    time.sleep(1.5)
    x, y, z = c.pos
    for dy in (5.0, -5.0):
        since = c.mark()
        c.move(x, y + dy, z + 0.5, 0.0)
        got = c.wait(lambda r: r.HasField("world_snapshot") and r.world_snapshot.HasField("self_correction"), 3, since)
        check(got is not None, f"no self_correction for y{dy:+.0f}")
        check(abs(got.world_snapshot.self_correction.position.y - y) < 0.6, "correction y is not the last accepted y")
        time.sleep(0.8)
    since = c.mark()
    c.move(x, y, z + 0.5, 0.0)
    time.sleep(1.5)
    cor = _corrections(c, since)
    c.close()
    check(not cor, "valid move after corrections still rejected")


def t_login_rate_limit():
    c = Client()
    msgs = []
    for _ in range(14):
        r = c.login("nouser_" + rand_name(), "badpass")
        msgs.append(r.header.status.message if r else "<none>")
    check(any("Too many" in m for m in msgs[8:]), f"no lockout after 14 failures: {msgs[-3:]}")
    check(not any("Too many" in m for m in msgs[:5]), "lockout too early")
    c.close()
    time.sleep(62)
    c = Client()
    r = c.login(S["u1"])
    c.close()
    check(r and r.header.status.code == STATUS_OK, "lockout did not expire after 62s")


class Hunter:
    def __init__(self):
        self.A = a = make_player(1, "la")
        self.B = b = make_player(1, "lb")
        enter_world(a)
        enter_world(b)
        self.ptr = {id(a): 0, id(b): 0}
        self.mons = {}
        self.players = set()
        self.drops = {}
        self.used = set()
        self.left_t = {}
        self.hits = []
        self.vitals = []
        time.sleep(2)
        self.ingest()

    def ingest(self):
        for c in (self.A, self.B):
            with c.cv:
                new = [m.response for _, m in c.log[self.ptr[id(c)]:] if m.response.HasField("world_snapshot")]
                self.ptr[id(c)] = len(c.log)
            for r in new:
                s = r.world_snapshot
                now = time.time()
                for e in s.entered:
                    if e.kind == 1:
                        self.mons[e.entity_id] = [e.position.x, e.position.z, not e.is_dead]
                    elif e.kind == 3:
                        if e.entity_id not in self.drops and e.entity_id not in self.left_t:
                            self.drops[e.entity_id] = dict(e=e, t=now, x=e.position.x, z=e.position.z)
                    elif e.kind == 0:
                        self.players.add(e.entity_id)
                if s.HasField("self_correction"):
                    c.pos = (s.self_correction.position.x, c.pos[1], s.self_correction.position.z)
                for m in s.moved:
                    if m.entity_id in self.mons:
                        self.mons[m.entity_id][0], self.mons[m.entity_id][1] = m.position.x, m.position.z
                for v in s.vitals:
                    if v.entity_id in self.mons:
                        self.mons[v.entity_id][2] = not v.is_dead
                    self.vitals.append(v.entity_id)
                for ev in s.combat:
                    for h in ev.hits:
                        self.hits.append(h.target_id)
                for i in s.left:
                    self.left_t[i] = now
                    self.drops.pop(i, None)

    def nearest(self):
        x, y, z = self.A.pos
        alive = {i: m for i, m in self.mons.items() if m[2]}
        if not alive:
            return None
        return min(alive.items(), key=lambda kv: math.hypot(kv[1][0] - x, kv[1][1] - z))

    def step(self, attack_only=False):
        self.ingest()
        a, b = self.A, self.B
        x, y, z = a.pos
        n = self.nearest()
        if n:
            i, m = n
            d = math.hypot(m[0] - x, m[1] - z)
            yaw = math.atan2(m[0] - x, m[1] - z)
            if d <= 2.2:
                a.attack(yaw)
            elif not attack_only:
                st = min(3.0, d - 1.5)
                nx, nz = x + (m[0] - x) / d * st, z + (m[1] - z) / d * st
                a.move(nx, y, nz, yaw)
                a.pos = (nx, y, nz)
        elif not attack_only:
            nz = min(z + 3.0, 235.0)
            a.move(x, y, nz, 0.0)
            a.pos = (x, y, nz)
        bx, by, bz = b.pos
        d = math.hypot(a.pos[0] - bx, a.pos[2] - bz)
        if d > 2.0 and not attack_only:
            st = min(3.0, d - 1.0)
            nx, nz = bx + (a.pos[0] - bx) / d * st, bz + (a.pos[2] - bz) / d * st
            b.move(nx, by, nz, 0.0)
            b.pos = (nx, by, nz)
        time.sleep(0.7)

    def fresh_drop(self, max_age=3.0, timeout=240):
        end = time.time() + timeout
        while time.time() < end:
            self.step()
            for i, d in list(self.drops.items()):
                if i in self.used:
                    continue
                self.used.add(i)
                if time.time() - d["t"] <= max_age:
                    return i, d
        raise AssertionError("no fresh ground item dropped within timeout")

    def go(self, c, x, z):
        walk_to(c, x, z, c.pos[1])

    def wait_until(self, t_abs):
        while time.time() < t_abs:
            self.step(attack_only=True)


def hunter():
    if "H" not in S:
        S["H"] = Hunter()
    return S["H"]


def inv_update_since(c, mark, timeout=3.0):
    return c.wait(lambda r: r.HasField("inventory_update"), timeout, mark)


def t_loot_owner_pickup_and_not_owner():
    h = hunter()
    i, d = h.fresh_drop()
    e = d["e"]
    check(e.kind == 3 and (e.template_id == 0 and e.gold > 0 or e.template_id != 0 and e.count > 0), f"bad ground item spawn {e}")
    h.go(h.A, d["x"], d["z"])
    h.go(h.B, d["x"], d["z"])
    check(time.time() - d["t"] < 9.0, "setup too slow for ownership window")
    rb = h.B.pickup(i)
    check(rb == 2, f"non-owner inside window expected NOT_OWNER(2), got {rb}")
    mark = h.A.mark()
    ra = h.A.pickup(i)
    check(ra == 0, f"owner expected OK, got {ra}")
    check(inv_update_since(h.A, mark) is not None, "no InventoryUpdate after owner pickup")


def t_loot_non_owner_after_window():
    h = hunter()
    i, d = h.fresh_drop()
    h.go(h.B, d["x"], d["z"])
    h.wait_until(d["t"] + 11.5)
    mark = h.B.mark()
    rb = h.B.pickup(i)
    check(rb == 0, f"non-owner after 10s expected OK, got {rb}")
    check(inv_update_since(h.B, mark) is not None, "no InventoryUpdate after non-owner pickup")


def t_loot_too_far():
    h = hunter()
    i, d = h.fresh_drop()
    h.go(h.A, d["x"] + 6.0, d["z"])
    r = h.A.pickup(i)
    check(r == 3, f"6 units away expected TOO_FAR(3), got {r}")
    if time.time() - d["t"] < 9.0:
        h.go(h.A, d["x"], d["z"])
        r = h.A.pickup(i)
        check(r == 0, f"after TOO_FAR the owner could not pick up: {r}")


def t_loot_forged_ids():
    h = hunter()
    h.ingest()
    check(h.mons, "no monster seen to forge with")
    mon = next(iter(h.mons))
    ply = next(iter(h.players)) if h.players else h.A.cid
    bad = {"monster": mon, "player": ply, "own character": h.A.cid, "zero": 0, "random": 0x1234567890,
           "random huge": 0xFFFFFFFFFFFF0001, "monster id range": (1 << 62) + 99999}
    for name, v in bad.items():
        r = h.A.pickup(v)
        check(r == 1, f"forged {name} id {v} expected NOT_FOUND(1), got {r}")


def t_loot_double_pickup():
    h = hunter()
    i, d = h.fresh_drop()
    h.go(h.A, d["x"], d["z"])
    since = h.A.mark()
    ids = [h.A.send(lambda q: setattr(q.pickup_item_request, "entity_id", i)) for _ in range(2)]
    res = []
    for rid in ids:
        r = h.A.wait(lambda r, rid=rid: r.header.reply_to == rid, 5, since)
        check(r is not None, "no pickup response")
        res.append(r.pickup_item_response.result)
    check(res.count(0) == 1, f"same id twice expected exactly one OK, got {res}")


def t_loot_despawn_and_no_damage():
    h = hunter()
    i, d = h.fresh_drop()
    for k in range(6):
        yaw = math.atan2(d["x"] - h.A.pos[0], d["z"] - h.A.pos[2])
        h.A.attack(yaw)
        h.step(attack_only=True)
    t_end = d["t"] + 120
    while time.time() < t_end and i not in h.left_t:
        h.step(attack_only=True)
    check(i not in h.hits and i not in h.vitals, "ground item was hit / got vitals")
    check(i in h.left_t, "ground item never appeared in `left` within 120s")
    age = h.left_t[i] - d["t"]
    check(55 <= age <= 70, f"despawn after {age:.1f}s, expected about 60s")


TESTS = [t_register_ok, t_register_dup_username, t_register_dup_email, t_register_invalid, t_login_ok_and_bad,
         t_character_flow, t_dup_name_other_account, t_enter_foreign_char, t_enter_world,
         t_two_players_see_each_other, t_whisper, t_nearby_chat, t_chat_rate_limit, t_global_cooldown,
         t_safe_zone_no_damage, t_pvp_hit_death_respawn, t_monster_kill_and_respawn,
         t_vertical_normal_walk_no_correction, t_vertical_offset_rejected, t_login_rate_limit,
         t_loot_owner_pickup_and_not_owner, t_loot_non_owner_after_window, t_loot_too_far, t_loot_forged_ids,
         t_loot_double_pickup, t_loot_despawn_and_no_damage]


def main():
    srv = Server()
    srv.start()
    S["srv"] = srv
    try:
        only = sys.argv[1:]
        for t in TESTS:
            if only and t.__name__ not in only:
                continue
            t0 = time.time()
            try:
                t()
                RESULTS.append((t.__name__, True, ""))
            except Exception as e:
                msg = str(e) or e.__class__.__name__
                if not isinstance(e, AssertionError):
                    msg += " | " + traceback.format_exc().strip().splitlines()[-3].strip()
                RESULTS.append((t.__name__, False, msg[:300]))
            print(f"{'PASS' if RESULTS[-1][1] else 'FAIL'}  {t.__name__}  ({time.time() - t0:.1f}s) {RESULTS[-1][2]}", flush=True)
    finally:
        for k in ("A", "B"):
            if k in S:
                S[k].close()
        if "H" in S:
            S["H"].A.close()
            S["H"].B.close()
        srv.stop()
    p = sum(1 for r in RESULTS if r[1])
    print(f"\n{p}/{len(RESULTS)} passed")
    if p != len(RESULTS):
        print("--- server log tail ---")
        print("\n".join(srv.log_lines[-25:]))
    sys.exit(0 if p == len(RESULTS) else 1)


main()
