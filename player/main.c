/*
 * Neo Geo Media Centre: 68k player (template)
 *
 * The Foundry (browser app) patches two tables into the built P ROM, so this
 * file never needs to change when the music or the look changes:
 *   ngmc_table  the track list      ("NGMC-PTAB-01")
 *   ngmc_theme  colours, title, tempo and screensaver ("NGMC-THEM-01")
 * Built on the ngdevkit ADPCM-B example (LGPL, Damien Ciabrini).
 *
 * Copyright (c) 2026 bLiGhTy. Part of Neo Geo Media Centre.
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version. It is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; see the licence for details.
 *
 * Controls (player 1):
 *   Up/Down    choose a track
 *   A          play the chosen track
 *   B          stop
 *   C          repeat all on/off
 *   Left/Right previous / next track
 *   any button wakes the screensaver (and does nothing else)
 *
 * Extra graphics (chain belts, cogs, solid colour blocks) are tiles 1280 and
 * up of the fix ROM, made by tools/gen_tiles.py and appended by the Makefile.
 *
 * Life on the screen: turning cogs, two chain belts that slide in opposite
 * directions and back again, a white sweep along the progress bar, a glint
 * that crosses the title now and then, and long track names that scroll.
 * After a while without input a screensaver starts (see the SAVER_* list).
 */

#include <stdint.h>
#include <ngdevkit/neogeo.h>
#include <ngdevkit/bios-ram.h>
#include <ngdevkit/ng-fix.h>
#include <ngdevkit/ng-video.h>

extern u8 bios_p1current;
extern u8 bios_p1change;
#define REG_STATCHANGE (*(volatile uint8_t *)0x10FDAD)   /* BIOS: buttons pressed this frame: bit 0 P1 Start, 1 P1 Select, 2 P2 Start, 3 P2 Select */
#define START_HIT()    (REG_STATCHANGE & 5)

/* BIOS controller bits */
#define BTN_UP    0x01
#define BTN_DOWN  0x02
#define BTN_LEFT  0x04
#define BTN_RIGHT 0x08
#define BTN_A     0x10
#define BTN_B     0x20
#define BTN_C     0x40
#define BTN_D     0x80

/* z80 sound driver commands (see user_commands.s) */
#define REG_SOUND ((volatile uint8_t*)0x320000)
#define CMD_RESET 3
#define CMD_STOP  4
#define CMD_PLAY0 5          /* command CMD_PLAY0 + n plays track n */

/* ---- track table, patched into the ROM by the Foundry ---------------
 * Big-endian. Must stay in step with MAX_TRACKS in user_commands.s and
 * the Foundry. 'volatile' stops the compiler folding the empty template
 * values into the code; the data has to be read from ROM at run time. */
#define MAX_TRACKS 64
#define NAME_LEN   32

typedef struct {
    char     name[NAME_LEN];   /* upper-case ASCII, NUL padded       */
    uint32_t frames;           /* length in video frames (59.19 Hz)  */
    uint16_t seconds;          /* length in whole seconds, display   */
    uint16_t reserved;
} track_t;

typedef struct {
    char     magic[12];        /* "NGMC-PTAB-01", found by the Foundry */
    uint16_t count;
    uint16_t version;
    track_t  track[MAX_TRACKS];
} table_t;

const volatile table_t ngmc_table __attribute__((used)) = {
    { 'N','G','M','C','-','P','T','A','B','-','0','1' },
    0, 1,
    { { { 0 }, 0, 0, 0 } }
};

static const volatile table_t * const T = &ngmc_table;

#include "xdata.h"       /* extra sprites: made by tools/gen_sprites.py */
#include "spin_data.h"       /* gold lettering frames: made by tools/gen_spin.py */
#include "title_data.h"      /* title page art: made by tools/gen_title.py */
#include "skins_data.h"      /* MVS Cabinet, Neon Horizon and Aurora layouts: made by tools/gen_skins.py */

/* ---- theme table, also patched by the Foundry -------------------------
 * Big-endian. Offsets (the Foundry relies on them):
 *   0 magic, 12 version, 14 backdrop, 16 saver_mode, 17 cog_idle, 18 cog_play (bits 6-7: fix set),
 *   19 belt_idle, 20 belt_play, 21 pad, 22 saver_secs, 24 title[20],
 *   44 pal[11][8] (16-bit colours, 0x0RGB), 220 title_tile (version 2: first of the 20 sprite tiles
 *   holding the title letters; the Foundry fills them and sets bit 7 of saver_mode)
 * The values below are the original brass look, so an unpatched build looks
 * exactly as before and has the screensaver off. */
#define NPAL        11
#define TITLE_LEN   20

typedef struct {
    char     magic[12];        /* "NGMC-THEM-01" */
    uint16_t version;
    uint16_t backdrop;
    uint8_t  saver_mode;       /* SAVER_*                                   */
    uint8_t  cog_idle;         /* frames per cog step while stopped          */
    uint8_t  cog_play;         /* ... and while playing                      */
    uint8_t  belt_idle;        /* frames per belt step while stopped         */
    uint8_t  belt_play;        /* ... and while playing                      */
    uint8_t  extras;           /* 1: extras may appear */
    uint16_t saver_secs;       /* idle seconds before the screensaver starts */
    char     title[TITLE_LEN]; /* big title, already centred with spaces     */
    uint16_t pal[NPAL][8];
    uint16_t title_tile;       /* version 2: C-ROM tile of the first title letter */
} theme_t;

const volatile theme_t ngmc_theme __attribute__((used)) = {
    { 'N','G','M','C','-','T','H','E','M','-','0','1' },
    3, 0x0210,
    0, 9, 3, 4, 2, 1,
    120,
    { 'N','E','O',' ','G','E','O',' ','M','E','D','I','A',' ','C','E','N','T','R','E' },
    {
        /* 0 parchment */ { 0x8000, 0x0eda, 0x0210 },
        /* 1 brass     */ { 0x8000, 0x0da4, 0x0420, 0x0fd8 },
        /* 2 amber     */ { 0x8000, 0x0fb4, 0x0630, 0x0ffd },
        /* 3 verdigris */ { 0x8000, 0x05ba, 0x0132 },
        /* 4 dim       */ { 0x8000, 0x0875, 0x0110 },
        /* 5 copper    */ { 0x8000, 0x0b63, 0x0310 },
        /* 6 rust      */ { 0x8000, 0x0e53, 0x0210, 0x0fa8 },
        /* 7 cogs      */ { 0x8000, 0x0da4, 0x0420, 0x0fd8, 0x0b63, 0x0310, 0x0fa7 },
        /* 8 white     */ { 0x8000, 0x0fff, 0x0132 },
        /* 9 pale teal */ { 0x8000, 0x09ed, 0x0132 },
        /* 10 glint    */ { 0x8000, 0x0ffc, 0x0420 }
    },
    TTL_TILE0
};

/* Extra looks. Every cartridge can carry up to 11 more complete themes (the other skins, all twelve in all) next to ngmc_theme, plus a list
 * of names. The options page switches between them while the machine runs. The Foundry fills these in (it finds each block by its
 * magic text); an unpatched build has a list of one, so no picker shows. Index 0 is ngmc_theme, 1.. are ngmc_alt[0..]. */
#define N_ALT 11
#define ALTM(c) { { 'N','G','M','C','-','A','L','T',c,'-','0','1' } }
const volatile theme_t ngmc_alt[N_ALT] __attribute__((used)) = {
    ALTM('0'), ALTM('1'), ALTM('2'), ALTM('3'), ALTM('4'), ALTM('5'), ALTM('6'), ALTM('7'), ALTM('8'), ALTM('9'), ALTM('A')
};
#define SKIN_NAME_LEN 16
typedef struct {
    char    magic[12];                         /* "NGMC-SKINS-1" */
    uint8_t count;                             /* looks available, 1..12 */
    uint8_t start;                             /* the look the cartridge starts on */
    uint8_t pad[2];
    char    name[N_ALT + 1][SKIN_NAME_LEN];    /* space padded */
} skinlist_t;
const volatile skinlist_t ngmc_skins __attribute__((used)) = {
    { 'N','G','M','C','-','S','K','I','N','S','-','1' }, 1, 0, { 0, 0 }, { { 0 } }
};

static const volatile theme_t *TH = &ngmc_theme;
static int cur_skin;


#define PAL_TEXT   0
#define PAL_BRASS  1
#define PAL_AMBER  2
#define PAL_GREEN  3
#define PAL_DIM    4
#define PAL_COPPER 5
#define PAL_RUST   6
#define PAL_COGS   7
#define PAL_WHITE  8
#define PAL_TEAL   9
#define PAL_GLINT  10

/* screensaver modes (theme.saver_mode; bit 7 means the title letters are in the C ROM) */
#define SKIN_ID   ((TH->extras >> 1) & 7)  /* theme byte 21 bits 1-3: the screen layout. 0 normal, 1 Gilded, 2.. the layouts in skins_data.h */
#define GILDED    (SKIN_ID == 1)           /* the title page art behind the player screen */
#define SKIN_NEW  (SKIN_ID >= SK_FIRST_ID && SKIN_ID < SK_FIRST_ID + SK_COUNT)
#define SKIN_ANY  (SKIN_ID != 0)           /* a layout with sprites of its own: no cogs, no extras, no ambient effects */
static void gl_build(void);
static void gl_pals(int lvl);
static int sm_on(void) { return SKIN_ID == 2; }
static void sk_build(void);
static void sk_tick(void);
static void sk_press(int ch);
static void sk_pals_all(void);
static void sk_pals_part(int k);
static int saver_anim;
#define SAVER_MODE (TH->saver_mode & 0x7f)
#define TITLE_SPRITES (TH->saver_mode & 0x80)
#define SAVER_OFF     0
#define SAVER_DIM     1   /* everything fades to a quarter brightness        */
#define SAVER_PARTIAL 2   /* all but the now-playing lines fade to 37%       */
#define SAVER_BOUNCE  3   /* the title drifts around and bounces off edges   */
#define SAVER_STARS   4   /* three layers of stars drifting left             */
#define SAVER_WARP    5   /* a starfield flying at you; sprites scaled by the hardware */
#define SAVER_LIFE    6   /* Conway's Game of Life, reseeded when it settles */
#define SAVER_RAIN    7   /* falling glyph trails                            */
#define SAVER_RANDOM  8   /* one of the animated ones, picked at each start  */
#define SAVER_LIGHTS  9   /* light show: glowing orbs tracing curves, ripples */
#define SAVER_GOLD    10  /* big gold NEOGEO letters flipping in a wave, shine sweeping across */

/* colour brightness is in eighths: 8 = as designed */
/* dim_t[level][value] = value * level / 8. A table is much quicker than multiplying on a 68000, and a fade rewrites
 * a thousand colours at a time: done slowly, the new colours reach the screen part way down a frame (grey bands). */
static const uint8_t dim_t[9][16] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3 },
    { 0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5 },
    { 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7 },
    { 0, 0, 1, 1, 2, 3, 3, 4, 5, 5, 6, 6, 7, 8, 8, 9 },
    { 0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11 },
    { 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 8, 9, 10, 11, 12, 13 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
};
static const uint8_t dim_b[9][256] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17},
    {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51, 48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51, 48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51, 48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51},
    {0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 16, 16, 16, 17, 17, 17, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 16, 16, 16, 17, 17, 17, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 16, 16, 16, 17, 17, 17, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 32, 32, 32, 33, 33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 32, 32, 32, 33, 33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 48, 48, 48, 49, 49, 49, 50, 50, 51, 51, 51, 52, 52, 52, 53, 53, 48, 48, 48, 49, 49, 49, 50, 50, 51, 51, 51, 52, 52, 52, 53, 53, 48, 48, 48, 49, 49, 49, 50, 50, 51, 51, 51, 52, 52, 52, 53, 53, 64, 64, 64, 65, 65, 65, 66, 66, 67, 67, 67, 68, 68, 68, 69, 69, 64, 64, 64, 65, 65, 65, 66, 66, 67, 67, 67, 68, 68, 68, 69, 69, 64, 64, 64, 65, 65, 65, 66, 66, 67, 67, 67, 68, 68, 68, 69, 69, 80, 80, 80, 81, 81, 81, 82, 82, 83, 83, 83, 84, 84, 84, 85, 85, 80, 80, 80, 81, 81, 81, 82, 82, 83, 83, 83, 84, 84, 84, 85, 85},
    {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 32, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 38, 39, 39, 32, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 38, 39, 39, 48, 48, 49, 49, 50, 50, 51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 48, 48, 49, 49, 50, 50, 51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 64, 64, 65, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 64, 64, 65, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 80, 80, 81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 80, 80, 81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 96, 96, 97, 97, 98, 98, 99, 99, 100, 100, 101, 101, 102, 102, 103, 103, 96, 96, 97, 97, 98, 98, 99, 99, 100, 100, 101, 101, 102, 102, 103, 103, 112, 112, 113, 113, 114, 114, 115, 115, 116, 116, 117, 117, 118, 118, 119, 119, 112, 112, 113, 113, 114, 114, 115, 115, 116, 116, 117, 117, 118, 118, 119, 119},
    {0, 0, 1, 1, 2, 3, 3, 4, 5, 5, 6, 6, 7, 8, 8, 9, 0, 0, 1, 1, 2, 3, 3, 4, 5, 5, 6, 6, 7, 8, 8, 9, 16, 16, 17, 17, 18, 19, 19, 20, 21, 21, 22, 22, 23, 24, 24, 25, 16, 16, 17, 17, 18, 19, 19, 20, 21, 21, 22, 22, 23, 24, 24, 25, 32, 32, 33, 33, 34, 35, 35, 36, 37, 37, 38, 38, 39, 40, 40, 41, 48, 48, 49, 49, 50, 51, 51, 52, 53, 53, 54, 54, 55, 56, 56, 57, 48, 48, 49, 49, 50, 51, 51, 52, 53, 53, 54, 54, 55, 56, 56, 57, 64, 64, 65, 65, 66, 67, 67, 68, 69, 69, 70, 70, 71, 72, 72, 73, 80, 80, 81, 81, 82, 83, 83, 84, 85, 85, 86, 86, 87, 88, 88, 89, 80, 80, 81, 81, 82, 83, 83, 84, 85, 85, 86, 86, 87, 88, 88, 89, 96, 96, 97, 97, 98, 99, 99, 100, 101, 101, 102, 102, 103, 104, 104, 105, 96, 96, 97, 97, 98, 99, 99, 100, 101, 101, 102, 102, 103, 104, 104, 105, 112, 112, 113, 113, 114, 115, 115, 116, 117, 117, 118, 118, 119, 120, 120, 121, 128, 128, 129, 129, 130, 131, 131, 132, 133, 133, 134, 134, 135, 136, 136, 137, 128, 128, 129, 129, 130, 131, 131, 132, 133, 133, 134, 134, 135, 136, 136, 137, 144, 144, 145, 145, 146, 147, 147, 148, 149, 149, 150, 150, 151, 152, 152, 153},
    {0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 16, 16, 17, 18, 19, 19, 20, 21, 22, 22, 23, 24, 25, 25, 26, 27, 32, 32, 33, 34, 35, 35, 36, 37, 38, 38, 39, 40, 41, 41, 42, 43, 48, 48, 49, 50, 51, 51, 52, 53, 54, 54, 55, 56, 57, 57, 58, 59, 48, 48, 49, 50, 51, 51, 52, 53, 54, 54, 55, 56, 57, 57, 58, 59, 64, 64, 65, 66, 67, 67, 68, 69, 70, 70, 71, 72, 73, 73, 74, 75, 80, 80, 81, 82, 83, 83, 84, 85, 86, 86, 87, 88, 89, 89, 90, 91, 96, 96, 97, 98, 99, 99, 100, 101, 102, 102, 103, 104, 105, 105, 106, 107, 96, 96, 97, 98, 99, 99, 100, 101, 102, 102, 103, 104, 105, 105, 106, 107, 112, 112, 113, 114, 115, 115, 116, 117, 118, 118, 119, 120, 121, 121, 122, 123, 128, 128, 129, 130, 131, 131, 132, 133, 134, 134, 135, 136, 137, 137, 138, 139, 144, 144, 145, 146, 147, 147, 148, 149, 150, 150, 151, 152, 153, 153, 154, 155, 144, 144, 145, 146, 147, 147, 148, 149, 150, 150, 151, 152, 153, 153, 154, 155, 160, 160, 161, 162, 163, 163, 164, 165, 166, 166, 167, 168, 169, 169, 170, 171, 176, 176, 177, 178, 179, 179, 180, 181, 182, 182, 183, 184, 185, 185, 186, 187},
    {0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 8, 9, 10, 11, 12, 13, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 8, 9, 10, 11, 12, 13, 16, 16, 17, 18, 19, 20, 21, 22, 23, 23, 24, 25, 26, 27, 28, 29, 32, 32, 33, 34, 35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 48, 48, 49, 50, 51, 52, 53, 54, 55, 55, 56, 57, 58, 59, 60, 61, 64, 64, 65, 66, 67, 68, 69, 70, 71, 71, 72, 73, 74, 75, 76, 77, 80, 80, 81, 82, 83, 84, 85, 86, 87, 87, 88, 89, 90, 91, 92, 93, 96, 96, 97, 98, 99, 100, 101, 102, 103, 103, 104, 105, 106, 107, 108, 109, 112, 112, 113, 114, 115, 116, 117, 118, 119, 119, 120, 121, 122, 123, 124, 125, 112, 112, 113, 114, 115, 116, 117, 118, 119, 119, 120, 121, 122, 123, 124, 125, 128, 128, 129, 130, 131, 132, 133, 134, 135, 135, 136, 137, 138, 139, 140, 141, 144, 144, 145, 146, 147, 148, 149, 150, 151, 151, 152, 153, 154, 155, 156, 157, 160, 160, 161, 162, 163, 164, 165, 166, 167, 167, 168, 169, 170, 171, 172, 173, 176, 176, 177, 178, 179, 180, 181, 182, 183, 183, 184, 185, 186, 187, 188, 189, 192, 192, 193, 194, 195, 196, 197, 198, 199, 199, 200, 201, 202, 203, 204, 205, 208, 208, 209, 210, 211, 212, 213, 214, 215, 215, 216, 217, 218, 219, 220, 221},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255},
};
static const uint16_t dim_h[9][16] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 256, 256, 256, 256, 256, 256, 256, 256},
    {0, 0, 0, 0, 256, 256, 256, 256, 512, 512, 512, 512, 768, 768, 768, 768},
    {0, 0, 0, 256, 256, 256, 512, 512, 768, 768, 768, 1024, 1024, 1024, 1280, 1280},
    {0, 0, 256, 256, 512, 512, 768, 768, 1024, 1024, 1280, 1280, 1536, 1536, 1792, 1792},
    {0, 0, 256, 256, 512, 768, 768, 1024, 1280, 1280, 1536, 1536, 1792, 2048, 2048, 2304},
    {0, 0, 256, 512, 768, 768, 1024, 1280, 1536, 1536, 1792, 2048, 2304, 2304, 2560, 2816},
    {0, 0, 256, 512, 768, 1024, 1280, 1536, 1792, 1792, 2048, 2304, 2560, 2816, 3072, 3328},
    {0, 256, 512, 768, 1024, 1280, 1536, 1792, 2048, 2304, 2560, 2816, 3072, 3328, 3584, 3840},
};
/* Same result as looking up the three colour nibbles one by one, but with one lookup for the low byte (green and blue)
 * and one for the red nibble: this runs hundreds of times per frame during a fade, and on the real 68000 the older
 * version was too slow to finish inside the gap between frames. Full brightness is a plain copy. */
