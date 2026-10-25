"""MVS Cabinet skin: the player screen set inside a Neo Geo MVS arcade cabinet (after a photo of the real thing, with our
own lettering): lit orange marquee in a black frame with an NGMC plate, black bezel round a recessed screen, a red and white
striped control panel with two white-topped sticks and eight coloured buttons, and a red front. Player one's stick leans
with the d-pad and buttons A to D sink and flash on a press; the marquee light sweeps along and the equaliser bars on the
plate dance while a track plays (slowly when stopped)."""
import numpy as np
from skinlib import *

S = 2
RED = hexrgb('#e8202c'); YEL = hexrgb('#ffd21e'); GRN = hexrgb('#28c850'); BLU = hexrgb('#2e6cf0')
A = lambda *c: np.array(c, np.float32)
NF = 8                                                       # equaliser frames
NB = 12                                                      # frames of the sweeping light
SWEEP_PER = 8                                                # ticks per sweep frame
SWEEP_LUT = 74                                               # sweep steps in one cycle: 74 * 8 ticks is about ten seconds, 12 of them are the sweep
P1_STICK = (30, 148); P2_STICK = (190, 148)
BTN_Y = [151, 147, 147, 147]; P1_BX = [70, 87, 104, 121]; P2_BX = [226, 243, 260, 277]
BTN_R = 5.6
GL={'A':['01110','10001','10001','11111','10001','10001','10001'],'B':['11110','10001','10001','11110','10001','10001','11110'],'C':['01111','10000','10000','10000','10000','10000','01111'],'D':['11110','10001','10001','10001','10001','10001','11110'],
'1':['00100','01100','00100','00100','00100','00100','01110'],'2':['01110','10001','00001','00110','01000','10000','11111'],'P':['11110','10001','10001','11110','10000','10000','10000'],
'S':['01111','10000','10000','01110','00001','00001','11110'],'T':['11111','00100','00100','00100','00100','00100','00100'],'R':['11110','10001','10001','11110','10100','10010','10001'],
'M':['10001','11011','10101','10101','10001','10001','10001'],'U':['10001','10001','10001','10001','10001','10001','01110'],'L':['10000','10000','10000','10000','10000','10000','11111'],
'I':['01110','00100','00100','00100','00100','00100','01110'],'V':['10001','10001','10001','10001','10001','01010','00100'],'D':['11110','10001','10001','10001','10001','10001','11110'],
'E':['11111','10000','10000','11110','10000','10000','11111'],'O':['01110','10001','10001','10001','10001','10001','01110'],'Y':['10001','10001','01010','00100','00100','00100','00100'],'N':['10001','11001','10101','10011','10001','10001','10001'],
'G':['01110','10001','10000','10111','10001','10001','01110'],'H':['10001','10001','10001','11111','10001','10001','10001'],'C':['01111','10000','10000','10000','10000','10000','01111'],' ':['0']*7,'-':['00000','00000','00000','11111','00000','00000','00000'],'.':['0','0','0','0','0','0','1'],'W':['10001','10001','10001','10101','10101','11011','10001']}


H, W = 224 * S, 320 * S
X = lambda v: v * S
yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)


def noise(seed, sx, sy, amp):
    r = np.random.RandomState(seed); n = gaussian_filter(r.normal(size=(H, W)).astype(np.float32), (sy, sx))
    return (n / (np.abs(n).max() + 1e-6)) * amp


def hline(bg, x0, x1, y, col, w=1.2, a=1.0): return over(bg, A(*col), line_alpha(H, W, X(x0), X(y), X(x1), X(y), w, 1.0) * a)
def vline(bg, x, y0, y1, col, w=1.2, a=1.0): return over(bg, A(*col), line_alpha(H, W, X(x), X(y0), X(x), X(y1), w, 1.0) * a)


def glowblob(bg, cx, cy, rx, ry, col, k):
    g = np.exp(-(((xx - X(cx)) / X(rx)) ** 2 + ((yy - X(cy)) / X(ry)) ** 2))
    return np.minimum(255, bg + g[..., None] * A(*col) * k)


def chrome_ring(bg, cx, cy, r, w=2.2):
    o = disc(H, W, X(cx), X(cy), X(r + w), 1.0); i = disc(H, W, X(cx), X(cy), X(r), 1.0)
    cr = ramp((yy - X(cy - r - w)) / X(2 * (r + w)), [(0, (235, 238, 250)), (0.35, (110, 114, 134)), (0.55, (40, 42, 54)), (0.8, (120, 124, 144)), (1, (60, 62, 76))])
    return over(bg, cr, np.clip(o - i, 0, 1))


