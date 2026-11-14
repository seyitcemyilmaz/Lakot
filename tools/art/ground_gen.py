import argparse
import os
import time

import numpy as np
from PIL import Image

SIZE = 512
SEED = 8200


def grid(size):
    y, x = np.mgrid[0:size, 0:size]
    return x / size, y / size


def periodic_noise(size, cells, rng):
    base = rng.random((cells, cells)).astype(np.float32)
    ext = np.pad(base, ((0, 3), (0, 3)), mode="wrap")
    u, v = grid(size)
    fx = u * cells
    fy = v * cells
    ix = fx.astype(int)
    iy = fy.astype(int)
    tx = fx - ix
    ty = fy - iy

    def cubic(p0, p1, p2, p3, t):
        return p1 + 0.5 * t * (p2 - p0 + t * (2 * p0 - 5 * p1 + 4 * p2 - p3 + t * (3 * (p1 - p2) + p3 - p0)))

    rows = []
    for j in range(4):
        r = [ext[(iy + j) % (cells + 3), (ix + i) % (cells + 3)] for i in range(4)]
        rows.append(cubic(r[0], r[1], r[2], r[3], tx))
    n = cubic(rows[0], rows[1], rows[2], rows[3], ty)
    return np.clip(n, 0.0, 1.0)


def fbm(size, rng, base=4, octaves=5, gain=0.5):
    total = np.zeros((size, size), np.float32)
    amp = 1.0
    norm = 0.0
    cells = base
    for _ in range(octaves):
        total += amp * periodic_noise(size, cells, rng)
        norm += amp
        amp *= gain
        cells *= 2
    total /= norm
    lo, hi = total.min(), total.max()
    return (total - lo) / (hi - lo + 1e-6)


def blades(size, rng, count, length, width, bright, dark_tone):
    img = np.zeros((size, size), np.float32)
    for _ in range(count):
        cx = rng.random() * size
        cy = rng.random() * size
        ang = rng.normal(0, 0.9) + np.pi / 2
        ln = length * (0.5 + rng.random())
        val = dark_tone + (bright - dark_tone) * rng.random()
        steps = int(ln * 2)
        for s in range(steps):
            t = s / steps
            px = cx + np.cos(ang) * t * ln + np.sin(t * 3 + cx) * 0.4 * t * ln * 0.2
            py = cy + np.sin(ang) * t * ln
            w = max(0.6, width * (1 - t))
            for dx in range(-int(w), int(w) + 1):
                xi = int(px + dx) % size
                yi = int(py) % size
                img[yi, xi] = max(img[yi, xi], val * (0.55 + 0.45 * t))
    return img


def lerp_color(a, b, t):
    a = np.array(a, np.float32)
    b = np.array(b, np.float32)
    return a[None, None, :] + (b - a)[None, None, :] * t[..., None]


def finish(rgb):
    return np.clip(rgb, 0, 255).astype(np.uint8)


def grass(rng, dark, light, blade_count, blade_len, dry_tint=0.0):
    big = fbm(SIZE, rng, 3, 5)
    fine = fbm(SIZE, rng, 32, 3)
    bl = blades(SIZE, rng, blade_count, blade_len, 1.2, 1.0, 0.3)
    t = np.clip(0.30 + 0.45 * big + 0.25 * fine + 0.35 * bl - 0.12, 0, 1)
    rgb = lerp_color(dark, light, t)
    shadow = 1.0 - 0.18 * np.clip(0.6 - fine, 0, 1)[..., None]
    return finish(rgb * shadow)


def make_grass(rng):
    return grass(rng, (46, 68, 30), (126, 156, 82), 9000, 14)


def make_dry_grass(rng):
    return grass(rng, (118, 100, 56), (206, 184, 112), 9500, 18)


def make_dirt(rng):
    big = fbm(SIZE, rng, 4, 5)
    fine = fbm(SIZE, rng, 64, 3, 0.6)
    clods = np.clip((fbm(SIZE, rng, 16, 4) - 0.5) * 3, -1, 1) * 0.5 + 0.5
    t = np.clip(0.2 + 0.4 * big + 0.25 * fine + 0.25 * clods, 0, 1)
    rgb = lerp_color((84, 62, 42), (156, 122, 86), t)
    pebbles = (fbm(SIZE, rng, 96, 2) > 0.82).astype(np.float32)
    rgb = rgb * (1 - 0.12 * pebbles[..., None]) + 18 * pebbles[..., None]
    return finish(rgb)


