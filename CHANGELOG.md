# OpenVikings Remaster Changelog

## 2026-10-29 · 14:02 CET

C++ source formatting now follows one consistent project style.

- Every `for` loop and every branch of an `if`/`else` chain uses braces, with statements on separate lines.
- Control blocks and final returns have consistent spacing; source files have no trailing empty lines.
- A shared clang-format configuration and formatting command keep future contributions consistent. Gameplay logic and data formats are unchanged.

## 2026-10-29 · 12:35 CET

Build outputs now use matching architecture subdirectories.

- Win32 executables and the engine library are written to `OpenVikings/Debug/x86/` or `OpenVikings/Release/x86/`, alongside the existing `x64/` directories.
- Symbols, intermediate files, copied assets and licenses follow the selected architecture's output directory.

## 2026-10-29 · 12:00 CET

Switching between Win32 and x64 now preserves both FFmpeg installations.

- Each architecture uses a separate vcpkg installation folder, preventing one restore from removing the other architecture's headers and libraries.
- Missing FFmpeg headers or libraries invalidate stale restore markers automatically, fixing the reported missing `libavcodec/avcodec.h` error.

## 2026-10-29 · 10:21 CET

The graphics, audio and video core now uses native 64-bit memory layouts.

- Display, animation, character variants, house graphics and particle graphics use matching native allocation and cleanup paths.
- Sound and WAV playback owners use native Windows handles and pointers; music and FFmpeg video source checks pass on x64.
- Win32 layouts and the shared savegame format remain compatible at source level. Full x64 playback and gameplay validation are still pending.

## 2026-10-29 · 09:31 CET

Win32 and x64 retain the same savegame format while using different runtime layouts.

- Human graphics and palettes now use explicit legacy file records; native pointers and padding stay out of saves.
- Statistics, embedded saves, AI state and message history retain their existing data formats with native memory ownership.
- Existing chunk versions, colors, names, IDs and checksum data remain unchanged. Gameplay save exchange awaits full x64 runtime validation.

## 2026-10-29 · 09:01 CET

The shared GUI foundation and in-game message storage now support native 64-bit pointers.

- Desktop and window lists, GUI resources, clocks and message buttons use matching native layouts and allocation sizes.
- Converted GUI virtual entries preserve argument order under the Windows x64 calling convention.
- Messages keep their existing save format while retaining native references in memory.
- Specialized windows and complete x64 runtime validation remain in progress.

Gameplay improvements and fixes, grouped by day. Experimental multiplayer is included in `remaster`. Times are local to Vienna, Austria (CET/CEST).

## 2026-10-29 · 08:42 CET

The core 64-bit source conversion for maps, sectors and game callbacks is complete.

- Map searches carry complete native references through AI, human, vehicle and target-point contexts. Numeric coordinates, IDs, queues and saved formats retain their encodings.
- Map loading, sector services, callback dispatch and teardown now share native ownership and independently checked map modules.
- All 25 core modules pass native syntax checks with both debug and release preprocessing. Remaining GUI/graphics conversion and complete x64 runtime validation are still in progress.

## 2026-10-29 · 00:00 CET

The 64-bit source conversion of the base runtime, memory and input layer is complete.

- Memory helpers retain native allocation sizes. Mouse callback access and text-input storage match their native layouts.
- File, window and input owners prevent accidental copies; the random generator retains its existing sequence while no longer depending on the complete game-state layout.
- All 25 audited base/platform modules pass native syntax checks, and the complete Win32 source still passes. The full x64 game, remaining GUI/object conversion and runtime validation are still in progress.

## 2026-10-28 · 23:36 CET

The 64-bit source migration now includes map areas and landscape graphics resources.

- Landscape areas, frame groups and resource tables use native pointer layouts and matching allocation/free paths.
- Landscape drawing, selection, ambient-sound lookups and saved resource-name lookup use the native record stride. Saved names, coordinates and frame IDs retain their formats.
- The complete Win32 source passes syntax checks. The x64 game still requires further GUI and game-object conversion.

## 2026-10-28 · 23:23 CET

