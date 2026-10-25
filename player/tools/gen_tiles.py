#!/usr/bin/env python3
"""Generate assets/cogs.fix (and the alternative tile sets the Foundry can patch in, assets/fixsets.json): extra fix-layer tiles for the Media Centre player.

    python3 tools/gen_tiles.py            # writes assets/cogs.fix and tools/tiles-preview.png

The tiles are appended after ngdevkit's text tiles in the S ROM (see the
Makefile), so tile number COG_BASE = 1280 is the first of ours.

Tile order (matches main.c):
    0        blank
    1..8     chain belt, the same 8-pixel link pattern shifted 0..7 pixels, so a row of
             them slides smoothly (the two '=' lines on the screen)
    9..40    cog pair, 4 frames x (2 rows x 4 columns) tiles:  9 + frame*8 + row*4 + col
    41..55   solid blocks in colour index 1..15 (plasma and life screensavers)
    56..64   cobweb for the top-left corner, 3 x 3 tiles (row-major), colour index 1 of palette 4
    65..73   the same web mirrored for the top-right corner

Colour indexes inside a tile (palette 7 in main.c):
    1 brass body, 2 dark edge, 3 brass highlight, 4 copper body, 5 dark edge, 6 copper highlight
    belts use palette 1 (brass) and colour indexes 1 (body), 2 (dark edge) and 3 (shine)

The tile encoding was checked byte for byte against ngdevkit's own tiletool output:
four 8-byte groups for pixel columns (4,5) (6,7) (0,1) (2,3); within a byte the low
nibble is the left pixel.
"""
import math
import os
import sys

from PIL import Image, ImageDraw

SS = 16                      # supersampling for smooth edges before reducing to 16x16 pixel art
W, H = 32, 16                # one cog pair = 4 x 2 tiles
TEETH = 8
R_OUT, R_ROOT, HOLE = 7.5, 5.3, 1.6
CA = (9.0, 8.0)              # centre of the first cog
CB = (CA[0] + 13.4, 8.0)     # centre of the second cog; teeth of each dip into the gaps of the other
FRAMES = 4                   # 4 frames x 11.25 degrees = one tooth pitch (45 degrees), so the loop is seamless


def gear_cov(cx, cy, rot_deg):
    big = Image.new('L', (W * SS, H * SS), 0)
    d = ImageDraw.Draw(big)
    pitch = 2 * math.pi / TEETH
    rot = math.radians(rot_deg)
    pts = []
    for k in range(TEETH):
        for f, r in ((-.30, R_ROOT), (-.17, R_OUT), (.17, R_OUT), (.30, R_ROOT)):
            a = rot + (k + f) * pitch
            pts.append(((cx + math.cos(a) * r) * SS, (cy + math.sin(a) * r) * SS))
    d.polygon(pts, fill=255)
    d.ellipse([(cx - HOLE) * SS, (cy - HOLE) * SS, (cx + HOLE) * SS, (cy + HOLE) * SS], fill=0)
    return big.resize((W, H), Image.BOX)


def shade(cov, x, y, cx, cy, base):
    c = cov / 255.0
    dx, dy = x + .5 - cx, y + .5 - cy
    dist = math.hypot(dx, dy)
    if c >= 0.5:
        if dist < 2.7:
            return base + 1                       # dark ring around the hub
        if dx + dy < -3.2 and dist > 3.0:
            return base + 2                       # light catching the upper-left rim
        return base
    if c >= 0.18:
        return base + 1                           # dark edge
    return 0


def cog_frame(f):
    delta = f * 360.0 / TEETH / FRAMES
    a = gear_cov(CA[0], CA[1], delta)
    b = gear_cov(CB[0], CB[1], 180 + 180.0 / TEETH - delta)   # offset half a tooth, turning the other way
    img = Image.new('P', (W, H), 0)
    pa, pb = a.load(), b.load()
    out = img.load()
    for y in range(H):
        for x in range(W):
            ia = shade(pa[x, y], x, y, CA[0], CA[1], 1)
            ib = shade(pb[x, y], x, y, CB[0], CB[1], 4)
            if ia and ib:
                out[x, y] = ia if pa[x, y] >= pb[x, y] else ib
            else:
                out[x, y] = ia or ib
    return img


