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

OUT_DIR = os.path.join("src", "game", "assets", "ui", "textures", "items")


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


# Must match .inv-doll in theme.rcss and kDollSlots in InventoryRmlController.
DOLL_WIDTH, DOLL_HEIGHT = 188, 194
DOLL_SCALE = 2


def paper_doll():
    k = DOLL_SCALE
    image = Image.new("RGBA", (DOLL_WIDTH * k, DOLL_HEIGHT * k), (0, 0, 0, 0))
    d = ImageDraw.Draw(image)

    body = (58, 58, 70, 255)
    edge = (82, 82, 96, 255)

    def pts(*points):
        return [(x * k, y * k) for x, y in points]

    d.ellipse(pts((68, 14), (120, 70)), fill=body, outline=edge, width=2 * k)            # head
    d.rectangle(pts((86, 66), (102, 82)), fill=body, outline=edge)                       # neck
    d.polygon(pts((50, 80), (138, 80), (130, 152), (58, 152)), fill=body, outline=edge)  # torso
    d.polygon(pts((50, 82), (30, 94), (24, 160), (42, 158)), fill=body, outline=edge)    # left arm
    d.polygon(pts((138, 82), (158, 94), (164, 160), (146, 158)), fill=body, outline=edge)  # right arm
    d.polygon(pts((60, 152), (92, 152), (90, 172), (64, 172)), fill=body, outline=edge)  # left leg
    d.polygon(pts((96, 152), (128, 152), (124, 172), (98, 172)), fill=body, outline=edge)  # right leg
    d.rectangle(pts((62, 172), (92, 182)), fill=edge)                                    # left foot
    d.rectangle(pts((96, 172), (126, 182)), fill=edge)                                   # right foot

    return image


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
