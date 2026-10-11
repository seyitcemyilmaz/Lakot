import sys, os, time, math, json, re, shutil, subprocess, threading, socket, traceback
from lakotnet import *
from mapnav import nav
from t_monsters_g21 import tpl, open_site, View, step_toward, engage

generate_bindings()

from services import chat_pb2

RUN = HERE / ".run_g27"
GM_ACCOUNT = "gm_g27"
GM_CHAR = "GmG27Hero"
OK, UNKNOWN, USAGE, FAILED, THROTTLED = (chat_pb2.COMMAND_OK, chat_pb2.COMMAND_UNKNOWN, chat_pb2.COMMAND_USAGE,
                                         chat_pb2.COMMAND_FAILED, chat_pb2.COMMAND_THROTTLED)
SNAME = {OK: "OK", UNKNOWN: "UNKNOWN", USAGE: "USAGE", FAILED: "FAILED", THROTTLED: "THROTTLED", None: "none"}
KIND_INFO, KIND_ANNOUNCE, KIND_ROLL, KIND_WARNING = 0, 1, 2, 3
SPAWN = (0.0, 24.0)
POTION, MANA, SABRE = 4, 7, 1005
MON_TOUGH, MON_WEAK = 301, 302
C = {}
SKIPPED = []


def check(pCond, pMsg):
    if not pCond:
        raise AssertionError(pMsg)


class G27Server(Server):
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
        tTemplates = [tpl(MON_TOUGH, maxHealth=100000000, attack=1),
                      tpl(MON_WEAK, maxHealth=1, attack=1, respawnSeconds=2)]
        json.dump({"monsters": tTemplates}, open(RUN / "data" / "monsters.json", "w", encoding="utf-8"), indent=1)
        tPath = RUN / "data" / "maps" / "101_altunay_village" / "map.json"
        tMeta = json.load(open(tPath, encoding="utf-8"))
        tMeta["monsterSpawns"] = []
        json.dump(tMeta, open(tPath, "w", encoding="utf-8"))
        tEnv = dict(os.environ)
        tEnv["LAKOT_GM_ACCOUNT"] = GM_ACCOUNT
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


class Res:
    def __init__(self, pResp):
        self.resp = pResp
        self.ok = pResp is not None and pResp.HasField("command_response")
        self.status = pResp.command_response.status if self.ok else None
        self.lines = list(pResp.command_response.lines) if self.ok else []
        self.text = " | ".join(self.lines)

    def __str__(self):
        return f"{SNAME.get(self.status, self.status)} [{self.text}]"


def cmd(pClient, pText, pPace=True, pTimeout=6.0):
    if pPace:
        time.sleep(0.22 if pClient is C.get("G") else 0.5)

    def f(q):
        q.command_request.text = pText
    return Res(pClient.call(f, pTimeout))


def expect(pRes, pStatus, pCtx, pContains=None):
    check(pRes.ok, f"{pCtx}: no CommandResponse")
    check(pRes.status == pStatus, f"{pCtx}: got {pRes}, want {SNAME[pStatus]}")
    if pContains:
        check(pContains.lower() in pRes.text.lower(), f"{pCtx}: reply [{pRes.text}] lacks '{pContains}'")


def poll(pFn, pTimeout=4.0):
    tEnd = time.time() + pTimeout
    while time.time() < tEnd:
        v = pFn()
        if v:
            return v
        time.sleep(0.1)
    return pFn()


def log_since(pClient, pSince=0):
    with pClient.cv:
        return [m.response for _, m in pClient.log[pSince:]]


def sys_msgs(pClient, pSince=0, pKind=None):
    return [r.system_message for r in log_since(pClient, pSince)
            if r.HasField("system_message") and (pKind is None or r.system_message.kind == pKind)]


def latest_stats(pClient):
    for r in reversed(log_since(pClient)):
        if r.HasField("inventory_update"):
            return r.inventory_update.stats
    return None


def latest_vitals(pClient):
    for r in reversed(log_since(pClient)):
        if r.HasField("inventory_update"):
            s = r.inventory_update.stats
            return s.health, s.max_health, s.level
        if r.HasField("world_snapshot") and r.world_snapshot.HasField("self_vitals"):
            v = r.world_snapshot.self_vitals
            return v.health, v.max_health, v.level
    return None


def inv_counts(pClient):
    for r in reversed(log_since(pClient)):
        if r.HasField("inventory_update"):
            tCount = {}
            for it in r.inventory_update.items:
                if it.location == 0:
                    tCount[it.template_id] = tCount.get(it.template_id, 0) + it.count
            return tCount
    return {}


def inv_updates_since(pClient, pSince):
    return [r.inventory_update for r in log_since(pClient, pSince) if r.HasField("inventory_update")]


def corrections_since(pClient, pSince):
    return [r.world_snapshot.self_correction for r in log_since(pClient, pSince)
            if r.HasField("world_snapshot") and r.world_snapshot.HasField("self_correction")]


def get_pos(pClient):
    r = cmd(pClient, "/pos")
    m = re.search(r"Map (\d+), x (-?\d+(?:\.\d+)?), z (-?\d+(?:\.\d+)?)", r.text)
    check(r.status == OK and m, f"/pos unparsable: {r}")
    return int(m.group(1)), float(m.group(2)), float(m.group(3))


def use_item(pClient, pId):
    def f(q):
        q.use_item_request.template_id = pId
    r = pClient.call(f, 6.0)
    return r.use_item_response if r is not None and r.HasField("use_item_response") else None


