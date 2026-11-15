"""Shared building blocks for the map generators in this folder.

Each map script (altunay_village.py, ...) holds only its own layout and calls
into this module for terrain maths, roads, the block grid, objects, the
spawn/safe-zone/portal records and the file writers. Coordinates: world units,
origin at the map centre, x east, z south. Maps span [-1024, 1024].

Needs numpy and Pillow.
"""

import json
import math
import os
import re

import numpy as np
from PIL import Image

MAPS_DIR = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "maps"))

WORLD = 2048.0
HALF = WORLD / 2.0
HEIGHT_RES = 513            # vertices per side -> 4 units apart
BLOCK_RES = 1024            # cells per side -> 2 units each
HEIGHT_MIN, HEIGHT_MAX = -20.0, 110.0
WATER = 0.0

GATE_HALF = 6.0             # each gate is a 12-unit opening

# Stone wall pieces from Kenney's Castle Kit (CC0, data/models/castle/): unit
# tiles scaled up so one wall tile is WALL_TILE units wide and deep. The
# village half-size is chosen so a whole number of tiles runs from each gate
# tower to the corner tower.
WALL_TILE = 6.0
ROAD_HALF = 6.0
RIM_BLOCK = 180.0           # border band that cannot be walked


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def grid(res, cell_centred):
    step = WORLD / (res - 1) if not cell_centred else WORLD / res
    offset = 0.5 * step if cell_centred else 0.0
    axis = -HALF + offset + np.arange(res) * step
    x, z = np.meshgrid(axis, axis)  # rows are z, columns are x
    return x, z


def polyline_distance(x, z, points):
    best = np.full(x.shape, np.inf)
    for (ax, az), (bx, bz) in zip(points[:-1], points[1:]):
        dx, dz = bx - ax, bz - az
        t = np.clip(((x - ax) * dx + (z - az) * dz) / (dx * dx + dz * dz), 0.0, 1.0)
        d = np.hypot(x - (ax + t * dx), z - (az + t * dz))
        best = np.minimum(best, d)
    return best


def rect_mask(x, z, x0, z0, x1, z1):
    return (x >= min(x0, x1)) & (x <= max(x0, x1)) & (z >= min(z0, z1)) & (z <= max(z0, z1))


def rect_distance(x, z, x0, z0, x1, z1):
    dx = np.maximum(np.maximum(x0 - x, x - x1), 0.0)
    dz = np.maximum(np.maximum(z0 - z, z - z1), 0.0)
    return np.hypot(dx, dz)


def border_norm(x, z):
    # A rounded square: like max(|x|, |z|) but without a crease at the corners.
    return (np.abs(x) ** 8 + np.abs(z) ** 8) ** 0.125


def blob_distance(x, z, circles):
    # Signed distance to a union of circles with a wobbling edge: < 0 inside.
    d = np.full(np.shape(x), np.inf)
    for cx, cz, r in circles:
        d = np.minimum(d, np.hypot(x - cx, z - cz) - r)
    return d + 22.0 * np.sin(x * 0.031 + 0.7) * np.cos(z * 0.027 - 1.1) + 9.0 * np.sin((x - z) * 0.07)



def mountain_rim(x, z):
    edge = HALF - border_norm(x, z)
    rim = smoothstep(280.0, 80.0, edge)
    ridge = (52.0 + 16.0 * np.sin(x * 0.0061 + 0.4) * np.cos(z * 0.0053 - 0.9)
             + 9.0 * np.sin(x * 0.017 + z * 0.004) + 5.0 * np.cos(z * 0.023 - x * 0.006))
    return rim * ridge


def bridge_mask(x, z, bridge, half_width, half_length=26):
    (bx, bz), axis = bridge
    if axis == "ns":
        return rect_mask(x, z, bx - half_width, bz - half_length, bx + half_width, bz + half_length)
    return rect_mask(x, z, bx - half_length, bz - half_width, bx + half_length, bz + half_width)


def make_height_sampler(h):
    step = WORLD / (HEIGHT_RES - 1)

    def h_at(px, pz):
        i = int(round((px + HALF) / step))
        j = int(round((pz + HALF) / step))
        return float(h[min(max(j, 0), HEIGHT_RES - 1), min(max(i, 0), HEIGHT_RES - 1)])

    return h_at


