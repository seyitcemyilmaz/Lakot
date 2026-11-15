"""Draws map 302 - Sisvadi Valley - into data/maps/302_sisvadi_valley/.

A one-off sketching tool. A misty forested valley between Sisvadi Village and
the Borderlands: a river runs north to south across the road under a plank
bridge, a bandit stockade stands on the east bank, a bear cliff rises to the
north-east and a ring of dark stones hides in the western pines. Shared
building blocks live in mapkit.py.

Needs numpy and Pillow.
"""

import math

import numpy as np

from mapkit import *

MAP_ID = 302
OUT = map_directory(MAP_ID, "sisvadi_valley")

ROAD = [(-800, 0), (-450, 30), (-75, 4), (300, -30), (800, 0)]
RIVER = [(-90, -900), (-70, -450), (-100, -200), (-75, 4), (-40, 250), (-80, 520), (-120, 900)]
POND = [((-520, -120), 60), ((-470, -60), 38)]
LAKE = [((470, 520), 90), ((540, 470), 60)]
BRIDGES = [((-75, 4), "ew")]
FOREST = [(-420, -430, 200), (-560, 300, 190), (-220, -650, 160), (180, -520, 170), (250, 700, 160),
          (650, -150, 150), (-300, 650, 150), (680, 330, 90)]
ROCKY = [(450, -430, 150), (-650, 560, 110)]
STOCKADE = (330.0, 300.0)
STOCKADE_HALF = 62.0
HOLLOW = (-380.0, -380.0)
DEN = (450.0, -340.0)

BUMPS = [(450, -430, 180, 26), (-650, 560, 150, 16), (-300, -300, 220, 8), (600, 100, 200, 7),
         (-200, 500, 200, 7), (100, -300, 150, 6)]

WEST_PORTAL = (-800.0, 0.0)
WEST_ARRIVAL = (-735.0, 14.0)
EAST_PORTAL = (800.0, 0.0)
EAST_ARRIVAL = (735.0, -14.0)
PORTAL_RADIUS = 14.0
SPECIALS = [STOCKADE, HOLLOW, DEN, WEST_PORTAL, WEST_ARRIVAL, EAST_PORTAL, EAST_ARRIVAL]

GRASS = np.array([66, 102, 68])
MOSS = np.array([50, 84, 58])
FOREST_DARK = np.array([38, 68, 46])
DIRT = np.array([124, 104, 80])
MUD = np.array([96, 84, 66])
SAND = np.array([172, 164, 130])
ROCK = np.array([116, 114, 110])
RIVERBED = np.array([80, 96, 88])
PLANKS = np.array([118, 86, 54])
STONE_DARK = [0.30, 0.31, 0.30]
DARK = [0.08, 0.07, 0.07]
HIDE = [0.62, 0.52, 0.38]


def build_height():
    x, z = grid(HEIGHT_RES, cell_centred=False)
    h = np.full(x.shape, 7.0)
    h += 1.0 * np.sin(x * 0.011 + 1.1) * np.cos(z * 0.009 - 0.4)
    for bx, bz, r, amp in BUMPS:
        h += amp * np.exp(-((x - bx) ** 2 + (z - bz) ** 2) / (2.0 * (r * 0.5) ** 2))
    h = flatten_along(h, x, z, [ROAD], 8.0, strength=0.6, reach=40.0, core=8.0)
    h = blend_toward(h, 8.0, 0.9 * smoothstep(120.0, 85.0, np.maximum(np.abs(x - STOCKADE[0]), np.abs(z - STOCKADE[1]))))
    h = carve_river(h, x, z, RIVER)
    h = carve_ponds(h, x, z, POND + LAKE)
    for bridge in BRIDGES:
        h = np.where(bridge_mask(x, z, bridge, 7), np.maximum(h, 1.6), h)
    h = h + mountain_rim(x, z)
    return x, z, np.clip(h, HEIGHT_MIN, HEIGHT_MAX)