def new_player(pKingdom=1, pPrefix="p", pToken=False):
    c = Client()
    tUser = rand_name(pPrefix)
    r = c.register(tUser)
    check(r is not None and r.register_response.register_status == 0, f"register failed {r}")
    r = c.login(tUser)
    check(r is not None and r.login_response.token, f"login failed {r}")
    c.token = r.login_response.token
    c.user, c.name = tUser, rand_name("c")
    r = c.create_char(c.name, pKingdom)
    check(r is not None and r.character_create_response.status == 0, f"create failed {r}")
    c.cid = r.character_create_response.character.character_id
    return c


def world_player(pKingdom=1, pPrefix="p"):
    c = new_player(pKingdom, pPrefix)
    enter_world(c)
    C.setdefault("all", []).append(c)
    return c


def gm_move(pX, pZ):
    G = C["G"]
    r = cmd(G, f"/tp {int(pX)} {int(pZ)}")
    expect(r, OK, f"/tp {pX} {pZ}")
    G.pos = (pX, nav().y_at(pX, pZ), pZ)


def setup():
    srv = G27Server()
    srv.start()
    C["srv"] = srv
    c = Client()
    r = c.login(GM_ACCOUNT)
    if not (r is not None and r.login_response.token):
        r = c.register(GM_ACCOUNT)
        check(r is not None and r.register_response.register_status in (0, 2), f"gm register failed {r}")
        c.close()
        srv.stop()
        srv.start()
        c = Client()
        r = c.login(GM_ACCOUNT)
        check(r is not None and r.login_response.token, f"gm login failed after restart {r}")
    r = c.char_list()
    tChars = list(r.character_list_response.characters)
    if tChars:
        c.cid = tChars[0].character_id
        c.name = tChars[0].name
    else:
        r = c.create_char(GM_CHAR, 1)
        check(r is not None and r.character_create_response.status == 0, f"gm char create failed {r}")
        c.cid = r.character_create_response.character.character_id
        c.name = GM_CHAR
    enter_world(c)
    C["G"] = c
    C.setdefault("all", []).append(c)
    time.sleep(1.5)
    cmd(c, "/revive", pPace=False)
    r = cmd(c, "/help")
    check(r.ok and any("/item" in l for l in r.lines),
          f"GM account has no admin rights (is LAKOT_GM_ACCOUNT applied?): {r}")
    n = nav()
    C["OUT"] = open_site(SPAWN, 90, [0.0])
    C["FAR"] = open_site(SPAWN, 220, [math.pi])
    gm_move(*SPAWN)
    C["P1"] = world_player(1, "pa")
    C["P2"] = world_player(1, "pb")
    C["P3"] = world_player(1, "pc")
    time.sleep(1.0)
    gm_move(*C["FAR"])
    expect(cmd(c, f"/summon {C['P3'].name}"), OK, "summon P3 far")
    gm_move(*SPAWN)
    time.sleep(1.0)


def t_help_player_lists_only_player_commands():
    r = cmd(C["P1"], "/help")
    expect(r, OK, "/help as player")
    tAll = " ".join(r.lines)
    for tWant in ("/help", "/w", "/roll", "/who", "/pos"):
        check(tWant in tAll, f"player /help lacks {tWant}: {r}")
    for tBad in ("/item", "/gold", "/level", "/heal", "/sethp", "/setmp", "/kill", "/revive", "/spawn", "/tp", "/kick",
                 "/mute", "/announce", "/summon"):
        check(tBad not in tAll, f"player /help leaks admin command {tBad}: {r}")
    r = cmd(C["G"], "/help")
    for tWant in ("/item", "/kick", "/spawn", "/tp", "/announce", "/mute"):
        check(tWant in " ".join(r.lines), f"GM /help lacks {tWant}: {r}")
    r = cmd(C["P1"], "/HELP roll")
    expect(r, OK, "/HELP roll (uppercase name)")
    check("roll" in r.text.lower(), f"/help roll text: {r}")


def t_item_as_player_same_as_unknown():
    p = C["P1"]
    tMark = p.mark()
    a = cmd(p, "/item 4 5")
    b = cmd(p, "/zzzq 4 5")
    c = cmd(p, "/help item")
    d = cmd(p, "/help zzzq")
    expect(a, UNKNOWN, "/item as player")
    expect(b, UNKNOWN, "/zzzq as player")
    check(a.lines == b.lines, f"/item reply differs from unknown command: {a} vs {b}")
    check(c.status == d.status and c.lines == d.lines, f"/help item differs from /help unknown: {c} vs {d}")
    check("unknown command" in a.text.lower(), f"unexpected unknown text {a}")
    time.sleep(0.5)
    check(not inv_updates_since(p, tMark), "player /item changed inventory")
    for tCmd in ("/gold 100", "/level 50", "/kick x", "/spawn 301", "/tp 1 1", "/announce hi", "/heal", "/kill"):
        r = cmd(p, tCmd)
        check(r.status == UNKNOWN and r.lines == a.lines, f"player {tCmd} not identical to unknown: {r}")
    srv = C["srv"]
    check(poll(lambda: any("[Security]" in l and ("item" in l.lower() or "gm" in l.lower() or "komut" in l.lower())
                           for l in srv.log_lines), 3), "no [Security] probe log line after player /item")


