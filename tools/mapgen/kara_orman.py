"""Draws map 403 - Kara Orman - into data/maps/403_kara_orman/.

A one-off sketching tool. Shared, kingdom-less black forest at the end of the
Kizilkum Desert: a trail leaves the west gate, thins out among dense dark
pines and oaks, passes spider nests and a ring of standing stones around an
altar, and ends at the dark hollow where Erlik's servant waits in the east.
Shared building blocks live in mapkit.py.

Needs numpy and Pillow.
"""

import math

import numpy as np

from mapkit import *

MAP_ID = 403
OUT = map_directory(MAP_ID, "kara_orman")

ROAD = [(-800, 0), (-600, 40), (-380, -10), (-160, 60), (60, 20), (220, 90)]
FOREST = [(-520, -260, 170), (-520, 300, 170), (-250, -300, 160), (-200, 330, 170), (90, -330, 170),
          (60, 330, 160), (350, -220, 170), (430, 260, 160), (620, 40, 150), (-60, -40, 110),
          (-700, -80, 120), (250, -520, 120), (260, 600, 120), (-400, 560, 110), (-380, -600, 110)]
ROCKY = [(500, -480, 120), (-640, 560, 110)]
ALTAR = (220.0, 100.0)
HOLLOW = (560.0, -140.0)
NESTS = [(-300.0, -330.0), (-240.0, 370.0), (130.0, -380.0), (300.0, 480.0)]

BUMPS = [(500, -480, 140, 22), (-640, 560, 130, 16), (300, -100, 220, 7), (-300, 150, 240, 6),
         (100, 450, 200, 7), (-500, -400, 200, 6)]

WEST_PORTAL = (-800.0, 0.0)
WEST_ARRIVAL = (-735.0, 14.0)
PORTAL_RADIUS = 14.0
SPECIALS = [ALTAR, HOLLOW, WEST_PORTAL, WEST_ARRIVAL] + NESTS
MAX_TREES = 2600

MOSS_FLOOR = np.array([40, 66, 46])
GRASS_DARK = np.array([34, 58, 42])
FOREST_BLACK = np.array([26, 42, 34])
DIRT = np.array([92, 78, 62])
LEAF_LITTER = np.array([78, 62, 44])
ROCK = np.array([92, 92, 96])
STONE_DARK = [0.26, 0.27, 0.29]
DARK = [0.06, 0.06, 0.07]
WEB = [0.82, 0.82, 0.84]
BONE = [0.80, 0.78, 0.70]


def build_height():
    x, z = grid(HEIGHT_RES, cell_centred=False)
    h = np.full(x.shape, 8.0)
    h += 1.4 * np.sin(x * 0.012 + 0.4) * np.cos(z * 0.01 - 0.8)
    for bx, bz, r, amp in BUMPS:
        h += amp * np.exp(-((x - bx) ** 2 + (z - bz) ** 2) / (2.0 * (r * 0.5) ** 2))
    h = flatten_along(h, x, z, [ROAD], 8.0, strength=0.55, reach=40.0, core=8.0)
    h = blend_toward(h, 8.0, 0.8 * smoothstep(50.0, 30.0, np.hypot(x - ALTAR[0], z - ALTAR[1])))
    h = h + mountain_rim(x, z)
    return x, z, np.clip(h, HEIGHT_MIN, HEIGHT_MAX)


def build_ground(x, z, h):
    c = np.zeros(x.shape + (3,))
    c[:] = MOSS_FLOOR
    c += (6.0 * np.sin(x * 0.05) * np.cos(z * 0.043))[..., None]
    litter = smoothstep(-0.1, 0.4, smooth_noise(x, z, 4031))[..., None]
    c = c * (1.0 - 0.5 * litter) + LEAF_LITTER * 0.5 * litter
    edge = smoothstep(30.0, -10.0, blob_distance(x, z, FOREST))[..., None]
    c = c * (1.0 - edge) + FOREST_BLACK * edge
    edge = smoothstep(20.0, -20.0, blob_distance(x, z, ROCKY))[..., None]
    c = c * (1.0 - edge) + ROCK * edge
    edge = smoothstep(ROAD_HALF + 2.0, ROAD_HALF - 2.0, polyline_distance(x, z, ROAD))[..., None]
    c = c * (1.0 - edge) + DIRT * edge
    rock = smoothstep(30.0, 42.0, h)[..., None]
    c = c * (1.0 - rock) + ROCK * rock
    return np.clip(c, 0, 255).astype(np.uint8)


