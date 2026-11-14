"""Draws map 102 - Altunay Steppe - into data/maps/102_altunay_steppe/.

A one-off sketching tool. Open rolling grassland east of Altunay Village: a
road runs west (gate to the village) to east (gate to the Borderlands), with a
pond, two copses and a rocky knoll. Shared building blocks live in mapkit.py.

Needs numpy and Pillow.
"""

import math

import numpy as np

from mapkit import *

MAP_ID = 102
OUT = map_directory(MAP_ID, "altunay_steppe")

ROAD = [(-800, 0), (-600, 40), (-300, 20), (0, -40), (300, -20), (600, 50), (800, 0)]
POND = [((-150, 330), 70), ((-60, 380), 45)]
COPSES = [(-420, -380, 170), (380, 400, 150), (120, -520, 120)]
KNOLL = [(520, -330, 130), (430, -430, 80)]

BUMPS = [(-300, -250, 200, 12), (250, 250, 220, 10), (600, -300, 150, 16),
         (-600, 400, 200, 9), (0, -600, 180, 8), (700, 400, 160, 7)]

WEST_PORTAL = (-800.0, 0.0)
WEST_ARRIVAL = (-735.0, 14.0)
EAST_PORTAL = (800.0, 0.0)
EAST_ARRIVAL = (735.0, -14.0)
PORTAL_RADIUS = 14.0

GRASS = np.array([112, 140, 72])
GRASS_DARK = np.array([66, 104, 56])
DIRT = np.array([152, 122, 86])
SAND = np.array([204, 188, 142])
ROCK = np.array([126, 121, 114])
RIVERBED = np.array([96, 110, 92])


def build_height():
    x, z = grid(HEIGHT_RES, cell_centred=False)
    h = np.full(x.shape, 7.0)
    h += 1.2 * np.sin(x * 0.009 + 0.6) * np.cos(z * 0.012 - 0.2)
    for bx, bz, r, amp in BUMPS:
        h += amp * np.exp(-((x - bx) ** 2 + (z - bz) ** 2) / (2.0 * (r * 0.5) ** 2))
    d = polyline_distance(x, z, ROAD)
    flat = 0.5 * smoothstep(40.0, 8.0, d)
    h = h * (1.0 - flat) + 7.0 * flat
    for (cx, cz), r in POND:
        dl = np.hypot(x - cx, z - cz)
        shore = smoothstep(r + 40.0, r + 15.0, dl)
        h = h * (1.0 - shore) + np.minimum(h, 1.5) * shore
        dip = smoothstep(r + 15.0, r - 10.0, dl)
        floor = WATER - 0.6 - 4.0 * smoothstep(r - 10.0, r * 0.3, dl)
        h = np.minimum(h, h * (1.0 - dip) + floor * dip)
    h = h + mountain_rim(x, z)
    return x, z, np.clip(h, HEIGHT_MIN, HEIGHT_MAX)


def build_ground(x, z, h):
    c = np.zeros(x.shape + (3,))
    c[:] = GRASS
    c += (8.0 * np.sin(x * 0.045) * np.cos(z * 0.038))[..., None]
    c += (12.0 * np.sin(x * 0.004 - 0.3) * np.cos(z * 0.005 + 0.8))[..., None]
    edge = smoothstep(30.0, -10.0, blob_distance(x, z, COPSES))[..., None]
    c = c * (1.0 - edge) + GRASS_DARK * edge
    edge = smoothstep(20.0, -20.0, blob_distance(x, z, KNOLL))[..., None]
    c = c * (1.0 - edge) + ROCK * edge
    edge = smoothstep(ROAD_HALF + 2.0, ROAD_HALF - 2.0, polyline_distance(x, z, ROAD))[..., None]
    c = c * (1.0 - edge) + DIRT * edge
    near = np.zeros(x.shape)
    for (cx, cz), r in POND:
        near = np.maximum(near, smoothstep(r + 26.0, r + 12.0, np.hypot(x - cx, z - cz)))
    sand = (near * smoothstep(3.0, 1.5, h))[..., None]
    c = c * (1.0 - sand) + SAND * sand
    bed = smoothstep(0.5, -1.0, h)[..., None]
    c = c * (1.0 - bed) + RIVERBED * bed
    rock = smoothstep(38.0, 50.0, h)[..., None]
    c = c * (1.0 - rock) + ROCK * rock
    return np.clip(c, 0, 255).astype(np.uint8)


MAX_TREES = 3200
COPSES_EXTRA = [(-100, 560, 120), (650, 550, 110), (-650, -150, 100), (200, 150, 90)]