The 64-bit source migration now includes game callbacks and core map ownership.

- Internal callbacks retain complete native addresses for save/load, effects and window references, while game values keep their existing widths.
- Roads, walls, map sectors and map buffers use matching native storage and checked allocation sizes. Saved map/build data and checksums retain their formats.
- The complete Win32 source still passes syntax checks. Further graphics and GUI conversion is required before a complete x64 executable can be built.

## 2026-10-24 · 11:08 CEST

The 64-bit source migration now includes shared game-object array controllers, merchant storage and chunk I/O.

- Array data and callback pointers use native storage; saved array headers and trade-agreement records keep their existing byte formats.
- The shared file/chunk helper uses native runtime storage while retaining its 32-byte saved headers.
- Startup, save loading and resource teardown use matching native allocation sizes. The complete x64 game and GUI migration remains in progress.

## 2026-10-24 · 10:46 CEST

The 64-bit source migration now covers core graphics resources, shared lists and resource caches.

- Bitmap, palette, sprite and font objects use native pointer storage and matching allocation sizes, while saved graphic formats stay unchanged.
- Palette shading arrays, texture tables and BOB caches allocate and release native objects consistently. Resource owners reject accidental shallow copies.
- Shared lists and the map, savegame, property and display managers have native layouts. Further game and GUI objects remain to convert; a complete x64 executable is still in progress.

## 2026-10-24 · 10:01 CEST

The 64-bit graphics migration now includes native pixel-converter storage and typed renderer access.

- BOB shadow and blending paths use named converter accessors instead of fixed x86 offsets for RGB565 flags, channel masks/shifts and the darkening table.
- Pixel copy/fill helpers support native addresses while retaining fixed-width pixel values and the existing count arithmetic. The bitmap/palette object migration remains in progress.

## 2026-10-24 · 09:33 CEST

The 64-bit migration now includes native window, input and configuration-file storage.

- Application/window handles, input queues and callback storage have explicit Win32/x64 layouts, while keyboard and mouse event formats stay unchanged.
- String and INI objects use native allocation sizes; compiled string data retains the original 20-byte header and 32-bit string offsets.
- Temporary INI definitions and file buffers have explicit ownership, preserving include order and definition lookup. The complete x64 engine port is still in progress.

## 2026-10-22 · 18:57 CEST

Computer villages keep construction moving, and villagers can escape blocked construction sites. Work on the 64-bit version and Cultures 2 movies continues.

- **AI:**
  - Computer players now keep diagnostic logs of building plans, resource requests, worker assignments, family planning and military activity. Logs include reasons for waiting and failed routes, making stalled villages easier to investigate. Enable them with `-log:ai`.
  - Computer players keep a builder available instead of assigning everyone to other work. Villages with no builder recruit a suitable existing worker while keeping their last warehouse carrier.
  - Small computer villages start with one builder, freeing additional idle builders for gathering and other work. Larger settlements can expand their construction crews as their workforce grows.
  - Computer villages build a basic family home immediately after their first farm, giving housing priority over more production buildings.
  - Computer players use building materials already in their warehouses instead of waiting for those materials to be produced again. Construction still needs real deliveries and consumes the normal materials.
- Diagnostic logging is off by default. Enable AI, network or renderer logs individually with `-log:<category>`; fixed files are saved in the game's logs folder through a background queue that drains on normal shutdown.
- Egyptian headquarters use a pyramid at one third of its previous width and height, with matching selection outlines and effect positions.
- Villagers do not prevent placing construction sites. Villagers trapped in an enclosed construction site can walk out, including when continuing an older save.
- **64-bit development (in progress):**
  - Builds exclude unrelated codecs, encoders, output formats and network/hardware video backends, while retaining Bink video/audio and the existing picture/sound conversion.
  - The next build automatically replaces the previous broad package with the size-optimized variant. The executable-size reduction still needs measurement after rebuilding.
  - Original BIK movies are decoded through statically linked FFmpeg libraries, replacing the `binkw32.dll` calls on Win32 and x64.
  - Video feeds the existing renderer; movie audio uses native Windows PCM output with audio-clock synchronization and bounded playback buffers.
  - Visual Studio restores the pinned dependency automatically on the first build. Playback in the game still needs verification; the complete x64 engine port remains in progress.
  - Music uses the reconstructed DirectMusic driver built into the engine, removing the need for `gedx8musicdrv.dll`. Playback still needs verification after rebuilding.
  - Visual Studio includes Debug and Release configurations for x64, with separate output files. The complete game port is still in progress.
  - Runtime allocation sizes and directory-search handles use the native architecture width. Buffer ownership and allocation-overflow handling are being modernized while retaining the existing file formats.

