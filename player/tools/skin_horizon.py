"""Neon Horizon skin: a retro-future night. A setting sun with slits that slide down it, a glowing grid floor that scrolls
towards you, wireframe mountains, twinkling stars and one rare visitor."""
import numpy as np
from skinlib import *

S = 2
NF = 8                                    # frames in the floor and sun loops
HZ = 128                                  # horizon, in pixels: the bottom edge of the playlist
SUN_X, SUN_Y, SUN_R = 160, 82, 50
SAUCER_Y = 20                             # sprite y that puts the saucer between the title text and the top band
MAG = np.array([255, 60, 170], np.float32); CYA = np.array([50, 235, 255], np.float32)


def mountains(rng):
    xs = np.arange(0, 321, 8)
    h = 8 + 10 * np.abs(np.sin(xs / 37.0 + 0.6)) + 8 * np.abs(np.sin(xs / 17.0 + 2.0)) + 4 * np.abs(np.sin(xs / 7.3))
    h *= 0.5 + 0.8 * (np.abs(xs - 160) / 160.0) ** 1.2          # lower in the middle so the sun is seen, taller to the sides
    return xs, h


def frame(f, mx, my):
    H, W = 224 * S, 320 * S
    X = lambda v: v * S
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    s = f / NF
    # sky
    sky = vgrad(H, W, [(0, (3, 1, 12)), (0.25, (14, 4, 38)), (0.45, (46, 8, 74)), (0.57, (150, 24, 110))])
    img = np.where((yy < X(HZ))[..., None], sky, 0)
    # floor
    fl = ramp((yy - X(HZ)) / X(224 - HZ), [(0, (34, 8, 62)), (0.12, (18, 5, 40)), (1, (4, 1, 12))])
    img = np.where((yy >= X(HZ))[..., None], fl, img)
    # sun with a halo
    d = np.hypot(xx - X(SUN_X), yy - X(SUN_Y))
    halo = np.exp(-((d / X(SUN_R * 1.7)) ** 2)) * (yy < X(HZ))
    img += halo[..., None] * np.array([110, 30, 70], np.float32) * 0.8
    disc_a = np.clip((X(SUN_R) - d) / 1.2 + 0.5, 0, 1) * (yy < X(HZ))
    sun_t = (yy - X(SUN_Y - SUN_R)) / X(2 * SUN_R)
    sun = ramp(sun_t, [(0, (255, 244, 110)), (0.45, (255, 150, 60)), (0.78, (255, 60, 120)), (1, (230, 30, 150))])
    # slits that slide down, thicker as they go
    cut = np.zeros((H, W), np.float32)
    for j in range(5):
        u = j + s
        if u >= 5: continue
        yc = 68 + 60 * (u / 5) ** 1.15
        th = 7.0 * (u / 5) ** 1.0
        cut = np.maximum(cut, np.clip((th / 2 - np.abs(yy / S - yc)) * S + 0.5, 0, 1))
    img = over(img, sun, disc_a * (1 - cut))
    # mountains (wireframe)
    prof = np.interp(xx / S, mx, my)
    ymt = HZ - prof
    ma = np.clip((yy / S - ymt) * S + 0.5, 0, 1) * (yy < X(HZ + 0.5))
    img = over(img, np.array([14, 4, 32], np.float32), ma)
    edge = np.exp(-(((yy / S - ymt) * S / 1.1) ** 2)) * (yy < X(HZ + 0.5))
    img = np.minimum(255, img + edge[..., None] * CYA * 0.8)
    edge2 = np.exp(-(((yy / S - ymt - 0.5) * S / 3.5) ** 2)) * (yy < X(HZ + 0.5))
    img += edge2[..., None] * CYA * 0.18
    # horizon glow
    hg = np.exp(-(((yy - X(HZ)) / X(1.0)) ** 2))
    img = np.minimum(255, img + hg[..., None] * np.array([255, 120, 200], np.float32) * 0.9)
    hg2 = np.exp(-(((yy - X(HZ)) / X(7)) ** 2)) * (yy >= X(HZ))
    img += hg2[..., None] * np.array([120, 20, 110], np.float32) * 0.6
    # grid: horizontals scroll with f, verticals fan out from the vanishing point
    N = 10
    gl = np.zeros((H, W), np.float32)
    for k in range(N + 1):
        dd = (k + s)
        y = HZ + (224 - HZ) * (dd / N) ** 2
        if y > 226: continue
        th = 0.6 + 1.7 * (dd / N)
        gl = np.maximum(gl, np.clip((th / 2 - np.abs(yy / S - y)) * S + 0.5, 0, 1) * (yy >= X(HZ + 1)))
    for i in range(-16, 17):
        xb = 160 + i * 30
        for_th = 1.4
        a = line_alpha(H, W, X(160), X(HZ), X(xb), X(224), for_th * S, 1.0) * (yy >= X(HZ + 1))
        # thinner towards the horizon
        thin = np.clip((yy / S - HZ) / 40, 0.25, 1)
        gl = np.maximum(gl, a * thin * 0.9)
    gcol = ramp((yy / S - HZ) / (224 - HZ), [(0, (255, 90, 190)), (0.5, (230, 50, 200)), (1, (150, 40, 220))])
    fade = np.clip((yy / S - HZ) / 14, 0.25, 1) * 0.8
    img = np.minimum(255, img + (gl * fade)[..., None] * gcol * 0.95)
    # dark glass behind the text: the playlist panel and the now-playing panel, with a thin neon edge
    listm = rrect(H, W, X(8), X(44), X(312), X(129), X(4), 1.0)
    infom = rrect(H, W, X(8), X(158), X(312), X(223), X(4), 1.0)
    img = img * (1 - 0.58 * listm - 0.40 * infom)[..., None]
    ring1 = np.clip(listm - rrect(H, W, X(8) + 2, X(44) + 2, X(312) - 2, X(129) - 2, X(4) - 1, 1.0), 0, 1)
    ring2 = np.clip(infom - rrect(H, W, X(8) + 2, X(158) + 2, X(312) - 2, X(223) - 2, X(4) - 1, 1.0), 0, 1)
    img = over(img, CYA * 0.6, ring1 * 0.75)
    img = over(img, MAG * 0.6, ring2 * 0.7)
    return np.clip(down(img, S), 0, 255)


