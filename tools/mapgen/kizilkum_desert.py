"""Draws map 402 - Kizilkum Desert - into data/maps/402_kizilkum_desert/.

A one-off sketching tool. Shared, kingdom-less red-sand desert between the
Borderlands (south gate) and the Black Forest (east gate): a faint caravan
trail winds past a ruined caravanserai, an oasis lies in the east, red rock
mesas break the dunes and the Shahmaran's lair hides in the far north-west.
Shared building blocks live in mapkit.py.

Needs numpy and Pillow.
"""

import math

import numpy as np

from mapkit import *

MAP_ID = 402
OUT = map_directory(MAP_ID, "kizilkum_desert")

ROAD = [(0, 800), (40, 520), (-70, 280), (-20, 60), (140, -20), (420, 10), (620, -30), (800, 0)]
OASIS = [((330, 260), 52), ((380, 300), 30)]
MESAS = [(-480, 260, 150), (420, -420, 160), (-300, -560, 120), (600, 560, 100)]
PALMS = [(345, 275, 120)]
CARAVANSERAI = (-170.0, -130.0)
CARAVAN_HALF = 36.0
LAIR = (-470.0, -470.0)
NEST = (560.0, -380.0)
CAMP = (360.0, 600.0)

BUMPS = [(-480, 260, 160, 22), (420, -420, 170, 26), (-300, -560, 130, 20), (600, 560, 110, 16),
         (-650, -100, 220, 9), (150, 400, 220, 8), (200, -300, 200, 7)]

SOUTH_PORTAL = (0.0, 800.0)
SOUTH_ARRIVAL = (14.0, 735.0)
EAST_PORTAL = (800.0, 0.0)
EAST_ARRIVAL = (735.0, -14.0)
PORTAL_RADIUS = 14.0
SPECIALS = [CARAVANSERAI, LAIR, NEST, CAMP, SOUTH_PORTAL, SOUTH_ARRIVAL, EAST_PORTAL, EAST_ARRIVAL]

SAND = np.array([212, 184, 126])
SAND_PALE = np.array([226, 204, 150])
SAND_RED = np.array([196, 142, 96])
RED_ROCK = np.array([150, 86, 62])
TRAIL = np.array([176, 150, 106])
OASIS_GRASS = np.array([96, 124, 70])
BED = np.array([150, 132, 96])
ROCK_BROWN = [0.52, 0.30, 0.22]
SANDSTONE = [0.80, 0.66, 0.44]
DARK = [0.08, 0.07, 0.07]
BONE = [0.85, 0.82, 0.72]


def build_height():
    x, z = grid(HEIGHT_RES, cell_centred=False)
    h = np.full(x.shape, 9.0)
    dune = np.sin(x * 0.013 + 0.6 * np.sin(z * 0.006)) * np.cos(z * 0.008 - 0.5)
    h += 3.2 * dune + 1.6 * np.sin((x + z) * 0.021) * np.cos((x - z) * 0.017 + 1.0)
    for bx, bz, r, amp in BUMPS:
        h += amp * np.exp(-((x - bx) ** 2 + (z - bz) ** 2) / (2.0 * (r * 0.5) ** 2))
    h = flatten_along(h, x, z, [ROAD], 9.0, strength=0.5, reach=40.0, core=8.0)
    h = blend_toward(h, 9.0, 0.9 * smoothstep(60.0, 40.0, np.maximum(np.abs(x - CARAVANSERAI[0]), np.abs(z - CARAVANSERAI[1]))))
    h = carve_ponds(h, x, z, OASIS)
    h = h + mountain_rim(x, z)
    return x, z, np.clip(h, HEIGHT_MIN, HEIGHT_MAX)