# ---- Objects ---------------------------------------------------------------

WOOD = [0.47, 0.34, 0.22]
ROOF_BROWN = [0.45, 0.30, 0.20]
STONE_WALL = [0.82, 0.66, 0.50]
ROOF_BLUE = [0.36, 0.44, 0.74]
STONE = [0.58, 0.56, 0.52]
HAY = [0.86, 0.72, 0.36]
TRUNK = [0.36, 0.25, 0.15]
LEAVES = [[0.22, 0.42, 0.20], [0.27, 0.48, 0.22], [0.18, 0.36, 0.19]]
BOULDER = [0.50, 0.48, 0.45]
AWNINGS = [[0.72, 0.20, 0.18], [0.20, 0.42, 0.66], [0.84, 0.68, 0.22]]


def box(kind, x, z, sx, sy, sz, color, block=True, elevation=0.0, model=None, yaw=0.0, scale=1.0, stretch=1.0, hidden=False):
    obj = {"type": kind, "x": round(float(x), 2), "z": round(float(z), 2),
           "sx": round(float(sx), 2), "sy": round(float(sy), 2), "sz": round(float(sz), 2),
           "elevation": round(float(elevation), 2),
           "color": [round(v, 3) for v in color], "block": block}
    if hidden:
        obj["hidden"] = True
    if model:
        obj["model"] = model
        obj["yaw"] = round(float(yaw), 4)
        obj["scale"] = round(float(scale), 3)
        if stretch != 1.0:
            obj["stretch"] = round(float(stretch), 3)
    return obj


# Map-picture colours for Fantasy Town Kit buildings.
TOWN_PLASTER = [0.66, 0.64, 0.80]
TOWN_WOOD = [0.55, 0.36, 0.24]
TOWN_ROOF_TEAL = [0.30, 0.62, 0.55]
TOWN_ROOF_RED = [0.80, 0.30, 0.30]


def building(kind, cx, cz, cells, facing, T, wood=False, high_roof=False):
    """A gabled building from Kenney's Fantasy Town Kit (CC0, data/models/town/).

    `cells` kit tiles long (local z) and two deep (local x), each tile T
    units, walls one tile high. The door is in the middle of the local +x
    face; `facing` turns the whole building about y, with the same rotation
    the game's model shader uses. The kit's wall pieces are panels on a
    tile's +x edge and its roof pieces rise towards +x, so both are placed by
    tile and turned to the side they belong on.
    """
    c, s = math.cos(facing), math.sin(facing)
    walls = "town/wall-wood" if wood else "town/wall"
    roof = "town/roof-high" if high_roof else "town/roof"
    wall_colour = TOWN_WOOD if wood else TOWN_PLASTER
    roof_colour = TOWN_ROOF_RED if high_roof else TOWN_ROOF_TEAL
    roof_height = (1.14 if high_roof else 0.63) * T
    parts = []

    def place(lx, lz, model, yaw, elevation, height, colour):
        wx = cx + c * lx + s * lz
        wz = cz - s * lx + c * lz
        parts.append(box(kind + "_part", wx, wz, T, height, T, colour, block=False,
                         elevation=elevation, model=model, yaw=yaw + facing, scale=T))

    door = cells // 2
    for iz in range(cells):
        lz = (iz - (cells - 1) / 2.0) * T
        for ix, face_yaw in ((0, math.pi), (1, 0.0)):
            lx = (ix - 0.5) * T

            if ix == 1 and iz == door:
                piece = walls + "-door"
            elif (iz + ix) % 2 == 0:
                piece = walls + "-window-shutters"
            else:
                piece = walls
            place(lx, lz, piece, face_yaw, 0.0, T, wall_colour)

            if iz == 0:
                place(lx, lz, walls + ("-window-shutters" if ix == 0 else ""), math.pi / 2.0, 0.0, T, wall_colour)
            if iz == cells - 1:
                place(lx, lz, walls + ("-window-shutters" if ix == 1 else ""), -math.pi / 2.0, 0.0, T, wall_colour)

            # Row 0 rises towards +x, row 1 (turned half way) towards -x: the
            # ridge runs along the middle. The end tiles use the kit's
            # trimmed pieces; turning row 1 swaps which end is its left.
            if iz == 0:
                end = "-left" if ix == 0 else "-right"
            elif iz == cells - 1:
                end = "-right" if ix == 0 else "-left"
            else:
                end = ""
            place(lx, lz, roof + end, 0.0 if ix == 0 else math.pi, T, roof_height, roof_colour)

    # One box for the whole footprint: what blocks walking and what picking
    # and the map picture use. Not drawn - the pieces are the look.
    along_x = abs(s) > 0.5
    fx, fz = (cells * T, 2 * T) if along_x else (2 * T, cells * T)
    parts.append(box(kind, cx, cz, fx, T + roof_height, fz, roof_colour, hidden=True))
    return parts