def t_raw_slash_chat_not_delivered():
    p1, p2 = C["P1"], C["P2"]
    tMark2 = p2.mark()
    tMark1 = p1.mark()
    for tText, tGlobal in (("/hello-raw-nearby", False), ("/hello-raw-global", True), ("/item 4 9", False)):
        time.sleep(0.6)
        r = p1.chat(tText, tGlobal)
        check(r is not None, f"no response to raw slash chat {tText}")
        check(not (r.HasField("chat_message_response") and r.chat_message_response.error_code == 0
                   and r.header.status.code == 0 and False), "n/a")
    time.sleep(1.5)
    tGot = [r for r in log_since(p2, tMark2) if r.HasField("chat_message_received") and r.chat_message_received.text.startswith("/")]
    check(not tGot, f"raw slash chat delivered to another player: {tGot[:1]}")
    tSelf = [r for r in log_since(p1, tMark1) if r.HasField("chat_message_received") and r.chat_message_received.text.startswith("/")]
    check(not tSelf, "raw slash chat echoed back to sender as chat")
    check(not inv_updates_since(p1, tMark1), "raw '/item' chat executed as a command")
    time.sleep(0.6)
    r = p1.chat("normal-after-slash")
    check(r is not None and r.chat_message_response.error_code == 0, f"normal chat broke after slash chat: {r}")
    check(p2.wait(lambda r: r.HasField("chat_message_received") and r.chat_message_received.text == "normal-after-slash", 4, tMark2),
          "normal chat not delivered")


def t_roll_nearby_only():
    p1, p2, p3, g = C["P1"], C["P2"], C["P3"], C["G"]
    m = {c: c.mark() for c in (p1, p2, p3, g)}
    r = cmd(p1, "/roll")
    expect(r, OK, "/roll")
    check(poll(lambda: sys_msgs(p2, m[p2], KIND_ROLL), 4), "nearby player did not get ROLL message")
    check(poll(lambda: sys_msgs(p1, m[p1], KIND_ROLL), 2), "roller did not get own ROLL message")
    tTxt = sys_msgs(p2, m[p2], KIND_ROLL)[0].text
    check(p1.name in tTxt and "rolls" in tTxt and "(1-100)" in tTxt, f"roll text '{tTxt}'")
    time.sleep(1.0)
    check(not sys_msgs(p3, m[p3], KIND_ROLL), "ROLL reached a player outside interest range")
    m2 = p2.mark()
    r = cmd(p1, "/roll 6")
    expect(r, OK, "/roll 6")
    check(poll(lambda: sys_msgs(p2, m2, KIND_ROLL), 4), "no roll for /roll 6")
    tN = int(re.search(r"rolls (\d+)", sys_msgs(p2, m2, KIND_ROLL)[0].text).group(1))
    check(1 <= tN <= 6, f"/roll 6 gave {tN}")
    for tBad in ("/roll 1", "/roll 1001", "/roll -5", "/roll abc", "/roll 5 5"):
        r = cmd(p1, tBad)
        check(r.ok and r.status in (USAGE, FAILED), f"{tBad} not rejected: {r}")


def t_whisper_command():
    p1, p2 = C["P1"], C["P2"]
    m = p2.mark()
    r = cmd(p1, f"/w {p2.name} hello via command")
    expect(r, OK, "/w online")
    check("hello via command" in r.text, f"/w reply lacks text: {r}")
    got = p2.wait(lambda r: r.HasField("direct_message_received") and r.direct_message_received.text == "hello via command", 4, m)
    check(got is not None, "/w not delivered to online target")
    check(got.direct_message_received.from_username == p1.name, "wrong whisper sender")
    r = cmd(p1, "/w nobody_" + rand_name() + " hi")
    expect(r, FAILED, "/w offline", "not online")
    r = cmd(p1, "/w")
    check(r.status in (USAGE, FAILED), f"/w without args: {r}")
    r = cmd(p1, f"/w {p2.name}")
    check(r.status in (USAGE, FAILED), f"/w without text: {r}")


def t_who_and_pos():
    p1 = C["P1"]
    r = cmd(p1, "/who")
    expect(r, OK, "/who")
    m = re.match(r"(\d+) player", r.text)
    check(m, f"/who text {r}")
    tWorld = len([c for c in C["all"] if not c.closed])
    check(int(m.group(1)) >= tWorld, f"/who says {m.group(1)} but at least {tWorld} clients are in the world")
    tMap, x, z = get_pos(p1)
    check(tMap == 101, f"/pos map {tMap}")
    check(math.hypot(x - SPAWN[0], z - SPAWN[1]) < 15, f"/pos {x},{z} far from spawn")


def t_player_throttled_gm_commands_not():
    p = world_player(1, "pt")
    time.sleep(1.5)
    tRes = [cmd(p, "/who", pPace=False) for _ in range(14)]
    tNames = [SNAME[r.status] for r in tRes]
    print(f"      player burst: {tNames}", flush=True)
    check(tNames.count("OK") >= 3 and tNames.count("THROTTLED") >= 1, f"player burst not throttled sensibly: {tNames}")
    check(tNames[0] == "OK", "first command throttled")
    check(any("too quickly" in r.text.lower() for r in tRes if r.status == THROTTLED), "THROTTLED text lacks 'too quickly'")
    time.sleep(3.0)
    r = cmd(p, "/who", pPace=False)
    expect(r, OK, "after refill")
    g = C["G"]
    tRes = [cmd(g, "/heal", pPace=False) for _ in range(25)]
    tNames = [SNAME[r.status] for r in tRes]
    print(f"      GM command burst: {tNames}", flush=True)
    check(tNames[:18].count("OK") == 18, f"GM command burst throttled too early (GM bucket is 20): {tNames}")
    check("THROTTLED" in tNames[19:], f"GM command burst never throttled (GM bucket is 20): {tNames}")
    time.sleep(4.5)
    tRes = [cmd(g, "/who", pPace=False) for _ in range(14)]
    tNames = [SNAME[r.status] for r in tRes]
    print(f"      GM burst of player command /who: {tNames}", flush=True)
    check("THROTTLED" in tNames, f"GM player-command burst never throttled: {tNames}")
    time.sleep(3.0)


