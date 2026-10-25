#!/usr/bin/env python3
"""Title page art for the Neo Geo Media Centre: backdrop, two turning brass gears, the shining NGMC lettering,
a small logo for the menus, and the glossy menu plates. Everything is pre-rendered here and stored as
C ROM tiles (sprites), the same way the gold lettering is.

Writes assets/title.c1, assets/title.c2 (tiles appended after the spin art), title_data.h (tile numbers,
palettes) and assets/title.json (the same tiles for the Foundry preview). Needs numpy, scipy and Pillow.

Palette ids (sprite palettes): backdrop 128..183, gears 192, gold lettering 193, gold plate 194,
steel plate 195, twinkling stars 196..199.
"""
import base64, json, math, os, re, sys, zlib
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy.ndimage import gaussian_filter

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONT = '/usr/share/fonts/truetype/google-fonts/Poppins-Bold.ttf'
FONTM = '/usr/share/fonts/truetype/google-fonts/Poppins-Medium.ttf'
sd = open(os.path.join(root, 'spin_data.h')).read()
FIRST = int(re.search(r'#define FACE_TILE\s+(\d+)', sd).group(1)) + 1
spin_pal = [int(x, 16) for x in re.search(r'spin_pal\[16\] = \{([^}]*)\}', sd).group(1).replace(' ', '').split(',')]

def w2rgb(w): return np.array([((w >> 8) & 15) * 17, ((w >> 4) & 15) * 17, (w & 15) * 17], np.float32)
def rgb2w(c): return (int(round(c[0] / 17)) << 8) | (int(round(c[1] / 17)) << 4) | int(round(c[2] / 17))

def enc(t):                                                  # same tile format as gen_sprites.py / gen_spin.py
    a = bytearray(); b = bytearray()
    for half in (1, 0):
        for y in range(16):
            row = t[y][half * 8:half * 8 + 8]
            pl = [sum(((int(p) >> k) & 1) << x for x, p in enumerate(row)) for k in range(4)]
            a += bytes(pl[:2]); b += bytes(pl[2:])
    return a, b

def ramp(t, stops):
    t = np.clip(t, 0, 1); out = np.zeros(t.shape + (3,), np.float32)
    for c in range(3): out[..., c] = np.interp(t, [s[0] for s in stops], [s[1][c] for s in stops])
    return out

def ramp15(stops): return ramp(np.linspace(0, 1, 15), stops)          # 15 colours for palette entries 1..15

GOLD = [(0, (255, 248, 205)), (0.42, (236, 176, 52)), (0.5, (86, 48, 8)), (0.56, (206, 138, 30)), (1, (255, 226, 130))]
BRASS = [(0, (246, 214, 140)), (0.5, (176, 118, 44)), (1, (96, 56, 16))]
STEELP = [(0, (120, 134, 160)), (0.5, (52, 60, 82)), (1, (30, 36, 52))]
GOLDP = [(0, (255, 240, 170)), (0.5, (226, 160, 40)), (1, (150, 90, 14))]
GEAR_RAMP = ramp15([(0, (20, 10, 2)), (0.45, (150, 98, 34)), (0.8, (232, 178, 80)), (1, (255, 240, 190))])
STEEL_RAMP = ramp15([(0, (12, 16, 28)), (0.5, (86, 100, 134)), (1, (236, 244, 255))])
GOLD_RAMP = np.array([w2rgb(w) for w in spin_pal[1:]], np.float32)       # the gold lettering's own 15 shades
PLATEGOLD_RAMP = ramp15([(0, (70, 38, 6)), (0.5, (226, 160, 40)), (1, (255, 246, 190))])

def nearest(rgb, pal):                                       # index 1..15 of the closest palette colour
    flat = rgb.reshape(-1, 3)
    d = ((flat[:, None, :] - pal[None, :, :]) ** 2).sum(-1)
    return (d.argmin(1) + 1).reshape(rgb.shape[:2])

def shade(mask, stops, blur, height=1.0, light=0.0):
    hm = gaussian_filter(mask, blur) * height * blur * 2
    gy, gx = np.gradient(hm)
    lx, ly = math.cos(light - 2.4), math.sin(light - 2.4)
    lit = np.clip(0.55 - (gx * lx + gy * ly) * 1.6, 0, 1.6)
    ys = np.where(mask.max(1) > 0.2)[0]
    yy = np.arange(mask.shape[0], dtype=np.float32)[:, None] * np.ones((1, mask.shape[1]), np.float32)
    tt = (yy - ys.min()) / (ys.max() - ys.min() + 1)
    return ramp(tt, stops) * lit[..., None]

