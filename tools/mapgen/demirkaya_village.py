"""Draws map 201 - Demirkaya Village - into data/maps/201_demirkaya_village/.

A one-off sketching tool. A stone mining village on a rocky foothill terrace:
a half-walled square facing the plains, a forge street, an iron mine cut into
the north hill, a quarry to the west and pine-dotted scree around. Shared
building blocks live in mapkit.py.

Needs numpy and Pillow.
"""

import math

import numpy as np

from mapkit import *

MAP_ID = 201
OUT = map_directory(MAP_ID, "demirkaya_village")

VILLAGE_HALF = 147.0
PLAZA_HALF = 26.0
TERRACE = 14.0

ROADS = [
    [(VILLAGE_HALF, 0), (500, -20), (700, 40), (810, -30)],
    [(0, -VILLAGE_HALF), (0, -280), (-30, -430), (-40, -480)],
    [(0, VILLAGE_HALF), (20, 400), (-30, 620), (-10, 800)],
    [(-VILLAGE_HALF, 0), (-450, 20), (-650, 120), (-770, 170)],
]
VILLAGE_LANES = [
    (-GATE_HALF, -VILLAGE_HALF, GATE_HALF, VILLAGE_HALF),
    (-VILLAGE_HALF, -GATE_HALF, VILLAGE_HALF, GATE_HALF),
    (-110, -56, 110, -50), (-110, 50, 110, 56),
]
ROCKY = [(-770, 190, 150), (-560, -420, 150), (560, -500, 170), (700, 420, 140), (-300, 640, 130)]
GROVES = [(-300, -280, 120), (320, -300, 130), (520, 300, 140), (-520, 420, 120), (150, 640, 110)]
MINE = (-40.0, -486.0)
QUARRY = (-780.0, 180.0)

BUMPS = [(-40, -620, 170, 44), (-560, -420, 170, 30), (560, -520, 190, 34), (-250, -560, 150, 22),
         (700, 420, 150, 26), (-300, 650, 140, 22), (330, 500, 200, 9), (-680, -120, 160, 12)]

KEEP_CLEAR = [(810.0, -32.0), (760.0, -30.0), MINE, QUARRY]

GRAVEL = np.array([118, 110, 100])
GRAVEL_DARK = np.array([86, 82, 78])
SPOIL = np.array([138, 96, 66])
DIRT = np.array([140, 114, 86])
PAVING = np.array([150, 146, 140])
ROCK = np.array([112, 108, 104])
SNOW = np.array([236, 236, 242])
IRON = [0.40, 0.34, 0.32]
DARK = [0.08, 0.07, 0.07]


def build_height():
    x, z = grid(HEIGHT_RES, cell_centred=False)
    h = np.full(x.shape, TERRACE)
    h += 1.6 * np.sin(x * 0.014 + 0.9) * np.cos(z * 0.011 - 0.5)
    h += 1.2 * np.sin(x * 0.043) * np.sin(z * 0.037 + 1.0)
    for bx, bz, r, amp in BUMPS:
        h += amp * np.exp(-((x - bx) ** 2 + (z - bz) ** 2) / (2.0 * (r * 0.5) ** 2))
    square = np.maximum(np.abs(x), np.abs(z))
    h = blend_toward(h, TERRACE, smoothstep(320.0, 220.0, square))
    h = flatten_along(h, x, z, ROADS, TERRACE, strength=0.7, reach=46.0, core=10.0)
    pit = smoothstep(70.0, 20.0, np.hypot(x - QUARRY[0], z - QUARRY[1]))
    h = blend_toward(h, 6.0, 0.9 * pit)
    h = h + mountain_rim(x, z)
    return x, z, np.clip(h, HEIGHT_MIN, HEIGHT_MAX)