static uint16_t dim_word(uint16_t w, int lvl) {
    if (lvl >= 8) { return w & 0x8fff; }
    if (lvl <= 0) { return w & 0x8000; }
    return (uint16_t)((w & 0x8000) | dim_h[lvl][(w >> 8) & 15] | dim_b[lvl][w & 0xff]);
}

static int fade_lvl, fade_tgt, fade_tick;
static uint16_t fade_keep;      /* bit n set: palette n is not dimmed */

/* A fade step rewrites hundreds of colours. The palette RAM is only safe to write in the short gap between frames
 * (writing it while the picture is being drawn shows as grey noise on a real console, though never in MAME), so the
 * colours are written a few palettes at a time: at most PAL_SLICE palettes (16 colours each) in one frame, each piece
 * right after the frame starts. The main loop writes one piece per frame. apply_palettes() writes them all at once and
 * is only used while the screen is dark. */
#define PAL_SLICE 8
static int pal_part = 99;            /* next piece the fade still has to write; 99 = none */
static int pal_main_n(void) { return (SKIN_NEW || GILDED) ? NPAL : NPAL + N_AMBPAL + (N_GX - 1); }
static int pal_main_parts(void) { return (pal_main_n() + PAL_SLICE - 1) / PAL_SLICE; }
static void pal_main_unit(int u) {
    if (u < NPAL) {
        int lvl = ((fade_keep >> u) & 1) ? 8 : fade_lvl;
        for (int c = 0; c < 16; c++) {
            uint16_t w = (c < 8) ? TH->pal[u][c] : 0;
            MMAP_PALBANK1[u * 16 + c] = dim_word(w, lvl);
        }
    } else if (u < NPAL + N_AMBPAL) {                            /* ambient particle palettes */
        int p = u - NPAL;
        for (int c = 0; c < 16; c++) { MMAP_PALBANK1[(AMB_PAL0 + p) * 16 + c] = dim_word(amb_pal[p][c], fade_lvl); }
    } else {                                                      /* extra sprite palettes follow the fade */
        int e = u - NPAL - N_AMBPAL + 1;
        for (int c = 0; c < 16; c++) { MMAP_PALBANK1[(16 + e) * 16 + c] = dim_word(gx_pal[e][c], fade_lvl); }
    }
}
#define GL_NPAL 60                    /* Gilded: 56 backdrop palettes, gears, gold, two star palettes */
static int pal_gl_parts(void) { return (GILDED && !saver_anim) ? (GL_NPAL + PAL_SLICE - 1) / PAL_SLICE : 0; }
static void gl_pals_part(int k);
static int sk_slices(void);
static void apply_palettes_part(int part) {
    int m = pal_main_parts();
    if (part < m) {
        int a = part * PAL_SLICE, b = a + PAL_SLICE, n = pal_main_n();
        if (b > n) { b = n; }
        for (int u = a; u < b; u++) { pal_main_unit(u); }
        if (part == 0) { *((volatile uint16_t *)0x401ffe) = dim_word(TH->backdrop, fade_lvl); }
        return;
    }
    part -= m;
    if (SKIN_NEW) { sk_pals_part(part); return; }
    if (GILDED && !saver_anim) { gl_pals_part(part); }
}
static int pal_total_parts(void) { return pal_main_parts() + (SKIN_NEW ? sk_slices() : pal_gl_parts()); }
static int fade_per(void) { int n = pal_total_parts(); return n < 3 ? 3 : n; }     /* frames per fade step: one piece of the palette each */
static void apply_palettes(void) {
    for (int part = 0, n = pal_total_parts(); part < n; part++) { apply_palettes_part(part); }
    pal_part = 99;
}
static void play_tick(void);
/* the same, but a piece per screen frame, for when the screen is lit (the music keeps being served) */
static void pal_sweep(void) {
    for (int part = 0, n = pal_total_parts(); part < n; part++) {
        apply_palettes_part(part);
        play_tick();
        ng_wait_vblank();
    }
    pal_part = 99;
}

/* ---- layout ----------------------------------------------------------
 * ng_text() rows are raw fix-map rows. Rows 0-1 and 30-31 are off screen,
 * so everything starts at row 3 (the first version used row 1, which put
 * the top half of the title off screen). Columns 0..39 are all visible. */
#define LIST_X     2
#define ROW_TITLE  3      /* tall text: rows 3-4, with a cog pair either side */
#define ROW_SEP1   6
#define ROW_HEAD   (SKIN_ID >= 3 ? 8 : 7)       /* Neon Horizon and Aurora: the text panel starts a row lower, so the list has 9 rows */
#define LIST_TOP   (SKIN_ID >= 3 ? 9 : 8)
#define LIST_ROWS  (SKIN_ID >= 3 ? 9 : 10)    /* rows 8-17 (9-17 on those two) */
#define ROW_SEP2   18
#define ROW_NOWL   22
#define ROW_NAME   23
#define ROW_TIME   24
#define ROW_BAR    25
#define ROW_HELP1  27
#define ROW_HELP2  28
#define BAR_LEN    34
#define FRAME_US   16896  /* one frame at 59.1856 Hz, microseconds */
#define ROW_FIRST  2      /* first and last visible fix rows */
#define ROW_LAST   29
#define SCR_W      40
#define SCR_H      28     /* visible rows */

/* ---- raw fix-layer tiles (belts, cogs, solid blocks) ------------------ */
#define NGMC_VADDR (*(volatile uint16_t *)0x3c0000)
#define NGMC_VDATA (*(volatile uint16_t *)0x3c0002)
#define FIXMAP     0x7000                 /* fix map: column-major, 32 rows per column */
#define TILE_BASE  1280                   /* first tile from assets/cogs.fix: blank */
#define TILE_BELT0 (TILE_BASE + 1)        /* 8 belt tiles, each shifted one more pixel */
#define TILE_COG0  (TILE_BASE + 9)        /* 4 frames x (2 rows x 4 columns) */
#define TILE_SOLID (TILE_BASE + 41)       /* 15 solid blocks: colour index 1..15 */
#define TILE_WEB_L (TILE_BASE + 56)       /* cobweb, 3 x 3 tiles, top-left corner */
#define TILE_WEB_R (TILE_BASE + 65)       /* ... and mirrored for the top-right corner */
/* Each look's own fix-layer art (belts, cogs, cobwebs): the 74-tile sets sit one after another after the small font, so
 * every look can show its own. Which set a look uses is in bits 6-7 of its cog_play byte (theme version 3):
 * 0 the standard set at 1280, 1 Halloween's, 2 Winter Workshop's. The blank tile, solid blocks and small font are shared. */
#define FX_HAL     138                    /* tile offset of Halloween's set (1418..) */
#define FX_WIN     212                    /* tile offset of Winter Workshop's set (1492..) */
static int fxo(void) { int s = (TH->cog_play >> 6) & 3; return s == 1 ? FX_HAL : (s == 2 ? FX_WIN : 0); }
#define COGS_LEFT  5
#define COGS_RIGHT 31
#define TITLE_X    10     /* 20 letters, centred on 40 columns */
#define BELT_X     1      /* belts run columns 1..38: centred on 40 and as wide as the text (columns 2..38) */
#define BELT_LEN   38
#define BELT_TRAVEL 24    /* pixels each way before the belts reverse */
#define SWEEP_PERIOD 150  /* frames between white sweeps on the progress bar */
#define SWEEP_LEN    20   /* frames a sweep takes to cross the bar */
#define GLINT_PERIOD 420  /* frames between glints on the title */
#define GLINT_STEP   3    /* frames per letter */
#define LIST_W       ((SKIN_ID >= 2) ? 36 : 37)   /* characters in a playlist row (the layouts with a text panel have a little less room) */
#define NAME_W       (LIST_W - 11)         /* width of a name in the list */

static void put_tile(int x, int y, int pal, int tile) {
    /* one tile per call: set the address, write once. The VRAM modulo register
     * is left alone, because ng_text() may depend on how it is set. */
    NGMC_VADDR = FIXMAP + (x << 5) + y;
    NGMC_VDATA = (uint16_t)((pal << 12) | tile);
}

/* the small 5 x 5 font (tiles SMALL0.. for ASCII 32..95, made by tools/smallfont.py) is used by the MVS Cabinet for the
 * now-playing box and the help strip; every other layout keeps the normal text */
#define SMALL0 1354
static int sm_tile(int c) {
    if (c >= 'a' && c <= 'z') { c -= 32; }
    return (c >= 32 && c <= 95) ? SMALL0 + c - 32 : SMALL0;
}
static void st_text(int x, int y, int pal, const char *s) {
    if (!sm_on()) { ng_text(x, y, pal, s); return; }
    for (; *s; s++, x++) { put_tile(x, y, pal, sm_tile((uint8_t)*s)); }
}
static void st_center(int y, int pal, const char *s) {
    int n = 0;
    while (s[n]) { n++; }
    st_text((40 - n) >> 1, y, pal, s);
}

/* ---- state, all set up in main() ------------------------------------ */
static int count, sel, top, playing, repeat_all, shuffle, shuf_n, name_rev;
static uint32_t elapsed_frames, sec_acc, bar_step, bar_acc;
static uint16_t elapsed_sec;
static int bar_fill;
static int dirty_list, dirty_now, dirty_bar;
static int cog_frame, cog_tick;
static int belt_pos, belt_dir, belt_hold, belt_tick;
static int sweep_t, glint_t, glint_drawn;
static int mq_t, mq_sel, mq_off;
static uint16_t rng, tick;
static uint32_t idle;
static int saver_on, saver_anim, saver_cur;
static int opt_saver, opt_secs;          /* the screensaver and delay in use: the theme's, until the options page changes them */

/* ---- extras: hardware sprites ----------------------------------------------------
 * Art lives in the C ROM (assets/x.c1/.c2 and xdata.h, both made by tools/gen_sprites.py).
 * A extra is a row of sprites (one per 16 pixel column, two tiles tall). While a track plays whose name
 * contains one of the extra's trigger words, the player rolls a die now and then; a win sends that extra
 * across the screen once. Extra 0 uses the skin's palette 7, the others have their own palette slots.
 * Every VRAM write below sets its own address, so nothing depends on the modulo register. */
#define SCB1       0x0000                 /* per sprite: 32 tiles of (tile number, attributes) */
#define SCB2       0x8000                 /* shrink */
#define SCB3       0x8200                 /* y position and height in tiles */
#define SCB4       0x8400                 /* x position */
#define N_SPRITES  381
#define SPR0       4                      /* first sprite used */
#define MAX_COLS   6
#define BAND_Y     131                    /* top of a sprite in the empty band above NOW PLAYING */
#ifndef GX_ROLL_FRAMES
#define GX_ROLL_FRAMES 1182              /* a roll every 20 seconds ... */
#endif
#ifndef GX_CHANCE
#define GX_CHANCE      6                 /* ... with this many percent chance of a visit */
#endif
#define TEXT_ROW   20
static const char gx_text[] = "\x25\x2e\x3d\x2e\x39\x4b\x2c\x24\x25\x25\x2a\x4b\x2c\x22\x3d\x2e\x4b\x32\x24\x3e\x4b\x3e\x3b\x4b\x4b\x4b\x25\x2e\x3d\x2e\x39\x4b\x2c\x24\x25\x25\x2a\x4b\x27\x2e\x3f\x4b\x32\x24\x3e\x4b\x2f\x24\x3c\x25";   /* scrambled: see xu() */
static const int8_t bob_tab[16] = { 0, 1, 1, 2, 2, 2, 1, 1, 0, -1, -1, -2, -2, -2, -1, -1 };

/* the options extra page (options page): one row per extra. 0..8 the extras, 9 the banner, 10..14 the ambient effects.
 * A extra is "found" once it has shown up since power-on; only found ones can be hidden. RAM only. */
#define PB_N 15
static uint8_t pb_found[PB_N], pb_hidden[PB_N];
static const char * const pb_names[PB_N] = {
    "\x31\x2e\x3b\x3b\x2e\x27\x22\x25",
    "\x3b\x39\x22\x38\x26",
    "\x2f\x22\x38\x28\x24\x4b\x29\x2a\x27\x27",
    "\x28\x39\x24\x3c\x25",
    "\x39\x24\x28\x20\x2e\x3f",
    "\x27\x22\x2c\x23\x3f\x25\x22\x25\x2c\x4b\x29\x24\x27\x3f",
    "\x38\x3e\x29\x26\x2a\x39\x22\x25\x2e",
    "\x39\x24\x29\x24\x3f",
    "\x29\x2a\x3f",
    "\x29\x2a\x25\x25\x2e\x39",
    "\x38\x25\x24\x3c",
    "\x2e\x2c\x2c\x4b\x23\x3e\x25\x3f",
    "\x38\x3b\x24\x24\x20\x32\x4b\x25\x22\x2c\x23\x3f",
    "\x28\x24\x25\x2d\x2e\x3f\x3f\x22",
    "\x23\x2e\x2a\x39\x3f\x38" };   /* scrambled: see xu() */

static int gx_cur;            /* extra on stage, or -1 */
static int gx_x, gx_y, gx_tm, gx_frame, gx_ftick, gx_roll, gx_last, gx_hold, gx_flash;
static int gx_shown;

static uint16_t rnd(void);

static void vwrite(uint16_t addr, uint16_t v) {
    NGMC_VADDR = addr;
    NGMC_VDATA = v;
}

/* A sprite that is shrunk vertically also draws the tiles below its first one from the sprite's tile list, whatever
 * height it was given. One-tile sprites that get shrunk (stars, orbs) therefore need the rest of their list to be
 * empty tiles, or leftovers from earlier sprites show underneath them. */
static void blank_below(int spr) {
    for (int r = 1; r < 32; r++) {
        vwrite(SCB1 + spr * 64 + r * 2, TT_BLANK);
        vwrite(SCB1 + spr * 64 + r * 2 + 1, 0);
    }
}

static void gx_hide(void) {
    for (int c = 0; c < MAX_COLS; c++) { vwrite(SCB3 + SPR0 + c, 0); }       /* height 0: not drawn */
    gx_shown = 0;
}

static void gx_tiles(void) {
    const gx_def_t *g = &gxs[gx_cur];
    for (int c = 0; c < g->cols; c++) {
        for (int r = 0; r < 2; r++) {
            int a = (SPR0 + c) * 64 + r * 2;
            vwrite(SCB1 + a,     (uint16_t)(g->tile0 + (gx_frame * g->cols + c) * 2 + r));
            vwrite(SCB1 + a + 1, (uint16_t)(g->pal << 8));                    /* palette, no flip */
        }
    }
}

static void gx_draw(void) {
    const gx_def_t *g = &gxs[gx_cur];
    uint16_t yreg = (uint16_t)(((496 - gx_y) & 0x1ff) << 7) | 2;            /* two tiles high */
    for (int c = 0; c < g->cols; c++) {
        vwrite(SCB3 + SPR0 + c, yreg);
        vwrite(SCB4 + SPR0 + c, (uint16_t)(((gx_x + 16 * c) & 0x1ff) << 7));
    }
    gx_shown = 1;
}