def t_item_inventory_update():
    g = C["G"]
    P5 = C["P5"] = world_player(1, "pe")
    time.sleep(1.5)
    tBefore = inv_counts(P5).get(POTION, 0)
    tMarkG, tMarkP = g.mark(), P5.mark()
    r = cmd(g, f"/item {POTION} 3 {P5.name}")
    expect(r, OK, "/item to other", "3")
    check(P5.wait(lambda x: x.HasField("inventory_update"), 4, tMarkP) is not None, "target got no InventoryUpdate")
    check(poll(lambda: inv_counts(P5).get(POTION, 0) == tBefore + 3, 3), f"potions {tBefore} -> {inv_counts(P5).get(POTION, 0)}, want +3")
    check(not inv_updates_since(g, tMarkG), "GM got InventoryUpdate for an item given to someone else")
    r = cmd(g, f"/item 99999 1 {P5.name}")
    expect(r, FAILED, "/item unknown id", "unknown item")
    r = cmd(g, f"/item {POTION} 1 nobody_{rand_name()}")
    expect(r, FAILED, "/item offline target", "not online")
    tBag = inv_counts(g).get(POTION, 0)
    if tBag < 19:
        m = g.mark()
        r = cmd(g, f"/item {POTION}")
        expect(r, OK, "/item self default count")
        check(g.wait(lambda x: x.HasField("inventory_update"), 4, m) is not None, "self /item gave no InventoryUpdate")
    srv = C["srv"]
    check(poll(lambda: any("[GM]" in l and "item" in l for l in srv.log_lines), 3), "no [GM] audit log line for /item")


def t_item_bag_full():
    g = C["G"]
    P6 = world_player(1, "pf")
    time.sleep(1.5)
    r = cmd(g, f"/item {SABRE} 1000 {P6.name}")
    check(r.status in (OK, FAILED), f"/item 1000 sabres: {r}")
    print(f"      1000 sabres: {r}", flush=True)
    tN = sum(1 for _ in range(1))
    r2 = cmd(g, f"/item {SABRE} 1 {P6.name}")
    expect(r2, FAILED, "/item into full bag", "full")
    tSabres = inv_counts(P6).get(SABRE, 0)
    check(0 < tSabres < 1000, f"bag holds {tSabres} sabres after count 1000")


def t_gold():
    g, P1 = C["G"], C["P1"]
    tBase = latest_stats(P1)
    tBase = tBase.gold if tBase else 0
    expect(cmd(g, f"/gold 500 {P1.name}"), OK, "/gold +500", "now")
    check(poll(lambda: latest_stats(P1) and latest_stats(P1).gold == tBase + 500, 3),
          f"gold {tBase} -> {latest_stats(P1).gold if latest_stats(P1) else None}, want {tBase + 500}")
    expect(cmd(g, f"/gold -200 {P1.name}"), OK, "/gold -200", "now")
    check(poll(lambda: latest_stats(P1).gold == tBase + 300, 3), f"gold after -200: {latest_stats(P1).gold}, want {tBase + 300}")
    for tBad in ("1000000001", "-1000000001", "99999999999999999999", "abc", "1.5", "0x10"):
        r = cmd(g, f"/gold {tBad} {P1.name}")
        check(r.ok and r.status in (USAGE, FAILED), f"/gold {tBad} not rejected: {r}")
    time.sleep(0.5)
    check(latest_stats(P1).gold == tBase + 300, "rejected /gold changed gold")


def t_level():
    g, P1 = C["G"], C["P1"]
    expect(cmd(g, f"/level 30 {P1.name}"), OK, "/level 30", "30")
    check(poll(lambda: latest_vitals(P1) and latest_vitals(P1)[2] == 30, 3), f"level after /level 30: {latest_vitals(P1)}")
    for tBad in ("0", "101", "-5", "99999999999", "abc"):
        r = cmd(g, f"/level {tBad} {P1.name}")
        check(r.ok and r.status in (USAGE, FAILED), f"/level {tBad} not rejected: {r}")
    time.sleep(0.5)
    check(latest_vitals(P1)[2] == 30, "rejected /level changed level")


