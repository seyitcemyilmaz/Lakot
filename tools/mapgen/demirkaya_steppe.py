"""Draws map 202 - Demirkaya Badlands - into data/maps/202_demirkaya_steppe/.

A one-off sketching tool. Dry red-grey highland between Demirkaya Village and
the Borderlands: mesas and scree, a dry wash crossing the road, a bear den, a
bandit fort and a ring of standing stones. Shared building blocks live in
mapkit.py.

Needs numpy and Pillow.
"""

import math

import numpy as np

from mapkit import *

MAP_ID = 202
OUT = map_directory(MAP_ID, "demirkaya_steppe")

ROAD = [(-800, 0), (-600, 50), (-300, 30), (0, -30), (300, -10), (600, 40), (800, 0)]
WASH = [(-200, -800), (-120, -300), (40, 100), (-60, 500), (-150, 850)]
ROCKY = [(-430, -420, 130), (600, -430, 120), (-600, 500, 110)]
GROVES = [(-100, -450, 110), (550, 320, 120), (-620, -120, 100), (250, 600, 90)]
FORT = (350.0, -250.0)
FORT_HALF = 75.0
CIRCLE = (-260.0, 420.0)
DEN = (-430.0, -330.0)

BUMPS = [(-300, -330, 180, 34), (380, 330, 200, 28), (100, -560, 160, 26), (-550, 330, 160, 22),
         (700, -250, 140, 20), (-700, -450, 150, 18)]

WEST_PORTAL = (-800.0, 0.0)
WEST_ARRIVAL = (-735.0, 14.0)
EAST_PORTAL = (800.0, 0.0)
EAST_ARRIVAL = (735.0, -14.0)
PORTAL_RADIUS = 14.0
SPECIALS = [FORT, CIRCLE, DEN, WEST_PORTAL, WEST_ARRIVAL, EAST_PORTAL, EAST_ARRIVAL]

SCRUB = np.array([128, 112, 90])
SCRUB_DARK = np.array([92, 88, 70])
DIRT = np.array([158, 126, 92])
SAND = np.array([190, 168, 128])
ROCK = np.array([122, 106, 98])
RED = np.array([150, 98, 70])
CANVAS = [0.78, 0.70, 0.52]
STONE_DARK = [0.34, 0.33, 0.31]
DARK = [0.08, 0.07, 0.07]


def build_height():
    x, z = grid(HEIGHT_RES, cell_centred=False)
    h = np.full(x.shape, 12.0)
    h += 1.8 * np.sin(x * 0.012 + 0.2) * np.cos(z * 0.01 - 0.9)
    h += 1.0 * np.sin(x * 0.05) * np.sin(z * 0.043)
    for bx, bz, r, amp in BUMPS:
        mesa = np.clip(np.exp(-((x - bx) ** 2 + (z - bz) ** 2) / (2.0 * (r * 0.5) ** 2)) * 1.5, 0.0, 1.0)
        h += amp * smoothstep(0.1, 0.9, mesa)
    h = flatten_along(h, x, z, [ROAD], 12.0, strength=0.75, reach=44.0, core=10.0)
    h = flatten_along(h, x, z, [[FORT, (FORT[0] + 1.0, FORT[1])]], 12.0, strength=0.9, reach=150.0, core=100.0)
    wash = smoothstep(34.0, 12.0, polyline_distance(x, z, WASH))
    h = blend_toward(h, np.minimum(h, 7.5), wash)
    h = h + mountain_rim(x, z)
    return x, z, np.clip(h, HEIGHT_MIN, HEIGHT_MAX)


def build_ground(x, z, h):
    c = np.zeros(x.shape + (3,))
    c[:] = SCRUB
    c += (9.0 * np.sin(x * 0.045) * np.cos(z * 0.038))[..., None]
    c += (12.0 * np.sin(x * 0.005 - 0.3) * np.cos(z * 0.006 + 0.8))[..., None]
    patch = smoothstep(0.0, 0.5, smooth_noise(x, z, 2221))[..., None]
    c = c * (1.0 - 0.45 * patch) + RED * 0.45 * patch
    edge = smoothstep(26.0, -10.0, blob_distance(x, z, GROVES))[..., None]
    c = c * (1.0 - edge) + SCRUB_DARK * edge
    edge = smoothstep(20.0, -20.0, blob_distance(x, z, ROCKY))[..., None]
    c = c * (1.0 - edge) + ROCK * edge
    edge = smoothstep(ROAD_HALF + 2.0, ROAD_HALF - 2.0, polyline_distance(x, z, ROAD))[..., None]
    c = c * (1.0 - edge) + DIRT * edge
    wash = smoothstep(26.0, 10.0, polyline_distance(x, z, WASH))[..., None]
    c = c * (1.0 - wash) + SAND * wash
    rock = smoothstep(28.0, 42.0, h)[..., None]
    c = c * (1.0 - rock) + ROCK * rock
    return np.clip(c, 0, 255).astype(np.uint8)


