"""Draws map 101 - Altunay Village - into data/maps/101_altunay_village/.

A one-off sketching tool, not part of the game. It lays the map out from
hand-placed shapes (a walled village on a plateau, roads, a river with two
bridges, a lake, fields, a forest, rocky hills and a mountain rim) and writes
the files the game loads. After that, those files are the map: edit
height.png / ground.png / block.png / roads.png / map.json directly. Re-running this
overwrites any hand edits.

Coordinates: world units, origin at the map centre, x east, z south (north is
-z). The map spans [-1024, 1024] on both axes.

Needs numpy and Pillow.
"""

import json
import math
import os

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.normpath(os.path.join(HERE, "..", "..", "data", "maps", "101_altunay_village"))

WORLD = 2048.0
HALF = WORLD / 2.0
HEIGHT_RES = 513            # vertices per side -> 4 units apart
BLOCK_RES = 1024            # cells per side -> 2 units each
HEIGHT_MIN, HEIGHT_MAX = -20.0, 110.0
WATER = 0.0

VILLAGE_HALF = 183.0        # the stone wall's centre line, this far from the centre
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


# ---- Layout ---------------------------------------------------------------

RIVER = [(-770, -640), (-650, -560), (-420, -420), (-200, -335), (40, -305),
         (260, -235), (390, -80), (420, 120), (470, 300), (540, 430)]
LAKE = [((560, 470), 150), ((470, 540), 90), ((650, 400), 80), ((-780, -650), 42)]

ROADS = [
    [(0, -VILLAGE_HALF), (0, -360), (-45, -560), (-20, -840)],                  # north
    [(VILLAGE_HALF, 0), (560, 0), (700, -45), (840, -30)],                      # east - future exit to shared lands
    [(0, VILLAGE_HALF), (0, 600), (45, 720), (30, 840)],                        # south
    [(-VILLAGE_HALF, 0), (-640, 0), (-740, 45), (-840, 35)],                    # west
]
BRIDGES = [((0, -305), "ns"), ((408, 0), "ew")]

FIELDS = [(-600, -140, -210, -12), (-600, 12, -210, 200), (-190, 215, -12, 560), (12, 215, 250, 560)]
FOREST = [(520, -570, 230), (710, -420, 150), (380, -690, 160), (660, -700, 140), (280, -470, 90)]
ROCKY = [(-520, 540, 190), (-660, 380, 120), (-380, 700, 130), (-700, 660, 110)]

BUMPS = [  # (x, z, radius, height) - gentle hand-placed hills
    (-450, 450, 220, 22), (-620, 620, 150, 18), (-380, 680, 120, 14),
    (520, -560, 260, 16), (700, -420, 160, 12), (380, -700, 140, 10),
    (-560, -160, 180, 4), (240, 620, 200, 7), (-150, -620, 220, 9),
    (760, 180, 170, 9), (-760, 240, 150, 8),
]


def build_height():
    x, z = grid(HEIGHT_RES, cell_centred=False)

    h = np.full(x.shape, 6.0)
    h += 0.8 * np.sin(x * 0.011 + 1.3) * np.cos(z * 0.009 - 0.4)
    for bx, bz, r, amp in BUMPS:
        h += amp * np.exp(-((x - bx) ** 2 + (z - bz) ** 2) / (2.0 * (r * 0.5) ** 2))

    # Fields sit flat and low - a gentle blend toward 7.
    for x0, z0, x1, z1 in FIELDS:
        w = 0.65 * smoothstep(60.0, 10.0, rect_distance(x, z, x0, z0, x1, z1))
        h = h * (1.0 - w) + 7.0 * w

    # The village plateau: flat inside the walls, easing out to the plains.
    square = np.maximum(np.abs(x), np.abs(z))
    plateau = smoothstep(300.0, 225.0, square)
    h = h * (1.0 - plateau) + 12.0 * plateau

    # River bed and banks.
    d = polyline_distance(x, z, RIVER)
    bank = smoothstep(55.0, 22.0, d)
    h = h * (1.0 - bank) + np.minimum(h, 2.0) * bank
    dip = smoothstep(22.0, 4.0, d)
    h = h * (1.0 - dip) + (WATER - 3.5) * dip

    # Lake.
    for (cx, cz), r in LAKE:
        dl = np.hypot(x - cx, z - cz)
        shore = smoothstep(r + 50.0, r + 20.0, dl)
        h = h * (1.0 - shore) + np.minimum(h, 1.5) * shore
        dip = smoothstep(r + 20.0, r - 12.0, dl)
        floor = WATER - 0.6 - 5.4 * smoothstep(r - 12.0, r * 0.3, dl)
        h = np.minimum(h, h * (1.0 - dip) + floor * dip)

    # Bridges: raised causeways across the river, above the water line.
    for (bx, bz), axis in BRIDGES:
        if axis == "ns":
            m = rect_mask(x, z, bx - 7, bz - 26, bx + 7, bz + 26)
        else:
            m = rect_mask(x, z, bx - 26, bz - 7, bx + 26, bz + 7)
        h = np.where(m, np.maximum(h, 1.6), h)

    # Mountain rim.
    edge = HALF - border_norm(x, z)
    rim = smoothstep(280.0, 80.0, edge)
    ridge = (52.0 + 16.0 * np.sin(x * 0.0061 + 0.4) * np.cos(z * 0.0053 - 0.9)
             + 9.0 * np.sin(x * 0.017 + z * 0.004) + 5.0 * np.cos(z * 0.023 - x * 0.006))
    h = h + rim * ridge

    return x, z, np.clip(h, HEIGHT_MIN, HEIGHT_MAX)