def mover_frames():
    """4 frames of a 32 x 16 sprite; the lights chase around the rim"""
    out = []
    ss = 4; W, H = 32, 16
    for f in range(4):
        n = np.zeros((H * ss, W * ss, 3), np.float32); a = np.zeros((H * ss, W * ss), np.float32)
        yy, xx = np.mgrid[0:H * ss, 0:W * ss].astype(np.float32)
        yy = 7.5 * ss + (yy - 7.5 * ss) / 0.62          # squashed: the saucer is only about 7 pixels tall so it fits between the title and the top band
        # dome
        dome = np.clip((1 - (((xx - 16 * ss) / (7 * ss)) ** 2 + ((yy - 6.5 * ss) / (5 * ss)) ** 2)) * 4, 0, 1) * (yy < 8 * ss)
        # body: flat ellipse
        body = np.clip((1 - (((xx - 16 * ss) / (15 * ss)) ** 2 + ((yy - 9.5 * ss) / (3.6 * ss)) ** 2)) * 5, 0, 1)
        col_body = ramp((yy / ss - 6) / 8, [(0, (210, 220, 245)), (0.45, (120, 130, 175)), (1, (40, 44, 80))])
        col_dome = ramp((yy / ss - 2) / 7, [(0, (200, 255, 255)), (0.6, (60, 200, 230)), (1, (20, 90, 150))])
        n = over(n, col_body, body); a = np.maximum(a, body)
        n = over(n, col_dome, dome); a = np.maximum(a, dome)
        # lights
        for i in range(6):
            lx = 5.5 + i * 4.2
            on = (i - f) % 4 == 0
            c = np.array([255, 240, 120], np.float32) if on else np.array([90, 40, 120], np.float32)
            l = np.clip((1.5 * ss - np.hypot(xx - lx * ss, yy - 10 * ss)) / 1.0 + 0.5, 0, 1)
            n = over(n, c, l); a = np.maximum(a, l)
            if on:
                g = np.clip((4.0 * ss - np.hypot(xx - lx * ss, yy - 10 * ss)) / (3 * ss), 0, 1) * 0.5
                gm = dither_mask(down(g, ss))
        rgb = down(n * a[..., None], ss) / np.maximum(down(a, ss)[..., None], 1e-3)
        al = down(a, ss)
        out.append((rgb, al > 0.5))
    return out


