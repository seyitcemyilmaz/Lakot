"""Draws map 301 - Sisvadi Village - into data/maps/301_sisvadi_village/.

A one-off sketching tool. A timber village on an open green in a misty river
valley: no walls, a longhouse, a mill and bridge where a river crosses the
east road, a mill pond to the south-west and dense pine and birch forest all
round. Shared building blocks live in mapkit.py.

Needs numpy and Pillow.
"""

import math

import numpy as np

from mapkit import *

MAP_ID = 301
OUT = map_directory(MAP_ID, "sisvadi_village")

GREEN_HALF = 150.0

ROADS = [
    [(GATE_HALF, 0), (330, 3), (560, -30), (700, 30), (810, -32)],
    [(0, -130), (0, -400), (-60, -700), (-40, -840)],
    [(0, 130), (0, 400), (60, 700), (40, 840)],
    [(-130, 0), (-420, -10), (-700, 30), (-840, 40)],
]
VILLAGE_LANES = [(-130, -4, 130, 4), (-4, -130, 4, 130)]
RIVER = [(300, -900), (300, -600), (340, -300), (330, -60), (322, 100), (380, 350), (480, 600), (560, 800)]
POND = [((-420, 300), 70), ((-340, 360), 45)]
BRIDGES = [((330, 3), "ew")]
FOREST = [(-450, -520, 210), (-620, 220, 170), (470, -560, 190), (540, 560, 170), (60, -780, 180),
          (-220, 720, 170), (-780, -300, 130), (230, 330, 90)]
ROCKY = [(-560, 640, 110), (700, -620, 110)]
MILL = (288.0, -40.0)

BUMPS = [(-300, -300, 220, 8), (250, -400, 200, 8), (-560, 640, 150, 14), (700, -620, 150, 14),
         (200, 560, 200, 6), (-700, -100, 170, 6)]

KEEP_CLEAR = [(810.0, -32.0), (760.0, -30.0), MILL]

GRASS = np.array([74, 112, 74])
MOSS = np.array([56, 90, 62])
FOREST_DARK = np.array([42, 74, 50])
DIRT = np.array([128, 108, 82])
SAND = np.array([176, 168, 134])
ROCK = np.array([120, 118, 112])
RIVERBED = np.array([84, 100, 92])
PLANKS = np.array([118, 86, 54])


def build_height():
    x, z = grid(HEIGHT_RES, cell_centred=False)
    h = np.full(x.shape, 7.0)
    h += 0.9 * np.sin(x * 0.01 + 0.4) * np.cos(z * 0.013 - 0.7)
    for bx, bz, r, amp in BUMPS:
        h += amp * np.exp(-((x - bx) ** 2 + (z - bz) ** 2) / (2.0 * (r * 0.5) ** 2))
    square = np.maximum(np.abs(x), np.abs(z))
    h = blend_toward(h, 9.0, smoothstep(250.0, 190.0, square))
    h = flatten_along(h, x, z, ROADS, 8.0, strength=0.5, reach=40.0, core=8.0)
    h = carve_river(h, x, z, RIVER)
    h = carve_ponds(h, x, z, POND)
    for bridge in BRIDGES:
        h = np.where(bridge_mask(x, z, bridge, 7), np.maximum(h, 1.6), h)
    h = h + mountain_rim(x, z)
    return x, z, np.clip(h, HEIGHT_MIN, HEIGHT_MAX)