# ---- Ground colours -------------------------------------------------------

GRASS = np.array([88, 128, 70])
GRASS_DARK = np.array([56, 94, 54])
DIRT = np.array([152, 122, 86])
PAVING = np.array([162, 156, 146])
FIELD_A = np.array([212, 176, 84])
FIELD_B = np.array([188, 152, 66])
SAND = np.array([204, 188, 142])
ROCK = np.array([126, 121, 114])
SNOW = np.array([236, 236, 242])
PLANKS = np.array([118, 86, 54])
RIVERBED = np.array([96, 110, 92])


def build_ground(x, z, h):
    c = np.zeros(x.shape + (3,))
    c[:] = GRASS
    c += (6.0 * np.sin(x * 0.05) * np.cos(z * 0.043))[..., None]

    c += (10.0 * np.sin(x * 0.004 - 0.3) * np.cos(z * 0.005 + 0.8))[..., None]

    forest = blob_distance(x, z, FOREST)
    edge = smoothstep(30.0, -10.0, forest)[..., None]
    c = c * (1.0 - edge) + GRASS_DARK * edge

    rocky = blob_distance(x, z, ROCKY)
    edge = smoothstep(20.0, -20.0, rocky)[..., None]
    c = c * (1.0 - edge) + ROCK * edge

    for x0, z0, x1, z1 in FIELDS:
        m = rect_mask(x, z, x0, z0, x1, z1)
        rows = (np.floor((z if abs(x1 - x0) > abs(z1 - z0) else x) / 8.0) % 2) == 0
        c[m & rows] = FIELD_A
        c[m & ~rows] = FIELD_B

    # Inside the walls: dirt lanes, a paved plaza.
    square = np.maximum(np.abs(x), np.abs(z))
    c[square <= VILLAGE_HALF] = DIRT * 0.55 + GRASS * 0.45
    for x0, z0, x1, z1 in VILLAGE_LANES:
        c[rect_mask(x, z, x0, z0, x1, z1)] = DIRT
    c[square <= 38.0] = PAVING

    for road in ROADS:
        edge = smoothstep(ROAD_HALF + 2.0, ROAD_HALF - 2.0, polyline_distance(x, z, road))[..., None]
        c = c * (1.0 - edge) + DIRT * edge

    near = smoothstep(32.0, 20.0, polyline_distance(x, z, RIVER))
    for (cx, cz), r in LAKE:
        near = np.maximum(near, smoothstep(r + 26.0, r + 12.0, np.hypot(x - cx, z - cz)))
    sand = (near * smoothstep(3.0, 1.5, h))[..., None]
    c = c * (1.0 - sand) + SAND * sand
    bed = smoothstep(0.5, -1.0, h)[..., None]
    c = c * (1.0 - bed) + RIVERBED * bed

    for (bx, bz), axis in BRIDGES:
        if axis == "ns":
            c[rect_mask(x, z, bx - 6, bz - 26, bx + 6, bz + 26)] = PLANKS
        else:
            c[rect_mask(x, z, bx - 26, bz - 6, bx + 26, bz + 6)] = PLANKS

    rock = smoothstep(38.0, 50.0, h)[..., None]
    c = c * (1.0 - rock) + ROCK * rock
    snow = smoothstep(84.0, 90.0, h)[..., None]
    c = c * (1.0 - snow) + SNOW * snow

    return np.clip(c, 0, 255).astype(np.uint8)