def down2(a): return a.reshape(a.shape[0] // 2, 2, a.shape[1] // 2, 2, *a.shape[2:]).mean((1, 3))

tiles = []                                                   # list of 16x16 uint8 index arrays
def add_tile(t):
    tiles.append(np.asarray(t, np.uint8)); return len(tiles) - 1 + FIRST

def cut(idx, alpha, x0, y0, tw, th):                         # split an image into tile columns, return tile numbers[c][r]
    out = []
    for c in range(tw):
        col = []
        for r in range(th):
            t = idx[y0 + r * 16:y0 + r * 16 + 16, x0 + c * 16:x0 + c * 16 + 16] * (alpha[y0 + r * 16:y0 + r * 16 + 16, x0 + c * 16:x0 + c * 16 + 16] > 0.5)
            col.append(add_tile(t))
        out.append(col)
    return out

# ------------------------------------------------------------------ backdrop 320 x 224
def backdrop():
    W, H = 640, 448
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    t = yy / H
    bg = ramp(t, [(0, (6, 10, 28)), (0.5, (14, 20, 48)), (0.62, (60, 38, 40)), (0.64, (24, 14, 24)), (1, (4, 4, 10))])
    cx, cy = W / 2, H * 0.40
    d = np.hypot((xx - cx) / 1.5, (yy - cy)) / (H * 0.55)
    bg += (np.clip(1 - d, 0, 1) ** 2.2)[..., None] * np.array([150, 92, 30], np.float32) * 1.15
    bg *= (1 - 0.55 * np.clip(np.hypot((xx - W / 2) / (W / 2), (yy - H / 2) / (H / 2)) - 0.5, 0, 1))[..., None]
    bg += np.exp(-((yy - H * 0.628) / 2.2) ** 2)[..., None] * np.array([255, 190, 110], np.float32) * 0.6
    fl = yy > H * 0.63
    for k in range(1, 14):
        yl = H * 0.63 + H * 0.37 * ((k / 14) ** 1.8)
        bg += (np.exp(-((yy - yl) / 0.9) ** 2) * fl)[..., None] * np.array([90, 60, 40], np.float32) * 0.35
    return np.clip(down2(bg), 0, 255)

bg_rgb = backdrop()
BG_COLS, BG_ROWS, GROUP = 20, 14, 5
bg_pals = []; bg_idx = np.zeros((224, 320), np.uint8)
for row in range(BG_ROWS):
    for g in range(BG_COLS // GROUP):
        x0, y0 = g * GROUP * 16, row * 16
        blk = np.clip((bg_rgb[y0:y0 + 16, x0:x0 + GROUP * 16] / 17).round(), 0, 15).astype(np.uint8) * 17     # 12 bit colour
        im = Image.fromarray(blk)
        q = im.quantize(colors=15, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
        pl = list(q.getpalette() or [])[:45]; pl += [0] * (45 - len(pl)) if len(pl) < 45 else []
        pal = np.array(pl, np.float32).reshape(15, 3)
        pal = np.clip((pal / 17).round(), 0, 15) * 17
        bay = (np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]], np.float32) / 16 - 0.47) * 15
        yy4, xx4 = np.mgrid[0:16, 0:GROUP * 16]
        src = bg_rgb[y0:y0 + 16, x0:x0 + GROUP * 16] + bay[yy4 % 4, xx4 % 4][..., None]
        bg_idx[y0:y0 + 16, x0:x0 + GROUP * 16] = nearest(np.clip(src, 0, 255), pal)
        bg_pals.append([0x8000] + [rgb2w(c) for c in pal])
bg_tile = {}
for c in range(BG_COLS):
    for r in range(BG_ROWS):
        bg_tile[(c, r)] = add_tile(bg_idx[r * 16:r * 16 + 16, c * 16:c * 16 + 16])
BG0 = bg_tile[(0, 0)]
assert all(bg_tile[(c, r)] == BG0 + c * BG_ROWS + r for c in range(BG_COLS) for r in range(BG_ROWS))