def build_objects(h_at, h):
    objs = []
    fx, fz = FORT
    for o in walled_square(FORT_HALF):
        o["x"] = round(o["x"] + fx, 2)
        o["z"] = round(o["z"] + fz, 2)
        objs.append(o)
    for i, (tx, tz) in enumerate([(-28, -22), (24, -30), (-30, 24), (30, 26), (0, -2)]):
        objs.append(box("tent", fx + tx, fz + tz, 9, 4.5, 7, CANVAS if i % 2 == 0 else AWNINGS[0]))
    objs.append(box("campfire", fx + 8, fz + 12, 3, 1, 3, [0.7, 0.32, 0.1], block=False))
    for cx, cz in [(-46, 8), (-44, 15), (46, -12)]:
        objs.append(box("crate", fx + cx, fz + cz, 3, 2.4, 3, WOOD))

    zx, zz = CIRCLE
    for i in range(8):
        a = i * math.pi / 4.0
        objs.append(box("standing_stone", zx + 22 * math.cos(a), zz + 22 * math.sin(a), 2.6, 6.5, 2.2, STONE_DARK))
    objs.append(box("altar", zx, zz, 6, 1.4, 4, STONE_DARK))

    dx, dz = DEN
    objs.append(box("den_mouth", dx, dz, 16, 6, 3, DARK))
    objs.append(box("bone_pile", dx - 16, dz + 8, 4, 1, 3, [0.85, 0.82, 0.72], block=False))

    def clear(px, pz):
        pt_x, pt_z = np.array([[px]]), np.array([[pz]])
        if polyline_distance(pt_x, pt_z, ROAD)[0, 0] < 16:
            return False
        if any(math.hypot(px - qx, pz - qz) < 45 for qx, qz in SPECIALS):
            return False
        if abs(px - fx) < FORT_HALF + 15 and abs(pz - fz) < FORT_HALF + 15:
            return False
        return HALF - max(abs(px), abs(pz)) > RIM_BLOCK + 20

    objs += scatter_boulders(np.random.RandomState(2022), 60, ROCKY, clear)

    def keepout(px, pz):
        hit = near_polylines(px, pz, [ROAD], ROAD_HALF + 14.0)
        hit |= near_points(px, pz, SPECIALS, 45.0)
        hit |= rect_mask(px, pz, fx - FORT_HALF - 15, fz - FORT_HALF - 15, fx + FORT_HALF + 15, fz + FORT_HALF + 15)
        hit |= near_objects(px, pz, objs, 10.0)
        return hit

    objs += scatter_forest(np.random.RandomState(2021), h, "border", GROVES, keepout, 2000, sparse_chance=0.0005)
    return objs


def make_splat(h, objs):
    x, z = splat_grid()
    hh = height_at_grid(h, x, z)
    road, shoulder = road_weight(x, z, [ROAD], [], [], 0.0)
    wooded = forest_floor(x, z, GROVES, objs)
    patch = smoothstep(-0.2, 0.3, smooth_noise(x, z, 2222))
    dry = 0.25 + 0.45 * patch - 0.2 * wooded
    wash = smoothstep(26.0, 10.0, polyline_distance(x, z, WASH))
    camp = smoothstep(110.0, 70.0, np.hypot(x - FORT[0], z - FORT[1]))
    dirt = 0.45 * shoulder + 1.0 * wooded + 0.8 * wash + 0.7 * camp + 0.15 * (1.0 - patch)
    rock = np.maximum(smoothstep(18.0, -18.0, blob_distance(x, z, ROCKY)), smoothstep(26.0, 40.0, hh))
    return build_splat(x, z, hh, dry, dirt, road, rock)


def main():
    x, z, h = build_height()
    objs = build_objects(make_height_sampler(h), h)
    ground = build_ground(x, z, h)
    block = build_block(objs)
    roads = build_roads([ROAD], [], [], 0.0)

    assert_walkable(h, block, [WEST_PORTAL, WEST_ARRIVAL, EAST_PORTAL, EAST_ARRIVAL], "map %d" % MAP_ID)

    meta = {
        "id": MAP_ID,
        "name": "Demirkaya Badlands",
        "kingdom": 2,
        "worldSize": WORLD,
        "heightMin": HEIGHT_MIN,
        "heightMax": HEIGHT_MAX,
        "waterLevel": WATER,
        "spawn": {"x": WEST_ARRIVAL[0], "z": WEST_ARRIVAL[1], "yaw": -math.pi / 2.0},
        "portals": [
            portal(WEST_PORTAL[0], WEST_PORTAL[1], PORTAL_RADIUS, 201, 760.0, -30.0, -math.pi / 2.0),
            portal(EAST_PORTAL[0], EAST_PORTAL[1], PORTAL_RADIUS, 401, 735.0, -14.0, math.pi / 2.0),
        ],
        "objects": objs,
        "safeZones": [
            safe_zone("Forge Camp", [[-770.0, -30.0], [-700.0, -30.0], [-700.0, 50.0], [-770.0, 50.0]]),
        ],
        "npcs": [],
        "monsterSpawns": [
            monster_spawn(8, -520.0, 120.0, 40.0, 4),
            monster_spawn(8, -200.0, -200.0, 40.0, 3),
            monster_spawn(8, -430.0, -400.0, 40.0, 5),
            monster_spawn(8, 150.0, 300.0, 40.0, 4),
            monster_spawn(9, 360.0, -250.0, 45.0, 7),
            monster_spawn(9, 560.0, -80.0, 35.0, 3),
            monster_spawn(7, 350.0, -240.0, 6.0, 1),
            monster_spawn(10, -260.0, 428.0, 25.0, 3),
            monster_spawn(10, 520.0, 360.0, 30.0, 3),
            monster_spawn(7, 620.0, -460.0, 15.0, 1),
        ],
    }
    check_map_access(h, objs, block, meta, "map %d" % MAP_ID)
    add_undergrowth(objs, h, block, roads, meta, "border", 7202)
    write_map(OUT, meta, h, ground, block, roads, objs, make_splat(h, objs))


if __name__ == "__main__":
    main()
