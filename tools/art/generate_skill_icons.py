"""Draws placeholder 64x64 skill icons from data/skills.json into src/game/assets/ui/textures/skills/<key>.tga, coloured by class path."""

import json
import math
import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SKILLS_JSON = os.path.join(ROOT, "data", "skills.json")
OUT_DIR = os.path.join(ROOT, "src", "game", "assets", "ui", "textures", "skills")

SIZE = 64
PALETTE = {
    ("alp", "fixed"): ((120, 120, 130), (235, 235, 240)),
    ("alp", "kalkan"): ((150, 110, 30), (250, 245, 225)),
    ("alp", "ofke"): ((140, 30, 20), (255, 150, 50)),
    ("kurtcu", "fixed"): ((70, 90, 70), (225, 235, 215)),
    ("kurtcu", "ok"): ((40, 100, 160), (235, 245, 255)),
    ("kurtcu", "hancer"): ((90, 40, 130), (110, 200, 90)),
    ("bahadir", "fixed"): ((90, 90, 120), (210, 220, 245)),
    ("bahadir", "ates"): ((170, 70, 10), (255, 215, 80)),
    ("bahadir", "golge"): ((50, 20, 80), (170, 110, 220)),
    ("kam", "fixed"): ((60, 100, 100), (210, 240, 235)),
    ("kam", "sifa"): ((40, 120, 60), (240, 205, 100)),
    ("kam", "ruh"): ((20, 110, 130), (130, 210, 240)),
}


def drawGlyph(pDraw, pKind, pTargeting, pIndex, pColor):
    c = SIZE // 2
    if pTargeting == "self":
        pDraw.ellipse((c - 16, c - 16, c + 16, c + 16), outline=pColor, width=4)
        pDraw.ellipse((c - 6, c - 6, c + 6, c + 6), fill=pColor)
    elif pTargeting == "area":
        for i in range(6):
            a = math.radians(60 * i + 15 * pIndex)
            x = c + 18 * math.cos(a)
            y = c + 18 * math.sin(a)
            pDraw.ellipse((x - 5, y - 5, x + 5, y + 5), fill=pColor)
    elif pTargeting == "cone":
        pDraw.polygon([(c, c + 18), (c - 22, c - 16), (c + 22, c - 16)], fill=pColor)
    elif pTargeting == "ally":
        pDraw.rectangle((c - 4, c - 18, c + 4, c + 18), fill=pColor)
        pDraw.rectangle((c - 18, c - 4, c + 18, c + 4), fill=pColor)
    else:
        pDraw.polygon([(c, c - 22), (c + 14, c + 18), (c, c + 8), (c - 14, c + 18)], fill=pColor)
    if pKind == "debuff":
        pDraw.line((c - 20, c - 20, c + 20, c + 20), fill=pColor, width=3)
    elif pKind == "summon":
        pDraw.ellipse((c - 24, c - 24, c + 24, c + 24), outline=pColor, width=2)
    elif pKind == "movement":
        pDraw.line((c - 24, c + 22, c + 24, c + 22), fill=pColor, width=3)


def drawIcon(pSkill, pIndex):
    base, glyph = PALETTE[(pSkill["class"], pSkill["path"])]
    img = Image.new("RGBA", (SIZE, SIZE), base + (255,))
    draw = ImageDraw.Draw(img)
    for y in range(SIZE):
        shade = 1.0 - 0.45 * y / SIZE
        draw.line((0, y, SIZE, y), fill=tuple(int(v * shade) for v in base) + (255,))
    drawGlyph(draw, pSkill["kind"], pSkill["targeting"], pIndex, glyph + (255,))
    draw.rectangle((0, 0, SIZE - 1, SIZE - 1), outline=glyph + (255,), width=2)
    return img


def main():
    with open(SKILLS_JSON, encoding="utf-8") as f:
        skills = json.load(f)["skills"]
    os.makedirs(OUT_DIR, exist_ok=True)
    for i, skill in enumerate(skills):
        drawIcon(skill, i).save(os.path.join(OUT_DIR, skill["key"] + ".tga"))
    print(f"{len(skills)} icons -> {OUT_DIR}")


if __name__ == "__main__":
    main()