# ------------------------------------------------------------------ gears
def gear_mask(size, r_out, r_in, r_hole, teeth, rot):
    S = 2
    spin = rot
    im = Image.new('L', (size * S, size * S), 0); dr = ImageDraw.Draw(im)
    cx = cy = size * S / 2
    pts = []
    for i in range(teeth):
        a = rot + 2 * math.pi * i / teeth; w = 2 * math.pi / teeth
        for da, r in ((0, r_in), (w * 0.18, r_out), (w * 0.5 - w * 0.18, r_out), (w * 0.5, r_in)):
            pts.append((cx + r * S * math.cos(a + da), cy + r * S * math.sin(a + da)))
    dr.polygon(pts, fill=255)
    # six windows between spokes, leaving a hub and a rim
    rin, rout = r_hole * 0.62, r_hole
    step = 2 * math.pi / teeth                      # one window per tooth so the pattern repeats with the tooth period
    for k in range(teeth):
        a0 = spin + k * step + step * 0.5 + 0.0; a1 = a0 + step * 0.5
        pw = [(cx + rin * S * math.cos(a0), cy + rin * S * math.sin(a0))]
        for i in range(5):
            a = a0 + (a1 - a0) * i / 4; pw.append((cx + rout * S * math.cos(a), cy + rout * S * math.sin(a)))
        pw.append((cx + rin * S * math.cos(a1), cy + rin * S * math.sin(a1)))
        dr.polygon(pw, fill=0)
    dr.ellipse([cx - 5 * S, cy - 5 * S, cx + 5 * S, cy + 5 * S], fill=0)
    return np.array(im, np.float32) / 255

def gear_frames(size, tilesn, r_out, r_in, r_hole, teeth, direction, nf=12):
    out = []
    for f in range(nf):
        rot = direction * f / nf * 2 * math.pi / teeth
        m = gear_mask(size, r_out, r_in, r_hole, teeth, rot)                 # at 2x
        col = shade(m, BRASS, blur=5.2, height=1.0)
        a = down2(m); c = down2(col * m[..., None]) / np.maximum(a[..., None], 1e-3)
        idx = nearest(np.clip(c, 0, 255), GEAR_RAMP)
        out.append(cut(idx, a, 0, 0, tilesn, tilesn))
    return out

GEAR_A = dict(size=160, tiles=10, frames=12); GEAR_B = dict(size=112, tiles=7, frames=12)
gA = gear_frames(160, 10, 73, 65.5, 52, 28, 1)
gB = gear_frames(112, 7, 50, 44.5, 36, 20, -1)
GA0 = gA[0][0][0]; GB0 = gB[0][0][0]
for f in range(12):
    for c in range(10):
        for r in range(10): assert gA[f][c][r] == GA0 + (f * 10 + c) * 10 + r
    for c in range(7):
        for r in range(7): assert gB[f][c][r] == GB0 + (f * 7 + c) * 7 + r

