"""Aurora skin: northern lights over a pine forest. Three curtains of light sway and shimmer (eight pre-drawn frames that
loop seamlessly), the stars twinkle and one rare visitor."""
import numpy as np
from PIL import Image, ImageDraw
from skinlib import *

S = 2
NF = 8
CYA = np.array([90, 230, 255], np.float32)

RIB = [
    dict(y0=40, a1=11, l1=210, p1=0.3, m1=1, a2=5, l2=95, p2=1.7, n2=2, hh=62, w=1.0,
         c=((90, 255, 160), (40, 220, 210), (150, 90, 255))),
    dict(y0=150, a1=8, l1=170, p1=2.1, m1=-1, a2=5, l2=80, p2=0.4, n2=-2, hh=70, w=0.95,
         c=((110, 255, 190), (50, 200, 240), (200, 110, 255))),
    dict(y0=100, a1=8, l1=250, p1=4.0, m1=1, a2=4, l2=120, p2=2.2, n2=1, hh=44, w=0.6,
         c=((140, 255, 205), (90, 225, 255), (240, 130, 255))),
]


def forest():
    """static mask and colours of the hills and pines, at S x"""
    H, W = 224 * S, 320 * S
    rng = np.random.RandomState(11)
    xs = np.arange(W) / S
    far = 180 + 7 * np.sin(xs / 41 + 1) + 4 * np.sin(xs / 17 + 0.5) + 3 * np.sin(xs / 9)
    yy = np.mgrid[0:H, 0:W][0].astype(np.float32) / S
    far_a = np.clip((yy - far[None, :]) * S + 0.5, 0, 1)
    im = Image.new('L', (W, H), 0); dr = ImageDraw.Draw(im)
    x = -6.0
    while x < 330:
        h = rng.uniform(16, 38); w = rng.uniform(8, 13)
        base = 226
        # a pine: stacked triangles
        for k in range(4):
            t0 = k / 4; t1 = (k + 1) / 4
            top = base - h * (1 - t0 * 0.15); wy = w * (0.35 + 0.65 * (t1))
            yt = base - h + h * t0 * 0.85
            dr.polygon([(x * S, yt * S), ((x - wy / 2) * S, (yt + h * 0.34) * S), ((x + wy / 2) * S, (yt + h * 0.34) * S)], fill=255)
        dr.rectangle([(x - 1) * S, (base - 6) * S, (x + 1) * S, base * S], fill=255)
        x += rng.uniform(6, 11)
    pines = np.array(im, np.float32) / 255
    return far_a, pines


def frame(f, forest_):
    H, W = 224 * S, 320 * S
    X = lambda v: v * S
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    px, py = xx / S, yy / S
    th = 2 * np.pi * f / NF
    sky = vgrad(H, W, [(0, (2, 4, 14)), (0.3, (5, 12, 34)), (0.62, (8, 30, 56)), (0.8, (22, 88, 104)), (1, (6, 18, 30))])
    # soft glow low on the horizon
    sky += (np.exp(-(((py - 176) / 16.0) ** 2)) * 0.9)[..., None] * np.array([30, 100, 110], np.float32)
    tot = np.zeros((H, W, 3), np.float32)
    for r in RIB:
        yc = r['y0'] + r['a1'] * np.sin(2 * np.pi * px / r['l1'] + r['p1'] + r['m1'] * th) + r['a2'] * np.sin(2 * np.pi * px / r['l2'] + r['p2'] - r['n2'] * th)
        hgt = r['hh'] * (0.75 + 0.25 * np.sin(2 * np.pi * px / 140 + r['p1'] * 2 + th))
        dist = yc - py                                           # above the lower edge: positive
        up = np.clip(dist / hgt, 0, None)
        I = np.where(dist >= 0, np.exp(-(up ** 1.25) * 2.4), np.exp(-((dist / 2.2) ** 2)))
        rays = 0.62 + 0.38 * np.sin(2 * np.pi * px / 9.5 + 1.3 * np.sin(2 * np.pi * px / 60 + 2 * th) + np.minimum(up, 2) * 3.0)
        I = I * np.where(dist >= 0, rays, 1.0)
        c0, c1, c2 = [np.array(c, np.float32) for c in r['c']]
        u = np.clip(up, 0, 1.2)
        col = ramp(u / 1.2, [(0, c0), (0.3, c1), (1.0, c2)])
        tot += (I * r['w'])[..., None] * col
    img = sky + np.minimum(tot, 255) * 0.95
    # hills and pines
    far_a, pines = forest_
    img = over(img, np.array([8, 26, 42], np.float32), far_a)
    ridge = 180 + 7 * np.sin(px / 41 + 1) + 4 * np.sin(px / 17 + 0.5) + 3 * np.sin(px / 9)
    rim = np.exp(-((((py - ridge) * S) / 1.2) ** 2))
    img = np.minimum(255, img + rim[..., None] * np.array([40, 110, 120], np.float32) * 0.5)
    img = over(img, np.array([2, 5, 9], np.float32), pines)
    # dark glass behind the playlist and the now-playing block
    listm = rrect(H, W, X(8), X(44), X(312), X(129), X(4), 1.0)
    infom = rrect(H, W, X(8), X(158), X(312), X(223), X(4), 1.0)
    img = img * (1 - 0.5 * listm - 0.16 * infom)[..., None]
    ring1 = np.clip(listm - rrect(H, W, X(8) + 2, X(44) + 2, X(312) - 2, X(129) - 2, X(4) - 1, 1.0), 0, 1)
    ring2 = np.clip(infom - rrect(H, W, X(8) + 2, X(158) + 2, X(312) - 2, X(223) - 2, X(4) - 1, 1.0), 0, 1)
    img = over(img, CYA * 0.55, ring1 * 0.6)
    img = over(img, CYA * 0.4, ring2 * 0.5)
    return np.clip(down(img, S), 0, 255)