def belt_tile(k):
    """Two thin brass rails with a 2 pixel gap every 8 pixels (a chain of links),
    shifted k pixels to the right. Rails sit on rows 2-3 and 5-6 like the '=' sign."""
    t = Image.new('P', (8, 8), 0)
    px = t.load()
    for x in range(8):
        if (x - k) % 8 in (0, 1):
            continue                                  # the gap between two links
        for y0 in (2, 5):
            px[x, y0] = 3                             # shine on top of each rail
            px[x, y0 + 1] = 1                         # body
    return t


def encode(tile):
    px = tile.load()
    out = bytearray()
    for x0 in (4, 6, 0, 2):
        for y in range(8):
            out.append(px[x0, y] | (px[x0 + 1, y] << 4))
    return bytes(out)


def web_canvas():
    """A 24 x 24 cobweb hanging in the top-left corner: threads from the corner plus sagging arcs."""
    N = 24
    t = Image.new('P', (N, N), 0)
    px = t.load()
    def dot(x, y):
        if 0 <= x < N and 0 <= y < N:
            px[x, y] = 1
    def line(x0, y0, x1, y1):
        n = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(n + 1):
            dot(round(x0 + (x1 - x0) * i / n), round(y0 + (y1 - y0) * i / n))
    angles = [0, 22, 45, 68, 90]
    ends = [(round(math.cos(math.radians(a)) * 23), round(math.sin(math.radians(a)) * 23)) for a in angles]
    for ex, ey in ends:
        line(0, 0, ex, ey)
    for r in (7, 13, 19):
        pts = [(math.cos(math.radians(a)) * r, math.sin(math.radians(a)) * r) for a in angles]
        for (ax, ay), (bx, by), a0, a1 in zip(pts, pts[1:], angles, angles[1:]):
            mid = math.radians((a0 + a1) / 2)
            mx, my = math.cos(mid) * r * 0.86, math.sin(mid) * r * 0.86       # sag towards the corner
            line(round(ax), round(ay), round(mx), round(my))
            line(round(mx), round(my), round(bx), round(by))
    return t


def web_tiles(mirror):
    c = web_canvas()
    if mirror:
        c = c.transpose(Image.FLIP_LEFT_RIGHT)
    return [c.crop((x * 8, y * 8, x * 8 + 8, y * 8 + 8)) for y in range(3) for x in range(3)]


def belt_tile_candy(k):
    """Halloween belt: a candy-striped band (orange and dark orange, four pixels each), sliding k pixels."""
    t = Image.new('P', (8, 8), 0)
    px = t.load()
    for x in range(8):
        stripe = 1 if ((x - k) % 8) < 4 else 2
        px[x, 2] = 3                                  # shine on top
        px[x, 3] = stripe
        px[x, 4] = stripe
        px[x, 5] = 2                                  # dark underside
    return t


PUMPKIN_GLOW = ([6, 6, 3, 6], [6, 3, 6, 6])           # glow colour per frame for the first and second pumpkin


def pumpkin(px, cx, cy, glow):
    for y in range(16):
        for x in range(32):
            dx, dy = (x + .5 - cx) / 6.7, (y + .5 - cy) / 5.7
            d = dx * dx + dy * dy
            if d <= 1:
                v = 1
                if d > .72:
                    v = 2
                if abs(abs(x + .5 - cx) - 3.3) < .5 and d < .72:
                    v = 2                             # ribs
                if (x + .5 - cx) + (y + .5 - cy) < -4.2 and d < .72:
                    v = 3                             # highlight
                px[x, y] = v
    ix, iy = int(cx), int(cy)
    for (x, y) in ((ix, 1), (ix, 2), (ix - 1, 3), (ix, 3)):
        px[x, y] = 4
    px[ix + 1, 1] = 5
    px[ix - 1, 2] = 5
    for x in (-4, -3, -2, 3, 4, 5):                   # eyes: little triangles
        px[ix + x, iy + 1] = glow
    px[ix - 3, iy] = glow
    px[ix + 4, iy] = glow
    px[ix, iy + 2] = glow
    for (dx, dy) in ((-4, 3), (-3, 4), (-2, 3), (-1, 4), (0, 3), (1, 4), (2, 3), (3, 4), (4, 3)):
        px[ix + dx, iy + dy] = glow                   # jagged grin


