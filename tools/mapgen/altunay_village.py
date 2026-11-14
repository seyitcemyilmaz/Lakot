"""Draws map 101 - Altunay Village - into data/maps/101_altunay_village/.

A one-off sketching tool, not part of the game. It lays the map out from
hand-placed shapes (a walled village on a plateau, roads, a river with two
bridges, a lake, fields, a forest, rocky hills and a mountain rim) and writes
the files the game loads. After that, those files are the map: edit
height.png / ground.png / block.png / roads.png / map.json directly. Re-running this
overwrites any hand edits. Shared building blocks live in mapkit.py.

Needs numpy and Pillow.
"""

import math

import numpy as np

from mapkit import *

MAP_ID = 101
OUT = map_directory(MAP_ID, "altunay_village")

VILLAGE_HALF = 183.0        # the stone wall's centre line, this far from the centre
PLAZA_HALF = 38.0

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
    for bridge in BRIDGES:
        h = np.where(bridge_mask(x, z, bridge, 7), np.maximum(h, 1.6), h)

    h = h + mountain_rim(x, z)

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
    c[square <= PLAZA_HALF] = PAVING

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

    for bridge in BRIDGES:
        c[bridge_mask(x, z, bridge, 6)] = PLANKS

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


def build_objects(h_at):
    objs = walled_square(VILLAGE_HALF)

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
    objs += bridge_railings(BRIDGES)

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

    placed = 0
    while placed < 190:
        px, pz = rng.uniform(150, 880), rng.uniform(-880, -250)
        inside = blob_distance(np.array(px), np.array(pz), FOREST) < -8.0
        if inside and clear_of_paths(px, pz) and h_at(px, pz) > 1.5:
            objs += tree(rng, px, pz, rng.uniform(0.85, 1.35))
            placed += 1
    placed = 0
    while placed < 70:
        px, pz = rng.uniform(-850, 850), rng.uniform(-850, 850)
        if clear_of_paths(px, pz) and 1.5 < h_at(px, pz) < 40:
            objs += tree(rng, px, pz, rng.uniform(0.8, 1.2))
            placed += 1

    placed = 0
    while placed < 34:
        px, pz = rng.uniform(-880, -200), rng.uniform(200, 880)
        inside = blob_distance(np.array(px), np.array(pz), ROCKY) < -5.0
        if inside and clear_of_paths(px, pz):
            objs.append(boulder(rng, px, pz))
            placed += 1

    return objs


def main():
    x, z, h = build_height()
    objs = build_objects(make_height_sampler(h))
    ground = build_ground(x, z, h)
    block = build_block(objs)
    roads = build_roads(ROADS, VILLAGE_LANES, BRIDGES, PLAZA_HALF)

    meta = {
        "id": MAP_ID,
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
            safe_zone("Village Square", [[-44.0, -44.0], [44.0, -44.0], [44.0, 44.0], [-44.0, 44.0]]),
        ],
        "npcs": [
            npc("Weaponsmith", "weaponsmith", "models/warrior/armor_l30.glb", -29.0, 27.0, math.pi,
                shop=[1001, 1003, 1005, 4, 7]),
            npc("Armorsmith", "armorsmith", "models/warrior/armor_l50.glb", 29.0, 27.0, math.pi,
                shop=[2001, 2002, 2003, 3001, 3002, 3003]),
        ],
        "monsterSpawns": [
            monster_spawn(2, -290.0, -200.0, 14.0, 5),
            monster_spawn(2, 290.0, -130.0, 14.0, 4),
            monster_spawn(2, -270.0, 270.0, 14.0, 4),
            monster_spawn(3, 350.0, 200.0, 30.0, 2),
            monster_spawn(3, -110.0, -430.0, 30.0, 2),
            monster_spawn(3, 330.0, 400.0, 30.0, 2),
            monster_spawn(1, -470.0, -250.0, 30.0, 4),
            monster_spawn(1, 430.0, -300.0, 30.0, 4),
            monster_spawn(4, -520.0, -290.0, 25.0, 3),
            monster_spawn(4, -600.0, -370.0, 25.0, 3),
            monster_spawn(5, -560.0, -330.0, 6.0, 1),
            monster_spawn(6, -480.0, 420.0, 40.0, 2),
            monster_spawn(6, 640.0, 260.0, 30.0, 2),
            monster_spawn(7, -180.0, 760.0, 15.0, 1),
        ],
    }
    write_map(OUT, meta, h, ground, block, roads, objs)


if __name__ == "__main__":
    main()