# ---- Village and scenery objects ------------------------------------------

VILLAGE_LANES = [
    (-GATE_HALF, -VILLAGE_HALF, GATE_HALF, VILLAGE_HALF),      # north-south street
    (-VILLAGE_HALF, -GATE_HALF, VILLAGE_HALF, GATE_HALF),      # east-west street
    (-120, -124, 120, -116), (-120, 116, 120, 124),            # ring lanes
    (-124, -120, -116, 120), (116, -120, 124, 120),
]

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


def build_objects(h_at):
    objs = []

    # Stone wall: model tiles along each side, a 12-unit gate in the middle
    # of each, towers either side of every gate and on every corner. The
    # model heights (1.31 for a wall tile; 1.01, 1.01 and 1.0 for the tower's
    # base, middle and roof) come from the Castle Kit's own bounds. `color`
    # is only what the map picture shows for the piece.
    T = WALL_TILE
    H = VILLAGE_HALF

    # The kit's wall tile tapers slightly at its ends, which leaves a V notch
    # between neighbours; stretching each tile 8% along the wall makes
    # neighbours overlap and closes it.
    def wall_tile(x, z, yaw):
        return box("wall", x, z, T, 1.31 * T, T, STONE_WALL, model="castle/wall", yaw=yaw, scale=T, stretch=1.08)

    def tower(x, z):
        return [box("tower", x, z, T, 1.01 * T, T, STONE_WALL, model="castle/tower-square-base", scale=T),
                box("tower", x, z, T, 1.01 * T, T, STONE_WALL, block=False, elevation=1.01 * T,
                    model="castle/tower-square-mid-windows", scale=T),
                box("tower_roof", x, z, T, 1.0 * T, T, ROOF_BLUE, block=False, elevation=2.02 * T,
                    model="castle/tower-square-top-roof", scale=T)]

    gate_tower = GATE_HALF + T / 2.0
    offsets = []
    c = gate_tower + T
    while c <= H - T + 0.01:
        offsets += [c, -c]
        c += T

    for side in (-1, 1):
        for o in offsets:
            objs.append(wall_tile(o, side * H, 0.0))                 # north / south
            objs.append(wall_tile(side * H, o, math.pi / 2.0))       # west / east
        for g in (-1, 1):
            objs += tower(g * gate_tower, side * H)
            objs += tower(side * H, g * gate_tower)
        for other in (-1, 1):
            objs += tower(side * H, other * H)

    # Plaza: the well, the hall to the north, market stalls.
    objs.append(box("well", 0, 0, 4, 1.2, 4, STONE))
    objs.append(box("well_roof", 0, 0, 5, 0.6, 5, ROOF_BROWN, block=False, elevation=3.2))
    objs += building("hall", 0, -72, 5, door_facing(0, -72, True), 5.0, high_roof=True)
    stalls = [(-29, 31), (-15, 31), (15, 31), (29, 31), (-31, -15), (31, -15)]
    for i, (sx, sz) in enumerate(stalls):
        objs.append(box("stall", sx, sz, 5, 1.2, 3, WOOD))
        objs.append(box("awning", sx, sz, 5.6, 0.4, 3.6, AWNINGS[i % 3], block=False, elevation=2.8))

    # Houses: one grid per quadrant between the streets and ring lanes.
    slots = [(54, 54), (90, 54), (54, 90), (90, 90), (150, 54), (150, 96), (54, 150), (98, 150)]
    for qi, (qx, qz) in enumerate([(-1, -1), (1, -1), (-1, 1), (1, 1)]):
        for si, (ox, oz) in enumerate(slots):
            if qi == 2 and si in (3, 7):
                continue  # the barnyard takes that corner
            if qz == -1 and ox < 60 and oz < 60:
                continue  # keep the hall's surroundings open
            along_x = si % 2 == 0
            hx, hz = qx * ox, qz * oz
            objs += building("house", hx, hz, 3, door_facing(hx, hz, along_x), 4.5,
                             wood=(qi + si) % 2 == 1, high_roof=(qi + si) % 3 == 0)
    # Barnyard, south-west.
    objs += building("barn", -94, 96, 4, door_facing(-94, 96, True), 5.0, wood=True)
    for hx, hz in [(-74, 154), (-84, 158), (-67, 161)]:
        objs.append(box("haystack", hx, hz, 4, 3, 4, HAY))

    # Bridge railings.
    for (bx, bz), axis in BRIDGES:
        for s in (-1, 1):
            if axis == "ns":
                objs.append(box("railing", bx + s * 6.5, bz, 1, 1.4, 52, WOOD))
            else:
                objs.append(box("railing", bx, bz + s * 6.5, 52, 1.4, 1, WOOD))

    # Trees and boulders, placed once here with a fixed seed - after this they
    # are just part of the map data.
    rng = np.random.RandomState(1017)

    def clear_of_paths(px, pz):
        pt_x, pt_z = np.array([[px]]), np.array([[pz]])
        if min(polyline_distance(pt_x, pt_z, r)[0, 0] for r in ROADS) < 14:
            return False
        if polyline_distance(pt_x, pt_z, RIVER)[0, 0] < 30:
            return False
        if any(math.hypot(px - cx, pz - cz) < r + 30 for (cx, cz), r in LAKE):
            return False
        if max(abs(px), abs(pz)) < 280:
            return False
        if any(x0 - 20 <= px <= x1 + 20 and z0 - 20 <= pz <= z1 + 20 for x0, z0, x1, z1 in FIELDS):
            return False
        return HALF - max(abs(px), abs(pz)) > RIM_BLOCK + 20

    def tree(px, pz, scale):
        return [box("tree_trunk", px, pz, 1.2 * scale, 4.0 * scale, 1.2 * scale, TRUNK),
                box("tree_canopy", px, pz, 5.5 * scale, 5.0 * scale, 5.5 * scale,
                    LEAVES[rng.randint(len(LEAVES))], block=False, elevation=3.2 * scale)]

    placed = 0
    while placed < 190:
        px, pz = rng.uniform(150, 880), rng.uniform(-880, -250)
        inside = blob_distance(np.array(px), np.array(pz), FOREST) < -8.0
        if inside and clear_of_paths(px, pz) and h_at(px, pz) > 1.5:
            objs += tree(px, pz, rng.uniform(0.85, 1.35))
            placed += 1
    placed = 0
    while placed < 70:
        px, pz = rng.uniform(-850, 850), rng.uniform(-850, 850)
        if clear_of_paths(px, pz) and 1.5 < h_at(px, pz) < 40:
            objs += tree(px, pz, rng.uniform(0.8, 1.2))
            placed += 1

    placed = 0
    while placed < 34:
        px, pz = rng.uniform(-880, -200), rng.uniform(200, 880)
        inside = blob_distance(np.array(px), np.array(pz), ROCKY) < -5.0
        if inside and clear_of_paths(px, pz):
            s = rng.uniform(2.0, 5.0)
            objs.append(box("boulder", px, pz, s * rng.uniform(0.8, 1.3), s * 0.7, s, BOULDER))
            placed += 1

    return objs