def door_facing(x, z, along_x):
    """Turns a building so its door faces the village's main streets."""
    if along_x:
        return math.pi / 2.0 if z > 0 else -math.pi / 2.0   # door north / south
    return math.pi if x > 0 else 0.0                          # door west / east



def wall_tile(x, z, yaw):
    T = WALL_TILE
    return box("wall", x, z, T, 1.31 * T, T, STONE_WALL, model="castle/wall", yaw=yaw, scale=T, stretch=1.08)


def tower(x, z):
    T = WALL_TILE
    return [box("tower", x, z, T, 1.01 * T, T, STONE_WALL, model="castle/tower-square-base", scale=T),
            box("tower", x, z, T, 1.01 * T, T, STONE_WALL, block=False, elevation=1.01 * T,
                model="castle/tower-square-mid-windows", scale=T),
            box("tower_roof", x, z, T, 1.0 * T, T, ROOF_BLUE, block=False, elevation=2.02 * T,
                model="castle/tower-square-top-roof", scale=T)]


def walled_square(half):
    T = WALL_TILE
    objs = []
    gate_tower = GATE_HALF + T / 2.0
    offsets = []
    c = gate_tower + T
    while c <= half - T + 0.01:
        offsets += [c, -c]
        c += T

    for side in (-1, 1):
        for o in offsets:
            objs.append(wall_tile(o, side * half, 0.0))
            objs.append(wall_tile(side * half, o, math.pi / 2.0))
        for g in (-1, 1):
            objs += tower(g * gate_tower, side * half)
            objs += tower(side * half, g * gate_tower)
        for other in (-1, 1):
            objs += tower(side * half, other * half)
    return objs


def bridge_railings(bridges):
    objs = []
    for (bx, bz), axis in bridges:
        for s in (-1, 1):
            if axis == "ns":
                objs.append(box("railing", bx + s * 6.5, bz, 1, 1.4, 52, WOOD))
            else:
                objs.append(box("railing", bx, bz + s * 6.5, 52, 1.4, 1, WOOD))
    return objs


TREE_BIOMES = {
    "village": [("oak", 5), ("poplar", 4), ("birch", 1)],
    "steppe": [("juniper", 5), ("birch", 2), ("poplar", 1)],
    "border": [("pine", 6), ("juniper", 2), ("oak", 1)],
    "mist": [("pine", 5), ("birch", 4), ("poplar", 1)],
    "desert": [("juniper", 6), ("poplar", 1)],
    "dark": [("pine", 6), ("oak", 3), ("juniper", 1)],
}
TREE_VARIANTS = 3
TREE_TRUNK_WIDTH = {"oak": 1.2, "poplar": 0.8, "birch": 0.6, "pine": 0.8, "juniper": 0.5}

FOREST_MIN_SPACING = 5.0
FOREST_CELL = 7.0
FOREST_CORE_CHANCE = 0.9
FOREST_EDGE_WIDTH = 55.0
FOREST_MAX_SLOPE = 0.7
FOREST_MIN_HEIGHT = 1.5