def make_road(rng):
    big = fbm(SIZE, rng, 4, 4)
    fine = fbm(SIZE, rng, 96, 3, 0.65)
    ruts_n = periodic_noise(SIZE, 4, rng)
    u, v = grid(SIZE)
    ruts = 0.5 + 0.5 * np.cos((u * 2 + 0.15 * ruts_n) * 2 * np.pi * 2)
    t = np.clip(0.25 + 0.35 * big + 0.3 * fine + 0.12 * ruts, 0, 1)
    rgb = lerp_color((118, 98, 72), (186, 160, 122), t)
    stones = (fbm(SIZE, rng, 128, 2) > 0.78).astype(np.float32)
    rgb = rgb * (1 - 0.1 * stones[..., None]) + 26 * stones[..., None]
    return finish(rgb)


def make_rock(rng):
    big = fbm(SIZE, rng, 4, 5)
    fine = fbm(SIZE, rng, 48, 3, 0.6)
    warp = fbm(SIZE, rng, 6, 3)
    u, v = grid(SIZE)
    strata = 0.5 + 0.5 * np.sin((v * 7 + warp * 2.5 + 0.4 * big) * 2 * np.pi)
    crack_n = fbm(SIZE, rng, 8, 4)
    cracks = np.clip(1.0 - np.abs(crack_n - 0.5) * 30, 0, 1) ** 2
    t = np.clip(0.25 + 0.35 * big + 0.2 * fine + 0.15 * strata, 0, 1)
    rgb = lerp_color((84, 80, 74), (176, 168, 154), t)
    rgb = rgb * (1 - 0.2 * cracks[..., None])
    warm = fbm(SIZE, rng, 3, 3)
    rgb = rgb + np.stack([10 * warm, 4 * warm, -6 * warm], -1)
    return finish(rgb)


def make_macro(rng):
    n = fbm(SIZE // 2, rng, 3, 5)
    n = np.array(Image.fromarray((n * 255).astype(np.uint8)).resize((SIZE, SIZE), Image.BICUBIC))
    return n


def wrap_error(arr):
    a = arr.astype(np.float32)
    seam_x = np.abs(a[:, 0] - a[:, -1]).mean()
    seam_y = np.abs(a[0] - a[-1]).mean()
    inner_x = np.abs(a[:, 1:] - a[:, :-1]).mean()
    inner_y = np.abs(a[1:] - a[:-1]).mean()
    return seam_x / (inner_x + 1e-6), seam_y / (inner_y + 1e-6)


def tile(img, n):
    return np.tile(img, (n, n, 1) if img.ndim == 3 else (n, n))


def save_checks(textures, outdir):
    os.makedirs(outdir, exist_ok=True)
    half = SIZE // 2
    for name, arr in textures.items():
        t2 = tile(arr, 2)[half:half + SIZE, half:half + SIZE]
        t8 = Image.fromarray(tile(arr, 8)).resize((SIZE * 2, SIZE * 2), Image.LANCZOS)
        Image.fromarray(t2).save(os.path.join(outdir, f"{name}_seam2x2.png"))
        t8.save(os.path.join(outdir, f"{name}_far8x8.png"))
    names = list(textures)
    sheet = Image.new("RGB", (SIZE * len(names), SIZE * 2))
    for i, name in enumerate(names):
        arr = textures[name]
        t2 = tile(arr, 2)[SIZE // 2:SIZE // 2 + SIZE, SIZE // 2:SIZE // 2 + SIZE]
        far = Image.fromarray(tile(arr, 8)).resize((SIZE, SIZE), Image.LANCZOS)
        if arr.ndim == 2:
            t2 = np.stack([t2] * 3, -1)
            far = far.convert("RGB")
        sheet.paste(Image.fromarray(t2), (i * SIZE, 0))
        sheet.paste(far, (i * SIZE, SIZE))
    sheet.save(os.path.join(outdir, "sheet.png"))


def main():
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(root, "data", "textures", "ground"))
    ap.add_argument("--check", default=os.path.join(root, "tasks", "vision", "ground-" + time.strftime("%Y%m%d-%H%M%S")))
    args = ap.parse_args()

    makers = [
        ("grass", make_grass),
        ("dry_grass", make_dry_grass),
        ("dirt", make_dirt),
        ("road", make_road),
        ("rock", make_rock),
        ("macro", make_macro),
    ]
    os.makedirs(args.out, exist_ok=True)
    textures = {}
    total = 0
    for i, (name, fn) in enumerate(makers):
        arr = fn(np.random.default_rng(SEED + i))
        textures[name] = arr
        path = os.path.join(args.out, name + ".png")
        Image.fromarray(arr).save(path, optimize=True)
        size = os.path.getsize(path)
        total += size
        ex, ey = wrap_error(arr)
        print(f"{name}: {size} bytes, seam/inner x={ex:.2f} y={ey:.2f}")
    print(f"total {total} bytes -> {args.out}")
    save_checks(textures, args.check)
    print("checks ->", args.check)


if __name__ == "__main__":
    main()