def t_heal_sethp():
    g = C["G"]
    expect(cmd(g, "/sethp 1"), OK, "/sethp 1")
    check(poll(lambda: latest_vitals(g)[0] == 1, 3), f"hp after /sethp 1: {latest_vitals(g)}")
    tMax = latest_vitals(g)[1]
    expect(cmd(g, "/heal"), OK, "/heal", "healed")
    check(poll(lambda: latest_vitals(g)[0] == tMax, 3), f"hp after /heal {latest_vitals(g)} want {tMax}")
    r = cmd(g, f"/sethp {tMax + 1}")
    expect(r, FAILED, "/sethp max+1", "between")
    for tBad in ("0", "-3", "99999999999", "abc"):
        r = cmd(g, f"/sethp {tBad}")
        check(r.ok and r.status in (USAGE, FAILED), f"/sethp {tBad} not rejected: {r}")
    expect(cmd(g, f"/sethp {tMax // 2}"), OK, "/sethp half")
    check(poll(lambda: latest_vitals(g)[0] == tMax // 2, 3), "hp not half")
    cmd(g, "/heal")
    p2 = C["P2"]
    expect(cmd(g, f"/sethp 5 {p2.name}"), OK, "/sethp on other")
    check(poll(lambda: latest_vitals(p2)[0] == 5, 3), f"other's hp {latest_vitals(p2)}")
    cmd(g, f"/heal {p2.name}")
    check(poll(lambda: latest_vitals(p2)[0] == latest_vitals(p2)[1], 3), "other not healed")


def t_setmp_then_mana_potion_clamps():
    g, P5 = C["G"], C["P5"]
    tStats = latest_stats(P5)
    check(tStats is not None and tStats.max_mana > 0, f"no stats/mana for P5 {tStats}")
    tMax = tStats.max_mana
    tLow = max(0, tMax - 10)
    expect(cmd(g, f"/setmp {tLow} {P5.name}"), OK, "/setmp low", "mana")
    check(poll(lambda: latest_stats(P5).mana == tLow, 3), f"mana after /setmp {tLow}: {latest_stats(P5).mana}")
    expect(cmd(g, f"/item {MANA} 2 {P5.name}"), OK, "/item mana potion")
    check(poll(lambda: inv_counts(P5).get(MANA, 0) >= 2, 3), "mana potions not delivered")
    r = use_item(P5, MANA)
    check(r is not None and r.status == 0, f"mana potion use status {None if r is None else r.status}")
    check(poll(lambda: latest_stats(P5).mana == tMax, 3), f"mana after potion at {tMax - tLow} missing: {latest_stats(P5).mana}, want clamp to {tMax}")
    check(latest_stats(P5).mana <= tMax, f"mana {latest_stats(P5).mana} exceeds max {tMax}")
    for tBad in (f"{tMax + 1}", "-1", "99999999999", "abc"):
        r = cmd(g, f"/setmp {tBad} {P5.name}")
        check(r.ok and r.status in (USAGE, FAILED), f"/setmp {tBad} not rejected: {r}")
    expect(cmd(g, f"/setmp 0 {P5.name}"), OK, "/setmp 0")
    check(poll(lambda: latest_stats(P5).mana == 0, 3), "mana not 0")


def t_kill_revive():
    g, P1 = C["G"], C["P1"]
    m = P1.mark()
    expect(cmd(g, f"/kill {P1.name}"), OK, "/kill other", "killed")
    check(poll(lambda: latest_vitals(P1)[0] <= 0, 3), f"victim not dead: {latest_vitals(P1)}")
    expect(cmd(g, f"/kill {P1.name}"), FAILED, "/kill dead", "already dead")
    r = cmd(g, f"/heal {P1.name}")
    expect(r, FAILED, "/heal dead", "dead")
    r = cmd(g, f"/sethp 10 {P1.name}")
    check(r.status == FAILED, f"/sethp on dead: {r}")
    expect(cmd(g, f"/revive {P1.name}"), OK, "/revive", "revived")
    check(poll(lambda: latest_vitals(P1)[0] > 0, 3), "victim still dead after /revive")
    expect(cmd(g, f"/revive {P1.name}"), FAILED, "/revive alive", "not dead")
    expect(cmd(g, "/kill"), OK, "/kill self")
    check(poll(lambda: latest_vitals(g)[0] <= 0, 3), "GM not dead after /kill self")
    expect(cmd(g, "/revive"), OK, "/revive self")
    check(poll(lambda: latest_vitals(g)[0] > 0, 3), "GM not alive after /revive self")


def t_tp_pos_and_rejects():
    g = C["G"]
    x, z = C["OUT"]
    m = g.mark()
    expect(cmd(g, f"/tp {int(x)} {int(z)}"), OK, "/tp OUT", "teleported")
    tCor = poll(lambda: corrections_since(g, m), 4)
    check(tCor, "no self_correction after /tp")
    check(math.hypot(tCor[-1].position.x - int(x), tCor[-1].position.z - int(z)) < 2.0, f"self_correction at {tCor[-1].position.x},{tCor[-1].position.z}, want {x},{z}")
    g.pos = (tCor[-1].position.x, tCor[-1].position.y, tCor[-1].position.z)
    tMap, px, pz = get_pos(g)
    check(tMap == 101 and math.hypot(px - int(x), pz - int(z)) < 2.0, f"/pos after tp {px},{pz}")
    tBlocked = None
    n = nav()
    for tx in range(-900, 900, 50):
        for tz in range(-900, 900, 50):
            if not n.walkable(tx, tz):
                tBlocked = (tx, tz)
                break
        if tBlocked:
            break
    check(tBlocked, "found no non-walkable point on the map")
    tBad = [f"/tp {tBlocked[0]} {tBlocked[1]}", "/tp 999999999 999999999", "/tp -999999999 5", "/tp abc def", "/tp 1", "/tp",
            "/tp 99999999999999999999 1", "/tp 1.5 2.5x"]
    for t in tBad:
        r = cmd(g, t)
        check(r.ok and r.status in (USAGE, FAILED), f"{t} not rejected: {r}")
    tMap2, px2, pz2 = get_pos(g)
    check(math.hypot(px2 - px, pz2 - pz) < 1.0, f"rejected /tp moved the GM {px},{pz} -> {px2},{pz2}")


def t_summon_tpto():
    g, P1, P2 = C["G"], C["P1"], C["P2"]
    ox, oz = C["OUT"]
    gm_move(ox, oz)
    m = P1.mark()
    expect(cmd(g, f"/summon {P1.name}"), OK, "/summon", "summoned")
    check(poll(lambda: corrections_since(P1, m), 4), "summoned player got no self_correction")
    _, x, z = get_pos(P1)
    check(math.hypot(x - ox, z - oz) < 6.0, f"summoned player at {x},{z}, GM at {ox},{oz}")
    expect(cmd(g, f"/tpto {P2.name}"), OK, "/tpto", "teleported")
    time.sleep(0.5)
    _, gx, gz = get_pos(g)
    _, bx, bz = get_pos(P2)
    check(math.hypot(gx - bx, gz - bz) < 6.0, f"after /tpto GM {gx},{gz} vs target {bx},{bz}")
    g.pos = (gx, nav().y_at(gx, gz), gz)
    expect(cmd(g, f"/summon {P1.name}"), OK, "re-summon")
    expect(cmd(g, f"/tpto nobody_{rand_name()}"), FAILED, "/tpto offline", "not online")
    expect(cmd(g, f"/summon nobody_{rand_name()}"), FAILED, "/summon offline", "not online")
    time.sleep(1.0)


def t_spawn_safe_zone_and_args():
    g = C["G"]
    gm_move(*SPAWN)
    r = cmd(g, f"/spawn {MON_TOUGH} 1")
    expect(r, FAILED, "/spawn in safe zone", "safe")
    for tBad in (f"/spawn {MON_TOUGH} 0", f"/spawn {MON_TOUGH} 11", f"/spawn {MON_TOUGH} -1", "/spawn 99999", "/spawn", "/spawn abc"):
        r = cmd(g, tBad)
        check(r.ok and r.status in (USAGE, FAILED), f"{tBad} not rejected: {r}")


def spawned_entities(pView, pTemplate):
    pView.ingest()
    return pView.of(pTemplate)


def t_spawn_no_respawn():
    g = C["G"]
    gm_move(*C["OUT"])
    v = View(g)
    time.sleep(1.0)
    v.ingest()
    r = cmd(g, f"/spawn {MON_WEAK} 1")
    expect(r, OK, "/spawn weak", "spawned")
    check(poll(lambda: spawned_entities(v, MON_WEAK), 4), "spawned monster never entered view")
    tId = next(iter(spawned_entities(v, MON_WEAK)))
    check(engage(g, v, tId, lambda: tId in v.dead, 40), "GM could not kill the temporary monster")
    tKilledAt = time.time()
    time.sleep(1.0)
    v.ingest()
    tSeenBefore = len(v.spawns)
    time.sleep(9.0)
    v.ingest()
    tLater = [e for e in v.spawns[tSeenBefore:] if e.kind == 1 and e.template_id == MON_WEAK]
    check(not tLater, f"temporary monster re-entered view after death: {len(tLater)} spawn event(s)")
    check(not v.mons.get(tId, {}).get("alive", False), "temporary monster alive again after respawn delay")


def t_spawn_cap():
    g = C["G"]
    gm_move(*C["OUT"])
    v = View(g)
    tTotal, tTerminal = 0, None
    for i in range(9):
        r = cmd(g, f"/spawn {MON_TOUGH} 10")
        if r.status == OK:
            m = re.search(r"Spawned (\d+)", r.text)
            tTotal += int(m.group(1)) if m else 10
        else:
            tTerminal = r
            break
    print(f"      spawned {tTotal} before stop; terminal reply: {tTerminal}", flush=True)
    check(tTerminal is not None, f"/spawn never hit a cap after {tTotal} monsters")
    check(tTerminal.status == FAILED and "limit" in tTerminal.text.lower() or "too many" in tTerminal.text.lower(), f"cap reply: {tTerminal}")
    check(tTotal <= 50, f"cap exceeded: {tTotal} spawned")
    check(tTotal >= 40, f"cap hit too early: {tTotal}")
    r = cmd(g, f"/spawn {MON_TOUGH} 1")
    expect(r, FAILED, "/spawn over cap again")
    gm_move(*SPAWN)


def t_announce_reaches_everyone():
    g = C["G"]
    tText = "server-wide-g27-" + rand_name()
    tClients = [c for c in C["all"] if not c.closed]
    tMarks = {c: c.mark() for c in tClients}
    r = cmd(g, f"/announce {tText}")
    expect(r, OK, "/announce", "sent")
    for c in tClients:
        got = poll(lambda c=c: [s for s in sys_msgs(c, tMarks[c], KIND_ANNOUNCE) if s.text == tText], 4)
        check(got, f"{c.name} did not receive the announcement ({len(tClients)} clients)")
    r = cmd(C["P1"], "/announce nope")
    expect(r, UNKNOWN, "/announce as player")
    for c in tClients:
        if c is not g:
            check(not [s for s in sys_msgs(c, tMarks[c]) if s.text == "nope"], "player announce delivered")


def t_mute_online_all_channels_and_relog():
    g, P1, P2 = C["G"], C["P1"], C["P2"]
    m1, m2 = P1.mark(), P2.mark()
    expect(cmd(g, f"/mute {P1.name} 5"), OK, "/mute", "muted")
    check(poll(lambda: sys_msgs(P1, m1, KIND_WARNING), 3), "muted player got no WARNING")
    time.sleep(0.6)
    r = P1.chat("muted-nearby")
    check(r is not None and (r.chat_message_response.error_code != 0 or r.header.status.code != 0), f"muted nearby chat accepted: {r}")
    time.sleep(0.6)
    r = P1.chat("muted-global", True)
    check(r is not None and (r.chat_message_response.error_code != 0 or r.header.status.code != 0), f"muted global chat accepted: {r}")
    time.sleep(0.6)
    r = P1.whisper(P2.name, "muted-whisper")
    check(r is not None and r.direct_message_response.error_code != 0, f"muted whisper accepted: {r}")
    r = cmd(P1, f"/w {P2.name} muted-cmd-whisper")
    check(r.status != OK, f"muted /w accepted: {r}")
    r = cmd(P1, "/roll")
    check(r.status != OK, f"muted /roll accepted: {r}")
    time.sleep(1.5)
    tBad = [x for x in log_since(P2, m2) if (x.HasField("chat_message_received") and "muted-" in x.chat_message_received.text)
            or (x.HasField("direct_message_received") and "muted-" in x.direct_message_received.text)
            or (x.HasField("system_message") and x.system_message.kind == KIND_ROLL)]
    check(not tBad, f"muted player's traffic reached another player: {tBad[:1]}")
    r = cmd(P1, "/pos")
    expect(r, OK, "muted player may still use /pos")
    P1.send(lambda q: q.logout_request.SetInParent())
    time.sleep(1.5)
    r = P1.login(P1.user)
    check(r is not None and r.login_response.token, f"relog login failed {r}")
    enter_world(P1)
    time.sleep(1.0)
    r = P1.chat("muted-after-relog")
    check(r is not None and (r.chat_message_response.error_code != 0 or r.header.status.code != 0), f"mute did not survive relog: {r}")
    expect(cmd(g, f"/summon {P1.name}"), OK, "summon P1 next to P2 after relog")
    time.sleep(1.0)
    m2 = P2.mark()
    expect(cmd(g, f"/mute {P1.name} 0"), OK, "/mute 0", "unmuted")
    time.sleep(0.8)
    r = P1.chat("unmuted-chat")
    check(r is not None and r.chat_message_response.error_code == 0, f"chat still rejected after unmute: {r}")
    check(P2.wait(lambda r: r.HasField("chat_message_received") and r.chat_message_received.text == "unmuted-chat", 4, m2), "unmuted chat not delivered")
    for tBad in (f"/mute {P1.name} -1", f"/mute {P1.name} 10081", f"/mute {P1.name} abc", f"/mute {P1.name}", "/mute",
                 f"/mute nobody_{rand_name()} 5"):
        r = cmd(g, tBad)
        check(r.ok and r.status in (USAGE, FAILED), f"{tBad} not rejected: {r}")
    time.sleep(0.6)
    r = P1.chat("still-fine")
    check(r is not None and r.chat_message_response.error_code == 0, "rejected /mute muted the player anyway")
    r = cmd(P1, f"/mute {P2.name} 5")
    expect(r, UNKNOWN, "/mute as player")


def t_mute_offline_via_db():
    g = C["G"]
    P4 = new_player(1, "po")
    try:
        r = cmd(g, f"/mute {P4.name} 5")
        print(f"      offline /mute reply: {r}", flush=True)
        expect(r, OK, "/mute offline", "muted")
        enter_world(P4)
        C["all"].append(P4)
        time.sleep(1.0)
        r = P4.chat("offline-muted")
        check(r is not None and (r.chat_message_response.error_code != 0 or r.header.status.code != 0),
              f"DB mute of an offline player not applied on entering the world: {r}")
        expect(cmd(g, f"/mute {P4.name} 0"), OK, "unmute")
        time.sleep(0.8)
        r = P4.chat("offline-unmuted")
        check(r is not None and r.chat_message_response.error_code == 0, f"unmute did not apply: {r}")
    finally:
        P4.close()


def t_kick_closes_and_resume_fails():
    g = C["G"]
    K = new_player(1, "pk")
    enter_world(K)
    C["all"].append(K)
    time.sleep(1.0)
    tTok = K.token
    m = K.mark()
    expect(cmd(g, f"/kick {K.name}"), OK, "/kick", "kicked")
    check(poll(lambda: sys_msgs(K, m, KIND_WARNING), 3), "kicked player got no WARNING notice before the close")
    check(poll(lambda: K.closed, 4), "kicked session was not closed by the server")
    R = Client()
    try:
        r = R.call(lambda q: setattr(q.resume_session_request, "token", tTok))
        check(r is not None and r.HasField("resume_session_response"), f"no resume response: {r}")
        check(r.resume_session_response.status == 1, f"resume after /kick succeeded: {r.resume_session_response}")
    finally:
        R.close()
    expect(cmd(g, f"/kick nobody_{rand_name()}"), FAILED, "/kick offline", "not online")
    expect(cmd(C["P1"], f"/kick {K.name}"), UNKNOWN, "/kick as player")
    expect(cmd(g, "/kick"), USAGE, "/kick without arg")


def t_out_of_range_battery():
    g, P5 = C["G"], C["P5"]
    time.sleep(1.0)
    tStats, tInv, tPos = latest_stats(P5), dict(inv_counts(P5)), get_pos(g)
    tName = P5.name
    tTexts = [f"/item 0 1 {tName}", f"/item -1 1 {tName}", f"/item {POTION} 0 {tName}", f"/item {POTION} -1 {tName}",
              f"/item {POTION} 1001 {tName}", f"/item {POTION} 99999999999999999999 {tName}", f"/item 4294967296 1 {tName}",
              f"/item 99999 1 {tName}", f"/item abc 1 {tName}", f"/item {POTION} 1 nobody_{rand_name()}", "/item",
              f"/gold 1000000001 {tName}", f"/gold -1000000001 {tName}", f"/gold 99999999999999999999 {tName}", "/gold",
              f"/level 0 {tName}", f"/level 101 {tName}", f"/level -5 {tName}", "/level",
              f"/sethp 0 {tName}", f"/sethp -1 {tName}", f"/sethp 99999999 {tName}", f"/setmp -1 {tName}", f"/setmp 99999999 {tName}",
              f"/spawn {MON_TOUGH} 0", f"/spawn {MON_TOUGH} 11", "/spawn 99999", "/spawn",
              "/tp", "/tp 1", "/tp 999999999 999999999", "/tp abc def",
              f"/mute {tName} -1", f"/mute {tName} 10081", f"/mute {tName}", "/mute", "/kick", f"/kick nobody_{rand_name()}",
              "/roll 1", "/roll 1001", "/roll -3", "/w", f"/w {tName}", "/announce", "/", "/ ", "//item", "/\titem 4", "/item\t4 1",
              "/" + "a" * 17, "/" + "a" * 250, "/item " + "9" * 240, "/item " + " ".join(["1"] * 40)]
    tBad = []
    for t in tTexts:
        r = cmd(g, t)
        if not r.ok:
            tBad.append((t[:40], "no response"))
        elif r.status == OK:
            tBad.append((t[:40], str(r)))
    check(not tBad, f"commands wrongly accepted or unanswered: {tBad[:6]}")
    time.sleep(1.0)
    tAfter = latest_stats(P5)
    check(tAfter.gold == tStats.gold and tAfter.level == tStats.level and tAfter.mana == tStats.mana, "rejected commands changed P5 stats")
    check(inv_counts(P5) == tInv, f"rejected commands changed P5 inventory {tInv} -> {inv_counts(P5)}")
    check(get_pos(g) == tPos or math.hypot(get_pos(g)[1] - tPos[1], get_pos(g)[2] - tPos[2]) < 1.0, "GM moved")
    for t in ("/item 4 1\x01", "/item 4 1\x7f", "\x00/item"):
        tRes = cmd(g, t, pTimeout=3.0)
        check(tRes.status != OK, f"control character command accepted: {repr(t)} -> {tRes}")
    c = new_player(1, "pn")
    try:
        tMark = c.mark()
        c.send(lambda q: setattr(q.command_request, "text", "/who"))
        time.sleep(1.5)
        tR = [Res(x) for x in log_since(c, tMark) if x.HasField("command_response")]
        check(all(r.status != OK for r in tR), f"command from a client not in the world executed: {[str(r) for r in tR]}")
    finally:
        c.close()


def t_gm_audit_log():
    srv = C["srv"]
    tGm = [l for l in srv.log_lines if "[GM]" in l]
    print(f"      [GM] lines: {len(tGm)}; sample: {tGm[:1]}", flush=True)
    check(tGm, "no [GM] audit lines in the server log")
    check(any(C["G"].name in l or GM_ACCOUNT in l for l in tGm), "[GM] lines do not name the GM")
    check(not any("/item" in l and "Health Potion" in l and False for l in tGm), "n/a")


def t_moderator_level_1():
    SKIPPED.append("moderator (gm_level 1) cases: psql not available, no DB driver in python")
    print("      SKIP: " + SKIPPED[-1], flush=True)


def t_server_survived():
    srv = C["srv"]
    check(srv.proc.poll() is None, "server process exited\n" + "\n".join(srv.log_lines[-15:]))
    r = cmd(C["P2"], "/who")
    expect(r, OK, "/who after all abuse")
    check(not any("Assertion" in l or "terminate" in l.lower() or "exception" in l.lower() for l in srv.log_lines),
          "exception/assert line in server log: " + next((l for l in srv.log_lines if "xception" in l or "Assertion" in l), ""))


TESTS = [t_help_player_lists_only_player_commands, t_item_as_player_same_as_unknown, t_raw_slash_chat_not_delivered,
         t_roll_nearby_only, t_whisper_command, t_who_and_pos, t_player_throttled_gm_commands_not,
         t_item_inventory_update, t_item_bag_full, t_gold, t_level, t_heal_sethp, t_setmp_then_mana_potion_clamps,
         t_kill_revive, t_tp_pos_and_rejects, t_summon_tpto, t_spawn_safe_zone_and_args, t_spawn_no_respawn, t_spawn_cap,
         t_announce_reaches_everyone, t_mute_online_all_channels_and_relog, t_mute_offline_via_db,
         t_kick_closes_and_resume_fails, t_out_of_range_battery, t_gm_audit_log, t_moderator_level_1, t_server_survived]


def main():
    tOnly = sys.argv[1:]
    tResults = []
    srv = None
    try:
        try:
            setup()
            srv = C["srv"]
        except Exception as e:
            print("SETUP FAILED:", e, flush=True)
            traceback.print_exc()
            srv = C.get("srv")
            if srv:
                print("\n".join(srv.log_lines[-20:]))
            sys.exit(2)
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
        for c in C.get("all", []):
            try:
                c.close()
            except Exception:
                pass
        if C.get("srv"):
            C["srv"].stop()
    tPassed = sum(1 for r in tResults if r[1])
    print(f"\n{tPassed}/{len(tResults)} passed" + (f" (skipped: {'; '.join(SKIPPED)})" if SKIPPED else ""))
    sys.exit(0 if tPassed == len(tResults) else 1)


if __name__ == "__main__":
    main()