def pick_species(rng, biome, height):
    kinds = TREE_BIOMES[biome]
    high = float(smoothstep(6.0, 30.0, height))
    weights = []
    for name, w in kinds:
        if name == "pine":
            w = w * (0.3 + 2.0 * high)
        elif name in ("oak", "poplar"):
            w = w * (1.5 - 1.2 * high)
        weights.append(max(w, 0.05))
    weights = np.array(weights, dtype=float)
    return kinds[rng.choice(len(kinds), p=weights / weights.sum())][0]


def tree(rng, px, pz, scale, biome="village", height=6.0):
    species = pick_species(rng, biome, height)
    variant = int(rng.randint(TREE_VARIANTS)) + 1
    s = min(1.2, max(0.8, scale))
    yaw = rng.uniform(0.0, 2.0 * math.pi)
    width = TREE_TRUNK_WIDTH[species] * s
    return box("tree", px, pz, width, 4.0 * s, width, TRUNK, block=True, model="nature/%s_%d" % (species, variant), yaw=yaw, scale=s)


def near_polylines(x, z, polylines, distance):
    best = np.full(np.shape(x), np.inf)
    for line in polylines:
        best = np.minimum(best, polyline_distance(x, z, line))
    return best < distance


def near_objects(x, z, objs, margin):
    hit = np.zeros(np.shape(x), dtype=bool)
    for o in objs:
        if o["block"] or o["type"] in ("well", "stall", "haystack"):
            hit |= rect_mask(x, z, o["x"] - o["sx"] / 2 - margin, o["z"] - o["sz"] / 2 - margin,
                             o["x"] + o["sx"] / 2 + margin, o["z"] + o["sz"] / 2 + margin)
    return hit


def near_points(x, z, points, distance):
    hit = np.zeros(np.shape(x), dtype=bool)
    for px, pz in points:
        hit |= np.hypot(x - px, z - pz) < distance
    return hit


def near_circles(x, z, circles, margin):
    hit = np.zeros(np.shape(x), dtype=bool)
    for cx, cz, r in circles:
        hit |= np.hypot(x - cx, z - cz) < r + margin
    return hit


def smooth_noise(x, z, seed):
    r = np.random.RandomState(seed)
    out = np.zeros(np.shape(x))
    for freq in (0.0045, 0.011, 0.027):
        a, b, c = r.uniform(0, 6.28, 3)
        out += np.sin(x * freq + a) * np.cos(z * freq * 1.3 + b) + 0.5 * np.sin((x + z) * freq * 0.7 + c)
    return out / 4.5