def pumpkin_frame(f):
    img = Image.new('P', (W, H), 0)
    px = img.load()
    pumpkin(px, 8.5, 8.4, PUMPKIN_GLOW[0][f])
    pumpkin(px, 24.5, 8.4, PUMPKIN_GLOW[1][f])
    return img



def belt_tile_cane(k):
    """Winter belt: a candy cane band (red body, pale stripe, four pixels each), sliding k pixels."""
    t = Image.new('P', (8, 8), 0)
    px = t.load()
    for x in range(8):
        stripe = 1 if ((x - k) % 8) < 4 else 3
        px[x, 2] = 3
        px[x, 3] = stripe
        px[x, 4] = stripe
        px[x, 5] = 2
    return t


def snowflake(px, cx, cy, rot_deg):
    """A six-armed snowflake with little side branches, drawn big and reduced: colour 1 arms, 2 soft edge, 3 tips and centre."""
    S2 = 16
    big = Image.new('L', (16 * S2, 16 * S2), 0)
    d = ImageDraw.Draw(big)
    for a in range(6):
        ang = math.radians(rot_deg + a * 60)
        ux, uy = math.cos(ang), math.sin(ang)
        d.line([(8 * S2, 8 * S2), ((8 + ux * 7.2) * S2, (8 + uy * 7.2) * S2)], fill=255, width=int(1.5 * S2))
        for frac, ln in ((0.45, 2.6), (0.72, 1.7)):
            bx, by = 8 + ux * 7.2 * frac, 8 + uy * 7.2 * frac
            for sgn in (-1, 1):
                ba = ang + sgn * math.radians(55)
                d.line([(bx * S2, by * S2), ((bx + math.cos(ba) * ln) * S2, (by + math.sin(ba) * ln) * S2)], fill=255, width=int(1.1 * S2))
    cov = big.resize((16, 16), Image.BOX)
    cp = cov.load()
    for y in range(16):
        for x in range(16):
            c = cp[x, y] / 255.0
            if c >= 0.5:
                px[cx - 8 + x, cy - 8 + y] = 3 if math.hypot(x + .5 - 8, y + .5 - 8) > 6.4 or math.hypot(x + .5 - 8, y + .5 - 8) < 1.3 else 1
            elif c >= 0.18:
                px[cx - 8 + x, cy - 8 + y] = 2


def bauble(px, cx, cy, f):
    """A red Christmas ornament with a cap and a highlight; a sparkle blinks beside it (colour 4 red, 5 dark, 6 light, 1/3 cap and sparkle)."""
    for y in range(16):
        for x in range(16):
            dx, dy = x + .5 - 8, y + .5 - 9.2
            d = math.hypot(dx, dy)
            if d <= 5.6:
                v = 4
                if d > 4.2 or dy + dx * 0.4 > 3.0:
                    v = 5
                if dx + dy < -3.0 and d < 4.4:
                    v = 6
                px[cx - 8 + x, cy - 8 + y] = v
    for x in (7, 8):
        px[cx - 8 + x, cy - 8 + 2] = 3
        px[cx - 8 + x, cy - 8 + 3] = 1
    px[cx - 8 + 8, cy - 8 + 1] = 1
    sx, sy = cx - 8 + 13, cy - 8 + 3
    if f in (1, 3):
        px[sx, sy] = 3
    if f == 2:
        for (ox, oy) in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
            px[sx + ox, sy + oy] = 3


def winter_frame(f):
    img = Image.new('P', (W, H), 0)
    px = img.load()
    snowflake(px, 8, 8, f * 15)
    bauble(px, 24, 8, f)
    return img


