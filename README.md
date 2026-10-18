# PKGarden (DSi)

<p align="center">
  <img src="https://img.shields.io/badge/License-GPLv3-informational.svg" alt="License: GPLv3">
</p>

A Pokémon chest for the Nintendo DSi: move Pokémon between your Generation 3, 4 and 5 save files and a 99-box chest
stored on the SD card.

PKGarden is a modified fork of [pkmn-chest](https://github.com/Universal-Team/pkmn-chest) by Universal-Team, modified in
October 2026 (see [Changes from pkmn-chest](#changes-from-pkmn-chest)). It is part of a small family of apps that share one
chest file format: a companion app for Gen 3 GBA saves on Knulli handhelds lives in `../PKGarden-Knulli`, and a self-hosted
sync server is planned. The format is described in `../PKGarden-Spec/SPEC.md`.

## What it does
- Starts with the lairLynx Studios splash screen, then a main menu: **Local Garden** (work with save files on this
  console), **Online Garden** (shown, but cannot be selected yet) and **Options**.
- Opens Pokémon `.sav` files from the SD card (Generations 3, 4 and 5) and shows the save's boxes next to the chest.
- Moves Pokémon between a save and the chest, and rearranges boxes. Withdrawing a Pokémon into a save converts it to that
  game's generation where possible.
- One chest of 99 boxes with 30 slots each, stored as `sd:/_nds/pkgarden/chest.pgc`. It holds Gen 3, 4 and 5 Pokémon.
- Saving writes the save file and the chest together. Before every write the save is backed up (`_nds/pkgarden/backups/`),
  and the previous chest is kept as `chest.pgc.bak`. If the power is lost half-way, the next start finishes or undoes the
  update, so a Pokémon is never lost or duplicated. If the chest file is damaged, the previous version is restored.
- Options menu: **Saves folder** comes first (choose a default folder so Local Garden opens straight into it instead of
  asking for the drive every time; press X on that row to clear it), followed by the options inherited from
  pkmn-chest: language, number of backups, music, sound effects, keyboard layout and theme.
- Inherited from pkmn-chest: the Pokémon editor, sorting and filtering, and the touch/D-pad keyboard.

Not (yet) part of this version:
- Online Garden (syncing the chest between devices). For now, copy `chest.pgc` between SD cards by hand.
- Importing old pkmn-chest `.bnk` chests, or its multiple-chest management. Old themes must be copied from `_nds/pkmn-chest/themes` to `_nds/pkgarden/themes` by hand.
- Storing Generation 1, 2 or 6+ Pokémon in the chest.
- Gamecard (Slot-1) and GBA cartridge (Slot-2) saves are still in the code from pkmn-chest, but they are not part of this version's scope and have not been tested.

## Using it
Run `pkgarden.nds` from the SD card (for example with TWiLight Menu++), choose **Local Garden**, pick a `.sav` file, move
Pokémon, then open the menu and choose **Save**. In the save browser, B goes up a folder; inside your default saves folder
it returns to the main menu. Nothing is written until you save. Make a copy of a save you care about before trying anything new.

Files are kept in `sd:/_nds/pkgarden/`: `chest.pgc`, `chest.pgc.bak`, `backups/`, `themes/` and `config.json`.

The app is meant for DSi mode. It also starts in plain DS mode in the MelonDS emulator (4 MB of RAM), but memory is tight there.

## Building
PKSM-Core is included in `arm9/core` (it is not a git submodule), so a plain copy of this folder is all you need.

### With Docker (recommended)
Install Docker Desktop, then run in PowerShell from this folder:

    .\docker-build.ps1

This uses the `skylyrac/blocksds:slim-v1.18.0` image, downloads the [bmp2ds](https://github.com/Universal-Team/bmp2ds/releases)
graphics converter once into `.tools/`, and produces `pkgarden.nds`.

### Without Docker
Install [blocksds](https://blocksds.skylyrac.net/docs/setup/) and put the
[bmp2ds](https://github.com/Universal-Team/bmp2ds/releases/latest) converter in your `PATH` as `bmp2ds`, then run `make`.
No git checkout is needed; without one the version string is `pkgarden-dev`.

### Shared chest code
The chest, journal and conversion code in `arm9/source/pkgarden/` is shared with the Knulli app. Its source of truth is
`../PKGarden-Knulli/src`; edit it there and run `.\sync-chest-code.ps1` (or `.\sync-chest-code.ps1 -Check` to verify the
copies match). `VENDORED.md` lists all included third-party code and our changes to it.

## Changes from pkmn-chest
Modified 2026-10-18 and later. In short:
- The multiple-chest `.bnk` system was replaced by one chest file (`chest.pgc`, 99 boxes) shared with the other PKGarden apps.
- Saving the save file and the chest is now one crash-safe step with a journal instead of two separate writes.
- The chest and options menus for choosing, creating and resizing chests were removed.
- PKSM-Core is included as a plain folder with local patches (listed in `VENDORED.md`); a Gen 3 save with missing data
  sections is now rejected instead of being read out of bounds, and an impossible conversion no longer crashes.
- The spinning-chest loading screen was replaced by the lairLynx Studios splash, and the app now starts at a main menu
  (Local Garden / Online Garden / Options) instead of the drive list. A default saves folder can be set in the options.
- The data folder moved from `_nds/pkmn-chest` to `_nds/pkgarden`, and the app was renamed.

## License
GPLv3, see `LICENSE`. This is a modified version of pkmn-chest and PKSM-Core; their copyright notices and additional terms
(GPLv3 section 7) remain in the source files and apply to this version.

## Credits
PKGarden is built on the work below. Thank you to everyone listed.

### lairLynx Studios
- [lairLynx Studios](https://github.com/lairLynx): the startup splash screen (logo, credit and layout, shared with NDSi-Shop and StarDSi). The logo is converted into `arm9/source/splash/splash_logo.c` by `tools/pack_logo.ps1` from `assets/lairlynx_logo.jpg`.

### pkmn-chest
- [Pk11](https://github.com/Epicpkmn11) and [Universal-Team](https://github.com/Universal-Team): [pkmn-chest](https://github.com/Universal-Team/pkmn-chest), the app this is forked from
- [StackZ](https://github.com/SuperSaiyajinStackZ): Idea to make a Pokémon Bank for the DS(i)
- [TotallyNotGuy](https://github.com/TotallyNotGuy): Finding and making graphics and the Pokémon Center generation 4 song

### Pokémon data and save handling
- [Flagbrew](https://github.com/FlagBrew), [piepie62](https://github.com/piepie62): [PKSM](https://github.com/FlagBrew/PKSM) and [PKSM-Core](https://github.com/FlagBrew/PKSM-Core), whose save and Pokémon management code is at the heart of this app
- [Kurt](https://github.com/kwsch): [PKHeX](https://github.com/kwsch/PKHeX), a major reference for the generation 3 code

### Tools and libraries
- [AntonioND](https://codeberg.org/SkyLyrac): [blocksds](https://blocksds.skylyrac.net), the toolchain this is built with
- [Universal-Team](https://github.com/Universal-Team): [bmp2ds](https://github.com/Universal-Team/bmp2ds), the graphics converter used at build time
- [nlohmann/json](https://github.com/nlohmann/json), bundled as `arm9/include/json.hpp`

### Inherited from pkmn-chest (not part of this version's scope)
- [edo9300](https://github.com/edo9300): [ndsi-savedumper](https://github.com/edo9300/ndsi-savedumper)'s gamecard save dumping and injecting code
- Pokedoc: [savegame-manager](https://code.google.com/archive/p/savegame-manager)'s GBA save dumping and injecting
- [RocketRobz](https://github.com/RocketRobz): Adding flashcard and SD being used together and code from [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu)

### Translators
The app still includes pkmn-chest's translations of its interface (messages added by PKGarden are English only):
- [antoine62](https://github.com/antoine62): French
- [5antos](https://github.com/5antos): Portuguese, some Spanish and French
- [cimo95](https://github.com/cimo95): Indonesian
- [David Pires](https://github.com/DavidPires): Portuguese
- [edo9300](https://github.com/edo9300): Italian
- [Pk11](https://github.com/Epicpkmn11): English and Japanese
- [Extocine](https://twitter.com/@ExtocineN): Russian
- [lemonnade0](https://steamcommunity.com/profiles/76561198276444028): Lithuanian
- [NightScript](https://github.com/NightYoshi370/): French
- [R-YaTian](https://github.com/R-YaTian): Chinese
- [StackZ](https://github.com/SuperSaiyajinStackZ): German
- [Viktor Varga](https://github.com/vargaviktor): Hungarian
- [whatter](https://github.com/whatter): Polish
- [XxPhoenix1996xX](https://github.com/XxPhoenix1996xX): Spanish

### Music
- [Pokémon Center (Gen 1)](https://modarchive.org/module.php?181718), [Elm's Lab](https://modarchive.org/module.php?181711), [Oak's Lab](https://modarchive.org/module.php?181717), [Game Corner](https://modarchive.org/module.php?181756), and [Twinleaf Town](https://modarchive.org/module.php?178770).