def build_ground(x, z, h):
    c = np.zeros(x.shape + (3,))
    c[:] = GRAVEL
    c += (9.0 * np.sin(x * 0.05) * np.cos(z * 0.041))[..., None]
    c += (10.0 * np.sin(x * 0.006 - 0.3) * np.cos(z * 0.005 + 0.8))[..., None]
    edge = smoothstep(26.0, -10.0, blob_distance(x, z, GROVES))[..., None]
    c = c * (1.0 - edge) + GRAVEL_DARK * edge
    edge = smoothstep(20.0, -20.0, blob_distance(x, z, ROCKY))[..., None]
    c = c * (1.0 - edge) + ROCK * edge
    square = np.maximum(np.abs(x), np.abs(z))
    c[square <= VILLAGE_HALF] = DIRT * 0.5 + GRAVEL * 0.5
    for x0, z0, x1, z1 in VILLAGE_LANES:
        c[rect_mask(x, z, x0, z0, x1, z1)] = DIRT
    c[square <= PLAZA_HALF] = PAVING
    for road in ROADS:
        edge = smoothstep(ROAD_HALF + 2.0, ROAD_HALF - 2.0, polyline_distance(x, z, road))[..., None]
        c = c * (1.0 - edge) + DIRT * edge
    spoil = np.maximum(smoothstep(60.0, 20.0, np.hypot(x - MINE[0], z - MINE[1])),
                       smoothstep(80.0, 30.0, np.hypot(x - QUARRY[0], z - QUARRY[1])))[..., None]
    c = c * (1.0 - 0.8 * spoil) + SPOIL * 0.8 * spoil
    rock = smoothstep(30.0, 44.0, h)[..., None]
    c = c * (1.0 - rock) + ROCK * rock
    snow = smoothstep(84.0, 90.0, h)[..., None]
    c = c * (1.0 - snow) + SNOW * snow
    return np.clip(c, 0, 255).astype(np.uint8)


def build_objects(h_at, h):
    objs = [o for o in walled_square(VILLAGE_HALF) if o["x"] > 100 or o["z"] > 100]

    objs.append(box("anvil_block", 0, 0, 5, 1.2, 3, IRON))
    objs.append(box("brazier", 0, -12, 2, 1.4, 2, [0.62, 0.30, 0.12]))
    objs += building("forge", 0, -86, 5, door_facing(0, -86, True), 5.0, high_roof=True)
    objs.append(box("chimney", 8, -92, 3, 9, 3, IRON, block=False, elevation=3.0))
    objs += building("hall", 0, 88, 5, door_facing(0, 88, True), 5.0, high_roof=True)
    for i, xx in enumerate((-100, -70, -42, 42, 70, 100)):
        objs += building("house", xx, -33, 3, door_facing(xx, -33, True), 4.5, wood=i % 3 == 1)
        objs += building("house", xx, 33, 3, door_facing(xx, 33, True), 4.5, wood=i % 3 == 0)
    for xx in (-105, -68, 68, 105):
        objs += building("house", xx, -76, 3, door_facing(xx, -76, True), 4.5, wood=xx < 0)
    for xx in (-105, 105):
        objs += building("house", xx, 76, 3, door_facing(xx, 76, True), 4.5)
    for sx in (-44, 44):
        objs.append(box("ore_cart", sx, 8, 5, 1.6, 3, WOOD))
    for sx in (-14, 14):
        objs.append(box("stall", sx, 20, 5, 1.2, 3, WOOD))
        objs.append(box("awning", sx, 20, 5.6, 0.4, 3.6, AWNINGS[1], block=False, elevation=2.8))

    mx, mz = MINE
    for s in (-1, 1):
        objs.append(box("mine_post", mx + s * 12, mz, 3, 7, 3, WOOD))
    objs.append(box("mine_lintel", mx, mz, 29, 2, 3, WOOD, block=False, elevation=6.0))
    objs.append(box("mine_mouth", mx, mz - 5, 22, 7, 3, DARK))
    for cx, cz in [(mx - 40, mz + 24), (mx + 36, mz + 18)]:
        objs.append(box("ore_cart", cx, cz, 5, 1.6, 3, WOOD))
    qx, qz = QUARRY
    for i in range(6):
        a = i * 1.05
        objs.append(box("stone_stack", qx + 46 * math.cos(a), qz + 40 * math.sin(a), 6, 2.4, 4, STONE))
    objs.append(box("crane", qx + 14, qz - 20, 3, 8, 3, WOOD))

    def clear(px, pz):
        pt_x, pt_z = np.array([[px]]), np.array([[pz]])
        if min(polyline_distance(pt_x, pt_z, r)[0, 0] for r in ROADS) < 16:
            return False
        if max(abs(px), abs(pz)) < 170:
            return False
        if any(math.hypot(px - kx, pz - kz) < 30 for kx, kz in KEEP_CLEAR):
            return False
        return HALF - max(abs(px), abs(pz)) > RIM_BLOCK + 20

    objs += scatter_boulders(np.random.RandomState(2012), 70, ROCKY, clear)

    def keepout(px, pz):
        hit = near_polylines(px, pz, ROADS, ROAD_HALF + 14.0)
        hit |= np.maximum(np.abs(px), np.abs(pz)) < 170
        hit |= near_points(px, pz, KEEP_CLEAR, 40.0)
        hit |= near_objects(px, pz, objs, 10.0)
        return hit

    objs += scatter_forest(np.random.RandomState(2011), h, "border", GROVES, keepout, 2400, sparse_chance=0.0006)
    return objs