static void gx_init(void) {
    /* the BIOS leaves sprite memory in an unknown state: switch every sprite off first */
    for (int n = 0; n < N_SPRITES; n++) { vwrite(SCB3 + n, 0); }
    for (int c = 0; c < MAX_COLS; c++) { vwrite(SCB2 + SPR0 + c, 0x0fff); }  /* full size */
    gx_cur = -1;
    gx_shown = 0;
    gx_roll = 0;
    gx_last = -1;
    gx_flash = 0;
    gx_hold = 0;
}

#define XKEY 0x6b                                   /* trigger words and a few labels are stored scrambled */
static char xbuf[2][40];
static int xn;
static const char *xu(const char *k) {
    char *o = xbuf[xn++ & 1]; int j = 0;
    for (; k[j] && j < 39; j++) { o[j] = (char)(k[j] ^ XKEY); }
    o[j] = 0; return o;
}

static int streq_at(const char *nm, const char *k) {
    int j = 0;
    while (k[j] && nm[j] == (char)(k[j] ^ XKEY)) { j++; }
    return !k[j];
}

/* which extra (or GX_TXT) does this track name call for? -1 for none */
static int gx_find(int track) {
    char nm[NAME_LEN + 1];
    for (int i = 0; i < NAME_LEN; i++) {
        char c = T->track[track].name[i];
        if (c >= 'a' && c <= 'z') { c -= 32; }
        nm[i] = c;
    }
    nm[NAME_LEN] = 0;
    for (unsigned w = 0; w < sizeof(gx_kw) / sizeof(gx_kw[0]); w++) {
        for (int i = 0; nm[i]; i++) {
            if (streq_at(nm + i, gx_kw[w].w)) { return gx_kw[w].gx; }
        }
    }
    return -1;
}

static void text_row(int offset) {                    /* the banner scrolls right to left along one row */
    int len = (int)sizeof(gx_text) - 1;
    for (int x = 0; x < SCR_W; x++) {
        int i = offset + x - SCR_W;
        put_tile(x, TEXT_ROW, PAL_AMBER, (i >= 0 && i < len) ? (unsigned char)(gx_text[i] ^ XKEY) : TILE_BASE);
    }
}

static void gx_cancel(void) {
    if (gx_cur == GX_TXT) { for (int x = 0; x < SCR_W; x++) { put_tile(x, TEXT_ROW, 0, TILE_BASE); } }
    if (gx_shown) { gx_hide(); }
    gx_cur = -1;
}

static void gx_start(int e) {
    gx_cur = e;
    pb_found[e] = 1;                                    /* (GX_TXT is 9, the banner's row) */
    gx_tm = 0; gx_frame = 0; gx_ftick = 0; gx_flash = 0; gx_hold = 0;
    if (e == GX_TXT) { return; }
    const gx_def_t *g = &gxs[e];
    int w = g->cols * 16;
    switch (g->motion) {
    case M_LR:    gx_x = -w;  gx_y = BAND_Y; break;
    case M_RL:    gx_x = 320; gx_y = BAND_Y; break;
    case M_UP:    gx_x = 24 + (int)(rnd() % (uint16_t)(272 - w)); gx_y = 224; break;
    case M_FLASH: gx_x = 24 + (int)(rnd() % (uint16_t)(272 - w)); gx_y = 96 + (int)(rnd() % 40); break;
    default:      gx_x = 40 + (int)(rnd() % (uint16_t)(240 - w)); gx_y = -32; break;   /* M_HANG */
    }
    gx_tiles();
    gx_draw();
}

static void gx_update(void) {
    if (!(TH->extras & 1) || saver_anim) {
        if (gx_cur >= 0) { gx_cancel(); }
        return;
    }
    if (gx_cur < 0) {
        /* waiting in the wings: roll the die while a matching track plays */
        if (playing != gx_last) {
            gx_last = playing;
            gx_roll = GX_ROLL_FRAMES - 300;              /* first roll five seconds in */
        }
        if (playing >= 0 && ++gx_roll >= GX_ROLL_FRAMES) {
            gx_roll = 0;
            int e = gx_find(playing);
            if (e >= 0 && !pb_hidden[e] && (int)(rnd() % 100) < GX_CHANCE) { gx_start(e); }
        }
        if (gx_cur < 0) { return; }
    }
    if (gx_cur == GX_TXT) {
        gx_tm++;
        text_row(gx_tm >> 2);
        if ((gx_tm >> 2) > (int)sizeof(gx_text) + SCR_W) { gx_cancel(); }
        return;
    }
    const gx_def_t *g = &gxs[gx_cur];
    int w = g->cols * 16, done = 0;
    gx_tm++;
    switch (g->motion) {
    case M_LR:    gx_x++; gx_y = BAND_Y + (g->bob ? (g->bob * bob_tab[(gx_tm >> 3) & 15]) / 2 : 0); done = gx_x > 320; break;
    case M_RL:    gx_x--; gx_y = BAND_Y + (g->bob ? (g->bob * bob_tab[(gx_tm >> 3) & 15]) / 2 : 0); done = gx_x < -w; break;
    case M_UP:    gx_y -= 2; done = gx_y < -32; break;
    case M_FLASH: {
        int ph = gx_tm % 30;
        if (ph == 0) { gx_flash = 3; }
        if (ph < 14) { gx_draw(); } else { gx_hide(); }
        done = gx_tm >= 120;
        break;
    }
    default:                                               /* M_HANG: lower from the ceiling, sway, rise again */
        if (gx_hold == 0) { if ((gx_tm & 1) == 0) { gx_y++; } if (gx_y >= 30) { gx_hold = 1; gx_tm = 0; } }
        else if (gx_hold == 1) { if (gx_tm > 420) { gx_hold = 2; } }
        else { if ((gx_tm & 1) == 0) { gx_y--; } done = gx_y < -32; }
        break;
    }
    if (done) { gx_cancel(); return; }

    if (g->frames > 1 && ++gx_ftick >= g->ftick) {
        gx_ftick = 0;
        gx_frame = (gx_frame + 1) % g->frames;
        gx_tiles();
    }
    if (g->motion != M_FLASH) { gx_draw(); }
}


/* ---- ambient effects: particle sprites, picked by the screen title (or forced by the Foundry) -------
 * Theme byte 21: bit 0 = extras on, bits 4-7 = effect: 0 decide from the title, 1 none, 2.. effect number + 2. */
#define AMB_SPR0   12
#define MAX_PART   18
static int amb_mode;                       /* 0 none, else effect index + 1 */
static int amb_x0[MAX_PART], amb_y[MAX_PART], amb_ph[MAX_PART];
static int amb_flap, amb_lt, amb_shown;

static void flash_update(void) {           /* lightning: the backdrop flashes white */
    if (gx_flash > 0) {
        gx_flash--;
        *((volatile uint16_t *)0x401ffe) = gx_flash ? 0x0fff : dim_word(TH->backdrop, fade_lvl);
    }
}

static int amb_from_title(void) {
    char nm[TITLE_LEN + 1];
    for (int i = 0; i < TITLE_LEN; i++) {
        char c = TH->title[i];
        if (c >= 'a' && c <= 'z') { c -= 32; }
        nm[i] = c;
    }
    nm[TITLE_LEN] = 0;
    for (unsigned w = 0; w < sizeof(amb_kw) / sizeof(amb_kw[0]); w++) {
        for (int i = 0; nm[i]; i++) {
            if (streq_at(nm + i, amb_kw[w].w)) { return amb_kw[w].effect + 1; }
        }
    }
    return 0;
}

static void amb_resolve(void) {
    int sel = TH->extras >> 4;
    if (sel == 0)      { amb_mode = amb_from_title(); }
    else if (sel == 1) { amb_mode = 0; }
    else               { amb_mode = sel - 1; if (amb_mode > N_AMB) { amb_mode = 0; } }
}

static void skin_pick(int i) {                    /* switch to look i (0 = the cartridge's own theme); the caller redraws */
    cur_skin = i;
    TH = (i == 0) ? &ngmc_theme : &ngmc_alt[i - 1];
    amb_resolve();
}

static int amb_on(void) { return amb_mode && !pb_hidden[10 + amb_mode - 1]; }

static void amb_hide(void) {
    if (amb_mode) { for (int i = 0; i < amb_defs[amb_mode - 1].count; i++) { vwrite(SCB3 + AMB_SPR0 + i, 0); } }
    amb_shown = 0;
}

static void amb_tile_write(int i, int tile_index) {
    const amb_def_t *d = &amb_defs[amb_mode - 1];
    vwrite(SCB1 + (AMB_SPR0 + i) * 64,     (uint16_t)(AMB_TILE0 + d->tile + tile_index));
    vwrite(SCB1 + (AMB_SPR0 + i) * 64 + 1, (uint16_t)((AMB_PAL0 + d->pal + (i % d->npal)) << 8));
}

static void amb_init(void) {
    amb_flap = 0;
    amb_shown = 0;
    amb_lt = 300 + (int)(rnd() % 600);
    if (!amb_mode) { return; }
    pb_found[10 + amb_mode - 1] = 1;
    const amb_def_t *d = &amb_defs[amb_mode - 1];
    for (int i = 0; i < d->count; i++) {
        amb_x0[i] = (int)(rnd() % 304);
        amb_ph[i] = (int)(rnd() & 255);
        amb_y[i] = (d->motion == AM_FLY) ? 24 + (int)(rnd() % 150) : (int)(rnd() % 240) - 16;
        if (d->motion == AM_FLY) { amb_x0[i] = (int)(rnd() % 400) - 40; }
        amb_tile_write(i, i % d->ntile);
        vwrite(SCB2 + AMB_SPR0 + i, 0x0fff);
    }
}

static void draw_webs(void) {                         /* cobwebs in the two top corners */
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            put_tile(c,       ROW_FIRST + r, PAL_DIM, TILE_WEB_L + FX_HAL + r * 3 + c);
            put_tile(37 + c,  ROW_FIRST + r, PAL_DIM, TILE_WEB_R + FX_HAL + r * 3 + c);
        }
    }
}

static void amb_update(void) {
    if (!amb_on() || saver_anim) {
        if (amb_shown) { amb_hide(); }
        return;
    }
    const amb_def_t *d = &amb_defs[amb_mode - 1];
    amb_flap++;
    for (int i = 0; i < d->count; i++) {
        int x, y = amb_y[i];
        int step = d->speed + (i % 3);                  /* particles fall at slightly different speeds */
        switch (d->motion) {
        case AM_FALL:
            if (tick % step == 0) { amb_y[i]++; }
            if (amb_y[i] > 224) { amb_y[i] = -16; amb_x0[i] = (int)(rnd() % 304); }
            x = amb_x0[i] + 6 * bob_tab[((tick >> 3) + amb_ph[i]) & 15];
            y = amb_y[i];
            break;
        case AM_RISE:
            if (tick % step == 0) { amb_y[i]--; }
            if (amb_y[i] < -16) { amb_y[i] = 224; amb_x0[i] = (int)(rnd() % 304); }
            x = amb_x0[i] + 6 * bob_tab[((tick >> 3) + amb_ph[i]) & 15];
            y = amb_y[i];
            break;
        default:                                         /* AM_FLY: bats */
            if ((i & 1) || (tick & 1)) { amb_x0[i] += 1; }
            if ((i & 1) == 0 && (tick & 3) == 0) { amb_x0[i] += 1; }
            if (amb_x0[i] > 320) { amb_x0[i] = -16 - (int)(rnd() % 60); amb_y[i] = 24 + (int)(rnd() % 150); }
            x = amb_x0[i];
            y = amb_y[i] + 5 * bob_tab[((tick >> 2) + amb_ph[i]) & 15];
            if ((amb_flap % 6) == 0) { amb_tile_write(i, ((amb_flap / 6) + i) % d->ntile); }
            break;
        }
        vwrite(SCB3 + AMB_SPR0 + i, (uint16_t)(((496 - y) & 0x1ff) << 7) | 1);   /* one tile high */
        vwrite(SCB4 + AMB_SPR0 + i, (uint16_t)((x & 0x1ff) << 7));
    }
    amb_shown = 1;
    if (amb_mode == 3 && --amb_lt <= 0) {                /* spooky: now and then a flash of lightning */
        gx_flash = 3;
        amb_lt = 480 + (int)(rnd() % 900);
    }
}

static uint16_t rnd(void) {                /* xorshift16, never returns to 0 */
    rng ^= rng << 7;
    rng ^= rng >> 9;
    rng ^= rng << 8;
    return rng;
}

static void put2(char *p, unsigned v) {
    p[0] = '0' + (v / 10) % 10;
    p[1] = '0' + v % 10;
}

static void put_time(char *p, unsigned sec) {
    unsigned m = sec / 60, s = sec % 60;
    if (m > 99) { m = 99; }
    put2(p, m);
    p[2] = ':';
    put2(p + 3, s);
}

static void fill(char *p, char c, int n) {
    for (int i = 0; i < n; i++) { p[i] = c; }
}

/* copy a name out of the (volatile) table, space padded to w characters */
static void put_name(char *dst, const volatile track_t *t, int w, int off) {
    int ended = 0;
    for (int i = 0; i < w; i++) {
        char c = 0;
        if (i + off < NAME_LEN && !ended) { c = t->name[i + off]; }
        if (!c) { ended = 1; c = ' '; }
        dst[i] = c;
    }
}

/* ---- the cogs: a meshing pair either side of the title ---------------- */
static void draw_cogs_at(int row) {
    int right = (4 - cog_frame) & 3;       /* mirrored pair turns the other way */
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 4; c++) {
            put_tile(COGS_LEFT + c,  row + r, PAL_COGS, TILE_COG0 + fxo() + cog_frame * 8 + r * 4 + c);
            put_tile(COGS_RIGHT + c, row + r, PAL_COGS, TILE_COG0 + fxo() + right * 8 + r * 4 + c);
        }
    }
}
static void draw_cogs(void) { draw_cogs_at(ROW_TITLE); }

/* ---- the chain belts: the two '=' lines slide against each other ------ */
static void draw_belts(void) {
    if (SKIN_ID == 2) { return; }                /* the MVS Cabinet has no '=' belts */
    int f = belt_pos & 7;
    int bp = (TH->belt_idle & 0x80) ? PAL_RUST : PAL_BRASS;      /* bit 7 of belt_idle: the belts take the Rust colour (Winter Workshop's red candy belts) */
    int t1 = TILE_BELT0 + fxo() + f;                 /* top belt slides one way ...        */
    int t2 = TILE_BELT0 + fxo() + ((8 - f) & 7);     /* ... the lower one the other way    */
    for (int i = 0; i < BELT_LEN; i++) {
        put_tile(BELT_X + i, ROW_SEP1, bp, t1);        /* columns 1..38: centred on 40 */
        put_tile(BELT_X + i, ROW_SEP2, bp, t2);
    }
}

/* slide a little way, pause, slide back, pause, and so on for ever */
static void update_belts(void) {
    int period = (playing >= 0 ? TH->belt_play : TH->belt_idle) & 0x7f;
    if (period < 1) { period = 1; }
    if (belt_hold > 0) { belt_hold--; return; }
    if (++belt_tick < period) { return; }
    belt_tick = 0;
    belt_pos += belt_dir;
    if (belt_pos >= BELT_TRAVEL) { belt_pos = BELT_TRAVEL; belt_dir = -1; belt_hold = 30; }
    if (belt_pos <= 0)           { belt_pos = 0;           belt_dir = 1;  belt_hold = 30; }
    draw_belts();
}

/* ---- the title: a glint now and then crosses the letters -------------- */
static int title_char(int i) {
    int c = (uint8_t)TH->title[i];
    return (c < 32 || c > 126) ? 32 : c;
}

static void draw_title_at(int x, int y, int pal, int glint) {
    for (int i = 0; i < TITLE_LEN; i++) {
        int p = (glint >= 0 && (i == glint || i == glint - 1)) ? PAL_GLINT : pal;
        put_tile(x + i, y,     p, 256 + title_char(i));
        put_tile(x + i, y + 1, p, 512 + title_char(i));
    }
}

static void draw_title(int glint) {
    draw_title_at(TITLE_X, ROW_TITLE, PAL_BRASS, glint);
    glint_drawn = glint;
}

static void update_title(void) {
    int g = -1;
    if (++glint_t >= GLINT_PERIOD) { glint_t = 0; }
    if (glint_t < (TITLE_LEN + 2) * GLINT_STEP) { g = glint_t / GLINT_STEP; }
    if (g != glint_drawn) { draw_title(g); }
}

/* ---- text screens ----------------------------------------------------- */
static void draw_row(int idx, int row) {
    char b[38];
    int pal = PAL_TEXT, lw = LIST_W;
    fill(b, ' ', lw);
    b[lw] = 0;
    if (idx < count) {
        b[0] = (idx == sel) ? '>' : ' ';
        b[1] = (idx == playing) ? '*' : ' ';
        put2(b + 2, idx + 1);
        put_name(b + 5, &T->track[idx], NAME_W, (idx == sel) ? mq_off : 0);
        put_time(b + lw - 5, T->track[idx].seconds);
        pal = (idx == playing) ? PAL_AMBER : ((idx == sel) ? PAL_BRASS : PAL_TEXT);
    }
    ng_text(LIST_X, row, pal, b);
}

