import sys, os, time, math, traceback
import t_gm_commands_g27 as g27
from t_gm_commands_g27 import (G27Server, cmd, expect, check, poll, latest_stats, latest_vitals, inv_counts,
                               get_pos, world_player, new_player, open_site, OK, UNKNOWN, FAILED, USAGE, SNAME,
                               GM_ACCOUNT, GM_CHAR, SPAWN, POTION, enter_world, rand_name, Client, C, SKIPPED)

GM_COMMANDS_TARGETED = ["/item {p} 5 {v}", "/gold 100 {v}", "/level 50 {v}", "/heal {v}", "/sethp 1 {v}", "/setmp 1 {v}",
                        "/kill {v}", "/revive {v}", "/summon {v}", "/tpto {v}", "/kick {v}", "/mute {v} 5"]
GM_COMMANDS_PLAIN = ["/tp 1 1", "/spawn 301 1", "/announce hijack-g53", "/heal", "/sethp 1", "/kill", "/gold 5", "/level 50"]


def snap(pClient):
    s = latest_stats(pClient)
    v = latest_vitals(pClient)
    check(s is not None and v is not None, f"no stats snapshot for {pClient.name}")
    return {"gold": s.gold, "mana": s.mana, "inv": dict(inv_counts(pClient)), "hp": v[0], "level": v[2], "pos": get_pos(pClient)}


def same(pA, pB):
    return (pA["gold"] == pB["gold"] and pA["mana"] == pB["mana"] and pA["inv"] == pB["inv"] and pA["hp"] == pB["hp"]
            and pA["level"] == pB["level"] and pA["pos"][0] == pB["pos"][0]
            and math.hypot(pA["pos"][1] - pB["pos"][1], pA["pos"][2] - pB["pos"][2]) < 1.0)


SFX = os.environ.get("G53_SUFFIX", "")
GM_A, GM_B, MOD_C = [n + SFX for n in ("gm_g53a", "gm_g53b", "mod_g53c")]
GM_ENV = f"{GM_A}:2,{GM_B}:2,{MOD_C}:1"


def login_gm(pAccount, pChar):
    c = Client()
    r = c.login(pAccount)
    if not (r is not None and r.login_response.token):
        c.close()
        return None
    r = c.char_list()
    tChars = list(r.character_list_response.characters)
    if tChars:
        c.cid, c.name = tChars[0].character_id, tChars[0].name
    else:
        r = c.create_char(pChar, 1)
        check(r is not None and r.character_create_response.status == 0, f"gm char create failed {pAccount} {r}")
        c.cid, c.name = r.character_create_response.character.character_id, pChar
    enter_world(c)
    C.setdefault("all", []).append(c)
    return c


def start_server(pSrv):
    tOld = g27.GM_ACCOUNT
    g27.GM_ACCOUNT = GM_ENV
    try:
        pSrv.start()
    finally:
        g27.GM_ACCOUNT = tOld