# ---- Block map -------------------------------------------------------------

def build_block(objs):
    x, z = grid(BLOCK_RES, cell_centred=True)
    blocked = (HALF - np.maximum(np.abs(x), np.abs(z))) < RIM_BLOCK
    for o in objs:
        if o["block"]:
            blocked |= rect_mask(x, z, o["x"] - o["sx"] / 2, o["z"] - o["sz"] / 2,
                                 o["x"] + o["sx"] / 2, o["z"] + o["sz"] / 2)
    return np.where(blocked, 255, 0).astype(np.uint8)


# ---- Road map --------------------------------------------------------------

def build_roads():
    # Same resolution as the block map. Walkers prefer these cells.
    x, z = grid(BLOCK_RES, cell_centred=True)
    road = np.zeros(x.shape, dtype=bool)
    for r in ROADS:
        road |= polyline_distance(x, z, r) <= ROAD_HALF + 1.0
    for x0, z0, x1, z1 in VILLAGE_LANES:
        road |= rect_mask(x, z, x0, z0, x1, z1)
    road |= np.maximum(np.abs(x), np.abs(z)) <= 38.0
    for (bx, bz), axis in BRIDGES:
        if axis == "ns":
            road |= rect_mask(x, z, bx - 6, bz - 26, bx + 6, bz + 26)
        else:
            road |= rect_mask(x, z, bx - 26, bz - 6, bx + 26, bz + 6)
    return np.where(road, 255, 0).astype(np.uint8)