def build_objects(h_at, h):
    objs = []
    rng = np.random.RandomState(1022)

    def clear(px, pz):
        pt_x, pt_z = np.array([[px]]), np.array([[pz]])
        if polyline_distance(pt_x, pt_z, ROAD)[0, 0] < 16:
            return False
        if any(math.hypot(px - cx, pz - cz) < r + 30 for (cx, cz), r in POND):
            return False
        for qx, qz in (WEST_PORTAL, WEST_ARRIVAL, EAST_PORTAL, EAST_ARRIVAL):
            if math.hypot(px - qx, pz - qz) < 40:
                return False
        return HALF - max(abs(px), abs(pz)) > RIM_BLOCK + 20

    placed = 0
    while placed < 26:
        px, pz = rng.uniform(-850, 850), rng.uniform(-850, 850)
        inside = blob_distance(np.array(px), np.array(pz), KNOLL) < -5.0
        if inside and clear(px, pz):
            objs.append(boulder(rng, px, pz))
            placed += 1

    def keepout(px, pz):
        hit = near_polylines(px, pz, [ROAD], ROAD_HALF + 14.0)
        hit |= near_circles(px, pz, [(cx, cz, r) for (cx, cz), r in POND], 30.0)
        hit |= near_points(px, pz, (WEST_PORTAL, WEST_ARRIVAL, EAST_PORTAL, EAST_ARRIVAL), 40.0)
        hit |= near_objects(px, pz, objs, 12.0)
        return hit

    objs += scatter_forest(np.random.RandomState(1021), h, "steppe", COPSES + COPSES_EXTRA, keepout, MAX_TREES, sparse_chance=0.0003)
    return objs


def make_splat(h, objs):
    x, z = splat_grid()
    hh = height_at_grid(h, x, z)
    road, shoulder = road_weight(x, z, [ROAD], [], [], 0.0)
    wooded = forest_floor(x, z, COPSES + COPSES_EXTRA, objs)
    patch = smoothstep(-0.25, 0.25, smooth_noise(x, z, 2202))
    dry = 0.55 + 0.4 * patch - 0.25 * wooded
    dirt = (0.45 * shoulder + 1.0 * wooded + 0.18 * (1.0 - patch) * smoothstep(0.2, -0.2, smooth_noise(x, z, 2203))
            + 0.75 * shore_weight(x, z, hh, [("circles", POND)]))
    rock = np.maximum(smoothstep(18.0, -18.0, blob_distance(x, z, KNOLL)), smoothstep(36.0, 50.0, hh))
    return build_splat(x, z, hh, dry, dirt, road, rock)


def main():
    x, z, h = build_height()
    objs = build_objects(make_height_sampler(h), h)
    ground = build_ground(x, z, h)
    block = build_block(objs)
    roads = build_roads([ROAD], [], [], 0.0)

    assert_walkable(h, block, [WEST_PORTAL, WEST_ARRIVAL, EAST_PORTAL, EAST_ARRIVAL], "map 102")

    meta = {
        "id": MAP_ID,
        "name": "Altunay Steppe",
        "kingdom": 1,
        "worldSize": WORLD,
        "heightMin": HEIGHT_MIN,
        "heightMax": HEIGHT_MAX,
        "waterLevel": WATER,
        "spawn": {"x": WEST_ARRIVAL[0], "z": WEST_ARRIVAL[1], "yaw": -math.pi / 2.0},
        "portals": [
            portal(WEST_PORTAL[0], WEST_PORTAL[1], PORTAL_RADIUS, 101, 760.0, -30.0, -math.pi / 2.0),
            portal(EAST_PORTAL[0], EAST_PORTAL[1], PORTAL_RADIUS, 401, -735.0, 14.0, -math.pi / 2.0),
        ],
        "objects": objs,
        "safeZones": [],
        "npcs": [],
        "monsterSpawns": [
            monster_spawn(2, -450.0, 120.0, 30.0, 5),
            monster_spawn(2, -200.0, -150.0, 30.0, 5),
            monster_spawn(3, 150.0, 150.0, 40.0, 4),
            monster_spawn(3, -400.0, -380.0, 40.0, 3),
            monster_spawn(1, 300.0, -150.0, 35.0, 4),
            monster_spawn(1, 380.0, 400.0, 35.0, 3),
            monster_spawn(4, 520.0, -330.0, 30.0, 3),
            monster_spawn(6, 120.0, -520.0, 40.0, 2),
            monster_spawn(6, 650.0, 200.0, 40.0, 2),
        ],
    }
    check_map_access(h, objs, block, meta, "map %d" % MAP_ID)
    write_map(OUT, meta, h, ground, block, roads, objs, make_splat(h, objs))


if __name__ == "__main__":
    main()