def build_ground(x, z, h):
    c = np.zeros(x.shape + (3,))
    c[:] = GRASS
    c += (7.0 * np.sin(x * 0.05) * np.cos(z * 0.043))[..., None]
    c += (10.0 * np.sin(x * 0.004 - 0.3) * np.cos(z * 0.005 + 0.8))[..., None]
    moss = smoothstep(0.0, 0.5, smooth_noise(x, z, 3011))[..., None]
    c = c * (1.0 - 0.5 * moss) + MOSS * 0.5 * moss
    edge = smoothstep(30.0, -10.0, blob_distance(x, z, FOREST))[..., None]
    c = c * (1.0 - edge) + FOREST_DARK * edge
    edge = smoothstep(20.0, -20.0, blob_distance(x, z, ROCKY))[..., None]
    c = c * (1.0 - edge) + ROCK * edge
    for x0, z0, x1, z1 in VILLAGE_LANES:
        c[rect_mask(x, z, x0, z0, x1, z1)] = DIRT
    for road in ROADS:
        edge = smoothstep(ROAD_HALF + 2.0, ROAD_HALF - 2.0, polyline_distance(x, z, road))[..., None]
        c = c * (1.0 - edge) + DIRT * edge
    near = smoothstep(32.0, 20.0, polyline_distance(x, z, RIVER))
    for (cx, cz), r in POND:
        near = np.maximum(near, smoothstep(r + 26.0, r + 12.0, np.hypot(x - cx, z - cz)))
    sand = (near * smoothstep(3.0, 1.5, h))[..., None]
    c = c * (1.0 - sand) + SAND * sand
    bed = smoothstep(0.5, -1.0, h)[..., None]
    c = c * (1.0 - bed) + RIVERBED * bed
    for bridge in BRIDGES:
        c[bridge_mask(x, z, bridge, 6)] = PLANKS
    rock = smoothstep(38.0, 50.0, h)[..., None]
    c = c * (1.0 - rock) + ROCK * rock
    return np.clip(c, 0, 255).astype(np.uint8)


def build_objects(h_at, h):
    objs = []
    objs += building("longhouse", 0, -64, 5, door_facing(0, -64, True), 5.0, wood=True, high_roof=True)
    objs.append(box("well", 0, 12, 4, 1.2, 4, STONE))
    objs.append(box("well_roof", 0, 12, 5, 0.6, 5, ROOF_BROWN, block=False, elevation=3.2))
    for xx in (-120, -84, 84, 120):
        objs += building("house", xx, -100, 3, door_facing(xx, -100, True), 4.5, wood=True, high_roof=xx in (-84, 84))
    for xx in (-120, -84, -48, 48, 84, 120):
        objs += building("house", xx, 100, 3, door_facing(xx, 100, True), 4.5, wood=True, high_roof=xx in (-120, 48))
    for zz in (-34, 34):
        objs += building("house", -62, zz, 3, door_facing(-62, zz, False), 4.5, wood=True)
        objs += building("house", 62, zz, 3, door_facing(62, zz, False), 4.5, wood=True, high_roof=True)
    for i, (rx, rz) in enumerate([(-24, 40), (24, 40), (-24, -20), (24, -22)]):
        objs.append(box("drying_rack", rx, rz, 5, 2.2, 1.2, WOOD))
    for i, (sx, sz) in enumerate([(-18, 26), (18, 26)]):
        objs.append(box("stall", sx, sz, 5, 1.2, 3, WOOD))
        objs.append(box("awning", sx, sz, 5.6, 0.4, 3.6, AWNINGS[2], block=False, elevation=2.8))
    mx, mz = MILL
    objs += building("mill", mx, mz, 3, math.pi / 2.0, 4.5, wood=True, high_roof=True)
    objs.append(box("mill_wheel", mx + 20, mz, 1.5, 8, 8, WOOD))
    objs += bridge_railings(BRIDGES)

    def clear(px, pz):
        pt_x, pt_z = np.array([[px]]), np.array([[pz]])
        if min(polyline_distance(pt_x, pt_z, r)[0, 0] for r in ROADS) < 14:
            return False
        if polyline_distance(pt_x, pt_z, RIVER)[0, 0] < 30:
            return False
        if max(abs(px), abs(pz)) < 190:
            return False
        return HALF - max(abs(px), abs(pz)) > RIM_BLOCK + 20

    objs += scatter_boulders(np.random.RandomState(3012), 30, ROCKY, clear)

    def keepout(px, pz):
        hit = near_polylines(px, pz, ROADS, ROAD_HALF + 14.0)
        hit |= polyline_distance(px, pz, RIVER) < 30
        hit |= near_circles(px, pz, [(cx, cz, r) for (cx, cz), r in POND], 30.0)
        hit |= np.maximum(np.abs(px), np.abs(pz)) < 190
        hit |= near_points(px, pz, KEEP_CLEAR, 40.0)
        hit |= near_objects(px, pz, objs, 10.0)
        for bridge in BRIDGES:
            hit |= bridge_mask(px, pz, bridge, 20, 40)
        return hit

    objs += scatter_forest(np.random.RandomState(3011), h, "mist", FOREST, keepout, 6200, sparse_chance=0.0006)
    return objs