def setup():
    srv = G27Server()
    start_server(srv)
    C["srv"] = srv
    tProbe = [Client() for _ in range(3)]
    tOk = []
    for c, n in zip(tProbe, (GM_A, GM_B, MOD_C)):
        r = c.login(n)
        tOk.append(bool(r is not None and r.login_response.token))
        c.close()
    if not all(tOk):
        for n in (GM_A, GM_B, MOD_C):
            c = Client()
            r = c.register(n)
            check(r is not None and r.register_response.register_status in (0, 2), f"gm register failed {n} {r}")
            c.close()
        srv.stop()
        start_server(srv)
    c = login_gm(GM_A, "GmG53A" + SFX)
    check(c is not None, "gm_a login failed")
    C["G"] = c
    C["GB"] = login_gm(GM_B, "GmG53B" + SFX)
    C["MOD"] = login_gm(MOD_C, "ModG53C" + SFX)
    check(C["GB"] is not None and C["MOD"] is not None, "second gm / moderator login failed")
    time.sleep(1.5)
    cmd(c, "/revive", pPace=False)
    r = cmd(c, "/help")
    check(r.ok and any("/item" in l for l in r.lines), f"GM account has no admin rights: {r}")
    r = cmd(C["GB"], "/help")
    check(r.ok and any("/item" in l for l in r.lines), f"gm_b has no level 2 rights: {r}")
    r = cmd(C["MOD"], "/help")
    check(r.ok and any("/kick" in l for l in r.lines) and not any("/item" in l for l in r.lines), f"moderator rights wrong: {r}")
    C["FAR"] = open_site(SPAWN, 220, [math.pi])
    expect(cmd(c, f"/tp {int(SPAWN[0])} {int(SPAWN[1])}"), OK, "gm to spawn")
    C["ATT"] = world_player(1, "pa")
    C["VIC"] = world_player(1, "pv")
    time.sleep(1.0)
    expect(cmd(c, f"/tp {int(C['FAR'][0])} {int(C['FAR'][1])}"), OK, "gm to far")
    expect(cmd(c, f"/summon {C['VIC'].name}"), OK, "summon victim far")
    expect(cmd(c, f"/gold 777 {C['VIC'].name}"), OK, "victim gold")
    expect(cmd(c, f"/item {POTION} 3 {C['VIC'].name}"), OK, "victim potions")
    expect(cmd(c, f"/tp {int(SPAWN[0])} {int(SPAWN[1])}"), OK, "gm back to spawn")
    time.sleep(1.5)


def t_baseline_positive_control():
    a, v = C["ATT"], C["VIC"]
    sa, sv = snap(a), snap(v)
    check(sv["gold"] >= 777 and sv["inv"].get(POTION, 0) >= 3, f"victim fixture missing gold/items: {sv}")
    check(math.hypot(sv["pos"][1] - sa["pos"][1], sv["pos"][2] - sa["pos"][2]) > 100, "victim not far from attacker")
    r = cmd(a, "/who")
    expect(r, OK, "player /who works (positive control)")
    check(cmd(a, "/pos").status == OK, "player /pos failed")


def t_player_gm_commands_unknown_no_side_effects():
    a, v = C["ATT"], C["VIC"]
    time.sleep(1.0)
    bV, bA = snap(v), snap(a)
    ref = cmd(a, "/zzzq 1")
    expect(ref, UNKNOWN, "reference unknown")
    tTexts = [t.format(p=POTION, v=v.name) for t in GM_COMMANDS_TARGETED] + GM_COMMANDS_PLAIN
    tTexts += [t.format(p=POTION, v=a.name) for t in ("/gold 100 {v}", "/heal {v}", "/sethp 1 {v}", "/level 50 {v}", "/item {p} 5 {v}")]
    tBad = []
    for t in tTexts:
        r = cmd(a, t)
        if not r.ok:
            tBad.append((t, "no response"))
        elif r.status != UNKNOWN or r.lines != ref.lines:
            tBad.append((t, str(r)))
    check(not tBad, f"player GM commands not rejected as unknown: {tBad[:5]}")
    time.sleep(1.5)
    check(same(snap(v), bV), f"victim changed by player's GM commands: {bV} -> {snap(v)}")
    check(same(snap(a), bA), f"attacker changed by own rejected commands: {bA} -> {snap(a)}")
    check(not v.closed, "victim kicked by a player /kick")
    time.sleep(0.6)
    r = v.chat("not-muted-g53")
    check(r is not None and r.chat_message_response.error_code == 0, f"victim muted by player /mute: {r}")
    check(cmd(a, "/who").status == OK, "player lost normal commands")
    srv = C["srv"]
    check(poll(lambda: sum(1 for l in srv.log_lines if "[Security]" in l) >= 3, 3), "no [Security] log for rejected player GM commands")