def well(bg, cx, cy, r, w=1.8):
    bg = chrome_ring(bg, cx, cy, r, w)
    return over(bg, A(8, 8, 10), disc(H, W, X(cx), X(cy), X(r), 1.0))


def txt(bg, x, y, s, col, sc=1.0, a=1.0):
    cx = x
    for ch in s:
        g = GL.get(ch, GL[' ']); w = len(g[0])
        for gy in range(7):
            for gx in range(w):
                if g[gy][gx] == '1':
                    bg = over(bg, A(*col), rect(H, W, X(cx + gx * sc), X(y + gy * sc), X(cx + gx * sc + sc), X(y + gy * sc + sc), 1.0) * a)
        cx += (w + 1) * sc
    return bg


def twidth(s, sc=1.0): return sum((len(GL.get(c, GL[' '])[0]) + 1) * sc for c in s) - sc


def screws(bg, pts):
    for sx, sy in pts:
        sc = disc(H, W, X(sx), X(sy), X(2.6), 1.0); bg = over(bg, A(130, 132, 148), sc)
        bg = over(bg, A(30, 30, 38), line_alpha(H, W, X(sx - 1.8), X(sy - 1.2), X(sx + 1.8), X(sy + 1.2), 1.1, 1.0) * sc)
    return bg


EQ = [[5, 3, 5, 2.5, 4, 3], [3, 5, 2.5, 5, 3, 4.5], [4.5, 2.5, 4, 3.5, 5.5, 2.5], [2.5, 4, 5.5, 2.5, 3, 5],
      [5.5, 3.5, 3, 4.5, 2.5, 4], [3, 5.5, 4.5, 3, 5, 2.5], [4, 2.5, 5, 5.5, 3.5, 3], [2.5, 4.5, 3.5, 4, 5.5, 5]]


