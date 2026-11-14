"""Generates the placeholder item icons and the paper-doll figure.

There is no art pipeline and no artist yet, but "no icons" was making the bag
unreadable - every cell showed a two-letter abbreviation. These are flat,
deliberately simple shapes in the UI's own palette so they read as
placeholders rather than as finished art someone might mistake for the real
thing.

Each icon is generated at its item's FOOTPRINT aspect ratio (a 1x2 relic is
64x128), so the grid can stretch it to the item's
rectangle without distortion.

Re-run after changing an item's footprint or adding a template:

    python tools/generate_placeholder_art.py
"""

import os
import re
from PIL import Image, ImageDraw

CELL = 64

# The UI palette.
BG_DARK = (26, 26, 32, 0)
STEEL = (170, 178, 190, 255)
WOOD = (122, 88, 52, 255)
GOLD = (217, 166, 64, 255)
GOLD_DARK = (156, 118, 44, 255)
RED = (198, 62, 62, 255)
GLASS = (206, 214, 226, 255)
STONE = (128, 132, 140, 255)
STONE_DARK = (92, 96, 104, 255)
OUTLINE = (24, 24, 30, 255)

OUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src", "game", "assets", "ui", "textures", "items")


def new_icon(cells_w, cells_h):
    image = Image.new("RGBA", (cells_w * CELL, cells_h * CELL), BG_DARK)
    return image, ImageDraw.Draw(image)


def potion():
    """1x1 - round flask, red contents."""
    image, d = new_icon(1, 1)
    w, h = image.size

    d.rectangle([w * 0.40, h * 0.12, w * 0.60, h * 0.34], fill=GLASS, outline=OUTLINE)
    d.rectangle([w * 0.36, h * 0.08, w * 0.64, h * 0.18], fill=WOOD, outline=OUTLINE)
    d.ellipse([w * 0.18, h * 0.32, w * 0.82, h * 0.92], fill=RED, outline=OUTLINE)
    # highlight
    d.ellipse([w * 0.30, h * 0.44, w * 0.44, h * 0.58], fill=(235, 150, 150, 255))
    return image


def ore():
    """1x1 - a chunk of rock with a metallic vein."""
    image, d = new_icon(1, 1)
    w, h = image.size

    d.polygon([(w * 0.18, h * 0.62), (w * 0.32, h * 0.26), (w * 0.66, h * 0.18),
               (w * 0.86, h * 0.48), (w * 0.74, h * 0.84), (w * 0.30, h * 0.86)],
              fill=STONE, outline=OUTLINE)
    d.polygon([(w * 0.40, h * 0.42), (w * 0.56, h * 0.34), (w * 0.62, h * 0.52),
               (w * 0.46, h * 0.60)], fill=STEEL, outline=STONE_DARK)
    return image


def relic():
    """1x2 - a hanging amulet."""
    image, d = new_icon(1, 2)
    w, h = image.size
    cx = w // 2

    # chain
    d.arc([cx - 22, 10, cx + 22, h * 0.42], 200, 340, fill=GOLD_DARK, width=4)
    # medallion
    d.ellipse([cx - 24, h * 0.38, cx + 24, h * 0.86], fill=GOLD, outline=OUTLINE)
    d.ellipse([cx - 13, h * 0.47, cx + 13, h * 0.77], fill=GOLD_DARK)
    d.ellipse([cx - 6, h * 0.55, cx + 6, h * 0.69], fill=(120, 200, 210, 255), outline=OUTLINE)
    return image


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOLL_HEADER = os.path.join(ROOT, "src", "game", "gui", "rml", "InventoryRmlController.h")
DOLL_SOURCE = os.path.join(ROOT, "src", "game", "gui", "rml", "InventoryRmlController.cpp")
DOLL_SCALE = 2


def read_doll_layout():
    header = open(DOLL_HEADER, encoding="utf-8").read()
    source = open(DOLL_SOURCE, encoding="utf-8").read()

    def constant(name):
        return int(re.search(name + r"\s*=\s*(\d+)", header).group(1))

    def entries(array):
        start = source.index(array + "[] =")
        block = source[start:source.index("};", start)]
        return re.findall(r"EquipSlotType::e(\w+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)", block)

    cell = constant("kCellSize")
    layout = {"width": constant("kDollWidth"), "height": constant("kDollHeight"), "slots": [], "reserved": []}

    for key, array in (("slots", "kDollSlots"), ("reserved", "kDollReserved")):
        for name, left, top, columns, rows in entries(array):
            layout[key].append((name, int(left), int(top), int(columns) * cell, int(rows) * cell))

    return layout