def scatter_forest(rng, h, biome, circles, keepout, max_trees, sparse_chance=0.0, bound=860.0):
    step = WORLD / (HEIGHT_RES - 1)
    gy, gx = np.gradient(h)
    slope = np.hypot(gx, gy) / step
    n = int(2 * bound / FOREST_CELL)
    axis = -bound + np.arange(n) * FOREST_CELL
    cx, cz = np.meshgrid(axis, axis)
    px = (cx + rng.uniform(0, FOREST_CELL, cx.shape)).ravel()
    pz = (cz + rng.uniform(0, FOREST_CELL, cz.shape)).ravel()
    roll = rng.uniform(0, 1, px.shape)
    fd = blob_distance(px, pz, circles)
    edge = smoothstep(0.0, -FOREST_EDGE_WIDTH, fd)
    clump = smoothstep(-0.25, 0.2, smooth_noise(px, pz, 4101))
    chance = FOREST_CORE_CHANCE * edge * (0.35 + 0.65 * clump) + sparse_chance * (1.0 - edge)
    ii = np.clip(np.round((px + HALF) / step).astype(int), 0, HEIGHT_RES - 1)
    jj = np.clip(np.round((pz + HALF) / step).astype(int), 0, HEIGHT_RES - 1)
    ground = h[jj, ii]
    ok = (roll < chance) & (ground > FOREST_MIN_HEIGHT) & (slope[jj, ii] < FOREST_MAX_SLOPE)
    ok &= (HALF - np.maximum(np.abs(px), np.abs(pz))) > RIM_BLOCK + 20
    ok &= ~keepout(px, pz)
    idx = np.nonzero(ok)[0]
    rng.shuffle(idx)
    cell = FOREST_MIN_SPACING
    taken = {}
    kept = []
    for i in idx:
        gx_, gz_ = int(px[i] // cell), int(pz[i] // cell)
        clash = False
        for a in (gx_ - 1, gx_, gx_ + 1):
            for b in (gz_ - 1, gz_, gz_ + 1):
                for j in taken.get((a, b), ()):
                    if (px[j] - px[i]) ** 2 + (pz[j] - pz[i]) ** 2 < cell * cell:
                        clash = True
        if not clash:
            taken.setdefault((gx_, gz_), []).append(i)
            kept.append(i)
    if len(kept) > max_trees:
        kept = [kept[i] for i in rng.permutation(len(kept))[:max_trees]]
    objs = []
    for i in kept:
        shrink = 0.8 + 0.4 * float(edge[i])
        objs.append(tree(rng, px[i], pz[i], shrink * rng.uniform(0.9, 1.35), biome, float(ground[i])))
    return objs


def boulder(rng, px, pz):
    s = rng.uniform(2.0, 5.0)
    return box("boulder", px, pz, s * rng.uniform(0.8, 1.3), s * 0.7, s, BOULDER)



# ---- Terrain helpers shared by the Demirkaya and Sisvadi maps --------------------

def blend_toward(h, level, weight):
    return h * (1.0 - weight) + level * weight


def flatten_along(h, x, z, roads, level, strength=0.55, reach=40.0, core=8.0):
    d = np.full(x.shape, np.inf)
    for r in roads:
        d = np.minimum(d, polyline_distance(x, z, r))
    return blend_toward(h, level, strength * smoothstep(reach, core, d))


def carve_river(h, x, z, river):
    d = polyline_distance(x, z, river)
    h = blend_toward(h, np.minimum(h, 2.0), smoothstep(55.0, 22.0, d))
    return blend_toward(h, WATER - 3.5, smoothstep(22.0, 4.0, d))


def carve_ponds(h, x, z, ponds):
    for (cx, cz), r in ponds:
        dl = np.hypot(x - cx, z - cz)
        h = blend_toward(h, np.minimum(h, 1.5), smoothstep(r + 40.0, r + 15.0, dl))
        dip = smoothstep(r + 15.0, r - 10.0, dl)
        floor = WATER - 0.6 - 4.0 * smoothstep(r - 10.0, r * 0.3, dl)
        h = np.minimum(h, h * (1.0 - dip) + floor * dip)
    return h


def scatter_boulders(rng, count, blobs, clear):
    out = []
    while len(out) < count:
        px, pz = rng.uniform(-850, 850), rng.uniform(-850, 850)
        if blob_distance(np.array(px), np.array(pz), blobs) < -5.0 and clear(px, pz):
            out.append(boulder(rng, px, pz))
    return out


# ---- Splat map ----------------------------------------------------------------

SPLAT_RES = BLOCK_RES


def splat_grid():
    return grid(SPLAT_RES, cell_centred=True)


def height_at_grid(h, x, z):
    step = WORLD / (HEIGHT_RES - 1)
    fx = np.clip((x + HALF) / step, 0.0, HEIGHT_RES - 1.001)
    fz = np.clip((z + HALF) / step, 0.0, HEIGHT_RES - 1.001)
    i0, j0 = fx.astype(int), fz.astype(int)
    tx, tz = fx - i0, fz - j0
    top = h[j0, i0] * (1.0 - tx) + h[j0, i0 + 1] * tx
    bottom = h[j0 + 1, i0] * (1.0 - tx) + h[j0 + 1, i0 + 1] * tx
    return top * (1.0 - tz) + bottom * tz


def blur(field, radius):
    taps = np.arange(-int(radius * 3), int(radius * 3) + 1)
    kernel = np.exp(-0.5 * (taps / radius) ** 2)
    kernel /= kernel.sum()
    out = np.apply_along_axis(lambda row: np.convolve(row, kernel, mode="same"), 0, field)
    return np.apply_along_axis(lambda row: np.convolve(row, kernel, mode="same"), 1, out)


def tree_density(x, z, objs, radius=6.0):
    cell = WORLD / SPLAT_RES
    hits = np.zeros(x.shape)
    for o in objs:
        if o["type"] == "tree":
            ci = min(max(int((o["x"] + HALF) / cell), 0), SPLAT_RES - 1)
            cj = min(max(int((o["z"] + HALF) / cell), 0), SPLAT_RES - 1)
            hits[cj, ci] = 1.0
    return np.clip(blur(hits, radius) * 14.0, 0.0, 1.0)


def forest_floor(x, z, circles, objs):
    inside = smoothstep(20.0, -30.0, blob_distance(x, z, circles))
    return np.maximum(0.7 * inside, tree_density(x, z, objs))


def road_weight(x, z, roads, lanes, bridges, plaza_half):
    d = np.full(x.shape, np.inf)
    for r in roads:
        d = np.minimum(d, polyline_distance(x, z, r))
    road = smoothstep(ROAD_HALF + 1.5, ROAD_HALF - 2.5, d)
    for x0, z0, x1, z1 in lanes:
        road = np.maximum(road, smoothstep(2.5, -1.5, rect_distance(x, z, x0, z0, x1, z1)))
    if plaza_half > 0.0:
        road = np.maximum(road, smoothstep(2.0, -2.0, np.maximum(np.abs(x), np.abs(z)) - plaza_half))
    for bridge in bridges:
        road = np.maximum(road, bridge_mask(x, z, bridge, 6).astype(float))
    shoulder = smoothstep(ROAD_HALF + 9.0, ROAD_HALF, d)
    return road, shoulder


def shore_weight(x, z, h, water_features):
    near = np.zeros(x.shape)
    for kind, spec in water_features:
        if kind == "line":
            near = np.maximum(near, smoothstep(32.0, 20.0, polyline_distance(x, z, spec)))
        else:
            for (cx, cz), r in spec:
                near = np.maximum(near, smoothstep(r + 26.0, r + 12.0, np.hypot(x - cx, z - cz)))
    return near * smoothstep(3.0, 1.5, h) + smoothstep(0.5, -1.0, h)


def patch_noise(x, z, seed):
    r = np.random.RandomState(seed)
    out = np.zeros(np.shape(x))
    for freq in (0.045, 0.09, 0.19):
        a, b, c = r.uniform(0, 6.28, 3)
        out += np.sin(x * freq + a) * np.cos(z * freq * 1.3 + b) + 0.5 * np.sin((x + z) * freq * 0.7 + c)
    return out / 4.5


def build_splat(x, z, h, dry, dirt, road, rock):
    dry = np.clip(dry, 0.0, 1.0)
    dirt = np.clip(dirt, 0.0, 1.0)
    road = np.clip(road, 0.0, 1.0)
    rock = np.clip(rock, 0.0, 1.0)
    floor = np.clip(dirt, 0.0, 1.0)
    dirt = np.maximum(dirt, 0.85 * blur(floor, 6.0))
    litter = patch_noise(x, z, 4101)
    moss = patch_noise(x, z, 4102)
    deep = smoothstep(0.55, 0.95, floor)
    dirt = dirt * (0.62 + 0.38 * smoothstep(-0.3, 0.3, litter)) * (1.0 - 0.45 * smoothstep(0.1, 0.5, moss) * deep)
    dry = dry + 0.55 * deep * smoothstep(0.0, 0.5, -litter) * (1.0 - 0.5 * smoothstep(0.1, 0.5, moss))
    rest = 1.0 - rock
    road = road * rest
    rest = rest - road
    dirt = dirt * rest
    rest = rest - dirt
    dry = dry * rest
    stack = np.stack([dry, dirt, road, rock], axis=-1)
    return np.floor(np.clip(stack, 0.0, 1.0) * 255.0 + 1e-6).astype(np.uint8)


# ---- Block and road maps ----------------------------------------------------

def build_block(objs):
    x, z = grid(BLOCK_RES, cell_centred=True)
    blocked = (HALF - np.maximum(np.abs(x), np.abs(z))) < RIM_BLOCK
    cell = WORLD / BLOCK_RES
    for o in objs:
        if o["type"] == "tree":
            ci = min(max(int((o["x"] + HALF) / cell), 0), BLOCK_RES - 1)
            cj = min(max(int((o["z"] + HALF) / cell), 0), BLOCK_RES - 1)
            blocked[cj, ci] = True
        elif o["block"]:
            blocked |= rect_mask(x, z, o["x"] - o["sx"] / 2, o["z"] - o["sz"] / 2,
                                 o["x"] + o["sx"] / 2, o["z"] + o["sz"] / 2)
    return np.where(blocked, 255, 0).astype(np.uint8)


def build_roads(roads, lanes, bridges, plaza_half):
    x, z = grid(BLOCK_RES, cell_centred=True)
    road = np.zeros(x.shape, dtype=bool)
    for r in roads:
        road |= polyline_distance(x, z, r) <= ROAD_HALF + 1.0
    for x0, z0, x1, z1 in lanes:
        road |= rect_mask(x, z, x0, z0, x1, z1)
    road |= np.maximum(np.abs(x), np.abs(z)) <= plaza_half
    for bridge in bridges:
        road |= bridge_mask(x, z, bridge, 6)
    return np.where(road, 255, 0).astype(np.uint8)


# ---- Map records -------------------------------------------------------------

def npc(name, role, model, x, z, yaw, shop=None):
    record = {"name": name, "role": role}
    if shop is not None:
        record["shop"] = shop
    record.update({"model": model, "x": x, "z": z, "yaw": yaw})
    return record


def monster_spawn(monster, x, z, radius, count):
    return {"monster": monster, "x": x, "z": z, "radius": radius, "count": count}


def safe_zone(name, points):
    return {"name": name, "points": points}


def portal(x, z, radius, target_map, target_x, target_z, target_yaw=0.0):
    return {"x": x, "z": z, "radius": radius, "targetMap": target_map,
            "targetX": target_x, "targetZ": target_z, "targetYaw": target_yaw}


# ---- Output ----------------------------------------------------------------

def map_directory(map_id, slug):
    return os.path.join(MAPS_DIR, "%d_%s" % (map_id, slug))


def encode_json(meta):
    text = json.dumps(meta, indent=1)
    text = re.sub(r'\{\s*("type": "[^"]*",[^{}]*?)\s*\}', lambda m: "{" + re.sub(r"\s+", " ", m.group(1)) + "}", text)
    return re.sub(r'("shop": )\[([^\]]*)\]',
                  lambda m: m.group(1) + "[" + ", ".join(v.strip() for v in m.group(2).split(",")) + "]", text)


def write_preview(out, ground, h, objs):
    step = WORLD / (HEIGHT_RES - 1)
    gy, gx = np.gradient(h)
    shade = np.clip(0.75 + (-gx * 0.6 - gy * 0.6) / step, 0.45, 1.25)
    prev = ground.astype(float) * shade[..., None]
    water = h < WATER
    prev[water] = np.array([40, 95, 150]) * (0.85 + 0.15 * np.clip(-h[water] / 6.0, 0, 1))[..., None]
    img = Image.fromarray(np.clip(prev, 0, 255).astype(np.uint8), "RGB").resize((1024, 1024), Image.BILINEAR)
    px = img.load()
    scale = 1024 / WORLD
    for o in objs:
        col = tuple(int(v * 255) for v in o["color"])
        x0 = int((o["x"] - o["sx"] / 2 + HALF) * scale)
        x1 = int((o["x"] + o["sx"] / 2 + HALF) * scale)
        z0 = int((o["z"] - o["sz"] / 2 + HALF) * scale)
        z1 = int((o["z"] + o["sz"] / 2 + HALF) * scale)
        for i in range(max(x0, 0), min(max(x1, x0 + 1), 1024)):
            for j in range(max(z0, 0), min(max(z1, z0 + 1), 1024)):
                px[i, j] = col
    img.save(os.path.join(out, "preview.png"))


def write_map(out, meta, h, ground, block, roads, objs, splat):
    os.makedirs(out, exist_ok=True)
    encoded = np.round((h - HEIGHT_MIN) / (HEIGHT_MAX - HEIGHT_MIN) * 65535.0).astype(np.uint16)
    Image.fromarray(encoded).save(os.path.join(out, "height.png"))
    Image.fromarray(ground, "RGB").save(os.path.join(out, "ground.png"))
    Image.fromarray(block, "L").save(os.path.join(out, "block.png"))
    Image.fromarray(roads, "L").save(os.path.join(out, "roads.png"))
    Image.fromarray(splat, "RGBA").save(os.path.join(out, "splat.png"))
    with open(os.path.join(out, "map.json"), "w", encoding="utf-8", newline="\n") as f:
        f.write(encode_json(meta))
    write_preview(out, ground, h, objs)
    print("wrote", out, "-", len(objs), "objects")


def assert_walkable(h, block, points, label):
    # Fails the run if any point a portal or spawn uses would be underwater or blocked.
    step = WORLD / (HEIGHT_RES - 1)
    cell = WORLD / BLOCK_RES
    for px, pz in points:
        i = int(round((px + HALF) / step))
        j = int(round((pz + HALF) / step))
        bi = int((px + HALF) / cell)
        bj = int((pz + HALF) / cell)
        if h[j, i] < WATER + 0.5 or block[bj, bi] != 0:
            raise SystemExit("%s: (%s, %s) is not walkable ground" % (label, px, pz))


def walkable_cells(h, block):
    cells = block == 0
    step = WORLD / (HEIGHT_RES - 1)
    cx = (-HALF + (np.arange(BLOCK_RES) + 0.5) * (WORLD / BLOCK_RES) + HALF) / step
    ii = np.clip(np.round(cx).astype(int), 0, HEIGHT_RES - 1)
    wet = h[np.ix_(ii, ii)] < WATER + 0.5
    return cells & ~wet


def flood_fill(walk, start):
    n = walk.shape[0]
    flat = walk.ravel()
    seen = np.zeros(flat.shape, dtype=bool)
    first = start[1] * n + start[0]
    if not flat[first]:
        return seen.reshape(walk.shape)
    seen[first] = True
    stack = [first]
    while stack:
        i = stack.pop()
        r, c = divmod(i, n)
        if c > 0 and flat[i - 1] and not seen[i - 1]:
            seen[i - 1] = True
            stack.append(i - 1)
        if c < n - 1 and flat[i + 1] and not seen[i + 1]:
            seen[i + 1] = True
            stack.append(i + 1)
        if r > 0 and flat[i - n] and not seen[i - n]:
            seen[i - n] = True
            stack.append(i - n)
        if r < n - 1 and flat[i + n] and not seen[i + n]:
            seen[i + n] = True
            stack.append(i + n)
    return seen.reshape(walk.shape)


def assert_connected(h, block_before, block_after, spawn, targets, label, keep=0.97):
    cell = WORLD / BLOCK_RES

    def index(p):
        return int((p[0] + HALF) / cell), int((p[1] + HALF) / cell)

    reached_before = flood_fill(walkable_cells(h, block_before), index(spawn))
    reached_after = flood_fill(walkable_cells(h, block_after), index(spawn))
    for p in targets:
        i, j = index(p)
        if reached_before[j, i] and not reached_after[j, i]:
            raise SystemExit("%s: (%s, %s) is cut off from the spawn" % (label, p[0], p[1]))
    ratio = float((reached_after & reached_before).sum()) / max(1, int(reached_before.sum()))
    if ratio < keep:
        raise SystemExit("%s: only %.1f%% of the walkable area stays connected" % (label, ratio * 100))
    print("%s: %.2f%% of the walkable area stays connected" % (label, ratio * 100))


def check_map_access(h, objs, block, meta, label):
    before = build_block([o for o in objs if o["type"] != "tree"])
    spawn = (meta["spawn"]["x"], meta["spawn"]["z"])
    targets = [(p["x"], p["z"]) for p in meta["portals"]]
    targets += [(n["x"], n["z"]) for n in meta["npcs"]]
    targets += [(m["x"], m["z"]) for m in meta["monsterSpawns"]]
    assert_connected(h, before, block, spawn, targets, label)