def backdrop(band=None, eq=0):
    """the whole cabinet front without the sticks and button domes; band = position step of the sweeping light (None: no sweep), eq = equaliser frame"""
    metal = noise(7, 12, 0.5, 2.6)
    bg = vgrad(H, W, [(0, (8, 8, 10)), (1, (4, 4, 6))]) + metal[..., None] * 0.8
    for side in (0, 1):                                     # red-brown cabinet sides with a black edge
        a0, a1 = (0, 9) if side == 0 else (311, 320)
        sh = 0.85 + 0.15 * np.sin(yy / X(40) * np.pi)
        bg = over(bg, A(168, 62, 40) * sh[..., None], rect(H, W, X(a0), 0, X(a1), H, 1.0))
        bg = vline(bg, 8.2 if side == 0 else 311.8, 0, 224, (0, 0, 0), 2.4, 1.0)
    # marquee: thick black frame with rounded corners, evenly lit orange-red panel
    fr = rrect(H, W, X(8), X(0), X(312), X(39), X(6), 1.0); bg = over(bg, A(4, 4, 6), fr)
    bg = over(bg, A(50, 50, 60), np.clip(fr - rrect(H, W, X(8) + 1.4, X(0) + 1.4, X(312) - 1.4, X(39) - 1.4, X(5), 1.0), 0, 1) * 0.55)
    pn = rrect(H, W, X(12), X(4), X(308), X(34), X(3), 1.0)
    ty = (yy - X(4)) / X(30)
    box = ramp(np.clip(ty, 0, 1), [(0, (255, 122, 52)), (0.5, (255, 104, 44)), (1, (240, 70, 34))])
    box = box * (0.94 + 0.06 * np.cos((xx / S - 160) / 150 * np.pi / 2))[..., None]
    bg = over(bg, box, pn)
    bg = over(bg, A(255, 180, 110), np.clip(pn - rrect(H, W, X(12), X(4) + 2.0, X(308), X(34), X(3), 1.0), 0, 1) * 0.55)
    if band is not None:                                    # a soft band of light travelling along the marquee (now only once in a while)
        cx0 = -30 + band * (350.0 / (NB - 1))
        bnd = np.exp(-(((xx / S - cx0) / 22.0) ** 2)) * (0.6 + 0.4 * np.exp(-(((yy / S - 14) / 20.0) ** 2)))
        bg = np.minimum(255, bg + (bnd * pn)[..., None] * A(90, 60, 30) * 0.9)
    for k, w in enumerate(('MULTI', 'VIDEO', 'SYSTEM')): bg = txt(bg, 17, 8 + k * 8.5, w, (255, 255, 255), 1)
    pl = rrect(H, W, X(268), X(7), X(306), X(31), X(3), 1.0)             # the NGMC plate with its equaliser
    bg = over(bg, A(40, 6, 6), pl); bg = over(bg, A(255, 225, 200), np.clip(pl - rrect(H, W, X(269.2), X(8.2), X(304.8), X(29.8), X(2.2), 1.0), 0, 1))
    bg = txt(bg, 287 - twidth('NGMC', 1.5) / 2, 10, 'NGMC', (255, 240, 225), 1.5)
    for k, (c_, h) in enumerate(zip((RED, YEL, GRN, BLU, RED, YEL), EQ[eq])):
        bg = over(bg, c_, rect(H, W, X(274 + k * 5), X(28 - h), X(277 + k * 5), X(28), 1.0))
    bg = glowblob(bg, 160, 40, 150, 8, (255, 120, 60), 0.5)
    # black bezel with the screen recessed in it
    bg = over(bg, A(10, 10, 12), rrect(H, W, X(9), X(36), X(311), X(134), X(5), 1.0))
    pm = rrect(H, W, X(12), X(38.5), X(308), X(131), X(5), 1.0)
    bg = over(bg, ramp((yy - X(38)) / X(92), [(0, (6, 12, 30)), (0.5, (3, 6, 16)), (1, (2, 3, 8))]), pm)
    bg = over(bg, A(70, 72, 90), np.clip(pm - rrect(H, W, X(12) + 1.6, X(38.5) + 1.6, X(308) - 1.6, X(131) - 1.6, X(4), 1.0), 0, 1) * 0.85)
    gl = np.clip(1 - np.abs((xx / S - 60 - (yy / S - 38) * 0.9) / 20), 0, 1) * pm * 0.07; bg = bg + gl[..., None] * A(180, 200, 255)
    bg = bg - ((np.sin(yy * np.pi / S) * 0.5 + 0.5) * pm)[..., None] * 3
    bg = screws(bg, [(10.6, 40), (309.4, 40), (10.6, 130), (309.4, 130)])
    # control panel: red and white stripes, mirrored about the middle
    cp = rrect(H, W, X(9), X(135), X(311), X(157.5), X(3), 1.0)
    st = np.floor((np.abs(xx - X(160)) + yy) / (S * 3.5)) % 2
    stripe = np.where(st[..., None] > 0, A(244, 244, 246), A(214, 40, 40)) * (0.82 + 0.18 * np.cos((yy / S - 148) / 12 * np.pi / 2))[..., None]
    bg = over(bg, stripe, cp)
    bg = over(bg, A(8, 8, 10), np.clip(cp - rrect(H, W, X(9) + 1.5, X(135) + 1.5, X(311) - 1.5, X(157.5) - 1.5, X(2), 1.0), 0, 1))
    bg = over(bg, A(10, 10, 14), rrect(H, W, X(148), X(136.5), X(172), X(157), X(2), 1.0))
    for cx in (P1_STICK[0], P2_STICK[0]): bg = well(bg, cx, P1_STICK[1], 9.5, 1.6)
    for bx0 in (P1_BX, P2_BX):
        for i in range(4): bg = well(bg, bx0[i], BTN_Y[i], BTN_R + 1.2, 1.8)
    for cx, lit, t in ((154.5, 1.0, '1P'), (165.5, 0.4, '2P')):          # start buttons and their labels
        bg = well(bg, cx, 143, 3.0, 1.0)
        bg = over(bg, A(230, 40, 40) * (0.4 + 0.6 * lit), disc(H, W, X(cx), X(143), X(2.6), 1.0))
        bg = txt(bg, cx - twidth(t, 0.8) / 2, 150.2, t, (255, 255, 255), 0.8)
    # black deck holding the now-playing box, then the red front with the white band and the help strip
    bg = over(bg, A(10, 10, 12), rect(H, W, X(9), X(157.5), X(311), X(197), 1.0)); bg = hline(bg, 9, 311, 158, (0, 0, 0), 1.2, 0.9)
    nm = rrect(H, W, X(12), X(159.5), X(308), X(194.5), X(4), 1.0)
    bg = over(bg, A(16, 4, 6), nm * 0.9)
    bg = over(bg, A(150, 28, 28), np.clip(nm - rrect(H, W, X(12) + 1.6, X(159.5) + 1.6, X(308) - 1.6, X(194.5) - 1.6, X(3), 1.0), 0, 1) * 0.95)
    bg = over(bg, A(222, 40, 26) * (0.9 + 0.1 * np.sin(yy / X(30)))[..., None], rect(H, W, X(9), X(197), X(311), X(224), 1.0))
    bg = over(bg, A(246, 246, 246), rect(H, W, X(9), X(197), X(311), X(199.4), 1.0))
    hs = rrect(H, W, X(12), X(199.6), X(308), X(218.4), X(3), 1.0)
    bg = over(bg, A(40, 4, 4), hs * 0.85)
    bg = over(bg, A(255, 140, 120), np.clip(hs - rrect(H, W, X(13.2), X(200.8), X(306.8), X(217.2), X(2), 1.0), 0, 1) * 0.6)
    bg = hline(bg, 9, 311, 223.4, (0, 0, 0), 2.0, 0.9)
    return np.clip(down(bg, S), 0, 255)