def build_ground(x, z, h):
    c = np.zeros(x.shape + (3,))
    c[:] = SAND
    c += (9.0 * np.sin(x * 0.045 + z * 0.02) * np.cos(z * 0.039))[..., None]
    pale = smoothstep(-0.1, 0.4, smooth_noise(x, z, 4021))[..., None]
    c = c * (1.0 - 0.6 * pale) + SAND_PALE * 0.6 * pale
    red = smoothstep(0.0, 0.5, smooth_noise(x, z, 4022))[..., None]
    c = c * (1.0 - 0.5 * red) + SAND_RED * 0.5 * red
    edge = smoothstep(26.0, -14.0, blob_distance(x, z, PALMS))[..., None]
    c = c * (1.0 - edge) + OASIS_GRASS * edge
    edge = smoothstep(ROAD_HALF + 2.0, ROAD_HALF - 2.0, polyline_distance(x, z, ROAD))[..., None]
    c = c * (1.0 - edge) + TRAIL * edge
    edge = smoothstep(20.0, -20.0, blob_distance(x, z, MESAS))[..., None]
    c = c * (1.0 - edge) + RED_ROCK * edge
    bed = smoothstep(0.5, -1.0, h)[..., None]
    c = c * (1.0 - bed) + BED * bed
    rock = smoothstep(26.0, 38.0, h)[..., None]
    c = c * (1.0 - rock) + RED_ROCK * rock
    return np.clip(c, 0, 255).astype(np.uint8)


def caravanserai(cx, cz, half):
    T = WALL_TILE
    out = []
    n = int(2 * half / T)
    for i in range(n):
        o = -half + T * (i + 0.5)
        if abs(o) < 9.0:
            continue
        out.append(wall_tile(cx + o, cz - half, 0.0))
        if i % 3 != 1:
            out.append(wall_tile(cx + o, cz + half, 0.0))
        out.append(wall_tile(cx - half, cz + o, math.pi / 2.0))
        if i % 4 != 2:
            out.append(wall_tile(cx + half, cz + o, math.pi / 2.0))
    out += tower(cx - half, cz - half)
    out += tower(cx + half, cz + half)
    out.append(box("well", cx, cz, 4, 1.4, 4, STONE))
    for ox, oz in [(-18, 14), (16, -16), (20, 18)]:
        out.append(box("crate", cx + ox, cz + oz, 3, 2.4, 3, WOOD))
    out.append(box("tent", cx - 16, cz - 14, 8, 4, 6, AWNINGS[2]))
    return out


def build_objects(h_at, h):
    objs = []
    cx, cz = CARAVANSERAI
    objs += caravanserai(cx, cz, CARAVAN_HALF)

    lx, lz = LAIR
    objs.append(box("den_mouth", lx, lz, 20, 7, 4, DARK))
    for ox, oz in [(-18, 12), (16, 14), (-6, 22)]:
        objs.append(box("bone_pile", lx + ox, lz + oz, 4, 1, 3, BONE, block=False))
    for i in range(6):
        a = i * 2.0 * math.pi / 6.0
        objs.append(box("standing_stone", lx + 28 * math.cos(a), lz + 28 * math.sin(a) + 8, 2.4, 6.5, 2.2, ROCK_BROWN))

    nx, nz = NEST
    objs.append(box("den_mouth", nx, nz, 12, 5, 3, DARK))
    objs.append(box("bone_pile", nx - 12, nz + 8, 4, 1, 3, BONE, block=False))

    px, pz = CAMP
    for i, (tx, tz) in enumerate([(-10, -6), (10, -4), (0, 12)]):
        objs.append(box("tent", px + tx, pz + tz, 8, 4, 6, AWNINGS[i % 3]))
    objs.append(box("campfire", px, pz, 3, 1, 3, [0.7, 0.32, 0.1], block=False))

    def clear(qx, qz):
        pt_x, pt_z = np.array([[qx]]), np.array([[qz]])
        if polyline_distance(pt_x, pt_z, ROAD)[0, 0] < 16:
            return False
        if any(math.hypot(qx - sx, qz - sz) < 45 for sx, sz in SPECIALS):
            return False
        if any(math.hypot(qx - ox, qz - oz) < r + 25 for (ox, oz), r in OASIS):
            return False
        return HALF - max(abs(qx), abs(qz)) > RIM_BLOCK + 20

    objs += scatter_boulders(np.random.RandomState(4022), 40, MESAS, clear)

    def keepout(qx, qz):
        hit = near_polylines(qx, qz, [ROAD], ROAD_HALF + 14.0)
        hit |= near_circles(qx, qz, [(ox, oz, r) for (ox, oz), r in OASIS], 22.0)
        hit |= near_points(qx, qz, SPECIALS, 40.0)
        hit |= rect_mask(qx, qz, cx - CARAVAN_HALF - 15, cz - CARAVAN_HALF - 15, cx + CARAVAN_HALF + 15, cz + CARAVAN_HALF + 15)
        hit |= near_objects(qx, qz, objs, 10.0)
        return hit

    objs += scatter_forest(np.random.RandomState(4021), h, "desert", PALMS, keepout, 700, sparse_chance=0.00012)
    return objs