def build_objects(h_at, h):
    objs = []
    ax, az = ALTAR
    for i in range(8):
        a = i * 2.0 * math.pi / 8.0
        objs.append(box("standing_stone", ax + 22 * math.cos(a), az + 22 * math.sin(a), 2.4, 6.5, 2.2, STONE_DARK))
    objs.append(box("altar", ax, az, 5, 1.4, 4, STONE_DARK))

    hx, hz = HOLLOW
    objs.append(box("den_mouth", hx, hz, 18, 7, 4, DARK))
    for ox, oz in [(-16, 12), (14, 14), (-4, 22)]:
        objs.append(box("bone_pile", hx + ox, hz + oz, 4, 1, 3, BONE, block=False))
    for i in range(6):
        a = i * 2.0 * math.pi / 6.0
        objs.append(box("standing_stone", hx + 30 * math.cos(a), hz + 30 * math.sin(a) + 6, 2.4, 6.0, 2.2, STONE_DARK))

    for nx, nz in NESTS:
        objs.append(box("den_mouth", nx, nz, 10, 4, 3, DARK))
        objs.append(box("web", nx + 12, nz + 4, 8, 0.4, 8, WEB, block=False))
        objs.append(box("bone_pile", nx - 9, nz + 7, 4, 1, 3, BONE, block=False))

    def clear(px, pz):
        pt_x, pt_z = np.array([[px]]), np.array([[pz]])
        if polyline_distance(pt_x, pt_z, ROAD)[0, 0] < 16:
            return False
        if any(math.hypot(px - qx, pz - qz) < 45 for qx, qz in SPECIALS):
            return False
        return HALF - max(abs(px), abs(pz)) > RIM_BLOCK + 20

    objs += scatter_boulders(np.random.RandomState(4032), 28, ROCKY, clear)

    def keepout(px, pz):
        hit = near_polylines(px, pz, [ROAD], ROAD_HALF + 10.0)
        hit |= near_points(px, pz, SPECIALS, 42.0)
        hit |= near_objects(px, pz, objs, 9.0)
        return hit

    objs += scatter_forest(np.random.RandomState(4031), h, "dark", FOREST, keepout, MAX_TREES, sparse_chance=0.0006)
    return objs


def make_splat(h, objs):
    x, z = splat_grid()
    hh = height_at_grid(h, x, z)
    road, shoulder = road_weight(x, z, [ROAD], [], [], 0.0)
    wooded = forest_floor(x, z, FOREST, objs)
    patch = smoothstep(-0.2, 0.3, smooth_noise(x, z, 4033))
    altar = smoothstep(60.0, 30.0, np.hypot(x - ALTAR[0], z - ALTAR[1]))
    dry = 0.05 + 0.1 * patch + 0.2 * wooded
    dirt = 0.4 * shoulder + 1.0 * wooded + 0.15 * (1.0 - patch) + 0.6 * altar
    rock = np.maximum(smoothstep(18.0, -18.0, blob_distance(x, z, ROCKY)), smoothstep(30.0, 44.0, hh))
    return build_splat(x, z, hh, dry, dirt, road, rock)


def main():
    x, z, h = build_height()
    objs = build_objects(make_height_sampler(h), h)
    ground = build_ground(x, z, h)
    block = build_block(objs)
    roads = build_roads([ROAD], [], [], 0.0)

    assert_walkable(h, block, [WEST_PORTAL, WEST_ARRIVAL], "map %d" % MAP_ID)

    meta = {
        "id": MAP_ID,
        "name": "Kara Orman",
        "kingdom": 0,
        "worldSize": WORLD,
        "heightMin": HEIGHT_MIN,
        "heightMax": HEIGHT_MAX,
        "waterLevel": WATER,
        "spawn": {"x": WEST_ARRIVAL[0], "z": WEST_ARRIVAL[1], "yaw": -math.pi / 2.0},
        "portals": [
            portal(WEST_PORTAL[0], WEST_PORTAL[1], PORTAL_RADIUS, 402, 735.0, -14.0, math.pi / 2.0),
        ],
        "objects": objs,
        "safeZones": [],
        "npcs": [],
        "monsterSpawns": [
            monster_spawn(15, -420.0, 120.0, 45.0, 5),
            monster_spawn(15, -300.0, -330.0, 35.0, 4),
            monster_spawn(15, -240.0, 370.0, 35.0, 4),
            monster_spawn(16, 0.0, -250.0, 45.0, 4),
            monster_spawn(16, 130.0, -380.0, 40.0, 4),
            monster_spawn(16, 60.0, 330.0, 45.0, 4),
            monster_spawn(17, 220.0, 100.0, 35.0, 4),
            monster_spawn(17, 430.0, 260.0, 45.0, 4),
            monster_spawn(17, 350.0, -220.0, 45.0, 4),
            monster_spawn(18, 560.0, -140.0, 15.0, 1),
        ],
    }
    check_map_access(h, objs, block, meta, "map %d" % MAP_ID)
    add_undergrowth(objs, h, block, roads, meta, "dark", 7403)
    write_map(OUT, meta, h, ground, block, roads, objs, make_splat(h, objs))


if __name__ == "__main__":
    main()