static void draw_list(void) {
    int lw = LIST_W;
    if (sel < top) { top = sel; }
    if (sel >= top + LIST_ROWS) { top = sel - LIST_ROWS + 1; }
    for (int r = 0; r < LIST_ROWS; r++) {
        draw_row(top + r, LIST_TOP + r);
    }
    char b[38];
    fill(b, ' ', lw);
    b[lw] = 0;
    b[0] = 'P'; b[1] = 'L'; b[2] = 'A'; b[3] = 'Y'; b[4] = 'L'; b[5] = 'I'; b[6] = 'S'; b[7] = 'T';
    if (count) {
        b[lw - 11] = 'T'; b[lw - 10] = 'R'; b[lw - 9] = 'A'; b[lw - 8] = 'C'; b[lw - 7] = 'K'; b[lw - 6] = ' ';
        put2(b + lw - 5, sel + 1);
        b[lw - 3] = '/';
        put2(b + lw - 2, count);
    }
    ng_text(LIST_X, ROW_HEAD, PAL_DIM, b);
}

static void draw_now(void) {
    char b[38];

    fill(b, ' ', 37);
    b[37] = 0;
    if (playing >= 0) {
        put_name(b, &T->track[playing], 32, 0);
        for (int i = name_rev; i < 32; i++) { b[i] = ' '; }        /* a new name is typed in, letter by letter */
    } else {
        b[0] = '-';
    }
    st_text(LIST_X, ROW_NAME, PAL_AMBER, b);
    if (playing >= 0 && name_rev > 0 && name_rev <= 32 && b[name_rev - 1] != ' ') {
        put_tile(LIST_X + name_rev - 1, ROW_NAME, PAL_WHITE, sm_on() ? sm_tile((uint8_t)b[name_rev - 1]) : (uint8_t)b[name_rev - 1]);     /* newest letter shines */
    }

    fill(b, ' ', 37);
    b[37] = 0;
    if (playing >= 0) {
        put_time(b, elapsed_sec);
        b[5] = ' '; b[6] = '/'; b[7] = ' ';
        put_time(b + 8, T->track[playing].seconds);
    } else {
        put_time(b, 0);
    }
    if (shuffle) { b[17] = 'S'; b[18] = 'H'; b[19] = 'U'; b[20] = 'F'; b[21] = 'F'; b[22] = 'L'; b[23] = 'E'; }
    b[25] = 'R'; b[26] = 'E'; b[27] = 'P'; b[28] = 'E'; b[29] = 'A'; b[30] = 'T'; b[31] = ' ';
    if (repeat_all) { b[32] = 'A'; b[33] = 'L'; b[34] = 'L'; }
    else            { b[32] = 'O'; b[33] = 'F'; b[34] = 'F'; }
    st_text(LIST_X, ROW_TIME, PAL_GREEN, b);
}

/* progress bar: '=' for time gone, '-' for time to come. A white sweep with a
 * short pale tail runs along the filled part every few seconds. */
static void draw_progress(void) {
    int head = -9;
    if (playing >= 0 && bar_fill > 0 && sweep_t < SWEEP_LEN) {
        head = (int)(((uint32_t)sweep_t * (uint32_t)(bar_fill + 3)) / SWEEP_LEN);
    }
    put_tile(LIST_X, ROW_BAR, PAL_GREEN, '[');
    for (int i = 0; i < BAR_LEN; i++) {
        int pal = PAL_GREEN;
        if (i < bar_fill) {
            if (i == head)                           { pal = PAL_WHITE; }
            else if (i == head - 1 || i == head - 2) { pal = PAL_TEAL; }
        }
        put_tile(LIST_X + 1 + i, ROW_BAR, pal, (i < bar_fill) ? '=' : '-');
    }
    put_tile(LIST_X + BAR_LEN + 1, ROW_BAR, PAL_GREEN, ']');
}

static int amb_mode;                       /* set in amb_resolve() */
static void draw_webs(void);

static void draw_frame(void) {
    ng_cls();
    if (GILDED) { gl_build(); gl_pals(fade_lvl); } else { draw_title(-1); if (SKIN_NEW) { sk_build(); sk_pals_all(); } }
    draw_belts();
    st_text(LIST_X, ROW_NOWL, PAL_DIM, "NOW PLAYING");
    if (SKIN_ID == 2) {                                     /* MVS Cabinet: each button's help in the button's colour, in the small font */
        st_text(2, ROW_HELP1, PAL_RUST, "A PLAY"); st_text(10, ROW_HELP1, PAL_AMBER, "B STOP");        /* four words spread evenly over columns 2 to 37 */
        st_text(19, ROW_HELP1, PAL_GREEN, "C REPEAT"); st_text(29, ROW_HELP1, PAL_COPPER, "D SHUFFLE");
    } else {
        ng_center_text(ROW_HELP1, PAL_DIM, "A PLAY  B STOP  C REPEAT  D SHUFFLE");
    }
    st_center(ROW_HELP2, PAL_DIM, "UP/DN SELECT  L/R SKIP  START MENU");
    if (count == 0) {
        ng_center_text(12, PAL_AMBER, "NO MUSIC ON THIS CARTRIDGE");
        ng_center_text(14, PAL_TEXT, "BUILD ONE WITH THE FOUNDRY");
    }
    if (amb_on() && amb_mode == 3) { draw_webs(); }        /* spooky effect: cobwebs in the corners */
    if (!SKIN_ANY) { draw_cogs(); }
    dirty_bar = 1;
}

/* ====================== screensavers ====================================
 * Most use only fix-layer tiles and palette writes; the title, light show,
 * gold lettering and warp savers use hardware sprites. None of them costs
 * anything in the audio ROM, and they take their colours from the theme
 * palettes, so each skin gets its own look (warp is plain white stars). */

static void blank_cell(int x, int y) { put_tile(x, y, 0, TILE_BASE); }

static int isin(int a);          /* defined with the light show below */
static uint16_t mix_word(uint16_t a, uint16_t b, int f);
static void ls_hide(void);

/* -- bounce: the title wanders around the screen --
 * With the title letters in the C ROM (the Foundry puts them there) the title is made of sprites: it glides one
 * pixel at a time, every frame, the letters ripple, colours flow along them in the skin's colours, and three
 * fading echoes follow. Without them the old tile-by-tile version below is used. */
#define TT_SPR0    40
#define TT_PAL0    64                /* 4 layers x 16 hues: slots 64..127 */
#define TT_N       20
static const uint8_t tt_back[4] = { 12, 8, 4, 0 };      /* echo delays in frames, oldest first */
static const uint8_t tt_lvl[4]  = { 1, 2, 4, 8 };       /* echo brightness in eighths */
static int tt_x, tt_y, tt_vx, tt_vy, tt_t;
static int16_t tt_hx[16], tt_hy[16];

static void tt_init(void) {
    uint16_t c0 = TH->pal[PAL_AMBER][1], c1 = TH->pal[PAL_GREEN][1], c2 = TH->pal[PAL_COGS][4], h8[8], h16[16];
    h8[0] = c0; h8[1] = mix_word(c0, c1, 2); h8[2] = c1; h8[3] = mix_word(c1, c2, 2);
    h8[4] = c2; h8[5] = mix_word(c2, c0, 2); h8[6] = TH->pal[PAL_WHITE][1]; h8[7] = mix_word(h8[6], c0, 2);
    for (int k = 0; k < 8; k++) { h16[2 * k] = h8[k]; h16[2 * k + 1] = mix_word(h8[k], h8[(k + 1) & 7], 2); }
    for (int l = 0; l < 4; l++) {
        for (int h = 0; h < 16; h++) {
            int pb = (TT_PAL0 + l * 16 + h) * 16;
            MMAP_PALBANK1[pb] = 0x8000;
            MMAP_PALBANK1[pb + 1] = dim_word(h16[h], tt_lvl[l]);                      /* letter face */
            MMAP_PALBANK1[pb + 2] = dim_word(dim_word(h16[h], 2), tt_lvl[l]);         /* its shadow  */
        }
    }
    for (int n = 0; n < 4 * TT_N; n++) {
        vwrite(SCB1 + (TT_SPR0 + n) * 64, (uint16_t)(TH->title_tile + (n % TT_N)));
        vwrite(SCB2 + TT_SPR0 + n, 0x0fff);
        vwrite(SCB3 + TT_SPR0 + n, 0);
    }
    tt_x = 40 << 8; tt_y = 40 << 8; tt_vx = 218; tt_vy = 141; tt_t = 0;
    for (int k = 0; k < 16; k++) { tt_hx[k] = 40; tt_hy[k] = 40; }
}
static void tt_step(void) {
    tt_x += tt_vx; tt_y += tt_vy;
    if ((tt_x >> 8) <= 0)                  { tt_x = 1 << 8;                 tt_vx = -tt_vx; }
    if ((tt_x >> 8) >= 320 - TT_N * 8)     { tt_x = (320 - TT_N * 8) << 8;  tt_vx = -tt_vx; }
    if ((tt_y >> 8) <= 6)                  { tt_y = 6 << 8;                 tt_vy = -tt_vy; }
    if ((tt_y >> 8) >= 224 - 16 - 6)       { tt_y = (224 - 16 - 6) << 8;    tt_vy = -tt_vy; }
    tt_hx[tt_t & 15] = (int16_t)(tt_x >> 8);
    tt_hy[tt_t & 15] = (int16_t)(tt_y >> 8);
    for (int l = 0; l < 4; l++) {
        int back = tt_back[l], k = (tt_t - back) & 15, ft = tt_t - back;
        if (ft < 0) { ft = 0; k = 0; }
        for (int i = 0; i < TT_N; i++) {
            int n = l * TT_N + i;
            int y = tt_hy[k] + ((isin(ft * 4 + i * 18) * 4) >> 7);
            int x = tt_hx[k] + i * 8;
            vwrite(SCB1 + (TT_SPR0 + n) * 64 + 1, (uint16_t)((TT_PAL0 + l * 16 + ((((ft >> 4) + (i >> 1)) & 15))) << 8));
            vwrite(SCB3 + TT_SPR0 + n, title_char(i) == ' ' ? 0 : (uint16_t)((((496 - y) & 0x1ff) << 7) | 1));
            vwrite(SCB4 + TT_SPR0 + n, (uint16_t)((x & 0x1ff) << 7));
        }
    }
    tt_t++;
}


#define BOUNCE_FRAMES 12
static int bn_x, bn_y, bn_dx, bn_dy, bn_col, bn_t;
static const uint8_t bn_pals[6] = { PAL_BRASS, PAL_AMBER, PAL_GREEN, PAL_TEAL, PAL_WHITE, PAL_GLINT };

static void bounce_init(void) {
    bn_x = 3; bn_y = 6; bn_dx = 1; bn_dy = 1; bn_col = 0; bn_t = 0;
    draw_title_at(bn_x, bn_y, bn_pals[bn_col], -1);
}
static void bounce_step(void) {
    if (++bn_t < BOUNCE_FRAMES) { return; }           /* one step every 12 frames: a third of the old speed */
    bn_t = 0;
    for (int i = 0; i < TITLE_LEN; i++) {
        blank_cell(bn_x + i, bn_y);
        blank_cell(bn_x + i, bn_y + 1);
    }
    bn_x += bn_dx;
    bn_y += bn_dy;
    if (bn_x <= 0)                    { bn_x = 0;                    bn_dx = 1;  bn_col = (bn_col + 1) % 6; }
    if (bn_x >= SCR_W - TITLE_LEN)    { bn_x = SCR_W - TITLE_LEN;    bn_dx = -1; bn_col = (bn_col + 1) % 6; }
    if (bn_y <= ROW_FIRST)            { bn_y = ROW_FIRST;            bn_dy = 1;  bn_col = (bn_col + 1) % 6; }
    if (bn_y >= ROW_LAST - 1)         { bn_y = ROW_LAST - 1;         bn_dy = -1; bn_col = (bn_col + 1) % 6; }
    draw_title_at(bn_x, bn_y, bn_pals[bn_col], -1);
}

/* -- stars: three layers drifting left at different speeds -- */
#define NSTARS 30
static uint8_t st_x[NSTARS], st_y[NSTARS], st_l[NSTARS];

static void star_draw(int i) {
    int l = st_l[i];
    put_tile(st_x[i], st_y[i], (l == 0) ? PAL_DIM : ((l == 1) ? PAL_TEXT : PAL_WHITE),
             (l == 2) ? '*' : ((l == 1) ? '+' : '.'));
}
static void stars_init(void) {
    for (int i = 0; i < NSTARS; i++) {
        st_x[i] = rnd() % SCR_W;
        st_y[i] = ROW_FIRST + rnd() % SCR_H;
        st_l[i] = i % 3;
        star_draw(i);
    }
}
static void stars_step(void) {
    for (int i = 0; i < NSTARS; i++) {
        int mask = (st_l[i] == 0) ? 3 : ((st_l[i] == 1) ? 1 : 0);   /* move every 4, 2 or 1 frames */
        if ((tick & mask) != 0) { continue; }
        blank_cell(st_x[i], st_y[i]);
        if (st_x[i] == 0) {
            st_x[i] = SCR_W - 1;
            st_y[i] = ROW_FIRST + rnd() % SCR_H;
        } else {
            st_x[i]--;
        }
        star_draw(i);
    }
}

static uint16_t mix_word(uint16_t a, uint16_t b, int f) {          /* f = 0..4 of 5 */
    unsigned r = ((((a >> 8) & 15) * (5 - f)) + (((b >> 8) & 15) * f)) / 5;
    unsigned g = ((((a >> 4) & 15) * (5 - f)) + (((b >> 4) & 15) * f)) / 5;
    unsigned bl = (((a & 15) * (5 - f)) + ((b & 15) * f)) / 5;
    return (uint16_t)((r << 8) | (g << 4) | bl);
}
/* -- life: Conway's Game of Life on a wrapped 80 x 56 board of rounded dots. One fix tile holds 2 x 2 cells (tools/gen_tiles.py makes
 * all 625 combinations of five cell states, from tile LIFE0). Newborn cells flash white, settle to bright amber and age to deep orange;
 * a cell that just died leaves a dim red trail for two generations. A generation takes ten frames: seven of them work out eight rows
 * each, then the whole board is drawn at once. Colours are palette 11 (the title code's, free in the player). -- */
#define LIFE_W     80
#define LIFE_H     56
#define LIFE0      1566
#define LIFE_PAL   11
#define LIFE_GEN   10                     /* frames per generation (it was 5: half the speed) */
#define LIFE_ROWS  8                      /* board rows worked out per frame */
static uint8_t life[2][LIFE_H][LIFE_W];     /* 0 dead, 1 alive */
static uint8_t life_age[LIFE_H][LIFE_W];    /* generations alive, saturating */
static uint8_t life_gh[LIFE_H][LIFE_W];     /* trail left by a cell that died: 2, 1, 0 */
static uint16_t life_tile[SCR_H][SCR_W];    /* ...as the tile number (so only changes are written) */
static int life_buf, life_gen, life_row, life_pop, life_t;

static int life_state(int y, int x) {        /* 0 dead, 1 trail, 2 newborn, 3 young, 4 old */
    if (life[life_buf][y][x]) { int a = life_age[y][x]; return a <= 1 ? 2 : (a < 6 ? 3 : 4); }
    return life_gh[y][x] ? 1 : 0;
}
static void life_draw_tile(int tx, int ty, int force) {
    int y = ty * 2, x = tx * 2;
    int t = LIFE0 + life_state(y, x) + 5 * life_state(y, x + 1) + 25 * life_state(y + 1, x) + 125 * life_state(y + 1, x + 1);
    if (!force && life_tile[ty][tx] == t) { return; }
    life_tile[ty][tx] = (uint16_t)t;
    put_tile(tx, ROW_FIRST + ty, LIFE_PAL, t);
}
static void life_draw_all(int force) {
    for (int ty = 0; ty < SCR_H; ty++) { for (int tx = 0; tx < SCR_W; tx++) { life_draw_tile(tx, ty, force); } }
}
static void life_pals(void) {                /* index 1 trail, 2 newborn, 3 young, 4 old */
    static const uint16_t c[5] = { 0x8000, 0x0300, 0x0fff, 0x0fc4, 0x0d62 };
    for (int i = 0; i < 16; i++) { MMAP_PALBANK1[LIFE_PAL * 16 + i] = (i < 5) ? c[i] : c[4]; }
}
static void life_seed(void) {
    life_buf = 0; life_gen = 0; life_row = 0; life_pop = 0; life_t = 0;
    for (int y = 0; y < LIFE_H; y++) {
        for (int x = 0; x < LIFE_W; x++) {
            int v = ((rnd() & 3) == 0) ? 1 : 0;
            life[0][y][x] = (uint8_t)v; life_age[y][x] = (uint8_t)v; life_gh[y][x] = 0;
        }
    }
    life_pals();
    life_draw_all(1);
}
static void life_step(void) {
    int cur = life_buf, nx = cur ^ 1, f = life_t % LIFE_GEN;
    life_t++;
    if (f < 7) {                                     /* frames 0..6: eight more rows each (7 x 8 = 56) */
        int y1 = life_row + LIFE_ROWS;
        if (f == 0) { life_row = 0; life_pop = 0; y1 = LIFE_ROWS; }
        for (int y = life_row; y < y1; y++) {
            int ym = (y == 0) ? LIFE_H - 1 : y - 1;
            int yp = (y == LIFE_H - 1) ? 0 : y + 1;
            for (int x = 0; x < LIFE_W; x++) {
                int xm = (x == 0) ? LIFE_W - 1 : x - 1;
                int xp = (x == LIFE_W - 1) ? 0 : x + 1;
                int n = life[cur][ym][xm] + life[cur][ym][x] + life[cur][ym][xp]
                      + life[cur][y][xm]                       + life[cur][y][xp]
                      + life[cur][yp][xm] + life[cur][yp][x] + life[cur][yp][xp];
                int v = life[cur][y][x], nv = v ? (n == 2 || n == 3) : (n == 3);
                life[nx][y][x] = (uint8_t)nv;
                if (nv) { life_pop++; }
            }
        }
        life_row = y1;
    } else if (f == 7) {                             /* age the cells, trails fade, swap boards */
        for (int y = 0; y < LIFE_H; y++) {
            for (int x = 0; x < LIFE_W; x++) {
                int was = life[cur][y][x], now = life[nx][y][x];
                if (now) { life_age[y][x] = was ? (uint8_t)(life_age[y][x] < 250 ? life_age[y][x] + 1 : 250) : 1; life_gh[y][x] = 0; }
                else if (was) { life_age[y][x] = 0; life_gh[y][x] = 2; }
                else if (life_gh[y][x]) { life_gh[y][x]--; }
            }
        }
        life_buf = nx; life_gen++;
    } else if (f == 8) {                             /* draw the new generation (only tiles that changed are written) */
        life_draw_all(0);
        if (life_pop < 48 || life_gen > 400) { life_seed(); }
    }
}