def build_set(kind):
    tiles = [Image.new('P', (8, 8), 0)]
    tiles += [{'halloween': belt_tile_candy, 'winter': belt_tile_cane}.get(kind, belt_tile)(k) for k in range(8)]
    for f in range(FRAMES):
        fr = {'halloween': pumpkin_frame, 'winter': winter_frame}.get(kind, cog_frame)(f)
        for r in range(2):
            for c in range(4):
                tiles.append(fr.crop((c * 8, r * 8, c * 8 + 8, r * 8 + 8)))
    for n in range(1, 16):
        tiles.append(Image.new('P', (8, 8), n))
    tiles += web_tiles(False) + web_tiles(True)
    assert len(tiles) == 9 + FRAMES * 8 + 15 + 18
    return b''.join(encode(t) for t in tiles)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    os.makedirs(os.path.join(root, 'assets'), exist_ok=True)
    base, hal, win = build_set('base'), build_set('halloween'), build_set('winter')
    import smallfont
    small = smallfont.tiles_bytes(encode)
    # Game of Life tiles (player 22): one tile holds a 2 x 2 block of cells, each cell 4 x 4 pixels drawn as a rounded dot, in one of five
    # states (0 dead, 1 fading trail, 2 newborn, 3 young, 4 old = colour index 0..4). Tile LIFE0 + s0 + 5 s1 + 25 s2 + 125 s3 (s0 top left,
    # s1 top right, s2 bottom left, s3 bottom right). They start at tile 1566, after the Halloween (1418) and Winter Workshop (1492) fix sets
    # the Foundry writes into the zero gap, so the gap is padded here.
    DOT = ['.XX.', 'XXXX', 'XXXX', '.XX.']
    life = bytearray()
    for n in range(625):
        st = [(n // 5 ** k) % 5 for k in range(4)]
        img = Image.new('P', (8, 8), 0)
        for k, v in enumerate(st):
            if not v: continue
            ox, oy = (k % 2) * 4, (k // 2) * 4
            for yy in range(4):
                for xx in range(4):
                    if DOT[yy][xx] == 'X': img.putpixel((ox + xx, oy + yy), v)
        life += encode(img)
    gap = bytes(32 * (1566 - 1280 - len(base) // 32 - len(small) // 32))
    open(os.path.join(root, 'assets', 'cogs.fix'), 'wb').write(base + small + gap + bytes(life))        # 74 tiles of ours, 64 small-font tiles (SMALL0 = 1354), the zero gap, then the 625 life tiles at 1566
    import base64, json
    json.dump({'base': base64.b64encode(base).decode(), 'halloween': base64.b64encode(hal).decode(), 'winter': base64.b64encode(win).decode(), 'tiles': len(base) // 32, 'small': base64.b64encode(small).decode()},
              open(os.path.join(root, 'assets', 'fixsets.json'), 'w'))
    print(f'{len(base) // 32} tiles + {len(small) // 32} small font tiles -> assets/cogs.fix (+ fixsets.json)')

    # preview: the four cog frames, enlarged, in the palette colours main.c uses
    pal = {0: (33, 16, 0), 1: (221, 170, 68), 2: (68, 34, 0), 3: (255, 221, 136),
           4: (187, 102, 51), 5: (51, 17, 0), 6: (255, 170, 119)}
    scale = 10
    prev = Image.new('RGB', (W * scale * 2 + 30, H * scale * 2 + 30), (33, 16, 0))
    for f in range(FRAMES):
        fr = cog_frame(f)
        big = Image.new('RGB', (W * scale, H * scale))
        for y in range(H):
            for x in range(W):
                c = pal[fr.getpixel((x, y))]
                for yy in range(scale):
                    for xx in range(scale):
                        big.putpixel((x * scale + xx, y * scale + yy), c)
        prev.paste(big, (10 + (f % 2) * (W * scale + 10), 10 + (f // 2) * (H * scale + 10)))
    prev.save(os.path.join(here, 'tiles-preview.png'))


if __name__ == '__main__':
    sys.exit(main())