def build(ctx):
    tiles = ctx.tiles
    layers = []
    mx, my = mountains(None)
    frames = [frame(f, mx, my) for f in range(NF)]
    pals, idx = quant_bg(frames, 2, amp=11.0)
    slot = {}
    for r in range(14):
        for g in range(2): slot[(r, g)] = ctx.pal(pals[(r, g)])
    bgt = cut_tiles(idx[0], tiles, 0, 0, 20, 14)
    attr = [slot[(r, c // 10)] for c in range(20) for r in range(14)]
    layers.append(dict(name='backdrop', kind='static', spr=40, cols=20, rows=14, row0=0, x=0, y=0, tile0=bgt[0][0], fstride=0,
                       nframes=1, period=1, attr=attr, init=True))
    # floor: rows 7..13 of every column, NF frames
    r0, r1 = 8, 13
    t0 = None
    for f in range(NF):
        ft = cut_tiles(idx[f], tiles, 0, r0 * 16, 20, r1 - r0 + 1)
        if f == 0: t0 = ft[0][0]
        assert ft[0][0] == t0 + f * 20 * (r1 - r0 + 1)
    layers.append(dict(name='floor', kind='loop', spr=40, cols=20, rows=r1 - r0 + 1, row0=r0, x=0, y=0, tile0=t0, fstride=20 * (r1 - r0 + 1),
                       nframes=NF, period=4, init=False))
    # sun slits: rows 3..6, columns 6..13
    s0, s1, c0, c1 = 2, 7, 6, 13
    t0 = None
    for f in range(NF):
        ft = cut_tiles(idx[f], tiles, c0 * 16, s0 * 16, c1 - c0 + 1, s1 - s0 + 1)
        if f == 0: t0 = ft[0][0]
    layers.append(dict(name='sun', kind='loop', spr=40 + c0, cols=c1 - c0 + 1, rows=s1 - s0 + 1, row0=s0, x=0, y=0, tile0=t0,
                       fstride=(c1 - c0 + 1) * (s1 - s0 + 1), nframes=NF, period=4, init=False))
    # a rare mover
    sf = mover_frames()
    pal, ix = quant_sprite_set([f[0] for f in sf], [f[1] for f in sf])
    spal = ctx.pal(pal)
    st = []
    for f in range(4): st.append(cut_tiles(ix[f], tiles, 0, 0, 2, 1))
    s0t = st[0][0][0]
    for f in range(4):
        for c in range(2): assert st[f][c][0] == s0t + f * 2 + c
    layers.append(dict(name='mover', kind='loop', spr=60, cols=2, rows=1, row0=0, x=-40, y=24, tile0=s0t, fstride=2, nframes=4, period=5,
                       pal=spal, init=True,
                       mover=dict(dx=1, dy=0, per=2, steps=392, gap=105700, ys=[SAUCER_Y] * 4, x0=-40)))
    return dict(id=3, name='Neon Horizon', layers=layers, stars=True)