/* -- rain: trails of glyphs falling down every column -- */
#define RAIN_TRAIL 10
static int8_t rn_head[SCR_W];
static uint8_t rn_spd[SCR_W], rn_cnt[SCR_W], rn_ph[SCR_W];

static void rain_column_reset(int x) {
    rn_head[x] = (int8_t)(ROW_FIRST - (int)(rnd() % 20));
    rn_spd[x] = 1 + rnd() % 3;
    rn_cnt[x] = 0;
    rn_ph[x] = (uint8_t)rnd();
}
static void rain_init(void) {
    for (int x = 0; x < SCR_W; x++) { rain_column_reset(x); }
}
static void rain_step(void) {
    for (int x = 0; x < SCR_W; x++) {
        if (++rn_cnt[x] < rn_spd[x]) { continue; }
        rn_cnt[x] = 0;
        rn_head[x]++;
        for (int d = 0; d <= RAIN_TRAIL; d++) {
            int y = rn_head[x] - d;
            int pal = (d == 0) ? PAL_WHITE : ((d <= 2) ? PAL_TEAL : ((d <= 5) ? PAL_GREEN : PAL_DIM));
            if (y >= ROW_FIRST && y <= ROW_LAST) {
                put_tile(x, y, pal, 33 + (unsigned)(x * 37 + y * 91 + rn_ph[x]) % 59);
            }
        }
        {
            int yb = rn_head[x] - RAIN_TRAIL - 1;
            if (yb >= ROW_FIRST && yb <= ROW_LAST) { blank_cell(x, yb); }
            if (yb > ROW_LAST) { rain_column_reset(x); }
        }
    }
}


/* -- light show: glowing orbs (hardware sprites) trace curves while the backdrop breathes.
 * One big Lissajous figure, four small turning rosettes and two ripples spreading from the centre;
 * the orbs shrink and grow along each curve, and colours flow round a palette built from the theme. -- */
#define LS_SPR0   40
#define LS_PAL0   48
#define LS_BIG    128
#define LS_ROSE   40
#define LS_RING   20
#define LS_COUNT  (LS_BIG + 4 * LS_ROSE + 2 * LS_RING)
static const uint8_t qsin[65] = { 0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49, 51, 54, 57, 60, 63, 65, 68, 71, 73, 76, 78, 81, 83, 85, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 107, 109, 111, 112, 113, 115, 116, 117, 118, 120, 121, 122, 122, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127, 127 };
static uint16_t ls_hue[8];
static int ls_t;
/* The 68000 has no 32 bit multiply or divide: gcc calls slow library routines for them. The light show touches
 * hundreds of sprites per frame, so its inner loops use 16 bit multiplies (one instruction), running angles
 * instead of products, and little tables worked out once at start-up instead of divisions. */
static inline int mul16(int a, int b) { return (int)(int16_t)a * (int16_t)b; }
static uint8_t ls_ru[LS_ROSE], ls_gu[LS_RING], ls_bp[LS_BIG], ls_rp[LS_ROSE];   /* angles and colour steps per sprite */
static uint8_t ls_cp[LS_COUNT], ls_cs[LS_COUNT];                                /* palette and size last written */

static int isin(int a) {                         /* 256 steps to the circle, result -127..127 */
    a &= 255;
    if (a < 64)  { return qsin[a]; }
    if (a < 128) { return qsin[128 - a]; }
    if (a < 192) { return -qsin[a - 128]; }
    return -qsin[256 - a];
}
static int icos(int a) { return isin(a + 64); }

static void ls_orb(int n, int x, int y, int size, int pal) {      /* centre x,y; size 0..15 */
    int s = (size < 0) ? 0 : ((size > 15) ? 15 : size);
    x -= (s + 1) >> 1;                                  /* the shrunk orb hangs from the sprite's top left: */
    y -= (s + 1) >> 1;                                  /* put its middle on x,y */
    if (ls_cp[n] != pal) { ls_cp[n] = (uint8_t)pal; vwrite(SCB1 + (LS_SPR0 + n) * 64 + 1, (uint16_t)(pal << 8)); }
    if (ls_cs[n] != s)   { ls_cs[n] = (uint8_t)s;   vwrite(SCB2 + LS_SPR0 + n, (uint16_t)((s << 8) | (s * 17))); }
    vwrite(SCB3 + LS_SPR0 + n, (uint16_t)((((496 - y) & 0x1ff) << 7) | 1));
    vwrite(SCB4 + LS_SPR0 + n, (uint16_t)((x & 0x1ff) << 7));
}

static void ls_hide(void) {
    for (int n = 0; n < LS_COUNT; n++) { vwrite(SCB3 + LS_SPR0 + n, 0); }
}

static void lights_init(void) {
    uint16_t c0 = TH->pal[PAL_AMBER][1], c1 = TH->pal[PAL_GREEN][1], c2 = TH->pal[PAL_COGS][4];
    ls_hue[0] = c0;               ls_hue[1] = mix_word(c0, c1, 2);
    ls_hue[2] = c1;               ls_hue[3] = mix_word(c1, c2, 2);
    ls_hue[4] = c2;               ls_hue[5] = mix_word(c2, c0, 2);
    ls_hue[6] = TH->pal[PAL_WHITE][1];
    ls_hue[7] = mix_word(ls_hue[6], c0, 2);
    for (int k = 0; k < 8; k++) {
        MMAP_PALBANK1[(LS_PAL0 + k) * 16] = 0x8000;
        for (int i = 1; i < 16; i++) {                          /* dark -> hue -> white-hot centre */
            unsigned comp[3], w = ls_hue[k];
            comp[0] = (w >> 8) & 15; comp[1] = (w >> 4) & 15; comp[2] = w & 15;
            for (int j = 0; j < 3; j++) {
                unsigned v = comp[j] * i / 11;
                if (v > 15) { v = 15; }
                if (i > 11) { v += ((15 - v) * (i - 11)) / 4; }
                comp[j] = v;
            }
            MMAP_PALBANK1[(LS_PAL0 + k) * 16 + i] = (uint16_t)((comp[0] << 8) | (comp[1] << 4) | comp[2]);
        }
    }
    for (int n = 0; n < LS_COUNT; n++) {
        vwrite(SCB1 + (LS_SPR0 + n) * 64, VIZ_TILE);
        blank_below(LS_SPR0 + n);
        vwrite(SCB3 + LS_SPR0 + n, 0);
    }
    for (int i = 0; i < LS_ROSE; i++) { ls_ru[i] = (uint8_t)((i * 256) / LS_ROSE); ls_rp[i] = (uint8_t)(i / 8); }
    for (int i = 0; i < LS_RING; i++) { ls_gu[i] = (uint8_t)((i * 256) / LS_RING); }
    for (int i = 0; i < LS_BIG; i++)  { ls_bp[i] = (uint8_t)(i / 12); }
    for (int n = 0; n < LS_COUNT; n++) { ls_cp[n] = 0xff; ls_cs[n] = 0xff; }      /* force the first write */
    ls_t = 0;
}

static void lights_step(void) {
    static const uint8_t fa[6] = { 3, 1, 3, 2, 4, 1 }, fb[6] = { 2, 2, 4, 3, 3, 3 };
    static const int8_t sk[6] = { 3, 5, -2, 7, 4, -3 };
    int cyc = ls_t % 720, pre = (ls_t / 720) % 6, scene = (ls_t / 1440) & 1;      /* every other 24 s the big figure becomes a spirograph */
    int edge = (cyc < 720 - cyc) ? cyc : 720 - cyc;
    int f = isin(edge > 64 ? 64 : edge);                          /* 0..127: curves grow in and out */
    int ph = ls_t >> 1;
    int wa = ph, wb = 0, ws = ph * 2, wz;                          /* running angles instead of products */
    int da = 2 * fa[pre], db = 2 * fb[pre], ds = 2 * sk[pre];
    int ax = (130 * f) >> 7, ay = (88 * f) >> 7;
    int ls3, par, n;
    ls_t++;
    par = ls_t & 1;                                               /* rosettes and ripples take turns: half each frame */
    ls3 = ls_t * 3;
    wz = -ls_t * 5;
    for (n = 0; n < LS_BIG; n++) {                                /* the big figure, every frame */
        int u = n * 2;
        int x, y;
        if (scene) {                                              /* two turning arms: a spirograph */
            int a = mul16(60, icos(u)) + mul16(66, icos(ws)), b = mul16(60, isin(u)) + mul16(66, isin(ws));
            x = 160 + (mul16(a >> 2, f) >> 12);
            y = 112 + (mul16(b >> 2, f * 3) >> 14);
            ws += ds;
        } else {
            x = 160 + (mul16(ax, isin(wa)) >> 7);
            y = 112 + (mul16(ay, isin(wb)) >> 7);
            wa += da; wb += db;
        }
        ls_orb(n, x, y, 9 + (((isin(wz) + 127) * 7) >> 8), LS_PAL0 + ((ls_bp[n] + (ls_t >> 2)) & 7));
        wz += 4;
    }
    for (int q = 0; q < 4; q++) {                                  /* four rosettes, turning against each other */
        int cx = (q & 1) ? 244 : 76, cy = (q & 2) ? 170 : 54;
        int rot = (q ^ (q >> 1)) & 1 ? -ls_t : ls_t;
        int petals = 2 + ((ls_t / 360 + q) & 1);
        int rmax = (40 * f) >> 7;
        int base = LS_BIG + q * LS_ROSE;
        if ((q & 1) != par) { continue; }                          /* the other pair moves on the next frame */
        for (int i = 0; i < LS_ROSE; i++) {
            int th = ls_ru[i];
            int r = mul16(rmax, isin(th * 2 + (petals == 3 ? th : 0) + ph)) >> 7;
            int x = cx + (mul16(r, icos(th + rot)) >> 7);
            int y = cy + (mul16(r, isin(th + rot)) >> 7);
            ls_orb(base + i, x, y, 7 + (((isin(i * 8 + ls3) + 127) * 5) >> 8), LS_PAL0 + ((q * 2 + ls_rp[i] + (ls_t >> 3)) & 7));
        }
    }
    for (int k = 0; k < 2; k++) {                                  /* ripples leaving the centre */
        int rad = ((ls_t + k * 60) % 120) * 5 / 4;
        int rsz = 14 - (rad >> 4);
        int base = LS_BIG + 4 * LS_ROSE + k * LS_RING;
        if (k != par) { continue; }
        for (int i = 0; i < LS_RING; i++) {
            int a = ls_gu[i] + k * 6;
            ls_orb(base + i, 160 + (mul16(rad, icos(a)) >> 7), 112 + ((mul16(rad, isin(a)) * 3) >> 9), rsz,
                   LS_PAL0 + (((ls_t >> 3) + k * 3 + i) & 7));
        }
    }
    if ((ls_t & 3) == 0) {                                          /* backdrop breathes in the colours of the show */
        *((volatile uint16_t *)0x401ffe) = dim_word(ls_hue[(ls_t >> 6) & 7], ((isin(ls_t * 3) + 127) * 3) >> 8);
    }
}

/* -- gold: the word NEOGEO in big polished gold letters, pre-rendered (tools/gen_spin.py) as 64 frames of
 * sprites. The letters flip one after another in a wave, rest, and a shine sweeps across; a few sparkles
 * twinkle round the edges of the screen. Each frame swaps the tile numbers of 20 sprites, nothing else. -- */
#define GS_PAL     56
#define GS_TICKS   5                 /* frames per picture: a turn takes about 5.4 s */
#define GS_SPARKS  20
static int gs_f, gs_t;
static uint8_t sp_x[GS_SPARKS], sp_y[GS_SPARKS], sp_s[GS_SPARKS];

static void gs_frame(void) {
    for (int c = 0; c < SPIN_COLS; c++) {
        for (int r = 0; r < SPIN_ROWS; r++) {
            vwrite(SCB1 + (LS_SPR0 + c) * 64 + r * 2, (uint16_t)(SPIN_TILE0 + (gs_f * SPIN_COLS + c) * SPIN_ROWS + r));
        }
    }
}
static void gs_spark_place(int i) {
    sp_x[i] = (uint8_t)(rnd() % SCR_W);
    sp_y[i] = (uint8_t)((rnd() & 1) ? ROW_FIRST + rnd() % 8 : 22 + rnd() % 8);   /* above and below the lettering */
    sp_s[i] = 0;
}
static void gs_init(void) {
    for (int k = 0; k < 16; k++) { MMAP_PALBANK1[GS_PAL * 16 + k] = spin_pal[k]; }
    for (int c = 0; c < SPIN_COLS; c++) {
        vwrite(SCB2 + LS_SPR0 + c, 0x0fff);
        for (int r = 0; r < SPIN_ROWS; r++) { vwrite(SCB1 + (LS_SPR0 + c) * 64 + r * 2 + 1, (uint16_t)(GS_PAL << 8)); }
        vwrite(SCB3 + LS_SPR0 + c, (uint16_t)((((496 - (112 - SPIN_ROWS * 8)) & 0x1ff) << 7) | SPIN_ROWS));
        vwrite(SCB4 + LS_SPR0 + c, (uint16_t)((16 * c) << 7));
    }
    *((volatile uint16_t *)0x401ffe) = 0x0100;
    gs_f = 0; gs_t = 0;
    gs_frame();
    for (int i = 0; i < GS_SPARKS; i++) { gs_spark_place(i); sp_s[i] = (uint8_t)(rnd() % 40); }
}
static void gs_step(void) {
    static const char glyph[5] = { '.', '+', '*', '+', '.' };
    static const uint8_t gpal[5] = { PAL_DIM, PAL_TEXT, PAL_WHITE, PAL_TEXT, PAL_DIM };
    if (++gs_t % GS_TICKS == 0) {
        if (++gs_f >= SPIN_FRAMES) { gs_f = 0; }
        gs_frame();
    }
    if ((tick & 1) == 0) {
        for (int i = 0; i < GS_SPARKS; i++) {
            if (++sp_s[i] >= 40) { blank_cell(sp_x[i], sp_y[i]); gs_spark_place(i); }
            if ((sp_s[i] & 7) == 0) { put_tile(sp_x[i], sp_y[i], gpal[sp_s[i] >> 3], glyph[sp_s[i] >> 3]); }
        }
    }
}

/* -- warp: looking out of a starship window at near light speed. Stars appear at random places and moments,
 * tiny and drifting out slowly; as one gets nearer it speeds up, grows and brightens and flashes past the
 * edge. Each star is a solid tile (STAR_TILE) the hardware shrinks to a dot. Where a star would be on screen
 * is its fixed spot at the far end times 255 / z, so nothing is divided in the loop (table made at start-up).
 * One rare visitor drifts by now and then at a quarter of the speed. -- */
#ifndef WP_V_ODDS
#define WP_V_ODDS 3600     /* a 1 in this roll every second */
#endif
#define WP_N      140
#define WP_V   WP_N                /* sprite after the stars */
#define WP_PAL0   LS_PAL0             /* four brightness levels: colour 15 of palettes 48..51; palette 52 is the visitor */
static int8_t   wp_x[WP_N], wp_y[WP_N];
static uint8_t  wp_z[WP_N], wp_w[WP_N], wp_cp[WP_N], wp_cs[WP_N];
static uint16_t wp_r[256];            /* 255 * 256 / z */
static int      fc_on, fc_z4, fc_x, fc_y, fc_cs, fc_t;