## 2026-10-21 · 19:58 CEST

- **AI:**
  - Egyptian computer players can produce and equip short bows, including when continuing an older save.
  - Attack waves start gathering available armed soldiers immediately and launch once their full strength and archer share are ready. Towers release surplus archers needed by the wave and protect soldiers already gathering or attacking. Threatened towers can still call for emergency defenders.
  - Food shortages pause recruitment, while existing soldiers continue requesting weapons, equipping and planning attacks.
  - Failed building placement is retried instead of permanently blocking that building type. Older saves can recover previously blocked weapon workshops; normal building permissions and costs still apply.
  - Qualified workers can advance to needed professions: hunters become breeders or tailors, and joiners become armorers. Spare adults replace them through normal work and experience; pending breeder/tailor vacancies maintain a pool of three hunters.
  - Computer armies aim for about 60% melee fighters and 40% archers across civilizations. Missing weapon production is requested normally; armor is optional, and unarmed soldiers return home to equip instead of joining attacks.
  - Computer troops gather at their village before launching a wave. Running expeditions keep their target and receive no stream of fresh recruits. Normal waves grow with population up to 96 soldiers; Easy is limited to 24 and Very easy to 12.
  - Computer players staff completed workshops and warehouses before recruiting unmarried, free civilists. Normal allows larger late-game armies while keeping existing workers and parents at home.
  - Computer families aim for roughly one woman per employed civilian worker on every difficulty, counting girls and pending daughters. There is no fixed female cap; soldiers and unemployed civilists do not raise the target. Once balanced, families request boys for workers and future recruits. Normal food, home, marriage, growth and map population limits still apply.
  - Computer women choose employed civilian workers as husbands. Unemployed civilists remain available for training; the worker assignment is checked again before the marriage completes.
  - Computer players can use stored food for military readiness while bread production pauses.
  - Computer players retry local production when a planned material import has no active delivery. This helps stalled construction obtain missing materials, including the final pillar for Egyptian barracks.
  - Computer players spread builders between construction sites and release excess builders, leaving workers available to build missing supply workshops.
  - Computer players can move an already qualified warehouse carrier into an unstaffed bakery or mill while keeping another carrier at the warehouse. This helps food production and family growth recover in existing saves.
  - Computer players recognize their completed barracks outside village boundaries, allowing spare workers to train as soldiers there.
  - Computer players put available workers into existing workplaces before building more, and plan new warehouses around their workforce and defenses.
  - Computer players build at most two farms, mills, bakeries, wells and beehives each, rather than covering the map with workplaces they cannot staff.
  - Computer players aim to assign twice as many collectors to needed raw materials. Carriers collect their goods, and qualified collectors can move into crafting jobs.
  - Bread production, homes for homeless families and more frequent family planning help computer villages grow, with the same food costs and population limits.
  - Computer players recruit soldiers from spare workers while keeping essential production and transport staffed.
  - Destroyed computer warehouses leave their actual stored goods behind instead of a fixed food reward, including quantities above 25.
