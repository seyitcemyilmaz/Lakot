"""Draws map 401 - Borderlands - into data/maps/401_borderlands/.

A one-off sketching tool. Shared, kingdom-less highland reached from the
Altunay Steppe: a road climbs from the west gate past a ruined watchtower to a
stony plateau, with ridges, pine-dark copses and boulder fields. Shared
building blocks live in mapkit.py.

Needs numpy and Pillow.
"""

import math

import numpy as np

from mapkit import *

MAP_ID = 401
OUT = map_directory(MAP_ID, "borderlands")

ROAD = [(-800, 0), (-620, -40), (-400, -20), (-200, 60), (0, 40), (240, -60), (480, -40), (650, -20), (800, 0)]
SOUTH_ROAD = [(0, 40), (30, 300), (-20, 560), (0, 800)]
ROADS = [ROAD, SOUTH_ROAD]
ROCKY = [(300, 300, 200), (500, 120, 130), (-300, -400, 170), (-500, 350, 140)]
COPSES = [(-450, 260, 150), (150, -450, 140), (600, -450, 110)]
RUIN = (-120.0, -110.0)

BUMPS = [(-300, -300, 200, 26), (300, 350, 220, 24), (550, -300, 180, 30),
         (-550, 380, 200, 20), (100, 560, 180, 18), (650, 80, 150, 28)]

WEST_PORTAL = (-800.0, 0.0)
WEST_ARRIVAL = (-735.0, 14.0)
EAST_PORTAL = (800.0, 0.0)
EAST_ARRIVAL = (735.0, -14.0)
SOUTH_PORTAL = (0.0, 800.0)
SOUTH_ARRIVAL = (14.0, 735.0)
GATES = (WEST_PORTAL, WEST_ARRIVAL, EAST_PORTAL, EAST_ARRIVAL, SOUTH_PORTAL, SOUTH_ARRIVAL)
PORTAL_RADIUS = 14.0

GRASS = np.array([96, 118, 78])
GRASS_DARK = np.array([50, 82, 56])
DIRT = np.array([140, 118, 90])
ROCK = np.array([126, 121, 114])
SNOW = np.array([236, 236, 242])


def build_height():
    x, z = grid(HEIGHT_RES, cell_centred=False)
    h = np.full(x.shape, 14.0)
    h += 2.0 * np.sin(x * 0.011 + 1.0) * np.cos(z * 0.009 - 0.7)
    for bx, bz, r, amp in BUMPS:
        h += amp * np.exp(-((x - bx) ** 2 + (z - bz) ** 2) / (2.0 * (r * 0.5) ** 2))
    h = flatten_along(h, x, z, ROADS, 14.0, strength=0.55)
    h = h + mountain_rim(x, z)
    return x, z, np.clip(h, HEIGHT_MIN, HEIGHT_MAX)


def build_ground(x, z, h):
    c = np.zeros(x.shape + (3,))
    c[:] = GRASS
    c += (8.0 * np.sin(x * 0.05) * np.cos(z * 0.04))[..., None]
    edge = smoothstep(30.0, -10.0, blob_distance(x, z, COPSES))[..., None]
    c = c * (1.0 - edge) + GRASS_DARK * edge
    edge = smoothstep(20.0, -20.0, blob_distance(x, z, ROCKY))[..., None]
    c = c * (1.0 - edge) + ROCK * edge
    for road in ROADS:
        edge = smoothstep(ROAD_HALF + 2.0, ROAD_HALF - 2.0, polyline_distance(x, z, road))[..., None]
        c = c * (1.0 - edge) + DIRT * edge
    rock = smoothstep(34.0, 46.0, h)[..., None]
    c = c * (1.0 - rock) + ROCK * rock
    snow = smoothstep(80.0, 88.0, h)[..., None]
    c = c * (1.0 - snow) + SNOW * snow
    return np.clip(c, 0, 255).astype(np.uint8)


MAX_TREES = 3200
COPSES_EXTRA = [(-100, 520, 130), (560, 420, 110), (-650, -300, 90), (350, -150, 90)]