static int wp_dz(int z) {             /* slow far away, faster and faster as it nears */
    int n = 255 - z;
    return 1 + (mul16(n, n) >> 13);
}
static void wp_place(int i) {         /* a new star: a random spot on the screen, after a random wait */
    int x = (int)(rnd() % 221) - 110, y = (int)(rnd() % 157) - 78;
    if (x > -8 && x < 8 && y > -8 && y < 8) { x += 24; }
    wp_x[i] = (int8_t)x; wp_y[i] = (int8_t)y; wp_z[i] = 255; wp_w[i] = (uint8_t)(rnd() % 150);
    vwrite(SCB3 + LS_SPR0 + i, 0);
}
static void wp_init(void) {
    static const uint16_t shade[4] = { 0x0334, 0x0668, 0x0aac, 0x0fff };
    for (int k = 0; k < 4; k++) {
        MMAP_PALBANK1[(WP_PAL0 + k) * 16] = 0x8000;
        for (int c = 1; c < 16; c++) { MMAP_PALBANK1[(WP_PAL0 + k) * 16 + c] = shade[k]; }
    }
    for (int c = 0; c < 16; c++) { MMAP_PALBANK1[(WP_PAL0 + 4) * 16 + c] = v_pal[c]; }
    wp_r[0] = 0;
    for (int z = 1; z < 256; z++) { wp_r[z] = (uint16_t)((255 * 256) / z); }
    for (int i = 0; i < WP_N; i++) {
        wp_place(i);
        if ((rnd() % 10) < 6) { wp_w[i] = 0; wp_z[i] = (uint8_t)(10 + rnd() % 246); }      /* a busy screen from the start */
        wp_cp[i] = 0xff; wp_cs[i] = 0xff;
        vwrite(SCB1 + (LS_SPR0 + i) * 64, STAR_TILE);
        blank_below(LS_SPR0 + i);
    }
    blank_below(LS_SPR0 + WP_V);
    vwrite(SCB1 + (LS_SPR0 + WP_V) * 64,     V_TILE);
    vwrite(SCB1 + (LS_SPR0 + WP_V) * 64 + 1, (uint16_t)((WP_PAL0 + 4) << 8));
    vwrite(SCB3 + LS_SPR0 + WP_V, 0);
    fc_on = 0; fc_t = 0; fc_cs = -1;
    *((volatile uint16_t *)0x401ffe) = 0x0000;                    /* black space */
}
static void wp_step(void) {
    for (int i = 0; i < WP_N; i++) {
        int z, x, y, near, s, pal;
        if (wp_w[i]) { wp_w[i]--; continue; }                       /* not here yet */
        z = wp_z[i];
        x = 160 + (mul16(wp_x[i], wp_r[z]) >> 8);
        y = 112 + (mul16(wp_y[i], wp_r[z]) >> 8);
        if (x < -8 || x > 327 || y < -8 || y > 231) { wp_place(i); continue; }   /* flashed past the edge */
        near = 255 - z;                                             /* 0 far .. 245 near */
        s = 1 + ((near * 7) >> 8);                                  /* dot size 1..7 */
        pal = WP_PAL0 + (near >> 6);
        x -= (s + 1) >> 1; y -= (s + 1) >> 1;
        if (wp_cp[i] != pal) { wp_cp[i] = (uint8_t)pal; vwrite(SCB1 + (LS_SPR0 + i) * 64 + 1, (uint16_t)(pal << 8)); }
        if (wp_cs[i] != s)   { wp_cs[i] = (uint8_t)s;   vwrite(SCB2 + LS_SPR0 + i, (uint16_t)((s << 8) | (s * 17))); }
        vwrite(SCB3 + LS_SPR0 + i, (uint16_t)((((496 - y) & 0x1ff) << 7) | 1));
        vwrite(SCB4 + LS_SPR0 + i, (uint16_t)((x & 0x1ff) << 7));
        z -= wp_dz(z);
        if (z < 10) { wp_place(i); } else { wp_z[i] = (uint8_t)z; }
    }
    /* the rare visitor: a roll once a second, 1 in 3600 */
    if (!fc_on) {
        if (++fc_t >= 60) {
            fc_t = 0;
            if (rnd() % WP_V_ODDS == 0) {
                fc_x = (int)(rnd() % 121) - 60; fc_y = (int)(rnd() % 81) - 40;
                if (fc_x > -8 && fc_x < 8 && fc_y > -8 && fc_y < 8) { fc_x += 24; }
                fc_z4 = 255 * 4; fc_cs = -1; fc_on = 1;
            }
        }
    }
    if (fc_on) {
        int z = fc_z4 >> 2;
        int x = 160 + (mul16(fc_x, wp_r[z]) >> 8), y = 112 + (mul16(fc_y, wp_r[z]) >> 8);
        int near = 255 - z, s = 2 + ((near * 13) >> 8);             /* grows to a full 16 pixels */
        if (x < -16 || x > 335 || y < -16 || y > 239) {
            fc_on = 0; vwrite(SCB3 + LS_SPR0 + WP_V, 0);
        } else {
            x -= (s + 1) >> 1; y -= (s + 1) >> 1;
            if (fc_cs != s) { fc_cs = s; vwrite(SCB2 + LS_SPR0 + WP_V, (uint16_t)((s << 8) | (s * 17))); }
            vwrite(SCB3 + LS_SPR0 + WP_V, (uint16_t)((((496 - y) & 0x1ff) << 7) | 1));
            vwrite(SCB4 + LS_SPR0 + WP_V, (uint16_t)((x & 0x1ff) << 7));
            fc_z4 -= wp_dz(z);                                      /* a quarter of the speed: z is kept times four */
            if ((fc_z4 >> 2) < 10) { fc_on = 0; vwrite(SCB3 + LS_SPR0 + WP_V, 0); }
        }
    }
}

/* -- starting and stopping -- */
static void saver_start(void) {
    int mode = opt_saver;
    if (mode == SAVER_RANDOM) {
        mode = SAVER_BOUNCE + (int)(rnd() % 7);                    /* ids 3..7, then 9 and 10 */
        if (mode >= SAVER_RANDOM) { mode++; }
    }
    saver_cur = mode;
    saver_on = 1;
    saver_anim = (mode >= SAVER_BOUNCE && mode <= SAVER_RAIN) || mode == SAVER_LIGHTS || mode == SAVER_GOLD;
    if (mode == SAVER_DIM)     { fade_tgt = 2; fade_keep = 0; }
    if (mode == SAVER_PARTIAL) {
        fade_tgt = 3;
        fade_keep = (1 << PAL_AMBER) | (1 << PAL_GREEN) | (1 << PAL_WHITE) | (1 << PAL_TEAL);
    }
    if (saver_anim) {
        ng_cls();
        if (SKIN_ANY) { for (int n = 40; n < 180; n++) { vwrite(SCB3 + n, 0); } }     /* the layout's own sprites make way */
        if (mode == SAVER_BOUNCE) { if (TITLE_SPRITES) { tt_init(); } else { bounce_init(); } }
        if (mode == SAVER_STARS)  { stars_init(); }
        if (mode == SAVER_WARP)   { wp_init(); }
        if (mode == SAVER_LIFE)   { life_seed(); }
        if (mode == SAVER_RAIN)   { rain_init(); }
        if (mode == SAVER_LIGHTS) { lights_init(); }
        if (mode == SAVER_GOLD)   { gs_init(); }
    }
}

static void saver_step(void) {
    if (saver_cur == SAVER_BOUNCE) { if (TITLE_SPRITES) { tt_step(); } else { bounce_step(); } }
    if (saver_cur == SAVER_STARS)  { stars_step(); }
    if (saver_cur == SAVER_WARP)   { wp_step(); }
    if (saver_cur == SAVER_LIFE)   { life_step(); }
    if (saver_cur == SAVER_RAIN)   { rain_step(); }
    if (saver_cur == SAVER_LIGHTS) { lights_step(); }
    if (saver_cur == SAVER_GOLD)   { gs_step(); }
}

/* Waking must not rewrite the whole palette in one go (grey noise on a real console, see PAL_SLICE). Dimmed screens are
 * brought back a piece per frame. After an animated saver the screen is rebuilt in the dark and fades up. */
static void saver_stop(void) {
    saver_on = 0;
    fade_keep = 0;
    if (saver_anim) {
        fade_lvl = 0; fade_tgt = 8; fade_tick = 0;
    } else {
        fade_lvl = 8; fade_tgt = 8; pal_part = 0;
    }
    if (saver_anim) {
        if (saver_cur == SAVER_LIGHTS || saver_cur == SAVER_GOLD || saver_cur == SAVER_WARP || (saver_cur == SAVER_BOUNCE && TITLE_SPRITES)) { ls_hide(); }
        saver_anim = 0;
        pal_sweep();
        draw_frame();
        dirty_list = 1;
        dirty_now = 1;
        dirty_bar = 1;
        glint_drawn = -1;
    }
}

/* ====================== playback ======================================= */
static void stop_track(void) {
    *REG_SOUND = CMD_STOP;
    playing = -1;
    name_rev = 99;
    elapsed_frames = 0;
    elapsed_sec = 0;
    sec_acc = 0;
    bar_fill = 0;
    dirty_list = 1;
    dirty_now = 1;
    dirty_bar = 1;
}

static void play_track(int i) {
    uint32_t f = T->track[i].frames;
    *REG_SOUND = CMD_PLAY0 + i;
    playing = i;
    name_rev = 0;
    sel = i;
    elapsed_frames = 0;
    elapsed_sec = 0;
    sec_acc = 0;
    bar_fill = 0;
    bar_acc = 0;
    bar_step = f / BAR_LEN;
    if (bar_step < 1) { bar_step = 1; }
    sweep_t = SWEEP_PERIOD - 40;           /* first sweep a moment after the start */
    dirty_list = 1;
    dirty_now = 1;
    dirty_bar = 1;
}

static void next_track(void) {
    int cur = (playing >= 0 ? playing : sel), n = cur + 1;
    if (shuffle && count > 1) {                            /* any track but the one just played */
        n = (int)(rnd() % (uint16_t)(count - 1));
        if (n >= cur) { n++; }
    } else if (n >= count) {
        n = 0;
    }
    play_track(n);
}

/* One frame of playback bookkeeping: the clock, the progress bar and moving on when a track ends. It runs from the
 * player's loop and from every title, menu and options frame, so the music carries on uninterrupted behind them.
 * The first track starts about a second after power-on (the sound chip needs a moment after its reset). */
#define AUTOPLAY_FRAMES 90
static int autoplay_t = AUTOPLAY_FRAMES;
static void play_tick(void) {
    if (autoplay_t > 0 && --autoplay_t == 0 && playing < 0 && count > 0) { play_track(0); shuf_n = 0; }
    if (playing < 0) { return; }
    elapsed_frames++;
    sec_acc += FRAME_US;
    if (sec_acc >= 1000000) {
        sec_acc -= 1000000;
        elapsed_sec++;
        dirty_now = 1;
    }
    bar_acc++;
    if (bar_acc >= bar_step) {
        bar_acc = 0;
        if (bar_fill < BAR_LEN) { bar_fill++; dirty_bar = 1; }
    }
    if (++sweep_t >= SWEEP_PERIOD) { sweep_t = 0; }
    if (sweep_t <= SWEEP_LEN) { dirty_bar = 1; }
    if (elapsed_frames >= T->track[playing].frames) {
        shuf_n++;
        if (shuffle ? (repeat_all || shuf_n < count) : (playing + 1 < count || repeat_all)) { next_track(); }
        else                                   { stop_track(); }
    }
}

static void prev_track(void) {
    int n = (playing >= 0 ? playing : sel) - 1;
    if (n < 0) { n = count - 1; }
    play_track(n);
}


/* ---- our own splash, shown after the ngdevkit logo: fades in, holds, fades out --------
 * Any button skips it. About 3.5 seconds: 0.8 in, 1.9 hold, 0.8 out. */
/* ====================== title page, start menu, options =====================================================
 * All the art is sprites from the C ROM (tools/gen_title.py): a backdrop of 20 columns, two turning gears, the
 * NGMC lettering with a shine that sweeps across it (big on the title page, small on the menus) and glossy
 * plates. Text goes on the fix layer on top. Palettes are written straight into the palette RAM and faded
 * with tp_pals(). The choices made on the options page live in RAM only (a cartridge has nowhere to keep
 * them), so they reset every time the machine is switched on. */
#define TP_BG      40                    /* 20 backdrop sprites, 14 tiles high */
#define TP_STB     60                    /* the title and menu stars: behind the gears, in front of the backdrop */
#define TP_GA      80                    /* big gear: 10 sprites */
#define TP_GB      90                    /* small gear: 7 sprites */
#define TP_LG      97                    /* lettering: 16 sprites (big) or 9 (small) */
#define TP_PL      181                   /* plates: up to 90 sprites (the options page uses 84) */
#define TP_ST_SK   160                   /* twinkling stars of a skin's player screen */
#define TP_NST     20
#define TP_FIXPAL  11                    /* fix palettes 11..15: dark text, silver, gold, white, dim */

static int tp_st = TP_STB, tp_avoid;        /* first star sprite; tp_avoid: keep stars off the Neon Horizon sun */
static int tp_big, tp_t, tp_lvl, tp_dimbg;     /* tp_dimbg: backdrop and gears dimmed (the the options extra page page) */
static int tp_ga, tp_gb, tp_lf, tp_lt;
static uint16_t tp_sx[TP_NST];
static uint8_t tp_sy[TP_NST], tp_sp[TP_NST];
static int opt_si, opt_ti;               /* their positions in the lists below */
static const uint8_t opt_ids[11] = { SAVER_OFF, SAVER_DIM, SAVER_PARTIAL, SAVER_BOUNCE, SAVER_STARS, SAVER_WARP,
                                     SAVER_LIFE, SAVER_RAIN, SAVER_LIGHTS, SAVER_GOLD, SAVER_RANDOM };
static const char * const opt_names[11] = { "OFF", "DIM", "PARTIAL DIM", "BOUNCING TITLE", "STARFIELD", "WARP STARFIELD",
                                            "GAME OF LIFE", "GLYPH RAIN", "LIGHT SHOW", "GOLD LETTERING", "RANDOM" };
static const uint16_t opt_secs_tab[5] = { 30, 60, 120, 300, 600 };
static const char * const opt_secs_names[5] = { "30 SECONDS", "1 MINUTE", "2 MINUTES", "5 MINUTES", "10 MINUTES" };

static uint16_t tp_dim(uint16_t w, int lvl) {
    return dim_word(w, lvl);
}
static void tp_pal16(int p, const uint16_t *src, int lvl) {
    for (int c = 0; c < 16; c++) { MMAP_PALBANK1[p * 16 + c] = tp_dim(src[c], lvl); }
}
static void tp_solid(int p, uint16_t col, int lvl) {              /* every colour but the transparent one */
    MMAP_PALBANK1[p * 16] = 0x8000;
    for (int c = 1; c < 16; c++) { MMAP_PALBANK1[p * 16 + c] = tp_dim(col, lvl); }
}
#define TP_NPAL 67                      /* 56 backdrop, gears, gold, gold plate, steel plate, 2 star, 5 text palettes */
static void tp_unit(int i, int lvl) {
    if (i < 56)       { tp_pal16(TT_PAL_BG + i, tt_bg_pal[i], tp_dimbg ? (mul16(lvl, 3) >> 3) : lvl); }
    else switch (i) {
    case 56: tp_pal16(TT_PAL_GEAR, tt_gear_pal, tp_dimbg ? (mul16(lvl, 5) >> 3) : lvl); break;
    case 57: tp_pal16(TT_PAL_GOLD, spin_pal, lvl); break;
    case 58: tp_pal16(TT_PAL_PGOLD, tt_pgold_pal, lvl); break;
    case 59: tp_pal16(TT_PAL_PSTEEL, tt_psteel_pal, lvl); break;
    case 60: tp_solid(TT_PAL_STAR, 0x0fff, lvl); break;
    case 61: tp_solid(TT_PAL_STAR + 1, 0x0fe9, lvl); break;
    case 62: tp_solid(TP_FIXPAL,     0x0420, lvl); break;
    case 63: tp_solid(TP_FIXPAL + 1, 0x0ccd, lvl); break;
    case 64: tp_solid(TP_FIXPAL + 2, 0x0fc4, lvl); break;
    case 65: tp_solid(TP_FIXPAL + 3, 0x0fff, lvl); break;
    default: tp_solid(TP_FIXPAL + 4, 0x0556, lvl); break;
    }
}
static void tp_anim(void);
/* Writes every palette of the page at brightness lvl, TP_SLICE palettes per screen frame. The palette RAM is only safe to
 * write in the gap between frames: writing hundreds of colours in one go ran on into the picture and showed as grey noise on
 * a real console (never in MAME). Call it straight after a frame has started. The animation and the music carry on while it
 * works, and it takes about nine frames. */
#define TP_SLICE 8
static void tp_pals(int lvl) {
    for (int i = 0; i < TP_NPAL; i += TP_SLICE) {
        for (int j = i; j < i + TP_SLICE && j < TP_NPAL; j++) { tp_unit(j, lvl); }
        tp_anim(); play_tick();
        ng_wait_vblank();
    }
    tp_lvl = lvl;
}
#define FP_DARK  TP_FIXPAL
#define FP_SILV  (TP_FIXPAL + 1)
#define FP_GOLD  (TP_FIXPAL + 2)
#define FP_WHITE (TP_FIXPAL + 3)
#define FP_DIM   (TP_FIXPAL + 4)

