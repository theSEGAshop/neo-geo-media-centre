# Neo Geo Media Centre

By bLiGhTy.

Play your own music on a real Neo Geo.

The Neo Geo Media Centre (NGMC) is a homebrew music-player cartridge for the SNK Neo Geo, plus the **Foundry**: a single web page that turns your music into a cartridge image. Nothing to install and nothing to compile. Everything happens in your browser and nothing is uploaded.

Free and made in spare time. If you enjoy it and would like to chip in, there is a [PayPal link](https://paypal.me/neogeomc). Entirely optional.

## How it works

1. Open the Foundry: use the **Open the Foundry** button on the [project web page](https://thesegashop.github.io/neo-geo-media-centre/), or download `foundry/neogeo-media-centre-foundry.html` (on its GitHub page, press the download arrow at the top right of the code box) and open the saved file in a modern browser (Edge, Chrome or Firefox). Clicking the file in GitHub's file list only shows its code, not the app.
2. Drop in your music (MP3, WAV, FLAC, OGG, M4A, AAC). Up to 64 tracks, 16 MB of cartridge audio in total. The Foundry fits your tracks to the space automatically.
3. Name and order the tracks, pick a look (twelve skins) and a screensaver, and listen to a preview of how the Neo Geo's sound chip will play each track.
4. Press **Build .neo** for a NeoSD flash cart, or **Build MAME set** to run it in the MAME emulator.

The player is built into the Foundry. See [docs/HOWTO.txt](docs/HOWTO.txt) for the full walk-through, including the controls on the Neo Geo.

## What you get

- A track list with play, stop, repeat, shuffle, previous/next and a progress bar
- Twelve skins, from brass-and-steam to full-screen artwork layouts, plus seasonal looks
- Ten screensavers (starfield, Game of Life, glyph rain, light show and more)
- Names, title and maker text of your choosing
- Tested on a NeoSD Pro and in MAME

## Try it quickly

`ngmc-sample-pack-01.zip` (download it from the Releases page) holds four public-domain classical recordings and a ready-made Foundry project. Unzip it, open the Foundry, use **Load project**, and press **Build .neo**.

## Folders

| Folder | What it is |
|---|---|
| `foundry/` | The Foundry (one HTML file) |
| `docs/` | The project web page and the how-to guide |
| `player/` | Source of the 68000 and Z80 player, for the curious. You do not need it to use the Foundry. |

## Building the player from source (optional)

Only needed if you want to change the player itself. It uses [ngdevkit](https://github.com/dciabrin/ngdevkit) and its `15-sound-adpcmb` example. See [player/README.md](player/README.md).

## Disclaimer

Use at your own risk. This is a free hobby project, provided as it is, with no warranty of any kind. It has been tested on a NeoSD Pro and in MAME, but I can’t promise it will work on every console or flash cart, and I’m not responsible for any damage, lost data or other problems from using it. Keep a backup of your SD card before trying a new file. Neo Geo is a trademark of its owners; this project is not affiliated with SNK.

## Credits

- Player built with **ngdevkit** by Damien Ciabrini and contributors (LGPL-3.0-or-later).
- The ngdevkit logo uses the font Reckoner by Alex Dale (1001fonts Free For Commercial Use licence); fix-layer text uses Unscii by Viznut (public domain).
- Fonts in the Foundry: **IM Fell English SC** and **Libre Baskerville** (SIL Open Font License 1.1), **Special Elite** (Apache License 2.0).
- Sample music: public-domain recordings from Musopen; see the sample pack's `CREDITS.txt`.
- Neo Geo is a trademark of its owners. This project is not affiliated with or endorsed by SNK.

See [NOTICE.txt](NOTICE.txt) for the full list.

## Licence

Copyright (c) 2026 bLiGhTy. Licensed under the GNU Lesser General Public License, version 3 or later (LGPL-3.0-or-later). See [LICENSE](LICENSE) and [COPYING](COPYING). If you share it or a changed version, keep the notices and say what you changed.