def make_splat(h, objs):
    x, z = splat_grid()
    hh = height_at_grid(h, x, z)
    road, shoulder = road_weight(x, z, ROADS, VILLAGE_LANES, BRIDGES, 0.0)
    wooded = forest_floor(x, z, FOREST, objs)
    patch = smoothstep(-0.2, 0.3, smooth_noise(x, z, 3012))
    dry = 0.04 + 0.12 * patch + 0.25 * wooded
    dirt = (0.4 * shoulder + 1.0 * wooded + 0.12 * (1.0 - patch)
            + 0.75 * shore_weight(x, z, hh, [("line", RIVER), ("circles", POND)]))
    rock = np.maximum(smoothstep(18.0, -18.0, blob_distance(x, z, ROCKY)), smoothstep(36.0, 50.0, hh))
    return build_splat(x, z, hh, dry, dirt, road, rock)


def main():
    x, z, h = build_height()
    objs = build_objects(make_height_sampler(h), h)
    ground = build_ground(x, z, h)
    block = build_block(objs)
    roads = build_roads(ROADS, VILLAGE_LANES, BRIDGES, 0.0)

    assert_walkable(h, block, [(810.0, -32.0), (760.0, -30.0), (0.0, 30.0)], "map %d" % MAP_ID)

    meta = {
        "id": MAP_ID,
        "name": "Sisvadi Village",
        "kingdom": 3,
        "worldSize": WORLD,
        "heightMin": HEIGHT_MIN,
        "heightMax": HEIGHT_MAX,
        "waterLevel": WATER,
        "spawn": {"x": 0.0, "z": 30.0, "yaw": -math.pi / 2.0},
        "portals": [
            portal(810.0, -32.0, 14.0, 302, -735.0, 14.0, -math.pi / 2.0),
        ],
        "objects": objs,
        "safeZones": [
            safe_zone("Village Green", [[-44.0, -44.0], [44.0, -44.0], [44.0, 44.0], [-44.0, 44.0]]),
        ],
        "npcs": [
            npc("Weaponsmith", "weaponsmith", "models/warrior/armor_l30.glb", -29.0, 27.0, math.pi,
                shop=[1001, 1003, 1005, 4, 7]),
            npc("Armorsmith", "armorsmith", "models/warrior/armor_l50.glb", 29.0, 27.0, math.pi,
                shop=[2001, 2002, 2003, 3001, 3002, 3003]),
        ],
        "monsterSpawns": [
            monster_spawn(2, -290.0, -200.0, 14.0, 5),
            monster_spawn(2, 200.0, -250.0, 14.0, 4),
            monster_spawn(2, -270.0, 150.0, 14.0, 4),
            monster_spawn(3, 200.0, 230.0, 30.0, 2),
            monster_spawn(3, -110.0, -430.0, 30.0, 2),
            monster_spawn(3, 150.0, 450.0, 30.0, 2),
            monster_spawn(1, -470.0, -250.0, 30.0, 4),
            monster_spawn(1, 430.0, -300.0, 30.0, 4),
            monster_spawn(4, -540.0, -330.0, 25.0, 3),
            monster_spawn(4, -620.0, -420.0, 25.0, 3),
            monster_spawn(5, -580.0, -380.0, 6.0, 1),
            monster_spawn(6, -480.0, 520.0, 40.0, 2),
            monster_spawn(6, 640.0, 180.0, 30.0, 2),
            monster_spawn(7, -180.0, 760.0, 15.0, 1),
        ],
    }
    check_map_access(h, objs, block, meta, "map %d" % MAP_ID)
    write_map(OUT, meta, h, ground, block, roads, objs, make_splat(h, objs))


if __name__ == "__main__":
    main()