static void tp_col(int spr, int x, int y, int h) {                 /* position and size of one column of sprites */
    vwrite(SCB2 + spr, 0x0fff);
    vwrite(SCB3 + spr, (uint16_t)((((496 - y) & 0x1ff) << 7) | h));
    vwrite(SCB4 + spr, (uint16_t)((x & 0x1ff) << 7));
}
static void tp_tiles(int spr, int tile, int h, int pal) {          /* a column of consecutive tiles */
    for (int r = 0; r < h; r++) {
        vwrite(SCB1 + spr * 64 + r * 2, (uint16_t)(tile + r));
        vwrite(SCB1 + spr * 64 + r * 2 + 1, (uint16_t)(pal << 8));
    }
}
static void tp_retile(int spr, int tile, int h) {
    for (int r = 0; r < h; r++) { vwrite(SCB1 + spr * 64 + r * 2, (uint16_t)(tile + r)); }
}
static void tp_gear_a(void) { for (int c = 0; c < 10; c++) { tp_retile(TP_GA + c, TT_GA0 + (tp_ga * 10 + c) * 10, 10); } }
static void tp_gear_b(void) { for (int c = 0; c < 7; c++)  { tp_retile(TP_GB + c, TT_GB0 + (tp_gb * 7 + c) * 7, 7); } }
static void tp_logo(void) {
    if (tp_big) { for (int c = 0; c < 16; c++) { tp_retile(TP_LG + c, TT_LG0 + (tp_lf * 16 + c) * 4, 4); } }
    else        { for (int c = 0; c < 9; c++)  { tp_retile(TP_LG + c, TT_SL0 + (tp_lf * 9 + c) * 2, 2); } }
}

static void tp_star_pos(int i) {
    for (int tries = 0; tries < 8; tries++) {
        int x = 4 + rnd() % 304, y = 4 + rnd() % 100;
        tp_sx[i] = (uint16_t)x; tp_sy[i] = (uint8_t)y;
        if (!tp_avoid) { return; }
        { int dx = x - 160, dy = y - 82; if (dx * dx + dy * dy >= 54 * 54) { return; } }       /* outside the sun's disc */
    }
}
/* twinkling stars: 20 one-pixel sprites that grow and shrink and then jump somewhere else */
static void tp_stars_init(void) {
    for (int i = 0; i < TP_NST; i++) {
        tp_star_pos(i);
        tp_sp[i] = (uint8_t)(rnd() % 48);
        vwrite(SCB1 + (tp_st + i) * 64, STAR_TILE);
        blank_below(tp_st + i);
        vwrite(SCB1 + (tp_st + i) * 64 + 1, (uint16_t)((TT_PAL_STAR + (i & 1)) << 8));
        vwrite(SCB3 + tp_st + i, 0);
    }
}
static void tp_stars_step(void) {
    for (int i = 0; i < TP_NST; i++) {
        int p = tp_sp[i], s;
        if (++p >= 48) { p = 0; tp_star_pos(i); }
        tp_sp[i] = (uint8_t)p;
        s = (p < 24) ? (p >> 3) : ((47 - p) >> 3);               /* 0..2 and back */
        if ((p & 3) == 0) {
            if (s == 0) { vwrite(SCB3 + tp_st + i, 0); }
            else {
                vwrite(SCB2 + tp_st + i, (uint16_t)((s << 8) | (s * 17)));
                vwrite(SCB3 + tp_st + i, (uint16_t)((((496 - tp_sy[i]) & 0x1ff) << 7) | 1));
                vwrite(SCB4 + tp_st + i, (uint16_t)((tp_sx[i] & 0x1ff) << 7));
            }
        }
    }
}

/* the sprites every page shares; big = 1 for the title page */
static void tp_build_m(int mode) {                /* 1 title page, 0 menus, 2 the Gilded player screen */
    int big = (mode == 1);
    for (int n = 4; n < N_SPRITES; n++) { vwrite(SCB3 + n, 0); }
    tp_big = big;
    for (int c = 0; c < 20; c++) {
        tp_col(TP_BG + c, c * 16, 0, 14);
        for (int r = 0; r < 14; r++) {
            vwrite(SCB1 + (TP_BG + c) * 64 + r * 2, (uint16_t)(TT_BG0 + c * 14 + r));
            vwrite(SCB1 + (TP_BG + c) * 64 + r * 2 + 1, (uint16_t)((TT_PAL_BG + r * 4 + c / 5) << 8));
        }
    }
    for (int c = 0; c < 10; c++) { tp_col(TP_GA + c, (mode == 2 ? -84 : -44) + 16 * c, (mode == 2 ? -44 : 40), 10); tp_tiles(TP_GA + c, TT_GA0 + c * 10, 10, TT_PAL_GEAR); }
    for (int c = 0; c < 7; c++)  { tp_col(TP_GB + c, (mode == 2 ? 252 : 232) + 16 * c, (mode == 2 ? -28 : 92), 7);  tp_tiles(TP_GB + c, TT_GB0 + c * 7, 7, TT_PAL_GEAR); }
    if (big) { for (int c = 0; c < 16; c++) { tp_col(TP_LG + c, 32 + 16 * c, 72, 4);  tp_tiles(TP_LG + c, TT_LG0 + c * 4, 4, TT_PAL_GOLD); } }
    else     { for (int c = 0; c < 9; c++)  { tp_col(TP_LG + c, 88 + 16 * c, (mode == 2 ? 0 : 16), 2); tp_tiles(TP_LG + c, TT_SL0 + c * 2, 2, TT_PAL_GOLD); } }
    tp_st = TP_STB; tp_avoid = 0;
    tp_stars_init();
    tp_ga = 0; tp_gb = 0; tp_lf = 0; tp_lt = 0; tp_t = 0;
    *((volatile uint16_t *)0x401ffe) = 0x0000;
}

static void tp_build(int big) { tp_build_m(big); }

/* one frame of movement: gears turn, the shine runs along the lettering after a rest, stars twinkle */
static void tp_anim(void) {
    int nf = tp_big ? 32 : 24, hold = tp_big ? 120 : 90;
    tp_t++;
    if ((tp_t & 3) == 0)  { if (++tp_ga >= 12) { tp_ga = 0; } tp_gear_a(); }
    if (tp_t % 6 == 2)    { if (++tp_gb >= 12) { tp_gb = 0; } tp_gear_b(); }
    if (++tp_lt >= 2) {
        tp_lt = 0;
        if (tp_lf > 0 || tp_t % (nf * 2 + hold) == 0) {
            if (++tp_lf >= nf) { tp_lf = 0; }
            tp_logo();
        }
    }
    tp_stars_step();
}
/* The title page, menu and options dim after a minute without a press, always to the Dim level and whatever the player's own screensaver
 * option says (that option, its delay and its timer are not touched). Any press wakes it and does nothing else. */
#define MENU_DIM_FRAMES (60 * 59)
static int md_idle, md_lvl = 8;
static int tp_ch(void) {                            /* this frame's presses: bit 8 is Start; zero when the press only woke the screen */
    int ch = bios_p1change | (START_HIT() ? 0x100 : 0);
    if (ch || bios_p1current) {
        md_idle = 0;
        if (md_lvl < 8) { md_lvl = 8; tp_pals(8); return 0; }
    }
    return ch;
}
static void tp_fade(int from, int to);
static void tp_frame(void) {
    int dim;
    tp_anim(); play_tick();
    dim = (md_lvl == 8 && tp_lvl == 8 && ++md_idle >= MENU_DIM_FRAMES);
    ng_wait_vblank();
    if (dim) { md_lvl = 2; tp_fade(8, 2); }
}
/* A fade moves in steps of two eighths: each step rewrites the whole page's palettes over several frames (see tp_pals). */
static void tp_fade(int from, int to) {
    int step = (to > from) ? 2 : -2;
    for (int l = from; l != to + step; l += step) { tp_pals(l); }
}

/* plates: a gold one for the choice, steel for the others; text goes on top in the fix layer */
static void tp_plate(int spr, int x, int y, int cols, int gold) {
    for (int c = 0; c < cols; c++) {
        int k = (c == 0) ? 0 : ((c == cols - 1) ? 2 : 1);
        tp_col(spr + c, x + 16 * c, y, 2);
        tp_tiles(spr + c, TT_PL0 + (gold ? 0 : 6) + k * 2, 2, gold ? TT_PAL_PGOLD : TT_PAL_PSTEEL);
    }
}
static void tp_text(int row, const char *s, int pal) { ng_center_text(row, pal, s); }
static void tp_blank_row(int row) { ng_text(0, row, 0, "                                        "); }

/* ---- Gilded layout: the title page art behind the player screen. The sprites are dimmed so the text stays readable.
 * Extras and ambient effects are off in this layout (the Foundry writes theme byte 21 that way). ---- */
static void gl_build(void) { tp_build_m(2); }
static void gl_anim(void) { tp_anim(); }
static void gl_unit(int u, int lvl) {                   /* one palette of the Gilded backdrop (GL_NPAL of them) */
    int lb = mul16(lvl, 3) >> 3, lg = mul16(lvl, 5) >> 3;
    if (u < 56)        { tp_pal16(TT_PAL_BG + u, tt_bg_pal[u], lb); }
    else if (u == 56)  { tp_pal16(TT_PAL_GEAR, tt_gear_pal, lg); }
    else if (u == 57)  { tp_pal16(TT_PAL_GOLD, spin_pal, lvl); }
    else if (u == 58)  { tp_solid(TT_PAL_STAR, 0x0fff, lg); }
    else               { tp_solid(TT_PAL_STAR + 1, 0x0fe9, lg); }
}
static void gl_pals_part(int k) {
    int a = k * PAL_SLICE, b = a + PAL_SLICE;
    if (b > GL_NPAL) { b = GL_NPAL; }
    for (int u = a; u < b; u++) { gl_unit(u, fade_lvl); }
}
static void gl_pals(int lvl) {
    for (int u = 0; u < GL_NPAL; u++) { gl_unit(u, lvl); }
}

/* ---- the new layouts (MVS Cabinet, Neon Horizon, Aurora): data in skins_data.h, made by tools/gen_skins.py ----
 * A layout is a few layers of sprites behind the normal player screen. Each layer is a row of 16 pixel wide sprite
 * columns; what a column shows is chosen every frame by the layer's rule (see sk_frame). The Foundry's preview
 * follows exactly the same rules. The first tick of a layout is written in sk_build(), after gx_init() has hidden
 * every sprite. Extras and ambient effects are off in these layouts, like Gilded. ---- */
#define SK_MAXL 16
#define SK_MAXC 20
static uint8_t sk_cf[SK_MAXL][SK_MAXC];       /* the frame each column shows now (255 = not written yet) */
static uint32_t sk_mph[SK_MAXL];              /* movers: ticks into the cycle (the rest between passes can be an hour long) */
static uint8_t sk_inited;
static uint8_t sk_mpass[SK_MAXL], sk_mvis[SK_MAXL];
static uint8_t sk_state[SK_STATE_N], sk_pulse[4];
static uint16_t sk_t;
#define SK_SLICE 8                            /* palettes written per piece (never more than fit between two frames) */

static const sk_skin_t *sk_get(void) { return &sk_skins[SKIN_ID - SK_FIRST_ID]; }
static int sk_slices(void) { const sk_skin_t *k = sk_get(); return (k->npal + SK_SLICE - 1) / SK_SLICE; }
static void sk_pals_part(int k) {
    const sk_skin_t *sk = sk_get();
    int a = k * SK_SLICE, b = a + SK_SLICE;
    if (saver_anim) { return; }                                       /* an animated screensaver has the sprites */
    if (b > sk->npal) { b = sk->npal; }
    for (int i = a; i < b; i++) { tp_pal16(sk->pslot[i], sk->pal[i], fade_lvl); }
    if (sk->stars && b == sk->npal) {
        tp_solid(TT_PAL_STAR, 0x0fff, fade_lvl);
        tp_solid(TT_PAL_STAR + 1, 0x0fe9, fade_lvl);
    }
}
static void sk_pals_all(void) {
    for (int k = 0, n = sk_slices(); k < n; k++) { sk_pals_part(k); }
}

static int sk_frame(const sk_layer_t *L, int c) {
    int per = (playing >= 0) ? L->period_play : L->period;
    if (per < 1) { per = 1; }
    if (L->kind == 1) { return (sk_t / per) % L->nframes; }                    /* loop */
    if (L->kind == 2) { return L->lut[(mul16(c, L->cstep) + sk_t / per) % L->lutlen]; }   /* chase */
    if (L->kind == 3) { int v = sk_state[L->src]; return v < L->nframes ? v : L->nframes - 1; }   /* state */
    return 0;
}
static void sk_write_tiles(const sk_layer_t *L, int c, int f) {
    uint16_t base = (uint16_t)(L->tile0 + mul16(f, L->fstride) + mul16(c, L->cstride));
    uint16_t at = (uint16_t)(SCB1 + (L->spr + c) * 64 + L->row0 * 2);
    for (int r = 0; r < L->rows; r++) { vwrite((uint16_t)(at + r * 2), (uint16_t)(base + r)); }
}
static void sk_place(const sk_layer_t *L, int c, int x, int y) {
    vwrite((uint16_t)(SCB4 + L->spr + c), (uint16_t)((x & 0x1ff) << 7));
    vwrite((uint16_t)(SCB3 + L->spr + c), (uint16_t)((((496 - y) & 0x1ff) << 7) | L->rows));
}
static uint8_t sk_inited_id;
static void sk_build(void) {
    const sk_skin_t *sk = sk_get();
    if (sk_inited_id != SKIN_ID) { sk_inited = 0; sk_inited_id = (uint8_t)SKIN_ID; }       /* a different layout: start its movers from scratch */
    for (int l = 0; l < sk->nlayers; l++) {
        const sk_layer_t *L = &sk->layers[l];
        if (!sk_inited && (L->flags & 2)) { sk_mph[l] = (uint32_t)mul16(L->msteps, L->mper); sk_mpass[l] = 0; }   /* a mover starts out resting; later redraws keep its timer */
        sk_mvis[l] = 0;
        for (int c = 0; c < L->cols; c++) { sk_cf[l][c] = 255; }
        if (!(L->flags & 1)) { continue; }
        for (int c = 0; c < L->cols; c++) {
            int spr = L->spr + c, f = sk_frame(L, c);
            vwrite((uint16_t)(SCB2 + spr), 0x0fff);
            for (int r = 0; r < L->rows; r++) {
                int pal = L->attr ? L->attr[mul16(c, L->rows) + r] : L->pal;
                vwrite((uint16_t)(SCB1 + spr * 64 + r * 2 + 1), (uint16_t)(pal << 8));
            }
            sk_write_tiles(L, c, f);
            sk_cf[l][c] = (uint8_t)f;
            if (L->flags & 2) { vwrite((uint16_t)(SCB3 + spr), 0); }       /* movers start out of sight */
            else { sk_place(L, c, L->x + 16 * c, L->y); }
        }
    }
    sk_inited = 1;
    if (sk->stars) { tp_st = TP_ST_SK; tp_avoid = (SKIN_ID == 3); tp_stars_init(); }
}
static void sk_mover(int l, const sk_layer_t *L) {
    int move = mul16(L->msteps, L->mper);
    uint32_t ph = sk_mph[l];
    if (ph < (uint32_t)move) {
        int k = (int)ph / L->mper;
        if ((int)ph % L->mper == 0 || !sk_mvis[l]) {
            int x = L->mx0 + mul16(L->mdx, k), y = L->mys[sk_mpass[l]] + mul16(L->mdy, k);
            for (int c = 0; c < L->cols; c++) { sk_place(L, c, x + 16 * c, y); }
            sk_mvis[l] = 1;
        }
    } else if (sk_mvis[l]) {
        for (int c = 0; c < L->cols; c++) { vwrite((uint16_t)(SCB3 + L->spr + c), 0); }
        sk_mvis[l] = 0;
    }
    if (++ph >= (uint32_t)move + L->mgap) { ph = 0; if (++sk_mpass[l] >= L->mysn) { sk_mpass[l] = 0; } }
    sk_mph[l] = ph;
}
static void sk_press(int ch) {                      /* a button press makes its button on the screen flash */
    if (ch & BTN_A) { sk_pulse[0] = 9; }
    if (ch & BTN_B) { sk_pulse[1] = 9; }
    if (ch & BTN_C) { sk_pulse[2] = 9; }
    if (ch & BTN_D) { sk_pulse[3] = 9; }
}
static void sk_tick(void) {
    const sk_skin_t *sk = sk_get();
    uint8_t cur = bios_p1current;
    int dx = ((cur & BTN_RIGHT) ? 1 : 0) - ((cur & BTN_LEFT) ? 1 : 0), dy = ((cur & BTN_DOWN) ? 1 : 0) - ((cur & BTN_UP) ? 1 : 0);
    sk_t++;
    sk_state[1] = playing >= 0;
    sk_state[2] = playing < 0;
    sk_state[3] = repeat_all != 0;
    sk_state[4] = shuffle != 0;
    sk_state[5] = (uint8_t)((dy + 1) * 3 + dx + 1);                       /* the joystick: 4 is the middle */
    for (int b = 0; b < 4; b++) {
        if (sk_pulse[b]) { sk_pulse[b]--; }
        sk_state[6 + b] = sk_pulse[b] ? 2 : 0;           /* a button is either up or pressed (no lit frame) */
    }
    for (int l = 0; l < sk->nlayers; l++) {
        const sk_layer_t *L = &sk->layers[l];
        if (L->kind != 0) {
            int f0 = (L->kind == 2) ? 0 : sk_frame(L, 0);
            for (int c = 0; c < L->cols; c++) {
                int f = (L->kind == 2) ? sk_frame(L, c) : f0;
                if (f != sk_cf[l][c]) { sk_cf[l][c] = (uint8_t)f; sk_write_tiles(L, c, f); }
            }
        }
        if (L->flags & 2) { sk_mover(l, L); }
    }
    if (sk->stars) { tp_stars_step(); }
}

