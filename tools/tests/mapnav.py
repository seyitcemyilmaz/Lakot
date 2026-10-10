import json, math, heapq, pathlib
import numpy as np
from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
MAP_DIR = ROOT / "data" / "maps" / "101_altunay_village"
HALF_HEIGHT = 1.0
CELL = 4.0


class MapNav:
    def __init__(self):
        self.meta = json.load(open(MAP_DIR / "map.json", encoding="utf-8"))
        self.size = float(self.meta["worldSize"])
        self.water = float(self.meta.get("waterLevel", 0.0))
        hmin, hmax = float(self.meta["heightMin"]), float(self.meta["heightMax"])
        raw = np.array(Image.open(MAP_DIR / "height.png"), dtype=np.float32)
        self.h = hmin + raw / 65535.0 * (hmax - hmin)
        self.res = self.h.shape[0]
        self.spacing = self.size / (self.res - 1)
        self.block = np.array(Image.open(MAP_DIR / "block.png").convert("L")) >= 128
        self.bres = self.block.shape[0]
        self.grid = None

    def spawns(self, template):
        return [s for s in self.meta["monsterSpawns"] if s["monster"] == template]

    def nearest_spawn(self, template, x, z):
        return min(self.spawns(template), key=lambda s: math.hypot(s["x"] - x, s["z"] - z))

    def height_at(self, x, z):
        last = self.res - 1
        gx = min(max((x + self.size / 2) / self.spacing, 0.0), last)
        gz = min(max((z + self.size / 2) / self.spacing, 0.0), last)
        cx, cz = min(int(gx), self.res - 2), min(int(gz), self.res - 2)
        u, v = gx - cx, gz - cz
        tl, tr, bl, br = self.h[cz, cx], self.h[cz, cx + 1], self.h[cz + 1, cx], self.h[cz + 1, cx + 1]
        if u + v <= 1.0:
            return float(tl + u * (tr - tl) + v * (bl - tl))
        return float(br + (1 - u) * (bl - br) + (1 - v) * (tr - br))

    def y_at(self, x, z):
        return self.height_at(x, z) + HALF_HEIGHT

    def walkable(self, x, z):
        half = self.size / 2
        if not (-half <= x <= half and -half <= z <= half):
            return False
        if self.height_at(x, z) < self.water:
            return False
        cs = self.size / self.bres
        cx, cz = int(math.floor((x + half) / cs)), int(math.floor((z + half) / cs))
        if cx < 0 or cz < 0 or cx >= self.bres or cz >= self.bres:
            return False
        return not self.block[cz, cx]

    def segment_ok(self, x0, z0, x1, z1, margin=0.0):
        d = math.hypot(x1 - x0, z1 - z0)
        n = max(1, int(math.ceil(d / 1.0)))
        for i in range(1, n + 1):
            t = i / n
            x, z = x0 + (x1 - x0) * t, z0 + (z1 - z0) * t
            if not self.walkable(x, z):
                return False
            if margin and not all(self.walkable(x + dx, z + dz) for dx, dz in
                                  ((margin, 0), (-margin, 0), (0, margin), (0, -margin))):
                return False
        return True

    def _cell_ok(self, i, j):
        x = i * CELL - self.size / 2 + CELL / 2
        z = j * CELL - self.size / 2 + CELL / 2
        return all(self.walkable(x + dx, z + dz) for dx in (-1.5, 0, 1.5) for dz in (-1.5, 0, 1.5))

    def _center(self, i, j):
        return i * CELL - self.size / 2 + CELL / 2, j * CELL - self.size / 2 + CELL / 2

    def path(self, x0, z0, x1, z1):
        n = int(self.size / CELL)
        ok = {}

        def good(i, j):
            if i < 0 or j < 0 or i >= n or j >= n:
                return False
            k = (i, j)
            if k not in ok:
                ok[k] = self._cell_ok(i, j)
            return ok[k]

        def cell(x, z):
            return int((x + self.size / 2) // CELL), int((z + self.size / 2) // CELL)

        s, g = cell(x0, z0), cell(x1, z1)
        openq = [(0.0, s)]
        came, cost = {s: None}, {s: 0.0}
        found = None
        while openq:
            _, cur = heapq.heappop(openq)
            if math.hypot(cur[0] - g[0], cur[1] - g[1]) * CELL <= 6.0:
                found = cur
                break
            for di in (-1, 0, 1):
                for dj in (-1, 0, 1):
                    if not di and not dj:
                        continue
                    nb = (cur[0] + di, cur[1] + dj)
                    if not good(*nb):
                        continue
                    if di and dj and not (good(cur[0] + di, cur[1]) and good(cur[0], cur[1] + dj)):
                        continue
                    c = cost[cur] + math.hypot(di, dj)
                    if nb not in cost or c < cost[nb]:
                        cost[nb] = c
                        came[nb] = cur
                        heapq.heappush(openq, (c + math.hypot(nb[0] - g[0], nb[1] - g[1]), nb))
        if found is None:
            return None
        cells = []
        cur = found
        while cur is not None:
            cells.append(cur)
            cur = came[cur]
        cells.reverse()
        pts = [(x0, z0)] + [self._center(*c) for c in cells[1:]]
        return pts


NAV = None


def nav():
    global NAV
    if NAV is None:
        NAV = MapNav()
    return NAV