def make_splat(h, objs):
    x, z = splat_grid()
    hh = height_at_grid(h, x, z)
    road, shoulder = road_weight(x, z, [ROAD], [], [], 0.0)
    oasis = smoothstep(24.0, -14.0, blob_distance(x, z, PALMS))
    patch = smoothstep(-0.2, 0.3, smooth_noise(x, z, 4023))
    dry = 0.55 + 0.35 * patch + 0.4 * oasis
    dirt = (0.3 * shoulder + 0.35 * (1.0 - patch) * smoothstep(0.0, 0.6, smooth_noise(x, z, 4024))
            + 0.9 * shore_weight(x, z, hh, [("circles", OASIS)]))
    rock = np.maximum(smoothstep(18.0, -18.0, blob_distance(x, z, MESAS)), smoothstep(24.0, 36.0, hh))
    return build_splat(x, z, hh, dry, dirt, road, rock)


def main():
    x, z, h = build_height()
    objs = build_objects(make_height_sampler(h), h)
    ground = build_ground(x, z, h)
    block = build_block(objs)
    roads = build_roads([ROAD], [], [], 0.0)

    assert_walkable(h, block, [SOUTH_PORTAL, SOUTH_ARRIVAL, EAST_PORTAL, EAST_ARRIVAL], "map %d" % MAP_ID)

    meta = {
        "id": MAP_ID,
        "name": "Kizilkum Desert",
        "kingdom": 0,
        "worldSize": WORLD,
        "heightMin": HEIGHT_MIN,
        "heightMax": HEIGHT_MAX,
        "waterLevel": WATER,
        "spawn": {"x": SOUTH_ARRIVAL[0], "z": SOUTH_ARRIVAL[1], "yaw": 0.0},
        "portals": [
            portal(SOUTH_PORTAL[0], SOUTH_PORTAL[1], PORTAL_RADIUS, 401, -14.0, -735.0, math.pi),
            portal(EAST_PORTAL[0], EAST_PORTAL[1], PORTAL_RADIUS, 403, -735.0, 14.0, -math.pi / 2.0),
        ],
        "objects": objs,
        "safeZones": [],
        "npcs": [],
        "monsterSpawns": [
            monster_spawn(11, 120.0, 560.0, 45.0, 5),
            monster_spawn(11, -250.0, 420.0, 45.0, 4),
            monster_spawn(12, 330.0, 600.0, 40.0, 4),
            monster_spawn(12, -200.0, -130.0, 40.0, 4),
            monster_spawn(12, 560.0, 180.0, 40.0, 4),
            monster_spawn(13, 330.0, 260.0, 45.0, 3),
            monster_spawn(13, 420.0, -400.0, 45.0, 4),
            monster_spawn(11, -520.0, -120.0, 45.0, 4),
            monster_spawn(13, 560.0, -380.0, 25.0, 3),
            monster_spawn(14, -470.0, -470.0, 15.0, 1),
        ],
    }
    check_map_access(h, objs, block, meta, "map %d" % MAP_ID)
    add_undergrowth(objs, h, block, roads, meta, "desert", 7402)
    write_map(OUT, meta, h, ground, block, roads, objs, make_splat(h, objs))


if __name__ == "__main__":
    main()
