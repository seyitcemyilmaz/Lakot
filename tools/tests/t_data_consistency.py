import json
import math
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent.parent
DATA = ROOT / "data"
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
FAILURES = []
NOTES = []

BANDS = {
    101: (1, 15), 201: (1, 15), 301: (1, 15),
    102: (15, 30), 202: (15, 30), 302: (15, 30),
    401: (25, 40), 402: (35, 50), 403: (45, 60),
}
BAND_LOW_TOLERANCE = {101: 0, 201: 0, 301: 0}
ARRIVAL_MAX_DISTANCE = 120.0
BOSS_HIGH_TOLERANCE = 5


def fail(message):
    FAILURES.append(message)


def load(path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as error:
        fail(f"{path.relative_to(ROOT)}: cannot parse ({error})")
        return None


def point_in_polygon(x, z, points):
    inside = False
    j = len(points) - 1
    for i in range(len(points)):
        xi, zi = points[i]
        xj, zj = points[j]
        if (zi > z) != (zj > z) and x < (xj - xi) * (z - zi) / (zj - zi) + xi:
            inside = not inside
        j = i
    return inside


class BlockGrid:
    def __init__(self, directory, world_size):
        image = Image.open(directory / "block.png").convert("L")
        self.size = image.size[0]
        self.world = world_size
        self.pixels = image.load()
        if image.size[0] != image.size[1]:
            fail(f"{directory.name}/block.png is not square")

    def blocked(self, x, z):
        cell = self.world / self.size
        cx = math.floor((x + self.world * 0.5) / cell)
        cz = math.floor((z + self.world * 0.5) / cell)
        if not (0 <= cx < self.size and 0 <= cz < self.size):
            return True
        return self.pixels[cx, cz] >= 128


def main():
    monsters_doc = load(DATA / "monsters.json")
    items_doc = load(DATA / "items.json")
    kingdoms_doc = load(DATA / "kingdoms.json")
    content = load(DATA / "locale" / "tr" / "content.json")
    if None in (monsters_doc, items_doc, kingdoms_doc, content):
        return

    monsters = {m["id"]: m for m in monsters_doc["monsters"]}
    item_ids = {i["id"] for i in items_doc["items"]}

    for mid, m in monsters.items():
        tag = f"monster {mid} ({m.get('name')})"
        if f"monster.{mid}.name" not in content:
            fail(f"{tag}: tr/content.json has no key monster.{mid}.name")
        elif not str(content[f"monster.{mid}.name"]).strip():
            fail(f"{tag}: tr/content.json monster.{mid}.name is empty")
        model = m.get("model", "")
        if model:
            if not (DATA / model).exists():
                NOTES.append(f"{tag}: model file {model} missing (warning only)")
        else:
            fail(f"{tag}: has no model path")
        for drop in m.get("drops", []):
            if drop.get("itemId") not in item_ids:
                fail(f"{tag}: drop item {drop.get('itemId')} is not in items.json")
        if m.get("minLevel", 0) > m.get("maxLevel", 0):
            fail(f"{tag}: minLevel > maxLevel")

    maps = {}
    for path in sorted((DATA / "maps").glob("*/map.json")):
        doc = load(path)
        if doc is not None:
            maps[doc["id"]] = (path.parent, doc)

    for expected in BANDS:
        if expected not in maps:
            fail(f"map {expected} missing from data/maps")

    for kingdom in kingdoms_doc["kingdoms"]:
        if kingdom["startMap"] not in maps:
            fail(f"kingdom {kingdom['id']}: startMap {kingdom['startMap']} has no map")
    if kingdoms_doc["defaultStartMap"] not in maps:
        fail("kingdoms.json defaultStartMap has no map")

    for mapId, (directory, doc) in maps.items():
        tag = f"map {mapId} ({directory.name})"
        grid = BlockGrid(directory, doc["worldSize"])

        start = doc["spawn"]
        if grid.blocked(start["x"], start["z"]):
            fail(f"{tag}: player spawn ({start['x']},{start['z']}) is on a blocked block.png cell")

        zones = [z["points"] for z in doc.get("safeZones", [])]

        for index, portal in enumerate(doc.get("portals", [])):
            ptag = f"{tag} portal #{index} -> {portal['targetMap']}"
            if grid.blocked(portal["x"], portal["z"]):
                NOTES.append(f"{ptag}: portal centre on blocked cell (warning)")
            target = maps.get(portal["targetMap"])
            if target is None:
                fail(f"{ptag}: target map does not exist")
                continue
            tdir, tdoc = target
            tgrid = BlockGrid(tdir, tdoc["worldSize"])
            if tgrid.blocked(portal["targetX"], portal["targetZ"]):
                fail(f"{ptag}: arrival ({portal['targetX']},{portal['targetZ']}) is on a blocked cell of map {portal['targetMap']}")
            back = [p for p in tdoc.get("portals", []) if p["targetMap"] == mapId]
            if not back:
                fail(f"{ptag}: map {portal['targetMap']} has no portal back to {mapId}")
            elif min(math.hypot(p["x"] - portal["targetX"], p["z"] - portal["targetZ"]) for p in back) > ARRIVAL_MAX_DISTANCE:
                fail(f"{ptag}: arrival is more than {ARRIVAL_MAX_DISTANCE} units from the return portal in map {portal['targetMap']}")

        low, high = BANDS.get(mapId, (None, None))
        for index, spawn in enumerate(doc.get("monsterSpawns", [])):
            stag = f"{tag} spawn #{index} (monster {spawn['monster']} at {spawn['x']},{spawn['z']})"
            m = monsters.get(spawn["monster"])
            if m is None:
                fail(f"{stag}: monster template does not exist")
                continue
            if grid.blocked(spawn["x"], spawn["z"]):
                fail(f"{stag}: centre is on a blocked cell")
            probes = [(spawn["x"], spawn["z"])]
            for k in range(8):
                a = k * math.pi / 4
                probes.append((spawn["x"] + spawn["radius"] * math.cos(a), spawn["z"] + spawn["radius"] * math.sin(a)))
            if any(point_in_polygon(px, pz, pts) for px, pz in probes for pts in zones):
                fail(f"{stag}: spawn area touches a safe zone")
            if low is not None:
                tolerance = BAND_LOW_TOLERANCE.get(mapId, 0)
                if m["minLevel"] < low - tolerance or m["maxLevel"] > high + (BOSS_HIGH_TOLERANCE if m.get("boss") else 0):
                    fail(f"{stag}: levels {m['minLevel']}-{m['maxLevel']} outside map band {low}-{high}")

        for npc in doc.get("npcs", []):
            for itemId in npc.get("shop", []):
                if itemId not in item_ids:
                    fail(f"{tag}: npc {npc.get('name')} sells item {itemId} not in items.json")


main()
for note in NOTES:
    print("note:", note)
if FAILURES:
    for f in FAILURES:
        print("FAIL:", f)
    print(f"{len(FAILURES)} data consistency failure(s)")
    sys.exit(1)
print("data consistency ok")
