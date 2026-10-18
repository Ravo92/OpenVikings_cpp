# OpenVikings Remaster

An open-source engine replacement for **Cultures: 8th Wonder of the World**, bringing a beloved village-building strategy game to modern Windows PCs.

Build a settlement, help your villagers learn new trades, explore the world and lead your tribe through its adventures. OpenVikings aims to preserve the game's character while improving its presentation, controls and reliability.

[Getting started](#getting-started) · [Build from source](#build-from-source) · [Changelog](CHANGELOG.md) · [Contributing](#contributing) · [Disclaimer](#disclaimer)

## What's new in the remaster?

- **Comfortable fullscreen:** DirectX 11 presentation, borderless fullscreen and mouse confinement, without repeatedly changing your desktop resolution.
- **An interface you can read:** Menus, HUD elements, the cursor and loading bar scale with your resolution.
- **A clearer view:** Correct aspect ratios, a larger minimap with an improved frame, and smoother fullscreen videos.
- **Weather restored:** Rain, snow and sandstorms render in the 32-bit graphics path.
- **Better solo play:** AI takes over the other active human starting positions when you launch a multiplayer map in single-player mode.
- **A modern runtime:** Current C++ and Windows services replace the reconstructed legacy runtime and private x87/compiler backends.
- **Updated rendering internals:** SSE2 handles packed sprite and terrain shading, replacing the old MMX paths.
- **Corrected sleep animations:** Viking wooden-spear soldiers lie down and get up at the intended pace when sleeping.
- **Clear vehicle controls:** Catapult and other vehicle action buttons display the correct translated labels and tooltips.
- **Natural stone colors:** The Viking stonemason's work animation shows rubble, stone blocks and marble in stone colors instead of metallic shades.
- **Livelier workshops:** Viking potters have an oven fire when making bricks, and bakers show their complete cake-baking animation.

## Project status

**In active development.** OpenVikings Remaster combines modern Windows presentation with improvements to village life, transport, navigation, loading and audio. See the [changelog](CHANGELOG.md) for the latest updates.

The `remaster` branch is where improvements happen. The `reversed-game-authentic-behaviour` branch preserves the legacy reconstruction, including the original game's known defects.

Next steps include further bug fixes, performance improvements and support for larger settlements. The separate `network` branch introduces experimental multiplayer modernization. The current target remains **32-bit Windows**.

## Getting started

You will need:

- Windows 10 version 1703 or later and a graphics device supporting DirectX 11.
- Your own legally obtained game and its original data. **8th Wonder of the World** is the main supported target; **Cultures 2** compatibility is experimental.
- A build of OpenVikings from the `remaster` branch.

Build the game using the instructions below. For **8th Wonder of the World**, prepare a game-data root with these folders from your original installation. Preserve the filenames and language subfolders:

```text
datax/
  libs/       Original data archives (data0000.lib, ...)
  dm2/        Original SGT/DLS music data
  mouse/      Original .cur cursor files
  pictures/   Original pre/post .bmp images
  FMV/        Original films and language subfolders
```

Keep the original `gedx8musicdrv.dll` available alongside the executable for music. Films use the Windows media components and available codecs. Run OpenVikings with the game-data root as its working directory; set this in your shortcut or Visual Studio debugger settings.

Each game has its own launcher, which selects its resource profile automatically. Use each game's own installation as its working directory. Initial **Cultures 2** compatibility skips the later game's vertex-color palette and terrain tinting; further loading differences are still being investigated. **Northland** currently displays a notice that its data loading is not implemented yet.

## Build from source

1. Check out `remaster`, or `network` for the experimental multiplayer replacement.
2. Open `OpenVikings.slnx` in Visual Studio with the C++ desktop development tools, **MSVC v145** and a Windows SDK installed.
3. Select **x86 (Win32)** and **Debug** or **Release**.
4. Right-click the desired game project and choose **Set as Startup Project**. Under **Properties > Debugging > Working Directory**, select that game's data root.
5. Build the game project; its shared engine dependency builds automatically. Find the executable in `OpenVikings/Debug/` or `OpenVikings/Release/`.

| Startup project | Executable | Status |
| --- | --- | --- |
| `OpenVikingsWeltwunder` | `OpenVikingsWeltwunder.exe` | Main supported target |
| `OpenVikingsCultures2` | `OpenVikingsCultures2.exe` | Experimental resource compatibility |
| `OpenVikingsNorthland` | `OpenVikingsNorthland.exe` | Launcher prepared; game loading pending |

`OpenVikingsEngine` is the common static library, maintained in `OpenVikings.vcxproj`. It is linked into each game executable and cannot be started by itself. Shared build settings are in `OpenVikings.Build.props`; symbols stay in `symbols/` and each project has its own directory beneath `obj/`. Game selection requires no command-line argument.

The project uses **C++17**. It does not currently target x64. Install the x86 Visual C++ runtime matching your build when running it on another PC.

## Savegame editor

Open [SavegameEditor.cmd](tools/savegame_editor/SavegameEditor.cmd) to edit a Windows Cultures `.sav` file with the standalone desktop editor. It requires **Python 3.10 or newer with Tkinter** and no third-party packages. You can also run `python tools/savegame_editor/editor.py` or pass a save filename on the command line.

- Edit villager names, needs, hitpoints, equipment condition and experience values.
- Edit stored goods in completed buildings, using your game data to identify the correct stock slots.
- Change save descriptions, map display names and slot IDs; inspect file blocks or export a JSON overview.
- Export a new map folder with **map.dat**, **map.ini**, existing buildings, villagers, animals, vehicles and signposts. Companion text files preserve villager names.
- Undo and redo edits, then save a new copy. The editor preserves untouched data and refuses to overwrite the source or an existing file.

Choose **Game data...** and select your game installation for building, good, job and experience names. When opening a save inside an installation containing `DataX/libs`, the editor finds it automatically. Names come from the game's internal definitions. **Export map...** requires matching game data and a new output folder. It creates a new starting map; mission definitions/progress, scripted AI and exact runtime state are not exported. Terrain editing, object creation in saves, job changes and linked mission editing remain outside the editor's scope.

Folder-based maps belong under `data/maps/<map-name>/`, including the generated `text` subfolder. The `UserMaps` folder continues to expect `.c2m` archives.

## Multiplayer on the network branch

The experimental `network` branch replaces DirectPlay with a native Windows TCP/IP transport. Use the same OpenVikings build and game profile on every computer; original Cultures executables use a different protocol.

Host or join through the multiplayer menu. Leave the search address empty for IPv4 LAN discovery, or enter a hostname, IPv4 address or IPv6 address for direct discovery. Allow **TCP and UDP port 25840** on the host. Up to ten players share a session; losing the host ends it. Map and save transfers verify their size and checksum before replacing local files.

This implementation targets trusted LANs and private VPNs. Public matchmaking, relay services, automatic NAT traversal, encrypted transport and host migration are outside this version.

## Contributing

Bug reports, code improvements and help checking the game are welcome.

For a useful bug report, include the revision you played, the map or savegame, steps to reproduce the issue and what you expected to happen. Screenshots help with visual problems. If you can compare the behavior with the original game, include what you found.

Keep pull requests focused and explain what changes for players. Preserve the Win32 target and use the original Windows behavior as the reference, with intentional remaster changes clearly identified. Please do not upload original game assets.

Also do not miss out the [CulturesNation official Discord Server](https://discord.gg/S3JTYnF)!

## Changelog

Read the [dated development updates and full changelog](CHANGELOG.md) for gameplay fixes, graphics and audio improvements, and project maintenance.

## Credits

Cultures was created by **Funatics Software**. OpenVikings is a community project built out of appreciation for these games.

Related projects include the [Open Northland](https://github.com/s-nems/open-northland) and [Cultures Saga Patches](https://github.com/Basssiiie/cultures-saga-patches), whose community fixes help inform our bug investigations.

## License

The source code is licensed under the [GNU General Public License v3.0](LICENSE). This license does not cover the original game's proprietary assets.

## Disclaimer

OpenVikings is an independent, unofficial fan project developed through reverse engineering for education, research and personal enjoyment. It is not affiliated with, endorsed by or supported by Funatics Software or Daedalic Entertainment.

The original game, its trademarks and its copyrighted content remain the property of their respective rights holders. The original game's data files are not distributed with this engine replacement. You must provide your own legally obtained game data; this project grants no rights to those assets and does not endorse copyright infringement.

Rights holders with concerns about this project are welcome to [contact the maintainer](https://github.com/Ravo92).