def build_ground(x, z, h):
    c = np.zeros(x.shape + (3,))
    c[:] = GRASS
    c += (7.0 * np.sin(x * 0.05) * np.cos(z * 0.043))[..., None]
    c += (10.0 * np.sin(x * 0.004 - 0.3) * np.cos(z * 0.005 + 0.8))[..., None]
    moss = smoothstep(-0.1, 0.4, smooth_noise(x, z, 3021))[..., None]
    c = c * (1.0 - 0.55 * moss) + MOSS * 0.55 * moss
    edge = smoothstep(30.0, -10.0, blob_distance(x, z, FOREST))[..., None]
    c = c * (1.0 - edge) + FOREST_DARK * edge
    edge = smoothstep(20.0, -20.0, blob_distance(x, z, ROCKY))[..., None]
    c = c * (1.0 - edge) + ROCK * edge
    camp = smoothstep(90.0, 62.0, np.maximum(np.abs(x - STOCKADE[0]), np.abs(z - STOCKADE[1])))[..., None]
    c = c * (1.0 - camp) + MUD * camp
    edge = smoothstep(ROAD_HALF + 2.0, ROAD_HALF - 2.0, polyline_distance(x, z, ROAD))[..., None]
    c = c * (1.0 - edge) + DIRT * edge
    near = smoothstep(32.0, 20.0, polyline_distance(x, z, RIVER))
    for (cx, cz), r in POND + LAKE:
        near = np.maximum(near, smoothstep(r + 26.0, r + 12.0, np.hypot(x - cx, z - cz)))
    sand = (near * smoothstep(3.0, 1.5, h))[..., None]
    c = c * (1.0 - sand) + SAND * sand
    bed = smoothstep(0.5, -1.0, h)[..., None]
    c = c * (1.0 - bed) + RIVERBED * bed
    for bridge in BRIDGES:
        c[bridge_mask(x, z, bridge, 6)] = PLANKS
    rock = smoothstep(36.0, 50.0, h)[..., None]
    c = c * (1.0 - rock) + ROCK * rock
    return np.clip(c, 0, 255).astype(np.uint8)


def palisade(cx, cz, half, gate_side):
    out = []
    step = 6.0
    n = int(2 * half / step)
    for i in range(n):
        o = -half + step * (i + 0.5)
        if abs(o) < 8.0:
            continue
        out.append(box("palisade", cx + o, cz - half, step, 4.2, 1.2, WOOD))
        out.append(box("palisade", cx + o, cz + half, step, 4.2, 1.2, WOOD))
        out.append(box("palisade", cx + half, cz + o, 1.2, 4.2, step, WOOD))
        if gate_side != "west":
            out.append(box("palisade", cx - half, cz + o, 1.2, 4.2, step, WOOD))
    if gate_side == "west":
        for i in range(n):
            o = -half + step * (i + 0.5)
            if abs(o) >= 8.0:
                out.append(box("palisade", cx - half, cz + o, 1.2, 4.2, step, WOOD))
    for sx in (-1, 1):
        for sz in (-1, 1):
            out.append(box("watchtower", cx + sx * half, cz + sz * half, 5, 10, 5, WOOD))
    return out


def build_objects(h_at, h):
    objs = []
    sx, sz = STOCKADE
    objs += palisade(sx, sz, STOCKADE_HALF, "west")
    for i, (tx, tz) in enumerate([(-24, -24), (22, -26), (24, 22), (-8, 4)]):
        objs.append(box("tent", sx + tx, sz + tz, 8, 4, 6, HIDE if i % 2 == 0 else AWNINGS[0]))
    objs.append(box("campfire", sx - 20, sz + 18, 3, 1, 3, [0.7, 0.32, 0.1], block=False))
    for cx, cz in [(36, -2), (38, 6), (-40, -38)]:
        objs.append(box("crate", sx + cx, sz + cz, 3, 2.4, 3, WOOD))

    hx, hz = HOLLOW
    for i in range(7):
        a = i * 2.0 * math.pi / 7.0
        objs.append(box("standing_stone", hx + 20 * math.cos(a), hz + 20 * math.sin(a), 2.4, 6.0, 2.2, STONE_DARK))
    objs.append(box("altar", hx, hz, 5, 1.4, 4, STONE_DARK))

    dx, dz = DEN
    objs.append(box("den_mouth", dx, dz, 16, 6, 3, DARK))
    objs.append(box("bone_pile", dx - 14, dz + 10, 4, 1, 3, [0.85, 0.82, 0.72], block=False))
    objs += bridge_railings(BRIDGES)

    def clear(px, pz):
        pt_x, pt_z = np.array([[px]]), np.array([[pz]])
        if polyline_distance(pt_x, pt_z, [(a, b) for a, b in ROAD])[0, 0] < 16:
            return False
        if polyline_distance(pt_x, pt_z, RIVER)[0, 0] < 30:
            return False
        if any(math.hypot(px - qx, pz - qz) < 45 for qx, qz in SPECIALS):
            return False
        return HALF - max(abs(px), abs(pz)) > RIM_BLOCK + 20

    objs += scatter_boulders(np.random.RandomState(3022), 30, ROCKY, clear)

    def keepout(px, pz):
        hit = near_polylines(px, pz, [ROAD], ROAD_HALF + 14.0)
        hit |= polyline_distance(px, pz, RIVER) < 30
        hit |= near_circles(px, pz, [(cx, cz, r) for (cx, cz), r in POND + LAKE], 30.0)
        hit |= near_points(px, pz, SPECIALS, 40.0)
        hit |= rect_mask(px, pz, sx - STOCKADE_HALF - 15, sz - STOCKADE_HALF - 15, sx + STOCKADE_HALF + 15, sz + STOCKADE_HALF + 15)
        hit |= near_objects(px, pz, objs, 10.0)
        for bridge in BRIDGES:
            hit |= bridge_mask(px, pz, bridge, 20, 40)
        return hit

    objs += scatter_forest(np.random.RandomState(3021), h, "mist", FOREST, keepout, 5800, sparse_chance=0.0007)
    return objs


