import t_portal_g22 as g


def hop(pFrom, pTo, pTp, pArrive):
    def run():
        a = g.C["A"]
        check_map = g.get_pos(a)[0]
        g.check(check_map == pFrom, f"expected to start on map {pFrom}, on {check_map}")
        tMark = a.mark()
        g.cmd(a, f"/tp {pTp[0]} {pTp[1]}")
        g.expect_transfer(a, tMark, pTo, pArrive, f"{pFrom}->{pTo}")
    run.__name__ = f"t_{pFrom}_to_{pTo}"
    return run


HOPS = [
    (101, 102, (800, -32), (-735.0, 14.0)),
    (102, 401, (795, 0), (-735.0, 14.0)),
    (401, 202, (795, 0), (735.0, -14.0)),
    (202, 201, (-795, 0), (760.0, -30.0)),
    (201, 202, (800, -32), (-735.0, 14.0)),
    (202, 401, (795, 0), (735.0, -14.0)),
    (401, 302, (0, 795), (735.0, -14.0)),
    (302, 301, (-795, 0), (760.0, -30.0)),
    (301, 302, (800, -32), (-735.0, 14.0)),
    (302, 401, (795, 0), (14.0, 735.0)),
    (401, 102, (-795, 0), (735.0, -14.0)),
]

g.TESTS = [hop(*h) for h in HOPS]

if __name__ == "__main__":
    g.main()