# ------------------------------------------------------------------ lettering (big and small), shine sweeping across
def lettering(txt, fsize2, tw, th, nf, track, band_w):
    S = 2; Wp, Hp = tw * 16 * S, th * 16 * S
    f = ImageFont.truetype(FONT, fsize2)
    adv = [f.getlength(ch) + track * S for ch in txt]
    im = Image.new('L', (Wp, Hp), 0); dr = ImageDraw.Draw(im)
    x = (Wp - sum(adv)) / 2
    for ch, a in zip(txt, adv): dr.text((x, 0), ch, font=f, fill=255); x += a
    m = np.array(im, np.float32) / 255
    ys = np.where(m.max(1) > 0.1)[0]
    m = np.roll(m, (Hp - (ys.max() - ys.min() + 1)) // 2 - ys.min(), axis=0)
    base = shade(m, GOLD, blur=3.2 * fsize2 / 150, height=1.0)
    yy, xx = np.mgrid[0:Hp, 0:Wp].astype(np.float32)
    frames = []
    for k in range(nf):
        lead = Hp * 0.9 + 3 * band_w * S
        sx = -lead + k / nf * (Wp + 2 * lead)
        band = np.exp(-(((xx - sx) - yy * 0.9) / (band_w * S)) ** 2)
        col = np.clip(base + band[..., None] * np.array([255, 255, 230], np.float32), 0, 255)
        a = down2(m); c = down2(col * m[..., None]) / np.maximum(a[..., None], 1e-3)
        idx = nearest(np.clip(c, 0, 255), GOLD_RAMP)
        frames.append(cut(idx, a, 0, 0, tw, th))
    return frames

LG = lettering('NGMC', 150, 16, 4, 32, 10, 22)
SL = lettering('NGMC', 62, 9, 2, 24, 4, 11)
LG0 = LG[0][0][0]; SL0 = SL[0][0][0]
for f in range(32):
    for c in range(16):
        for r in range(4): assert LG[f][c][r] == LG0 + (f * 16 + c) * 4 + r
for f in range(24):
    for c in range(9):
        for r in range(2): assert SL[f][c][r] == SL0 + (f * 9 + c) * 2 + r

# ------------------------------------------------------------------ menu plates: gold (selected) and steel, 3 columns x 2 tiles
def plates(stops, ramp15_):
    S = 2; Wp, Hp = 48 * S, 32 * S
    im = Image.new('L', (Wp, Hp), 0); dr = ImageDraw.Draw(im)
    dr.rounded_rectangle([0, 4 * S, Wp - 1, 28 * S - 1], radius=12 * S, fill=255)
    m = np.array(im, np.float32) / 255
    col = shade(m, stops, blur=3.0, height=1.0)
    a = down2(m); c = down2(col * m[..., None]) / np.maximum(a[..., None], 1e-3)
    idx = nearest(np.clip(c, 0, 255), ramp15_)
    return cut(idx, a, 0, 0, 3, 2)
PG = plates(GOLDP, PLATEGOLD_RAMP); PS = plates(STEELP, STEEL_RAMP)
PL0 = PG[0][0]
for v, P in enumerate((PG, PS)):
    for c in range(3):
        for r in range(2): assert P[c][r] == PL0 + v * 6 + c * 2 + r

# a tile with nothing in it: shrunk one-tile sprites also draw the tiles below them in the sprite's list, so the
# player fills those with this one (see blank_below() in main.c)
BLANK = add_tile(np.zeros((16, 16), np.uint8))

# ------------------------------------------------------------------ write everything
c1 = bytearray(); c2 = bytearray()
for t in tiles:
    a, b = enc(t); c1 += a; c2 += b
open(os.path.join(root, 'assets', 'title.c1'), 'wb').write(c1)
open(os.path.join(root, 'assets', 'title.c2'), 'wb').write(c2)
def wl(ws): return ', '.join('0x%04x' % w for w in ws)
gear_pal = [0x8000] + [rgb2w(c) for c in GEAR_RAMP]
steel_pal = [0x8000] + [rgb2w(c) for c in STEEL_RAMP]
plategold_pal = [0x8000] + [rgb2w(c) for c in PLATEGOLD_RAMP]
H = ['/* Generated by tools/gen_title.py. Do not edit. */',
     '#define TT_BG0   %d      /* backdrop: sprite column c (20 of them), row r (14 tiles each) = TT_BG0 + c * 14 + r */' % BG0,
     '#define TT_GA0   %d      /* big gear: frame f (12), column c (10), row r (10) = TT_GA0 + (f * 10 + c) * 10 + r */' % GA0,
     '#define TT_GB0   %d      /* small gear: frame f (12), column c (7), row r (7) = TT_GB0 + (f * 7 + c) * 7 + r */' % GB0,
     '#define TT_LG0   %d      /* big lettering: frame f (32), column c (16), row r (4) = TT_LG0 + (f * 16 + c) * 4 + r */' % LG0,
     '#define TT_SL0   %d      /* small lettering: frame f (24), column c (9), row r (2) = TT_SL0 + (f * 9 + c) * 2 + r */' % SL0,
     '#define TT_PL0   %d      /* plates: variant v (0 gold, 1 steel), column c (0 left cap, 1 middle, 2 right cap), row r (2) = TT_PL0 + v * 6 + c * 2 + r */' % PL0,
     '#define TT_BLANK %d     /* an empty tile, put under shrunk one-tile sprites */' % BLANK,
     '#define TT_PAL_BG 128',
     '#define TT_PAL_GEAR 192', '#define TT_PAL_GOLD 193', '#define TT_PAL_PGOLD 194', '#define TT_PAL_PSTEEL 195', '#define TT_PAL_STAR 196',
     'static const uint16_t tt_bg_pal[56][16] = {']
for p in bg_pals: H.append('    { %s },' % wl(p))
H += ['};',
      'static const uint16_t tt_gear_pal[16] = { %s };' % wl(gear_pal),
      'static const uint16_t tt_pgold_pal[16] = { %s };' % wl(plategold_pal),
      'static const uint16_t tt_psteel_pal[16] = { %s };' % wl(steel_pal), '']
open(os.path.join(root, 'title_data.h'), 'w').write('\n'.join(H))

# the same tiles for the Foundry preview: 4 bit pixels, deflated
raw = bytearray()
for t in tiles:
    f = t.flatten()
    raw += bytes((int(f[i]) << 4) | int(f[i + 1]) for i in range(0, 256, 2))
json.dump({'first': FIRST, 'n': len(tiles), 'tiles': base64.b64encode(zlib.compress(bytes(raw), 9)).decode(),
           'bg0': BG0, 'ga0': GA0, 'gb0': GB0, 'lg0': LG0, 'sl0': SL0, 'pl0': PL0,
           'bgpal': bg_pals, 'gear': gear_pal, 'gold': spin_pal, 'pgold': plategold_pal, 'psteel': steel_pal},
          open(os.path.join(root, 'assets', 'title.json'), 'w'))
print('tiles', len(tiles), 'first', FIRST, 'bytes per ROM', len(c1), 'json', os.path.getsize(os.path.join(root, 'assets', 'title.json')))