# ---------------------------------------------------------------- sprites
def sphere(sz, cx, cy, r, base, spec=0.7, dark=1.0, rim=0.45, ss=4):
    n = sz * ss
    y_, x_ = np.mgrid[0:n, 0:n].astype(np.float32)
    dx, dy = (x_ - cx * ss) / (r * ss), (y_ - cy * ss) / (r * ss)
    d2 = dx * dx + dy * dy
    a = (d2 <= 1).astype(np.float32)
    nz = np.sqrt(np.clip(1 - d2, 0, 1))
    L = np.array([-0.42, -0.55, 0.72]); L /= np.linalg.norm(L)
    diff = np.clip(dx * L[0] + dy * L[1] + nz * L[2], 0, 1)
    hl = diff ** 26
    col = base[None, None, :] * (0.28 + 0.9 * diff[..., None]) * dark + (hl * spec * 255)[..., None]
    rimf = np.clip((np.sqrt(d2) - 0.78) / 0.22, 0, 1)
    col = col * (1 - rimf[..., None] * (1 - rim))
    return down(np.clip(col, 0, 255) * a[..., None], ss) / np.maximum(down(a, ss)[..., None], 1e-3), down(a, ss)


def button_frames(base):
    """3 frames of a 16 x 16 dome: 0 at rest, 1 lit (brighter with a glow), 2 pressed (sunk, flashing)"""
    out = []
    for state in range(3):
        if state == 0: dark, spec, glow, r, oy = 0.85, 0.55, 0.0, BTN_R, 0
        elif state == 1: dark, spec, glow, r, oy = 1.12, 0.9, 1.0, BTN_R, 0
        else: dark, spec, glow, r, oy = 1.3, 0.95, 1.0, BTN_R - 0.6, 1
        rgb, a = sphere(16, 8, 8 + oy, r, base, spec=spec, dark=dark)
        rgb = np.where(a[..., None] > 0, rgb, 0); mask = a > 0.5
        if glow:
            y_, x_ = np.mgrid[0:16, 0:16].astype(np.float32)
            d = np.hypot(x_ - 7.5, y_ - 7.5)
            ha = np.clip((8.2 - d) / 1.6, 0, 1) * (d > r + 0.2) * 0.6
            hm = dither_mask(ha)
            rgb = np.where(hm[..., None] & ~mask[..., None], base[None, None, :] * 0.6, rgb); mask = mask | hm
        out.append((rgb, mask))
    return out


def stick_frames():
    """9 frames (dy+1)*3+(dx+1), 32 x 32: shaft and white ball, leaning"""
    out = []
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            tx, ty = 16 + dx * 5.5, 8 + dy * 4.5
            rgb = np.zeros((32, 32, 3), np.float32); mask = np.zeros((32, 32), bool)
            sa = line_alpha(32, 32, 16, 16, tx, ty + 2, 2.2, 1.0)
            sm = sa > 0.5
            rgb[sm] = A(150, 150, 168); mask |= sm
            srgb, a = sphere(32, tx, ty, 6.5, A(245, 245, 250), spec=0.95, dark=1.0, rim=0.55)
            bm = a > 0.5
            rgb = np.where(bm[..., None], srgb, rgb); mask |= bm
            out.append((rgb, mask))
    return out