def make_splat(h, objs):
    x, z = splat_grid()
    hh = height_at_grid(h, x, z)
    road, shoulder = road_weight(x, z, ROADS, VILLAGE_LANES, [], PLAZA_HALF)
    wooded = forest_floor(x, z, GROVES, objs)
    square = np.maximum(np.abs(x), np.abs(z))
    inside = smoothstep(VILLAGE_HALF + 4.0, VILLAGE_HALF - 4.0, square)
    patch = smoothstep(-0.1, 0.4, smooth_noise(x, z, 2211))
    spoil = np.maximum(smoothstep(60.0, 20.0, np.hypot(x - MINE[0], z - MINE[1])),
                       smoothstep(80.0, 30.0, np.hypot(x - QUARRY[0], z - QUARRY[1])))
    dry = 0.02 + 0.12 * patch + 0.15 * wooded
    dirt = 0.4 * shoulder + 0.7 * inside + 0.9 * spoil + 1.0 * wooded + 0.2 * (1.0 - patch)
    rock = np.maximum(np.maximum(smoothstep(18.0, -18.0, blob_distance(x, z, ROCKY)), smoothstep(26.0, 42.0, hh)),
                      0.7 * smoothstep(-0.3, 0.3, smooth_noise(x, z, 2212)) * (1.0 - 0.7 * inside))
    return build_splat(x, z, hh, dry, dirt, road, rock)


def main():
    x, z, h = build_height()
    objs = build_objects(make_height_sampler(h), h)
    ground = build_ground(x, z, h)
    block = build_block(objs)
    roads = build_roads(ROADS, VILLAGE_LANES, [], PLAZA_HALF)

    assert_walkable(h, block, [(810.0, -32.0), (760.0, -30.0), (0.0, 30.0)], "map %d" % MAP_ID)

    meta = {
        "id": MAP_ID,
        "name": "Demirkaya Village",
        "kingdom": 2,
        "worldSize": WORLD,
        "heightMin": HEIGHT_MIN,
        "heightMax": HEIGHT_MAX,
        "waterLevel": WATER,
        "spawn": {"x": 0.0, "z": 30.0, "yaw": -math.pi / 2.0},
        "portals": [
            portal(810.0, -32.0, 14.0, 202, -735.0, 14.0, -math.pi / 2.0),
        ],
        "objects": objs,
        "safeZones": [
            safe_zone("Forge Square", [[-44.0, -44.0], [44.0, -44.0], [44.0, 44.0], [-44.0, 44.0]]),
        ],
        "npcs": [
            npc("Weaponsmith", "weaponsmith", "models/warrior/armor_l30.glb", -29.0, 27.0, math.pi,
                shop=[1001, 1003, 1005, 4, 7]),
            npc("Armorsmith", "armorsmith", "models/warrior/armor_l50.glb", 29.0, 27.0, math.pi,
                shop=[2001, 2002, 2003, 3001, 3002, 3003]),
        ],
        "monsterSpawns": [
            monster_spawn(2, -300.0, -120.0, 14.0, 5),
            monster_spawn(2, 300.0, -140.0, 14.0, 4),
            monster_spawn(2, -280.0, 280.0, 14.0, 4),
            monster_spawn(3, 380.0, 220.0, 30.0, 2),
            monster_spawn(3, 150.0, -400.0, 30.0, 2),
            monster_spawn(3, 330.0, 440.0, 30.0, 2),
            monster_spawn(1, -430.0, -230.0, 30.0, 4),
            monster_spawn(1, 450.0, -280.0, 30.0, 4),
            monster_spawn(4, -540.0, -330.0, 25.0, 3),
            monster_spawn(4, 560.0, -400.0, 25.0, 3),
            monster_spawn(5, -600.0, -420.0, 6.0, 1),
            monster_spawn(6, -450.0, 400.0, 40.0, 2),
            monster_spawn(6, 640.0, 280.0, 30.0, 2),
            monster_spawn(7, -190.0, 740.0, 15.0, 1),
        ],
    }
    check_map_access(h, objs, block, meta, "map %d" % MAP_ID)
    write_map(OUT, meta, h, ground, block, roads, objs, make_splat(h, objs))


if __name__ == "__main__":
    main()