# ---- Output ----------------------------------------------------------------

def main():
    os.makedirs(OUT, exist_ok=True)

    x, z, h = build_height()
    step = WORLD / (HEIGHT_RES - 1)

    def h_at(px, pz):
        i = int(round((px + HALF) / step))
        j = int(round((pz + HALF) / step))
        return float(h[min(max(j, 0), HEIGHT_RES - 1), min(max(i, 0), HEIGHT_RES - 1)])

    objs = build_objects(h_at)
    ground = build_ground(x, z, h)
    block = build_block(objs)

    encoded = np.round((h - HEIGHT_MIN) / (HEIGHT_MAX - HEIGHT_MIN) * 65535.0).astype(np.uint16)
    Image.fromarray(encoded).save(os.path.join(OUT, "height.png"))
    Image.fromarray(ground, "RGB").save(os.path.join(OUT, "ground.png"))
    Image.fromarray(block, "L").save(os.path.join(OUT, "block.png"))
    Image.fromarray(build_roads(), "L").save(os.path.join(OUT, "roads.png"))

    meta = {
        "id": 101,
        "name": "Altunay Village",
        "kingdom": 1,
        "worldSize": WORLD,
        "heightMin": HEIGHT_MIN,
        "heightMax": HEIGHT_MAX,
        "waterLevel": WATER,
        "spawn": {"x": 0.0, "z": 24.0, "yaw": -math.pi / 2.0},
        "portals": [],
        "objects": objs,
        "safeZones": [
            {"name": "Village Square", "points": [[-44.0, -44.0], [44.0, -44.0], [44.0, 44.0], [-44.0, 44.0]]},
        ],
        "npcs": [
            {"name": "Weaponsmith", "role": "weaponsmith", "model": "models/warrior/armor_l30.glb",
             "x": -29.0, "z": 27.0, "yaw": math.pi},
            {"name": "Armorsmith", "role": "armorsmith", "model": "models/warrior/armor_l50.glb",
             "x": 29.0, "z": 27.0, "yaw": math.pi},
        ],
        "monsterSpawns": [
            {"monster": 2, "x": -290.0, "z": -200.0, "radius": 14.0, "count": 5},
            {"monster": 2, "x": 290.0, "z": -130.0, "radius": 14.0, "count": 4},
            {"monster": 2, "x": -270.0, "z": 270.0, "radius": 14.0, "count": 4},
            {"monster": 3, "x": 350.0, "z": 200.0, "radius": 30.0, "count": 2},
            {"monster": 3, "x": -110.0, "z": -430.0, "radius": 30.0, "count": 2},
            {"monster": 3, "x": 330.0, "z": 400.0, "radius": 30.0, "count": 2},
            {"monster": 1, "x": -470.0, "z": -250.0, "radius": 30.0, "count": 4},
            {"monster": 1, "x": 430.0, "z": -300.0, "radius": 30.0, "count": 4},
            {"monster": 4, "x": -520.0, "z": -290.0, "radius": 25.0, "count": 3},
            {"monster": 4, "x": -600.0, "z": -370.0, "radius": 25.0, "count": 3},
            {"monster": 5, "x": -560.0, "z": -330.0, "radius": 6.0, "count": 1},
            {"monster": 6, "x": -480.0, "z": 420.0, "radius": 40.0, "count": 2},
            {"monster": 6, "x": 640.0, "z": 260.0, "radius": 30.0, "count": 2},
            {"monster": 7, "x": -180.0, "z": 760.0, "radius": 15.0, "count": 1},
        ],
    }
    with open(os.path.join(OUT, "map.json"), "w", encoding="utf-8") as f:
        json.dump(meta, f, indent=1)

    # Preview: ground colours, hill shading, water, object footprints.
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
    img.save(os.path.join(OUT, "preview.png"))

    print("wrote", OUT, "-", len(objs), "objects")


if __name__ == "__main__":
    main()