- Hunters search for live prey within 100 map fields of their work flag, twice the original radius.
- Egyptian short-bow soldiers use the existing Saracen archer model and animations, with matching palettes. Egyptians remain selectable; their actual civilization and combat rules are retained.
- Egyptian soldiers consistently use white-and-gold headdress colors instead of solid-gold or reddish-white variants. The correction also applies to existing saves and their borrowed Saracen archer models.
- Clicking a villager's message moves the camera to that villager and selects them, as in the original game. Hold Ctrl to select without moving the camera.
- All civilizations use the original well and beehive appearances, making water and honey production easier to recognize.
- New Skirmish and multiplayer games give the Egyptians a pyramid headquarters, the Franks a castle, the Byzantines a cathedral and the Saracens a palace. Each remains a working headquarters, provided there is room to place it; existing saves keep their previous buildings.
- Soldiers whose civilization lacks an appearance for their weapon use their own civilization's existing soldier animations. Their actual weapons and fighting strength stay the same.
- Saracen woodcutters no longer lose their heads during axe swings.
- Clay carried by Byzantine villagers looks like clay instead of a gold bar.
- Egyptian settlements have an Arabian soundtrack with music for everyday life, prosperity and danger.
- Felled trees blocked by rocks no longer keep falling repeatedly or remain stuck forever. Their wood is placed nearby when the fallen trunk cannot fit; if there is no safe space yet, it is kept until space becomes available. This also helps affected trees in older saves.

## 2026-10-20 · 23:43 CEST

- **Single-player menus:** Story maps appear under Missions, while multiplayer maps played against the computer have their own Skirmish menu. Tutorials, campaigns and saved games remain available.
- **Skirmish setup:** Choose your map, starting position, opponents and difficulty on one screen. The map list has more room for names, and the preview shows the map's size beside its heading.
- **Clearer menus:** Map descriptions and previews have simple see-through frames, and menus use the game's familiar lettering. The player table shows who controls each position without repeating the same information in another column.
- **Player setup:** Choose Human, Computer or Off and the computer player's civilization from dropdowns in Skirmish and multiplayer. A button at the bottom of the player list fills open positions with computer opponents.
- Map colors remain clearly visible even when the map locks them, with an explanation of why they cannot be changed.
- The difficulty you choose in Skirmish applies to your starting position, including when you choose a different position from the first one.
- **AI:**
  - Other civilizations use their own starting buildings and can run farming, food production, crafts and military training. Where a civilization has no matching building, a suitable building from that civilization takes on the job.
  - Egyptian computer players recognize their large warehouses and use them to supply and expand their villages. Civilization-specific building roles are retained in new saves.
  - Computer players no longer receive free construction supplies or regular gifts of five units of every resource. They must gather, produce and transport their supplies.
  - Idle computer-controlled soldiers without weapons pick up suitable available weapons. Human players decide for themselves when and how to equip their soldiers.
  - The computer-controlled village behind the main menu no longer tears down its own buildings.
- Visible buildings belonging to other players show their purpose when you hover over them or open their information window, including farms, workshops and military buildings.
- **Multiplayer connection display:** See a graph of each player's connection, their current delay and connection stability, and the active synchronization rate. The game chooses the rate automatically to suit the connection.
- **Multiplayer fixes:** Games start correctly with the chosen civilizations, opening the menu handles long address lists safely, cancelled map downloads are reported correctly, and a slow connection no longer wrongly disconnects other players. Everyone needs the same updated game build.
- **Die Saga:** Its launcher opens the Nordland, Wunder and Saga campaigns with their own maps and progress, plus both user-campaign menus. User campaigns can also be loaded from unpacked folders; duplicate copies appear only once. Compatibility remains experimental.

## 2026-10-19 · 23:56 CEST

- **Text and names:** Type and paste names and messages in additional languages, including Chinese and other non-Western scripts. Editing no longer splits individual characters, and additional characters display the right way up. Available characters depend on installed Windows fonts; no new translations are included.
- Player names, session names and chat retain these characters in multiplayer. The savegame editor and exported map text support them too.
- **Cultures 2:** Menus, animals, music and the campaign map use the game's own content. Campaign missions appear at their proper positions and follow your existing progress.
- Cultures 2 maps start without showing a difficulty screen from the later games, and its Extras menu has the correct letters and four assistance options.
- Cultures 2 sounds play at their proper speed and pitch.
- Cultures 2 intro and mission videos play using the original installation's `binkw32.dll`. The loading screen has its original framed progress bar with grey shading and a moving highlight.
- Red speckles in the enlarged Cultures 2 minimap frame are removed.
- The game appears in the taskbar from the first loading screen, comes to the foreground at startup and responds normally when activated.