/* ---- the title page: any button leaves it ---- */
static void tp_title_page(void) {
    ng_cls();
    tp_pals(0);
    tp_build(1);
    tp_text(21, "NEO GEO MEDIA CENTRE", FP_GOLD);
    tp_text(23, "HOME-BREW MUSIC PLAYER", FP_SILV);
    tp_fade(0, 8);
    for (int t = 0; ; t++) {
        if (tp_ch()) { break; }
        if (t % 60 == 0)  { tp_text(27, "PRESS ANY BUTTON", FP_WHITE); }
        if (t % 60 == 36) { tp_blank_row(27); }
        tp_frame();
    }
    tp_fade(8, 0);
}

/* ---- the start menu ---- */
static void tp_menu_draw(int sel) {
    tp_plate(TP_PL, 96, 100, 8, sel == 0);
    tp_plate(TP_PL + 8, 96, 132, 8, sel == 1);
    tp_text(16, "START NGMC", sel == 0 ? FP_DARK : FP_SILV);
    tp_text(20, "OPTIONS", sel == 1 ? FP_DARK : FP_SILV);
}
static int tp_menu_page(void) {
    int sel = 0, res = -1;
    ng_cls();
    tp_pals(0);
    tp_build(0);
    tp_text(9, "NEO GEO MEDIA CENTRE", FP_GOLD);
    tp_text(26, "UP/DOWN CHOOSE   A SELECT", FP_DIM);
    tp_menu_draw(sel);
    tp_fade(0, 8);
    for (int redraw = 0; res < 0; ) {
        int ch = tp_ch();
        if ((ch & BTN_UP) && sel > 0)        { sel--; redraw = 1; }
        if ((ch & BTN_DOWN) && sel < 1)      { sel++; redraw = 1; }
        if (ch & (BTN_A | BTN_C | BTN_D))    { res = sel; }
        if (redraw) { redraw = 0; tp_menu_draw(sel); }
        tp_frame();
    }
    tp_fade(8, 0);
    return res;
}

/* ---- options ---- */
static void tp_opt_row(int row, const char *label, const char *val, int on) {
    char buf[24]; int n = 0;
    ng_text(3, row, on ? FP_DARK : FP_SILV, label);
    if (on) { buf[n++] = '<'; buf[n++] = ' '; }
    for (const char *p = val; *p; p++) { buf[n++] = *p; }
    if (on) { buf[n++] = ' '; buf[n++] = '>'; }
    buf[n] = 0;
    ng_text((uint8_t)(36 - n), row, on ? FP_DARK : FP_SILV, buf);
}
static void skin_label(char *out) {                    /* the current look's name, trimmed, for the options row */
    int n = 0;
    if (ngmc_skins.count > 1) {
        for (int i = 0; i < SKIN_NAME_LEN; i++) { out[n++] = ngmc_skins.name[cur_skin][i]; }
        while (n > 0 && out[n - 1] == ' ') { n--; }
    } else {
        out[n++] = 'O'; out[n++] = 'N'; out[n++] = 'L'; out[n++] = 'Y'; out[n++] = ' '; out[n++] = 'O'; out[n++] = 'N'; out[n++] = 'E';
    }
    out[n] = 0;
}
static void tp_opt_draw(int sel) {
    char nm[SKIN_NAME_LEN + 1];
    tp_plate(TP_PL, 8, 60, 19, sel == 0);
    tp_plate(TP_PL + 19, 8, 84, 19, sel == 1);
    tp_plate(TP_PL + 38, 8, 108, 19, sel == 2);
    tp_plate(TP_PL + 57, 8, 132, 19, sel == 3);
    tp_plate(TP_PL + 76, 96, 164, 8, sel == 4);
    for (int r = 11; r <= 20; r += 3) { ng_text(2, r, 0, "                                      "); }
    skin_label(nm);
    tp_opt_row(11, "SKIN", nm, sel == 0);
    tp_opt_row(14, "SCREENSAVER", opt_names[opt_si], sel == 1);
    tp_opt_row(17, "STARTS AFTER", opt_secs_names[opt_ti], sel == 2);
    ng_text(3, 20, sel == 3 ? FP_DARK : FP_SILV, xu("\x3b\x2a\x25\x2f\x24\x39\x2a\x4c\x38\x4b\x29\x24\x33"));
    ng_text(27, 20, sel == 3 ? FP_DARK : FP_SILV, sel == 3 ? "< OPEN >" : "  OPEN  ");
    tp_text(24, "BACK", sel == 4 ? FP_DARK : FP_SILV);
}
/* returns 1 if the options extra page was chosen, 0 for back */
static int tp_options_page(int sel0) {
    int sel = sel0, done = 0, res = 0;
    ng_cls();
    tp_dimbg = 0;
    tp_pals(0);
    tp_build(0);
    tp_text(9, "OPTIONS", FP_GOLD);
    tp_text(27, "UP/DOWN CHOOSE   LEFT/RIGHT CHANGE", FP_DIM);
    tp_text(29, "B BACK", FP_DIM);
    tp_opt_draw(sel);
    tp_fade(0, 8);
    for (int redraw = 0; !done; ) {
        int ch = tp_ch();
        if ((ch & BTN_UP) && sel > 0)    { sel--; redraw = 1; }
        if ((ch & BTN_DOWN) && sel < 4)  { sel++; redraw = 1; }
        if (ch & BTN_B) { done = 1; }
        if (sel == 0 && ngmc_skins.count > 1 && (ch & (BTN_LEFT | BTN_RIGHT))) {
            int n = ngmc_skins.count;
            skin_pick((cur_skin + ((ch & BTN_RIGHT) ? 1 : n - 1)) % n); redraw = 1;
        }
        if (sel == 1 && (ch & (BTN_LEFT | BTN_RIGHT))) {
            opt_si += (ch & BTN_RIGHT) ? 1 : 10;
            if (opt_si > 10) { opt_si -= 11; }
            opt_saver = opt_ids[opt_si]; redraw = 1;
        }
        if (sel == 2 && (ch & (BTN_LEFT | BTN_RIGHT))) {
            opt_ti += (ch & BTN_RIGHT) ? 1 : 4;
            if (opt_ti > 4) { opt_ti -= 5; }
            opt_secs = opt_secs_tab[opt_ti]; redraw = 1;
        }
        if (sel == 3 && (ch & (BTN_A | BTN_C | BTN_D | BTN_RIGHT))) { done = 1; res = 1; }
        if (sel == 4 && (ch & (BTN_A | BTN_C | BTN_D))) { done = 1; }
        if (redraw) { redraw = 0; tp_opt_draw(sel); }
        tp_frame();
    }
    tp_fade(8, 0);
    return res;
}

/* ---- the options extra page: every extra has a row; locked rows are all asterisks until the extra has shown up ---- */
static void tp_pb_row(int i, int sel) {
    int row = 11 + i;
    ng_text(2, row, 0, "                                      ");
    if (!pb_found[i]) { ng_text(4, row, FP_DIM, "********************************"); return; }
    {
        const char *v = pb_hidden[i] ? "OFF" : "ON";
        int pal = (sel == i) ? FP_WHITE : FP_SILV;
        if (sel == i) { ng_text(4, row, FP_GOLD, ">"); }
        ng_text(6, row, pal, xu(pb_names[i]));
        ng_text(pb_hidden[i] ? 31 : 32, row, (sel == i) ? FP_GOLD : pal, v);
    }
}
static void tp_pb_draw(int sel) {
    for (int i = 0; i < PB_N; i++) { tp_pb_row(i, sel); }
    tp_text(27, sel == PB_N ? ">BACK<" : " BACK ", sel == PB_N ? FP_GOLD : FP_SILV);
}
static int tp_pb_step(int sel, int dir) {           /* next row in that direction that can be chosen (found rows, then BACK) */
    int s = sel;
    for (;;) {
        s += dir;
        if (s < 0 || s > PB_N) { return sel; }
        if (s == PB_N || pb_found[s]) { return s; }
    }
}
static void tp_xpage(void) {
    int sel = PB_N, done = 0;
    for (int i = 0; i < PB_N; i++) { if (pb_found[i]) { sel = i; break; } }
    ng_cls();
    tp_dimbg = 1;
    tp_pals(0);
    tp_build(0);
    tp_text(9, xu("\x3b\x2a\x25\x2f\x24\x39\x2a\x4c\x38\x4b\x29\x24\x33"), FP_GOLD);
    tp_text(29, "UP/DOWN CHOOSE  L/R CHANGE  B BACK", FP_DIM);
    tp_pb_draw(sel);
    tp_fade(0, 8);
    for (int redraw = 0; !done; ) {
        int ch = tp_ch();
        if (ch & BTN_UP)   { int s = tp_pb_step(sel, -1); if (s != sel) { sel = s; redraw = 1; } }
        if (ch & BTN_DOWN) { int s = tp_pb_step(sel, 1);  if (s != sel) { sel = s; redraw = 1; } }
        if (ch & BTN_B) { done = 1; }
        if (sel == PB_N) { if (ch & (BTN_A | BTN_C | BTN_D)) { done = 1; } }
        else if (ch & (BTN_LEFT | BTN_RIGHT | BTN_A | BTN_C | BTN_D)) { pb_hidden[sel] ^= 1; redraw = 1; }
        if (redraw) { redraw = 0; tp_pb_draw(sel); }
        tp_frame();
    }
    tp_fade(8, 0);
    tp_dimbg = 0;
}

static void opt_init(void) {
#ifdef PB_DEMO
    pb_found[1] = 1; pb_found[3] = 1; pb_found[10] = 1; pb_hidden[3] = 1;   /* test builds only */
#endif
    opt_saver = SAVER_MODE;
    opt_si = 0;
    for (int i = 0; i < 11; i++) { if (opt_ids[i] == opt_saver) { opt_si = i; } }
    opt_ti = 4;
    for (int i = 4; i >= 0; i--) { if (opt_secs_tab[i] >= TH->saver_secs) { opt_ti = i; } }
    opt_secs = opt_secs_tab[opt_ti];
    if (TH->saver_secs == 0) { opt_saver = SAVER_OFF; opt_si = 0; }
}

/* title page, then the menu; returns when the player should start */
static void title_flow(int skip_title) {
    if (!skip_title) { tp_title_page(); }
    for (;;) {
        if (tp_menu_page() == 0) { break; }
        for (int sel = 0; tp_options_page(sel); sel = 3) { tp_xpage(); }
    }
    for (int n = 4; n < N_SPRITES; n++) { vwrite(SCB3 + n, 0); }
    ng_cls();
    fade_lvl = 0; fade_tgt = 8; fade_tick = 0;      /* the player is built in the dark and then fades in */
    apply_palettes();
}

/* ---- back to the menu from the player: ask first. A box floats over the playlist; the player screen is redrawn
 * if the answer is no. Fix palettes 11..15 belong to the title code and are set up here. ---- */
static void enter_player(int fadein) {            /* fadein: build the screen in the dark, then bring it up */
    fade_lvl = fadein ? 0 : 8; fade_tgt = 8; fade_keep = 0;
    fade_tick = fadein ? -12 : 0;                   /* stay dark for about half a fade before the colours come up */
    gx_init();                    /* hides every sprite, so it comes before anything that draws one */
    amb_init();
    apply_palettes();
    draw_frame();
    dirty_list = 1; dirty_now = 1; dirty_bar = 1;
    glint_drawn = -1;
    idle = 0;
}
/* Start: back to the title menu. The music carries on (the pages run play_tick) and the player comes back as it was. */
static void go_menu(void) {
    gx_hide();
    amb_hide();
    title_flow(1);
    enter_player(1);
}

int main(void) {
    *REG_SOUND = CMD_RESET;
    if (ngmc_skins.count > 1 && ngmc_skins.start > 0 && ngmc_skins.start < ngmc_skins.count) { skin_pick(ngmc_skins.start); }
    amb_resolve();                 /* which ambient effect, from the theme and the title */

    fade_lvl = 8;
    fade_tgt = 8;
    fade_tick = 0;
    fade_keep = 0;
    apply_palettes();

    count = T->count;
    if (count > MAX_TRACKS) { count = MAX_TRACKS; }
    sel = 0;
    top = 0;
    playing = -1;
    repeat_all = 1;
    shuffle = 0;
    shuf_n = 0;
    name_rev = 99;
    elapsed_frames = 0;
    elapsed_sec = 0;
    sec_acc = 0;
    bar_fill = 0;
    bar_acc = 0;
    bar_step = 1;
    cog_frame = 0;
    cog_tick = 0;
    dirty_bar = 0;
    belt_pos = 0;
    belt_dir = 1;
    belt_hold = 0;
    belt_tick = 0;
    sweep_t = SWEEP_PERIOD;
    glint_t = 200;                 /* first glint a few seconds after power on */
    glint_drawn = -1;
    mq_t = 0;
    mq_sel = -1;
    mq_off = 0;
    rng = 0xACE1;
    tick = 0;
    idle = 0;
    saver_on = 0;
    saver_anim = 0;
    saver_cur = 0;

    opt_init();
    title_flow(0);
    enter_player(1);

    for (;;) {
        uint8_t ch = bios_p1change;

        if (pal_part < pal_total_parts()) { apply_palettes_part(pal_part++); }     /* first thing after the frame starts */

        if (SKIN_NEW && !saver_on) { sk_press(ch); }

        /* idle time, and waking the screensaver: the waking press does nothing else */
        if (ch || bios_p1current || START_HIT()) { idle = 0; } else { idle++; }
        if (saver_on) {
            if (ch || bios_p1current || START_HIT()) { saver_stop(); ch = 0; }   /* held counts too: a press is not lost if a frame runs long */
        } else if (opt_saver != SAVER_OFF && opt_secs > 0 &&
                   idle >= (uint32_t)opt_secs * 59) {
            saver_start();
        }

        if (START_HIT() && !saver_on) { go_menu(); ch = 0; }   /* Start: back to the menu, the music plays on */

        if (count > 0) {
            if (ch & BTN_UP)         { if (sel > 0) { sel--; dirty_list = 1; } }
            else if (ch & BTN_DOWN)  { if (sel < count - 1) { sel++; dirty_list = 1; } }

            if (ch & BTN_A)          { play_track(sel); shuf_n = 0; }
            else if (ch & BTN_B)     { stop_track(); }
            else if (ch & BTN_C)     { repeat_all = !repeat_all; dirty_now = 1; }
            else if (ch & BTN_D)     { shuffle = !shuffle; shuf_n = 0; dirty_now = 1; }
            else if (ch & BTN_RIGHT) { next_track(); }
            else if (ch & BTN_LEFT)  { prev_track(); }
        }

        play_tick();

        tick++;
        flash_update();
        gx_update();
        amb_update();

        /* fade towards the dim level (or back up) one step at a time; the colours are written a piece per frame */
        if (fade_lvl != fade_tgt && ++fade_tick >= fade_per()) {
            fade_tick = 0;
            fade_lvl += (fade_tgt > fade_lvl) ? 1 : -1;
            pal_part = 0;
        }

        if (saver_anim) {
            saver_step();
        } else {
            /* the cogs always turn, faster while music plays */
            int cog_period = playing >= 0 ? (TH->cog_play & 0x3f) : TH->cog_idle;
            if (cog_period < 1) { cog_period = 1; }
            if (++cog_tick >= cog_period) {
                cog_tick = 0;
                cog_frame = (cog_frame + 1) & 3;
                if (!SKIN_ANY) { draw_cogs(); }
            }

            update_belts();
            if (GILDED) { gl_anim(); } else { update_title(); if (SKIN_NEW) { sk_tick(); } }
            if (name_rev < 34 && (tick & 1) == 0) { name_rev++; dirty_now = 1; }       /* type the name in */

            /* a long name in the list scrolls: hold, slide along, hold, start again */
            if (sel != mq_sel) { mq_sel = sel; mq_t = 0; mq_off = 0; dirty_list = 1; }
            if (count > 0) {
                int len = 0;
                while (len < NAME_LEN && T->track[sel].name[len]) { len++; }
                if (len > NAME_W) {
                    int span = len - NAME_W, off;
                    mq_t++;
                    off = (mq_t - 90) / 12;
                    if (off < 0) { off = 0; }
                    if (off > span) { off = span; }
                    if (mq_t > 90 + 12 * span + 90) { mq_t = 0; off = 0; }
                    if (off != mq_off) { mq_off = off; dirty_list = 1; }
                }
            }

            if (dirty_list && count > 0) { draw_list(); dirty_list = 0; }
            if (dirty_now)               { draw_now();  dirty_now = 0; }
            if (dirty_bar)               { draw_progress(); dirty_bar = 0; }
        }

        ng_wait_vblank();
    }
    return 0;
}