def t_gm_on_normal_player_and_self():
    g, v = C["G"], C["VIC"]
    time.sleep(0.5)
    b = snap(v)
    expect(cmd(g, f"/gold 50 {v.name}"), OK, "gm /gold on player")
    check(poll(lambda: latest_stats(v).gold == b["gold"] + 50, 3), "player gold unchanged after GM /gold")
    expect(cmd(g, f"/level 7 {v.name}"), OK, "gm /level on player")
    check(poll(lambda: latest_vitals(v)[2] == 7, 3), "player level unchanged after GM /level")
    expect(cmd(g, f"/sethp 3 {v.name}"), OK, "gm /sethp on player")
    check(poll(lambda: latest_vitals(v)[0] == 3, 3), "player hp unchanged after GM /sethp")
    expect(cmd(g, f"/heal {v.name}"), OK, "gm /heal on player")
    check(poll(lambda: latest_vitals(v)[0] == latest_vitals(v)[1], 3), "player not healed")
    tPot = inv_counts(v).get(POTION, 0)
    expect(cmd(g, f"/item {POTION} 2 {v.name}"), OK, "gm /item on player")
    check(poll(lambda: inv_counts(v).get(POTION, 0) == tPot + 2, 3), "player items unchanged after GM /item")
    expect(cmd(g, f"/summon {v.name}"), OK, "gm /summon player")
    expect(cmd(g, f"/tpto {v.name}"), OK, "gm /tpto player")
    expect(cmd(g, f"/setmp 0 {v.name}"), OK, "gm /setmp player")
    expect(cmd(g, f"/kill {v.name}"), OK, "gm /kill player")
    check(poll(lambda: latest_vitals(v)[0] <= 0, 3), "player not dead after GM /kill")
    expect(cmd(g, f"/revive {v.name}"), OK, "gm /revive player")
    check(poll(lambda: latest_vitals(v)[0] > 0, 3), "player still dead after GM /revive")
    tOwn = latest_stats(g).gold
    expect(cmd(g, f"/gold 10 {g.name}"), OK, "gm /gold own name")
    check(poll(lambda: latest_stats(g).gold == tOwn + 10, 3), "own-name /gold had no effect")
    expect(cmd(g, f"/sethp 2 {g.name}"), OK, "gm /sethp own name")
    check(poll(lambda: latest_vitals(g)[0] == 2, 3), "own-name /sethp no effect")
    expect(cmd(g, f"/heal {g.name}"), OK, "gm /heal own name")
    expect(cmd(g, "/heal"), OK, "gm /heal implicit self")
    expect(cmd(g, f"/mute {v.name} 0"), OK, "gm /mute 0 on player")


def _blocked(pActor, pText, pCtx, pBad):
    r = cmd(pActor, pText)
    if not r.ok:
        pBad.append((pCtx, pText, "no response"))
    elif r.status != FAILED or "cannot target" not in r.text.lower():
        pBad.append((pCtx, pText, str(r)))


def _effect_free(pTarget, pBefore, pCtx):
    time.sleep(1.5)
    check(not pTarget.closed, f"{pCtx}: target kicked")
    check(same(snap(pTarget), pBefore), f"{pCtx}: target changed {pBefore} -> {snap(pTarget)}")
    r = pTarget.chat("not-muted-g53-" + pCtx)
    check(r is not None and r.chat_message_response.error_code == 0, f"{pCtx}: target muted {r}")


def t_gm2_vs_gm2_cannot_target():
    g, t = C["G"], C["GB"]
    time.sleep(0.5)
    b = snap(t)
    tBad = []
    for tmpl in GM_COMMANDS_TARGETED:
        if tmpl.startswith("/mute"):
            continue
        _blocked(g, tmpl.format(p=POTION, v=t.name), "gm2->gm2", tBad)
    _blocked(g, f"/mute {t.name} 5", "gm2->gm2", tBad)
    check(not tBad, f"GM2 targeting another GM2 not rejected: {tBad[:4]}")
    _effect_free(t, b, "g2g2")
    r = cmd(C["GB"], f"/gold 5 {g.name}")
    check(r.ok and r.status == FAILED and "cannot target" in r.text.lower(), f"reverse direction not rejected: {r}")
    check(cmd(g, "/who").status == OK, "gm2 lost normal commands")