## 2026-10-18 · 18:45 CEST

- **Multiplayer:** Find games on your local network or join a host directly by name or address, without the old DirectPlay requirement. All players need the same OpenVikings build; original Cultures multiplayer, automatic host replacement and public matchmaking are not supported.
- Maps and saves received in multiplayer are checked before replacing local files, and download progress and completion are reported correctly.
- Each supported game has its own launcher. Northland's launcher is prepared, but loading its game data is not available yet.
- Music starts in the background so its initialization no longer holds up the menu or game startup.
- **Savegame editor:** Edit villager names, needs, health, equipment, experience and building inventories. Undo and redo changes, then save a new copy while keeping the original save untouched.
- **Map export:** Turn a saved world into a new starting map with its terrain, buildings, goods, villagers, animals, vehicles and signposts. Unlockable items remain available for progression. Mission progress, mission scripts and an exact continuation of the saved game are not included.
- Damaged game files and failed sound loading are handled more safely, reducing crashes and freezes during loading. Pasting text into menu fields is also safer.
- Fix incorrect red shading in the older color mode and timing problems affecting music recovery, menu updates and autosaves after long periods of system uptime.
- Prevent a crash caused by invalid walking routes.
- Builders are automatically assigned only to construction sites they can reach on their own landmass. Longer journeys require connected signposts, avoiding repeated complaints about unreachable sites on another island.
- Workers follow signposts toward their destination correctly, including tailors returning to their workshop with leather.
- **AI:**
  - Attack groups can find a reachable approach when their original route is blocked. This restores the stalled northern attack waves without changing their timing or troop requirements.
- The game starts on your Windows primary monitor, regardless of where the mouse is.

## 2026-10-17 · 03:35 CEST

- **AI:**
  - Collectors work at resource deposits and leave goods at their flags for warehouse carriers. Computer villages can continue gathering after starting supplies run out, pausing when storage is full and resuming when space is available.
- Viking wooden-spear soldiers lie down, rest and get up at the proper pace, with the same sleep duration and stamina recovery.
- Vehicle buttons show the correct translated labels and help text for disembarking, unloading and docking.
- The Viking stonemason's stone and marble look like stone rather than metal during work and carrying animations.
- The potter's oven burns while making bricks, and the baker appears during the full cake-making animation, carrying cake through the final step. Production times are unchanged.

## 2026-10-16 · 20:48 CEST

- **Display:** Use borderless fullscreen or a resizable window, switch with Alt+Enter and keep the mouse inside the game. Starting the game and switching between videos, loading screens and menus no longer repeatedly changes your desktop resolution.
- Menus, the interface, cursor and loading bar scale with your resolution. The game keeps its proportions on wider displays, with a larger minimap and improved video and cursor display.
- Rain, snow and sandstorms appear correctly with the updated graphics.
- **AI:**
  - When you play a multiplayer map alone, computer players take over the other active human starting positions rather than leaving those villages idle. Your own position, the map's alliances and saved player assignments are preserved.
- Schools teach the selected skill instead of awarding experience for the pupil's previous job. Equipment availability and soldier profession changes are handled correctly.
- Fix problems with routes over water and with vehicle loading, reservations and deliveries, including goods left permanently reserved after a failed journey.
- Builders stop working on vehicles that another worker has already finished.
- Fix a targeting problem that prevented soldiers from considering some enemies within their weapon's permitted range.
- Fix several crashes and display problems involving the minimap, brightness, button text and map edges. Help links open the correct section, and arrows that leave the map are cleaned up.
- Sound loading handles more valid files correctly and rejects damaged files safely. Failed sound recovery no longer retries forever, and missing jingles no longer play unrelated sounds.
- Music and sound fades reach the correct volume, muting works correctly, and changes preserve the balance and left/right position of individual sounds.
- Missing or damaged game data is handled more safely, reducing the risk of loading freezes and crashes.
