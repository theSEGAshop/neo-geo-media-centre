"""Shared helpers for tools/gen_skins.py: tile registry, colour quantising, drawing helpers.
Everything is 12 bit colour (4 bits per channel) like the Neo Geo palette."""
import math
import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter

BAYER = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]], np.float32)


def w2rgb(w): return np.array([((w >> 8) & 15) * 17, ((w >> 4) & 15) * 17, (w & 15) * 17], np.float32)
def rgb2w(c): return (int(round(float(c[0]) / 17)) << 8) | (int(round(float(c[1]) / 17)) << 4) | int(round(float(c[2]) / 17))
def snap(c): return np.clip(np.round(np.asarray(c, np.float32) / 17), 0, 15) * 17
def hexrgb(h): return np.array([int(h[i:i + 2], 16) for i in (1, 3, 5)], np.float32)


def enc(t):
    """16x16 array of 4 bit indexes -> (c1 bytes, c2 bytes), same format as gen_title.py"""
    a = bytearray(); b = bytearray()
    for half in (1, 0):
        for y in range(16):
            row = t[y][half * 8:half * 8 + 8]
            pl = [sum(((int(p) >> k) & 1) << x for x, p in enumerate(row)) for k in range(4)]
            a += bytes(pl[:2]); b += bytes(pl[2:])
    return a, b


class Tiles:
    def __init__(self, first):
        self.first = first; self.t = []

    def add(self, arr):
        self.t.append(np.asarray(arr, np.uint8)); return len(self.t) - 1 + self.first


def vgrad(h, w, stops):
    """vertical gradient; stops = [(0..1, (r,g,b)), ...]"""
    t = np.linspace(0, 1, h, dtype=np.float32)
    out = np.zeros((h, w, 3), np.float32)
    for c in range(3):
        col = np.interp(t, [s[0] for s in stops], [s[1][c] for s in stops])
        out[:, :, c] = col[:, None]
    return out


def ramp(t, stops):
    t = np.clip(t, 0, 1); out = np.zeros(t.shape + (3,), np.float32)
    for c in range(3): out[..., c] = np.interp(t, [s[0] for s in stops], [s[1][c] for s in stops])
    return out