def build_objects(h_at, h):
    objs = []
    T = WALL_TILE
    rx, rz = RUIN
    objs += tower(rx, rz)
    objs += tower(rx + 4 * T, rz)
    for i in range(1, 4):
        objs.append(wall_tile(rx + i * T, rz, 0.0))
    objs.append(wall_tile(rx, rz + T, math.pi / 2.0))
    objs.append(wall_tile(rx, rz + 2 * T, math.pi / 2.0))

    rng = np.random.RandomState(1032)

    def clear(px, pz):
        pt_x, pt_z = np.array([[px]]), np.array([[pz]])
        if min(polyline_distance(pt_x, pt_z, r)[0, 0] for r in ROADS) < 16:
            return False
        if any(math.hypot(px - gx, pz - gz) < 60 for gx, gz in GATES):
            return False
        if abs(px - rx - 2 * T) < 3 * T + 12 and abs(pz - rz) < 4 * T:
            return False
        return HALF - max(abs(px), abs(pz)) > RIM_BLOCK + 20

    placed = 0
    while placed < 60:
        px, pz = rng.uniform(-850, 850), rng.uniform(-850, 850)
        inside = blob_distance(np.array(px), np.array(pz), ROCKY) < -5.0
        if inside and clear(px, pz):
            objs.append(boulder(rng, px, pz))
            placed += 1

    def keepout(px, pz):
        hit = near_polylines(px, pz, ROADS, ROAD_HALF + 14.0)
        hit |= near_points(px, pz, GATES, 60.0)
        hit |= rect_mask(px, pz, rx - 3 * T - 12, rz - 4 * T, rx + 7 * T + 12, rz + 4 * T)
        hit |= near_objects(px, pz, objs, 12.0)
        return hit

    objs += scatter_forest(np.random.RandomState(1031), h, "border", COPSES + COPSES_EXTRA, keepout, MAX_TREES, sparse_chance=0.0003)
    return objs


def make_splat(h, objs):
    x, z = splat_grid()
    hh = height_at_grid(h, x, z)
    road, shoulder = road_weight(x, z, ROADS, [], [], 0.0)
    wooded = forest_floor(x, z, COPSES + COPSES_EXTRA, objs)
    patch = smoothstep(-0.2, 0.3, smooth_noise(x, z, 2204))
    dry = 0.35 + 0.4 * patch
    dirt = 0.5 * shoulder + 1.0 * wooded + 0.15 * (1.0 - patch)
    rock = np.maximum(smoothstep(18.0, -18.0, blob_distance(x, z, ROCKY)), smoothstep(32.0, 46.0, hh))
    return build_splat(x, z, hh, dry, dirt, road, rock)


def main():
    x, z, h = build_height()
    objs = build_objects(make_height_sampler(h), h)
    ground = build_ground(x, z, h)
    block = build_block(objs)
    roads = build_roads(ROADS, [], [], 0.0)

    assert_walkable(h, block, list(GATES), "map 401")

    meta = {
        "id": MAP_ID,
        "name": "Borderlands",
        "kingdom": 0,
        "worldSize": WORLD,
        "heightMin": HEIGHT_MIN,
        "heightMax": HEIGHT_MAX,
        "waterLevel": WATER,
        "spawn": {"x": WEST_ARRIVAL[0], "z": WEST_ARRIVAL[1], "yaw": -math.pi / 2.0},
        "portals": [
            portal(WEST_PORTAL[0], WEST_PORTAL[1], PORTAL_RADIUS, 102, 735.0, -14.0, math.pi / 2.0),
            portal(EAST_PORTAL[0], EAST_PORTAL[1], PORTAL_RADIUS, 202, 735.0, -14.0, math.pi / 2.0),
            portal(SOUTH_PORTAL[0], SOUTH_PORTAL[1], PORTAL_RADIUS, 302, 735.0, -14.0, math.pi / 2.0),
        ],
        "objects": objs,
        "safeZones": [],
        "npcs": [],
        "monsterSpawns": [
            monster_spawn(4, -300.0, -400.0, 40.0, 5),
            monster_spawn(4, -480.0, 340.0, 40.0, 4),
            monster_spawn(1, 150.0, -450.0, 40.0, 4),
            monster_spawn(5, 520.0, 120.0, 25.0, 1),
            monster_spawn(6, 300.0, 300.0, 50.0, 4),
            monster_spawn(6, 600.0, -450.0, 40.0, 3),
            monster_spawn(7, 320.0, 340.0, 20.0, 1),
            monster_spawn(7, -300.0, -380.0, 20.0, 1),
        ],
    }
    check_map_access(h, objs, block, meta, "map %d" % MAP_ID)
    write_map(OUT, meta, h, ground, block, roads, objs, make_splat(h, objs))


if __name__ == "__main__":
    main()
