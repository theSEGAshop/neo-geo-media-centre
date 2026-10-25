# NGMC version record

Which Foundry goes with which player. Update this every time a new file is made.
Rule: always use a Foundry from the same row as your player. "Works with" means the Foundry can fill that player's ROM with music and apply its look.

| Player zip | Foundry file(s) | Notes |
|---|---|---|
| ngmc-player-01 | neogeo-media-center-foundry-01 | First numbered pair. |
| ngmc-player-02 | neogeo-media-center-foundry-02 | Smooth title saver, light show, gold lettering, splash. |
| ngmc-player-03 | neogeo-media-center-foundry-03, -04, -05 | Adds shuffle (D), name typing, Winter Workshop skin, tag titles. Foundry 03 was the first made for it. |
| ngmc-player-04 | neogeo-media-center-foundry-06 | Warp starfield replaces Plasma, lighter light show, wider belts, held button wakes saver. Foundry 06 adds the 8 kHz floor and drop limits. Use Foundry 06 with it: older Foundries still call saver 5 Plasma and draw the old belts. |
| ngmc-player-05 | neogeo-media-center-foundry-07 | Warp starfield reworked (random appearances, accelerating rush) plus a rare visitor. Foundry 07 draws the same. |
| ngmc-player-06 | neogeo-media-center-foundry-08 | Glossy title page, start menu, options page (screensaver and timer, RAM only) and the RETURN TO MENU box. Foundry 08 previews all of them ("Show title page", keys or pad buttons). |
| ngmc-player-07 | neogeo-media-center-foundry-09 | RETURN TO MENU? box floats over the playlist (your choice from the two mock-ups). Foundry 09 previews it the same way. |
| ngmc-player-08 | neogeo-media-center-foundry-10 | TEST: Gilded skin and layout (title page art behind the player screen). Other skins unchanged; if Gilded disappoints, go back to player 07 + Foundry 09. |
| ngmc-player-09 | neogeo-media-center-foundry-11 | A new options page for the extras and the Foundry's extras tickbox removed. Foundry 11 also fixes the track-name cursor bug. Includes the Gilded test from 08. |
| ngmc-player-10 | neogeo-media-center-foundry-12 | Fixes the leftover tile data under the title-page stars, warp stars and light show orbs. The Foundry is unchanged from the player 09 pair (use 12). |

## Foundry-only changes (player stays the same)
- Foundry 04: Show splash preview animates the snow and other ambient effects (backlog 19).
- Foundry 05: Amber Terminal replaced by MVS Arcade skin (backlog 20). Includes the 04 fix.
- Foundry 12: the Gilded note no longer mentions the extras. Use with player 09, same as Foundry 11.
- Foundry 13: automatic fit toggle ("Fit automatically", on by default), re-fits the whole list on every drop or removal; rates set by hand stay put. Player unchanged: use with player 10.

- Player 11: fixes the grey stripes seen on real hardware going from the start menu into the player (backlog 45): the player screen is built in the dark and fades in. Foundry unchanged: use with Foundry 13. Needs a hardware look.

- Player 12: second try at the grey bars (45): the player stays dark for half a fade, and fade steps write colours in three pieces, one per frame. A test; fall back to player 11 if it behaves worse. Foundry unchanged: use with Foundry 13.

- Player 13 + Foundry 14 (test): three new full-screen skin layouts: MVS Cabinet, Neon Horizon and Aurora (backlog 48-50). The Foundry previews them (animated, d-pad and buttons react) and writes the layout number into the theme table. An older player ROM gets the colours only, with a note.
- Foundry 15: the MAME package puts the cartridge in roms/ (no neogeo folder). Player unchanged: use with player 13.
- Player 14 + Foundry 16: the name is spelled "Media Centre" everywhere (screens, Foundry, file names). Same features as player 13 + Foundry 15.
- Foundry 17: last visible "center" spellings changed to "centre" (default file names for saved projects and exports, instructions). Player unchanged: use with player 14.

## Newest pair to use
Player 14 + Foundry 17 (new skins are a test; fall back to player 12 + Foundry 13, or player 10 + Foundry 13 which is hardware-proven).

## Not yet verified
- Nothing from 02 onward has been tested in MAME or on a flash cart yet.
- How older Foundries behave with newer players (and the reverse) hasn't been tested. The Foundry shows a note when it detects a player older than its features.

Hardware: Player 10 + Foundry 13 confirmed running on a real NeoSD Pro via neosdconv (2026-10-24).
- Foundry 18 (test): use with player 15. MVS Cabinet rebuilt from the approved mock-up (marquee with the NGMC plate and equaliser, symmetrical control panel, working stick and buttons, small hand-drawn font for the now-playing box and help line, no belt bars). Neon Horizon and Aurora extras are much rarer. Secrets are no longer readable in the page source, and the player source and README no longer name them.
- Foundry 19 (test): use with player 16. Needs player 16 for the skin picker. The cartridge carries the clicked skin plus the original colour skins, with a Start on choice and a Start button in the preview (music carries on behind the menus, no confirm box). New ROM-space meters for MVS Cabinet, Neon Horizon and Aurora.
- Player 16 (test): music starts by itself and plays through the title page, menus and options; Start returns to the menu (no confirm box); a SKIN row on the options page switches between the packaged look and the original colour skins. The Start button register address is unchecked on real hardware.

