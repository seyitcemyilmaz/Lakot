"""Generates the 8 procedural 128x128 grayscale RGBA effect textures into data/effects/textures/<name>.png (fixed seed, edges alpha 0)."""

import os

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT_DIR = os.path.join(ROOT, "data", "effects", "textures")

SIZE = 128
SEED = 64


def grid():
    t = (np.arange(SIZE) + 0.5) / SIZE * 2.0 - 1.0
    x, y = np.meshgrid(t, t)
    return x, y, np.sqrt(x * x + y * y)


def smoothstep(pLow, pHigh, pValue):
    t = np.clip((pValue - pLow) / (pHigh - pLow), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def edge_window(pRadius):
    return 1.0 - smoothstep(0.82, 0.97, pRadius)


def fbm(pRng, pOctaves):
    total = np.zeros((SIZE, SIZE))
    amplitude = 1.0
    norm = 0.0
    for i in range(pOctaves):
        cells = 4 * (2 ** i)
        base = pRng.random((cells + 1, cells + 1))
        base[-1, :] = base[0, :]
        base[:, -1] = base[:, 0]
        img = Image.fromarray((base * 255).astype(np.uint8)).resize((SIZE, SIZE), Image.BICUBIC)
        total += np.asarray(img, dtype=np.float64) / 255.0 * amplitude
        norm += amplitude
        amplitude *= 0.5
    return total / norm


def pack(pShape, pTone):
    shape = np.clip(pShape, 0.0, 1.0)
    tone = np.clip(pTone, 0.0, 1.0)
    rgb = (tone * 255).astype(np.uint8)
    alpha = (shape * 255).astype(np.uint8)
    out = np.stack([rgb, rgb, rgb, alpha], axis=-1)
    out[0, :, 3] = 0
    out[-1, :, 3] = 0
    out[:, 0, 3] = 0
    out[:, -1, 3] = 0
    return Image.fromarray(out, "RGBA")


def soft_glow(pRng):
    _, _, r = grid()
    shape = np.exp(-(r * 2.6) ** 2) * edge_window(r)
    return pack(shape, 0.8 + 0.2 * shape)


def spark(pRng):
    x, y, r = grid()
    core = np.exp(-(x * 9.0) ** 2) * np.exp(-(y * 1.8) ** 2)
    halo = np.exp(-(x * 3.0) ** 2) * np.exp(-(y * 1.4) ** 2) * 0.25
    shape = (core + halo) * edge_window(r)
    return pack(shape, 0.7 + 0.3 * core)


def smoke(pRng):
    _, _, r = grid()
    noise = fbm(pRng, 5)
    body = smoothstep(0.95, 0.15, r)
    shape = np.clip(body * (0.35 + 0.9 * noise) - 0.08, 0.0, 1.0) * edge_window(r)
    return pack(shape * 0.9, 0.45 + 0.35 * noise)


def dust(pRng):
    _, _, r = grid()
    noise = fbm(pRng, 4)
    blotch = smoothstep(0.35, 0.75, noise)
    shape = blotch * smoothstep(0.9, 0.2, r) * edge_window(r) * 0.55
    return pack(shape, 0.6 + 0.2 * noise)


def ring(pRng):
    _, _, r = grid()
    shape = np.exp(-((r - 0.7) / 0.06) ** 2) * edge_window(r)
    return pack(shape, 0.85 + 0.15 * shape)


def star(pRng):
    x, y, r = grid()
    cross = np.exp(-(x * 14.0) ** 2) * np.exp(-(y * 1.6) ** 2) + np.exp(-(y * 14.0) ** 2) * np.exp(-(x * 1.6) ** 2)
    core = np.exp(-(r * 5.0) ** 2)
    shape = (cross * 0.9 + core) * edge_window(r)
    return pack(shape, 0.85 + 0.15 * core)


def ember(pRng):
    _, _, r = grid()
    core = np.exp(-(r * 9.0) ** 2)
    halo = np.exp(-(r * 3.2) ** 2) * 0.35
    shape = (core + halo) * edge_window(r)
    return pack(shape, 0.55 + 0.45 * core)


def shockwave(pRng):
    _, _, r = grid()
    ramp = smoothstep(0.35, 0.78, r)
    drop = 1.0 - smoothstep(0.78, 0.95, r)
    shape = ramp * drop * edge_window(r)
    return pack(shape, 0.7 + 0.3 * ramp)


GENERATORS = {
    "soft_glow": soft_glow,
    "spark": spark,
    "smoke": smoke,
    "dust": dust,
    "ring": ring,
    "star": star,
    "ember": ember,
    "shockwave": shockwave,
}


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    for index, (name, generator) in enumerate(GENERATORS.items()):
        rng = np.random.default_rng(SEED + index)
        generator(rng).save(os.path.join(OUT_DIR, name + ".png"))
    print("wrote %d textures to %s" % (len(GENERATORS), OUT_DIR))


if __name__ == "__main__":
    main()