def build(ctx):
    tiles = ctx.tiles
    layers = []
    # frame 0 is the cabinet at rest (the backdrop); frames 1..NB the sweeping light; the rest the equaliser dancing without the light
    frames = [backdrop(None, 0)] + [backdrop(k, 0) for k in range(NB)] + [backdrop(None, e) for e in range(NF)]
    pals, idx = quant_bg(frames, 2)
    slot = {}
    for r in range(14):
        for g in range(2): slot[(r, g)] = ctx.pal(pals[(r, g)])
    bgt = cut_tiles(idx[0], tiles, 0, 0, 20, 14)
    attr = [slot[(r, c // 10)] for c in range(20) for r in range(14)]
    layers.append(dict(name='backdrop', kind='static', spr=40, cols=20, rows=14, row0=0, x=0, y=0, tile0=bgt[0][0], fstride=0,
                       nframes=1, period=1, attr=attr, init=True))
    r0, r1 = 0, 2
    nr = r1 - r0 + 1
    SC = 16                                                 # the sweep covers columns 0..15; columns 16..19 hold the plate with the equaliser
    t0 = None
    for k in range(NB + 1):                                 # rest frame first (same picture as the backdrop), then the sweep
        ft = cut_tiles(idx[k], tiles, 0, r0 * 16, SC, nr)
        if k == 0: t0 = ft[0][0]
        assert ft[0][0] == t0 + k * SC * nr
    lut = [0] * (SWEEP_LUT - NB) + list(range(1, NB + 1))
    layers.append(dict(name='marquee', kind='chase', spr=40, cols=SC, rows=nr, row0=r0, x=0, y=0, tile0=t0, fstride=SC * nr, cstride=nr,
                       nframes=NB + 1, period=SWEEP_PER, period_play=SWEEP_PER, lut=lut, cstep=0, init=False))
    e0 = None
    for e in range(NF):
        ft = cut_tiles(idx[NB + 1 + e], tiles, SC * 16, r0 * 16, 20 - SC, nr)
        if e == 0: e0 = ft[0][0]
        assert ft[0][0] == e0 + e * (20 - SC) * nr
    layers.append(dict(name='equaliser', kind='loop', spr=40 + SC, cols=20 - SC, rows=nr, row0=r0, x=0, y=0, tile0=e0, fstride=(20 - SC) * nr, cstride=nr,
                       nframes=NF, period=40, period_play=6, init=False))
    # the sticks
    sf = stick_frames()
    pal, ix = quant_sprite_set([f[0] for f in sf], [f[1] for f in sf])
    spal = ctx.pal(pal)
    st = [cut_tiles(ix[f], tiles, 0, 0, 2, 2) for f in range(9)]
    s0 = st[0][0][0]
    for f in range(9):
        for c in range(2):
            for r in range(2): assert st[f][c][r] == s0 + f * 4 + c * 2 + r
    layers.append(dict(name='stick1', kind='state', spr=100, cols=2, rows=2, row0=0, x=P1_STICK[0] - 16, y=P1_STICK[1] - 16, tile0=s0, fstride=4,
                       nframes=9, period=1, src=5, pal=spal, init=True))
    layers.append(dict(name='stick2', kind='static', spr=106, cols=2, rows=2, row0=0, x=P2_STICK[0] - 16, y=P2_STICK[1] - 16, tile0=s0 + 4 * 4, fstride=0,
                       nframes=1, period=1, pal=spal, init=True))
    # the buttons: player one's change with the state, player two's sit at rest
    for i, base in enumerate((RED, YEL, GRN, BLU)):
        fr = button_frames(base)
        pal, ix = quant_sprite_set([f[0] for f in fr], [f[1] for f in fr])
        bp = ctx.pal(pal)
        b0 = tiles.add(ix[0]); tiles.add(ix[1]); tiles.add(ix[2])
        layers.append(dict(name='button1' + 'ABCD'[i], kind='state', spr=102 + i, cols=1, rows=1, row0=0, x=P1_BX[i] - 8, y=BTN_Y[i] - 8, tile0=b0,
                           fstride=1, nframes=3, period=1, src=6 + i, pal=bp, init=True))
        layers.append(dict(name='button2' + 'ABCD'[i], kind='static', spr=108 + i, cols=1, rows=1, row0=0, x=P2_BX[i] - 8, y=BTN_Y[i] - 8, tile0=b0,
                           fstride=0, nframes=1, period=1, pal=bp, init=True))
    return dict(id=2, name='MVS Cabinet', layers=layers, stars=False)