def paper_doll():
    layout = read_doll_layout()
    k = DOLL_SCALE
    image = Image.new("RGBA", (layout["width"] * k, layout["height"] * k), (0, 0, 0, 0))
    d = ImageDraw.Draw(image)

    body = (58, 58, 70, 255)
    edge = (82, 82, 96, 255)
    glyph = (120, 120, 136, 150)
    reserve = (64, 64, 76, 150)
    cx = layout["width"] // 2

    def pts(*points):
        return [(x * k, y * k) for x, y in points]

    def mirror(*points):
        return [(2 * cx - x, y) for x, y in points]

    d.ellipse(pts((cx - 14, 12), (cx + 14, 48)), fill=body, outline=edge, width=2 * k)
    d.rectangle(pts((cx - 6, 46), (cx + 6, 58)), fill=body, outline=edge)

    d.polygon(pts((cx - 26, 56), (cx + 26, 56), (cx + 20, 112), (cx - 20, 112)), fill=body, outline=edge)

    arm = [(cx - 26, 58), (cx - 34, 64), (cx - 36, 118), (cx - 28, 118), (cx - 24, 72)]
    d.polygon(pts(*arm), fill=body, outline=edge)
    d.polygon(pts(*mirror(*arm)), fill=body, outline=edge)

    leg = [(cx - 20, 112), (cx - 2, 112), (cx - 4, 178), (cx - 4, 188), (cx - 22, 188), (cx - 18, 178)]
    d.polygon(pts(*leg), fill=body, outline=edge)
    d.polygon(pts(*mirror(*leg)), fill=body, outline=edge)

    for name, left, top, width, height in layout["reserved"]:
        d.rounded_rectangle(pts((left, top), (left + width, top + height)), radius=4 * k, outline=reserve, width=k)

    for name, left, top, width, height in layout["reserved"] + layout["slots"]:
        draw_slot_glyph(d, name, (left + width / 2) * k, (top + height / 2) * k, k, glyph)

    return image


def draw_slot_glyph(d, name, x, y, k, color):
    u = 6 * k
    if name == "None":
        d.polygon([(x - 2 * u, y - 2.4 * u), (x + 2 * u, y - 2.4 * u), (x + 2 * u, y + 0.4 * u), (x, y + 2.8 * u),
                   (x - 2 * u, y + 0.4 * u)], fill=color)
    elif name == "Helmet":
        d.pieslice([x - 3 * u, y - 3 * u, x + 3 * u, y + 3 * u], 180, 360, fill=color)
        d.rectangle([x - 3 * u, y, x + 3 * u, y + u], fill=color)
    elif name == "Armor":
        d.polygon([(x - 3 * u, y - 2.5 * u), (x - 1.2 * u, y - 3 * u), (x + 1.2 * u, y - 3 * u), (x + 3 * u, y - 2.5 * u),
                   (x + 2.4 * u, y + 3 * u), (x - 2.4 * u, y + 3 * u)], fill=color)
    elif name == "Boots":
        d.polygon([(x - 2 * u, y - 1.6 * u), (x - 0.4 * u, y - 1.6 * u), (x - 0.4 * u, y + 0.6 * u), (x + 2.2 * u, y + 1.4 * u),
                   (x + 2.2 * u, y + 2.2 * u), (x - 2 * u, y + 2.2 * u)], fill=color)
    elif name == "Weapon":
        d.polygon([(x, y - 4 * u), (x + 0.8 * u, y - 3 * u), (x + 0.8 * u, y + 1.6 * u), (x - 0.8 * u, y + 1.6 * u),
                   (x - 0.8 * u, y - 3 * u)], fill=color)
        d.rectangle([x - 2 * u, y + 1.6 * u, x + 2 * u, y + 2.4 * u], fill=color)
        d.rectangle([x - 0.5 * u, y + 2.4 * u, x + 0.5 * u, y + 4 * u], fill=color)


ICONS = {
    "potion.tga": potion,
    "ore.tga": ore,
    "relic.tga": relic,
}


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    for name, factory in ICONS.items():
        image = factory()
        path = os.path.join(OUT_DIR, name)
        image.save(path)
        print(f"{path}  {image.size[0]}x{image.size[1]}")

    doll = paper_doll()
    doll_path = os.path.join(os.path.dirname(OUT_DIR), "paperdoll.tga")
    doll.save(doll_path)
    print(f"{doll_path}  {doll.size[0]}x{doll.size[1]}")


if __name__ == "__main__":
    main()