- Foundry 20 (test): use with player 17. Preview matches player 17: narrower MVS Cabinet playlist, evenly spaced button strip, buttons move without lighting, menu Dim after a minute.
- Player 17 (test): MVS Cabinet layout fixes (playlist and now-playing fit inside the panel, even button strip, no lit buttons, marquee sweep about every ten seconds); title, menus and options dim after a minute idle (wake on any press, music carries on).
- Foundry 21 (test): use with player 18. Winter Workshop is now the icy blue palette (candy and baubles stay red and white), the old green/red/gold palette is HOLLY & GOLD (was Cold Frost), Sunset Boulevard sits with the older skins, preview matches player 18.
- Player 18 (test): Aurora and Neon Horizon playlist text inside their panels; Neon Horizon stars stay off the sun; title and menu stars behind the cogs; title line 2 reads HOME-BREW MUSIC PLAYER so all lines are centred.

- Foundry 22 (test): use with player 18 (player unchanged). Fixes Foundry 21: Winter Workshop's belts were blue; its main colour is red again, the rest of the palette is the icy blue.

- Foundry 23 (test): use with player 19. Winter Workshop's belts are red and white again (via the player's new belt-colour bit) while the text stays icy blue. Needs player 19.
- Player 19 (test): belt colour bit (bit 7 of belt_idle) for the above.

- Foundry 24 (test): use with player 20. Preview matches (Neon Horizon and Aurora text lower, nine rows), MVS Arcade is now just ARCADE in the skin list and the player's skin picker.
- Player 20 (test): Neon Horizon and Aurora text sits inside the box (nine list rows); Neon Horizon saucer is smaller and flies between the title and the top band.

- Foundry 25 (test): use with player 21. All twelve skins on every cartridge (picker and Start on), each with its own cogs, belts and cobwebs; the standard, Halloween and Winter fix art are all written to the S ROM. Works with older players in the old way (eight original skins only).
- Player 21 (test): picker with twelve looks, run-time switching between layouts, per-look fix art (theme version 3).

- Foundry 26 (test): use with player 21 (player unchanged). Neon Arcade is renamed Ultraviolet.

- Foundry 35 (test): use with player 23. Licence: footer and page header now carry "(c) 2026 bLiGhTy, LGPL-3.0-or-later, built with ngdevkit by Damien Ciabrini". Nothing else changed. The player ROM is unchanged (player 23); main.c gained a licence comment only.
- Foundry 34 (test): use with player 23. Tidy-up: the Track ledger list always has room for about five rows and scrolls inside itself (the left column only sticks on windows 1100 px or taller); screensaver names shortened so they fit their box; new intro line; footer with the version and font credits.
- Foundry 33 (test): use with player 23. Fonts built into the page (IM Fell English SC, Libre Baskerville, Special Elite; Latin subsets, about 230 KB) so it looks the same offline. Fixes the right-hand panel running off the screen at 1180-1500 px wide, the stray "Choose File" box by Load project, and an out-of-date line in the Test bench. Font licences: SIL OFL 1.1 for the first two, Apache 2.0 for Special Elite; credit them in the public docs.
- Foundry 32 (test): use with player 23. Removes the "ROM files for NeoBuilder" export button (the .neo button replaces it). Exports left: Build .neo, Build MAME set, and the raw audio files under Advanced.
- Foundry 31 (test): use with player 23, which is built into the page (no player files to add). First streamlined Foundry: the player ROM is embedded, the chooser is gone, the raw audio buttons sit under Advanced. To embed a newer player: python3 embed-player.py foundry.html ngmc.zip neogeo.xml N out.html.
- Player 23 (hardware-proven) + Foundry 30: Player 23 fixes the grey noise during fades and dims (item 79, details in its README). Foundry 30 is identical to Foundry 29; the number only marks the pairing with player 23. Rebuild the player once (Step 1) to get the fix.
- Foundry 29 (test): use with player 22. Same as 28 but the .neo button is no longer "experimental" (it booted on a NeoSD and matches neosdconv's ROM data); instructions updated.
- Foundry 28 (test): use with player 22 (player unchanged). Skin names lose the "(title page look)" and "(new layout)" tags; Gilded is the default for a brand-new Foundry; new Maker box beside the Cartridge title (item 77) so the .neo header and the neosdconv line carry your own maker text.
- Foundry 27 (test): use with player 22. New page layout (Test bench under the Track ledger, three more even columns, the Cartridge look settings in two columns) and the Game of Life preview matches player 22.
- Player 22 (test): Game of Life screensaver: finer rounded dots, age colours, fading trails, half speed.

Player 23 + Foundry 35 (Player 23 + Foundry 30 hardware-proven: the grey noise is gone on the NeoSD). Before that: Player 21 + Foundry 26 (not yet tried; Foundry 25 is identical apart from the skin name).
Player 21 + Foundry 25 (not yet tried). Before that: Player 20 + Foundry 24 (works as far as I know).
Player 20 + Foundry 24 (not yet tried). Before that: Player 19 + Foundry 23 (works as far as I know).
Player 19 + Foundry 23 (not yet tried). Before that: Player 18 + Foundry 22 (belts and text both red in Winter Workshop).
Player 18 + Foundry 22 (not yet tried). Foundry 21 also goes with player 18 but has the blue belts. Fall back to Player 17 + Foundry 20 (works in MAME). Older: Player 16 + Foundry 19 (works in MAME), Player 15 + Foundry 18, Player 14 + Foundry 17, or the hardware-proven Player 10 + Foundry 13.