def make_splat(h, objs):
    x, z = splat_grid()
    hh = height_at_grid(h, x, z)
    road, shoulder = road_weight(x, z, [ROAD], [], BRIDGES, 0.0)
    wooded = forest_floor(x, z, FOREST, objs)
    patch = smoothstep(-0.2, 0.3, smooth_noise(x, z, 3022))
    camp = smoothstep(90.0, 62.0, np.maximum(np.abs(x - STOCKADE[0]), np.abs(z - STOCKADE[1])))
    dry = 0.04 + 0.1 * patch + 0.25 * wooded
    dirt = (0.4 * shoulder + 1.0 * wooded + 0.12 * (1.0 - patch) + 0.8 * camp
            + 0.75 * shore_weight(x, z, hh, [("line", RIVER), ("circles", POND + LAKE)]))
    rock = np.maximum(smoothstep(18.0, -18.0, blob_distance(x, z, ROCKY)), smoothstep(34.0, 48.0, hh))
    return build_splat(x, z, hh, dry, dirt, road, rock)


def main():
    x, z, h = build_height()
    objs = build_objects(make_height_sampler(h), h)
    ground = build_ground(x, z, h)
    block = build_block(objs)
    roads = build_roads([ROAD], [], BRIDGES, 0.0)

    assert_walkable(h, block, [WEST_PORTAL, WEST_ARRIVAL, EAST_PORTAL, EAST_ARRIVAL], "map %d" % MAP_ID)

    meta = {
        "id": MAP_ID,
        "name": "Sisvadi Valley",
        "kingdom": 3,
        "worldSize": WORLD,
        "heightMin": HEIGHT_MIN,
        "heightMax": HEIGHT_MAX,
        "waterLevel": WATER,
        "spawn": {"x": WEST_ARRIVAL[0], "z": WEST_ARRIVAL[1], "yaw": -math.pi / 2.0},
        "portals": [
            portal(WEST_PORTAL[0], WEST_PORTAL[1], PORTAL_RADIUS, 301, 760.0, -30.0, -math.pi / 2.0),
            portal(EAST_PORTAL[0], EAST_PORTAL[1], PORTAL_RADIUS, 401, 14.0, 735.0, math.pi),
        ],
        "objects": objs,
        "safeZones": [
            safe_zone("Gate Camp", [[-770.0, -30.0], [-700.0, -30.0], [-700.0, 50.0], [-770.0, 50.0]]),
        ],
        "npcs": [],
        "monsterSpawns": [
            monster_spawn(8, -600.0, 120.0, 40.0, 4),
            monster_spawn(8, -250.0, -150.0, 40.0, 3),
            monster_spawn(8, 450.0, -330.0, 40.0, 5),
            monster_spawn(8, -250.0, 420.0, 40.0, 4),
            monster_spawn(9, 330.0, 300.0, 45.0, 6),
            monster_spawn(9, 560.0, 60.0, 35.0, 3),
            monster_spawn(7, 330.0, 300.0, 6.0, 1),
            monster_spawn(10, -380.0, -372.0, 25.0, 4),
            monster_spawn(10, 640.0, 180.0, 30.0, 3),
            monster_spawn(7, -200.0, 740.0, 15.0, 1),
        ],
    }
    check_map_access(h, objs, block, meta, "map %d" % MAP_ID)
    add_undergrowth(objs, h, block, roads, meta, "mist", 7302)
    write_map(OUT, meta, h, ground, block, roads, objs, make_splat(h, objs))


if __name__ == "__main__":
    main()