def t_gm1_moderator():
    m, g, gb, v = C["MOD"], C["G"], C["GB"], C["VIC"]
    ref = cmd(m, "/zzzq 1")
    expect(ref, UNKNOWN, "mod reference unknown")
    bV, bG = snap(v), snap(gb)
    tBad = []
    for t in ("/item {p} 5 {v}", "/gold 100 {v}", "/level 50 {v}", "/heal {v}", "/sethp 1 {v}", "/setmp 1 {v}",
              "/kill {v}", "/revive {v}", "/spawn 301 1", "/heal", "/kill", "/gold 5", "/level 50"):
        r = cmd(m, t.format(p=POTION, v=v.name))
        if not r.ok:
            tBad.append((t, "no response"))
        elif r.status != UNKNOWN or r.lines != ref.lines:
            tBad.append((t, str(r)))
    check(not tBad, f"GM1 level-2 commands not Unknown: {tBad[:4]}")
    for t in ("/item {p} 5 {v}", "/gold 100 {v}", "/kill {v}"):
        r = cmd(m, t.format(p=POTION, v=gb.name))
        check(r.ok and r.status == UNKNOWN, f"GM1 level-2 cmd on GM2 not Unknown: {t} {r}")
    time.sleep(1.5)
    check(same(snap(v), bV), f"normal player changed by GM1 level-2 attempts: {bV} -> {snap(v)}")
    check(same(snap(gb), bG), "gm2 changed by GM1 attempts")
    for gt in (g, gb):
        b = snap(gt)
        tBad = []
        for tmpl in ("/summon {v}", "/tpto {v}", "/kick {v}", "/mute {v} 5"):
            _blocked(m, tmpl.format(v=gt.name), "gm1->gm2", tBad)
        check(not tBad, f"GM1 targeting GM2 not rejected: {tBad[:4]}")
        _effect_free(gt, b, "g1g2")
    expect(cmd(m, "/tpto " + v.name), OK, "gm1 /tpto normal player")
    expect(cmd(m, f"/summon {v.name}"), OK, "gm1 /summon normal player")
    expect(cmd(m, f"/mute {v.name} 5"), OK, "gm1 /mute normal player")
    r = v.chat("muted-g53")
    check(r is not None and r.chat_message_response.error_code != 0, f"player not muted by GM1: {r}")
    expect(cmd(m, f"/mute {v.name} 0"), OK, "gm1 unmute")
    expect(cmd(m, "/announce hello-g53"), OK, "gm1 /announce")
    expect(cmd(m, f"/kick {v.name}"), OK, "gm1 /kick normal player")
    check(poll(lambda: v.closed, 4), "player not disconnected by GM1 /kick")
    expect(cmd(g, f"/tpto {m.name}"), OK, "gm2 may target gm1 (lower)")


def t_server_survived():
    srv = C["srv"]
    check(srv.proc.poll() is None, "server process exited\n" + "\n".join(srv.log_lines[-15:]))
    check(cmd(C["ATT"], "/who").status == OK, "/who after the security run")
    check(not any("Assertion" in l or "terminate" in l.lower() or "exception" in l.lower() for l in srv.log_lines),
          "exception/assert line in server log")


TESTS = [t_baseline_positive_control, t_player_gm_commands_unknown_no_side_effects, t_gm_on_normal_player_and_self,
         t_gm2_vs_gm2_cannot_target, t_gm1_moderator, t_server_survived]


def main():
    tOnly = sys.argv[1:]
    tResults = []
    try:
        try:
            setup()
        except Exception as e:
            print("SETUP FAILED:", e, flush=True)
            traceback.print_exc()
            srv = C.get("srv")
            if srv:
                print("\n".join(srv.log_lines[-20:]))
            sys.exit(2)
        srv = C["srv"]
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