def down(a, f):
    h, w = a.shape[:2]
    return a.reshape(h // f, f, w // f, f, *a.shape[2:]).mean((1, 3))


def over(dst, src_rgb, alpha):
    """dst (H,W,3) float; alpha (H,W) 0..1"""
    return dst * (1 - alpha[..., None]) + src_rgb * alpha[..., None]


def disc(h, w, cx, cy, r, soft=1.0):
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    d = np.hypot(xx - cx, yy - cy)
    return np.clip((r - d) / soft + 0.5, 0, 1)


def rect(h, w, x0, y0, x1, y1, soft=1.0):
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    a = np.clip((xx - x0) / soft + 0.5, 0, 1) * np.clip((x1 - xx) / soft + 0.5, 0, 1)
    b = np.clip((yy - y0) / soft + 0.5, 0, 1) * np.clip((y1 - yy) / soft + 0.5, 0, 1)
    return a * b


def rrect(h, w, x0, y0, x1, y1, r, soft=1.0):
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    cx = np.clip(xx, x0 + r, x1 - r); cy = np.clip(yy, y0 + r, y1 - r)
    d = np.hypot(xx - cx, yy - cy)
    return np.clip((r - d) / soft + 0.5, 0, 1)


def line_alpha(h, w, x0, y0, x1, y1, width, soft=1.0):
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    dx, dy = x1 - x0, y1 - y0; L2 = dx * dx + dy * dy + 1e-9
    t = np.clip(((xx - x0) * dx + (yy - y0) * dy) / L2, 0, 1)
    d = np.hypot(xx - (x0 + t * dx), yy - (y0 + t * dy))
    return np.clip((width / 2 - d) / soft + 0.5, 0, 1)


def nearest_idx(rgb, pal):
    """rgb (...,3), pal (n,3) -> index 1..n of the closest"""
    flat = rgb.reshape(-1, 3)
    d = ((flat[:, None, :] - pal[None, :, :]) ** 2).sum(-1)
    return (d.argmin(1) + 1).reshape(rgb.shape[:2])


def median_cut_palette(pixels, n=15):
    """pixels (N,3) float 0..255 -> (n,3) palette snapped to 12 bit"""
    px = np.clip(np.round(pixels / 17), 0, 15).astype(np.uint8) * 17
    if len(px) == 0: return np.zeros((n, 3), np.float32)
    im = Image.fromarray(px.reshape(1, -1, 3).astype(np.uint8))
    q = im.quantize(colors=n, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pl = list(q.getpalette() or [])
    pl = pl[:3 * n] + [0] * max(0, 3 * n - len(pl))
    pal = np.array(pl[:3 * n], np.float32).reshape(n, 3)
    # pad out colours that the quantiser did not use with the darkest so the table is full
    return np.clip(np.round(pal / 17), 0, 15) * 17


def dither_nearest(rgb, pal, amp=15.0):
    h, w = rgb.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w]
    bay = (BAYER / 16 - 0.47) * amp
    src = np.clip(rgb + bay[yy % 4, xx % 4][..., None], 0, 255)
    return nearest_idx(src, pal)


def quant_bg(frames, gx, rows=14, amp=15.0):
    """frames: list of (224,320,3) float. One palette per 16 pixel row and per 320/gx pixel group, shared by all frames.
    Returns (palettes[(row,g)] -> (15,3), idx[frame] (224,320) uint8 values 1..15)"""
    gw = 320 // gx
    pals = {}; idx = [np.zeros((rows * 16, 320), np.uint8) for _ in frames]
    for row in range(rows):
        for g in range(gx):
            x0, y0 = g * gw, row * 16
            blocks = [f[y0:y0 + 16, x0:x0 + gw] for f in frames]
            same = all(np.array_equal(blocks[0], b) for b in blocks[1:])
            stack = blocks[0].reshape(-1, 3) if same else np.concatenate([b.reshape(-1, 3) for b in blocks], 0)
            pal = median_cut_palette(stack, 15)
            pals[(row, g)] = pal
            if same:
                ix = dither_nearest(blocks[0], pal, amp)
                for fi in range(len(frames)): idx[fi][y0:y0 + 16, x0:x0 + gw] = ix
            else:
                for fi, b in enumerate(blocks): idx[fi][y0:y0 + 16, x0:x0 + gw] = dither_nearest(b, pal, amp)
    return pals, idx


def dither_mask(alpha, ox=0, oy=0):
    """ordered-dither a soft alpha into an opaque/transparent mask"""
    h, w = alpha.shape
    yy, xx = np.mgrid[0:h, 0:w]
    th = (BAYER[(yy + oy) % 4, (xx + ox) % 4] + 0.5) / 16
    return alpha > th


def cut_tiles(idx, tiles, x0, y0, cols, rows):
    """idx (H,W) uint8 with 0 = transparent; returns tile numbers [c][r] (column-major as the sprites want them)"""
    out = []
    for c in range(cols):
        col = []
        for r in range(rows):
            col.append(tiles.add(idx[y0 + r * 16:y0 + r * 16 + 16, x0 + c * 16:x0 + c * 16 + 16]))
        out.append(col)
    return out


def quant_sprite_set(rgbs, alphas, n=15):
    """frames of one sprite (list of (H,W,3) float, list of (H,W) bool masks) -> palette (n,3), idx list (0 = transparent)"""
    px = np.concatenate([r[m] for r, m in zip(rgbs, alphas)], 0) if any(m.any() for m in alphas) else np.zeros((0, 3), np.float32)
    pal = median_cut_palette(px, n)
    out = []
    for r, m in zip(rgbs, alphas):
        ix = nearest_idx(r, pal)
        ix = np.where(m, ix, 0).astype(np.uint8)
        out.append(ix)
    return pal, out