def shooting_star():
    """32 x 32 sprite: a streak running from the top left (faint tail) to a bright head at the bottom right"""
    ss = 4; n = 32 * ss
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    x0, y0, x1, y1 = 2 * ss, 6 * ss, 28 * ss, 20 * ss
    dx, dy = x1 - x0, y1 - y0; L2 = dx * dx + dy * dy
    t = np.clip(((xx - x0) * dx + (yy - y0) * dy) / L2, 0, 1)
    d = np.hypot(xx - (x0 + t * dx), yy - (y0 + t * dy))
    wd = (0.4 + 1.9 * t) * ss
    a = np.clip((wd - d) / ss + 0.5, 0, 1) * (t ** 0.8)
    col = ramp(t, [(0, (60, 140, 200)), (0.6, (170, 230, 255)), (1, (255, 255, 255))])
    head = np.clip((3.2 * ss - np.hypot(xx - x1, yy - y1)) / ss + 0.5, 0, 1)
    col = over(col, np.array([255, 255, 255], np.float32), head)
    a = np.maximum(a, head)
    rgb = down(col * a[..., None], ss) / np.maximum(down(a, ss)[..., None], 1e-3)
    al = down(a, ss)
    # soft tail by ordered dither
    return rgb, dither_mask(al, 0, 0)


def build(ctx):
    tiles = ctx.tiles
    layers = []
    fo = forest()
    frames = [frame(f, fo) for f in range(NF)]
    pals, idx = quant_bg(frames, 2, amp=13.0)
    slot = {}
    for r in range(14):
        for g in range(2): slot[(r, g)] = ctx.pal(pals[(r, g)])
    bgt = cut_tiles(idx[0], tiles, 0, 0, 20, 14)
    attr = [slot[(r, c // 10)] for c in range(20) for r in range(14)]
    layers.append(dict(name='backdrop', kind='static', spr=40, cols=20, rows=14, row0=0, x=0, y=0, tile0=bgt[0][0], fstride=0,
                       nframes=1, period=1, attr=attr, init=True))
    r0, r1 = 0, 10                                                # the sky rows that shimmer
    t0 = None
    for f in range(NF):
        ft = cut_tiles(idx[f], tiles, 0, r0 * 16, 20, r1 - r0 + 1)
        if f == 0: t0 = ft[0][0]
        assert ft[0][0] == t0 + f * 20 * (r1 - r0 + 1)
    layers.append(dict(name='aurora', kind='loop', spr=40, cols=20, rows=r1 - r0 + 1, row0=r0, x=0, y=0, tile0=t0,
                       fstride=20 * (r1 - r0 + 1), nframes=NF, period=8, init=False))
    rgb, m = shooting_star()
    pal, ix = quant_sprite_set([rgb], [m])
    sp = ctx.pal(pal)
    st = cut_tiles(ix[0], tiles, 0, 0, 2, 2)
    layers.append(dict(name='mover', kind='static', spr=60, cols=2, rows=2, row0=0, x=-40, y=16, tile0=st[0][0], fstride=0, nframes=1, period=1,
                       pal=sp, init=True,
                       mover=dict(dx=3, dy=1, per=1, steps=130, gap=212900, ys=[14, 30, 8, 40, 22], x0=-40)))
    return dict(id=4, name='Aurora', layers=layers, stars=True)
