# OpenVikings Remaster Changelog

News and changes from the `remaster` branch. Times are local to Vienna, Austria; the entries below use CEST (UTC+02:00).

## 2026-10-19 · 23:04 CEST — Cultures 2 Extras window

Restore Cultures 2's own Extras page instead of the later game's assistant controls.

- Show **Letters** followed by **Extras for everyone**, using Cultures 2's original text.
- Restore the four requests for shoes, wooden tools, iron tools and mead, each with its explanation.
- Keep the later games' assistant page for their own profiles.

## 2026-10-19 · 22:52 CEST — Cultures 2 sound and options

Correct two remaining assumptions from the later game's shared engine.

- Play Cultures 2 effects at their original 44,100 Hz rate, correcting half-speed, lower-pitched playback caused by the 22,050 Hz setting.
- Omit the unsupported difficulty label from Cultures 2's options window, removing the undefined footer text.
- Restore the previous graphics presentation and remove the experimental smoothing filter and its option.

## 2026-10-19 · 22:35 CEST — Cultures 2 map start

Cultures 2 now starts a selected map directly, following its original Windows menu flow.

- Skip the later games' difficulty screen, whose captions are absent from Cultures 2, when starting a map from its selection menu.

## 2026-10-19 · 22:08 CEST — Campaign compatibility corrections

Correct three integration issues found while reviewing the Die Saga contribution.

- Keep Weltwunder's campaign selection on campaign 3 while Die Saga uses campaigns 2, 3 and 4.
- Register unpacked user campaign maps as folders, so reloading a save does not attempt to open an empty archive name.
- Keep user campaigns visible when a scan updates an existing map GUID, including an archive and its unpacked copy. The folder takes precedence and the map appears once.

## 2026-10-20 · 06:00 CEST — Die Saga campaigns and user campaigns

Die Saga's campaign screens and its user campaign menus are now reachable, and unpacked user
campaigns load alongside the original archives.

- Offer the game's single-player campaign entries: Nordland, Wunder, Saga and the user campaigns.
- Draw each campaign's own map artwork and unlocked mission markers; the three campaign screens
  previously shared one hard-coded campaign and only the first screen painted the map.
- List the two user campaign packages and their maps, following the game's own map-type bits.
- Mount `.c2m` archives from `DataX/UserCampaigns/Campaign00` and `Campaign01` with campaign
  ids 7 and 8.
- Beyond the original, also accept unpacked campaigns as loose folders, in either the
  `currentusermap/map.ini` layout produced by extracting an archive or a plain `map.ini`
  layout. Such a map loads through the existing folder-form path, so no archive is required.
- Suppress the wide Funatics logo on the campaign and user campaign screens, matching the
  original's screen list.

The campaign screens keep this engine's own numbering and append the extra screens instead of
shifting every later screen by five as the original does.

## 2026-10-20 · 05:10 CEST — Die Saga launcher

The solution gains a fourth launcher for **Cultures: Die Saga**, sharing the same engine library.

- Select `OpenVikingsDieSaga` as the startup project to build `OpenVikingsDieSaga.exe`.
- Identify the game's own disc by its `saga` volume name and `autorun.exe` marker instead of the Weltwunder pair.
- Add the game's fourteenth GUI string table (`data\text\<language>\strings\saga\saga001.ini`) and widen the shared table array to hold it.
- Keep the later game's vertex-color palette, `DataX/DM2` music directory, `8..41` animal-table range and four campaign screens.
- Report `OpenVikings - Die Saga` as the window title and `saga` as the single-instance mutex.

Game-data loading differences beyond these are not established yet, so the profile stays experimental.

## 2026-10-19 · 14:10 CEST — Correct Unicode text orientation

Text drawn with Windows fallback fonts now preserves its upright bitmap orientation.

- Remove the extra vertical scanline reversal affecting Chinese and other fallback text.
- Apply the shared renderer correction on both `remaster` and `network`.

## 2026-10-19 · 10:55 CEST — UTF-8 text and Unicode fonts

Text input and localized text can now use UTF-8, with Windows font fallback for characters missing from the original game fonts.

- Accept Unicode keyboard, system IME and clipboard input, including supplementary characters.
- Move, delete and shorten text without splitting UTF-8 characters; retain the existing save and object layouts and byte limits.
- Shape and wrap additional scripts through DirectWrite while keeping available original western font glyphs.
- Read and write Unicode names in the savegame editor and export UTF-8 map text.
- On the separate `network` branch, preserve Unicode player/session names, chat and rename records through the modern transport bridge.

Font coverage depends on installed Windows fonts; translations are not included. Build and in-game validation remain pending.

## 2026-10-19 · 10:51 CEST — Cultures 2 animals and compressed audio

The Cultures 2 profile now uses its original animal-type selection and music directory, and the sound loader supports its compressed effects.

- Resolve Cultures 2 animal names with its own allowed-type list and animal-table starting index.
- Decode IMA-ADPCM WAV files for menu clicks, voices, work, combat and ambient effects.
- Preserve short final audio blocks and the sound lengths recorded in the files.
- Find Cultures 2 music under `DataX/DM`.

## 2026-10-19 · 10:31 CEST — Cultures 2 menu resources

The experimental Cultures 2 launcher now selects the original game's menu scenery and single-player menu texts.

- Select the four Cultures 2 menu maps instead of requesting the later Weltwunder map.
- Show Free Play, Campaign and Load Game using the Cultures 2 text table.
- Load the original eleven GUI string tables and show the existing diagnostic text if a requested table is unavailable.

## 2026-10-18 · 16:10 CEST — Preserve map-export unlocks and DAT compatibility

The savegame map exporter now distinguishes permanent restrictions from items that have not yet been unlocked.

- Emit `forbid` commands only for saved permission restrictions; keep allowed, locked items eligible for progression.
- Write modified walking/building block planes with native RLE8 compression for readers that reject raw planes.
- Clarify that mission definitions/progress and scripted AI are not included in the current starting-map export.

## 2026-10-18 · 13:21 CEST — Export a saved world as a starting map

The savegame editor now has an **Export map...** button that writes a new map folder from the loaded save.

- Export terrain, graphics and fish data as `map.dat` and native startup settings as `map.ini`.
- Write source-save provenance as a comment so `map.ini` contains only supported game sections.
- Include players, buildings, whole-unit stock, villagers, animals, vehicles and signposts, with companion text files for names.
- Prepare house placement masks and native pathfinding sectors for recreated starting objects.
- Keep the source save unchanged and require a new destination folder. Mission definitions/progress, scripted AI and exact runtime state remain outside this starting-map export.

## 2026-10-18 · 13:11 CEST — One engine, three game launchers

The Visual Studio solution now shares one static engine library between separate game launchers. Each executable selects its own game profile.

- Choose `OpenVikingsWeltwunder`, `OpenVikingsCultures2` or `OpenVikingsNorthland` as the startup project.
- Keep shared engine code and build settings together, with separate project intermediates and symbols.
- Begin experimental Cultures 2 support by skipping the later game's vertex-color palette and terrain tinting.
- Keep Northland's launcher ready with a notice while its game-data loading remains to be implemented.

## 2026-10-18 · 12:23 CEST — Background music initialization

DirectMusic now initializes on a dedicated music thread so its startup work can run alongside the menu and game.

- Keep music loading and playback commands off the main thread.
- Retain queued music requests and the latest volume setting while initialization completes.
- Stop the music thread before releasing its driver during shutdown.

## 2026-10-18 · 10:54 CEST — A standalone savegame editor

A desktop tool can now inspect Windows Cultures saves and edit existing villagers, building inventories and save information.

- Change villager names, needs, hitpoints, equipment condition and experience values.
- Read your own game definitions to show building names and the correct goods for each stock slot.
- Undo or redo edits and export a readable JSON overview.
- Save edits to a new file while preserving unknown blocks, untouched compressed data and the source save.

## 2026-10-18 · 10:45 CEST — Safer loading and audio recovery

Loading and resource failure paths now reject invalid input and release their owned resources more consistently.

- Check archive tables, expanded paths and local map/save packing before copying data.
- Close directory searches and release sound-device resources after failed operations.
- Skip unknown sounds and invalid sound-group links instead of selecting unrelated audio.
- Keep music recovery, menu refresh and autosave timing correct across clock wrap.
- Keep villager slots available for rejected creation requests, preserve desktop bitmap ownership and correct RGB555 red blending.
- Lock and bound clipboard text before pasting into menu fields.

## 2026-10-18 · 02:26 CEST — Recruit builders for reachable construction sites

Building sites now check whether a free builder can reach their working position before assigning them. Nearby work remains available directly; longer journeys require a usable connected signpost route.

- Skip unreachable candidates and keep looking for another available builder.
- Check access to the construction work point and every leg of the chosen signpost route.
- Keep automatic recruitment on the same landmass and preserve manual assignments.

## 2026-10-18 · 02:25 CEST — Follow connected signposts to workshops

Villagers now approach the nearby end of a signpost route and follow it toward their destination. This fixes workers repeatedly failing to return with materials despite having a connected signpost network.

- Restore the direction of route traversal and the destination remembered between walking segments.
- Approach the nearby signpost before continuing along longer routes.
- Apply the correction to shared worker routing, including the leather delivery to a sewing workshop.

## 2026-10-18 · 02:04 CEST — Preserve AI attack movement retries

The reachable approach correction needed a follow-up: interrupting an idle soldier could cause the AI to replace the new movement order with the old unreachable target immediately.

- Finish processing that soldier after issuing an attack or regrouping movement retry.
- Let the normal human command processing execute the selected approach order.

## 2026-10-18 · 01:50 CEST — Reachable approach points for AI attacks

AI attack groups can now retry a failed march toward a reachable position within their existing approach radius. This addresses the northern COOP gold raid getting stuck on an isolated free tile surrounded by blocked ground.

- Search from the soldier's position when retrying an attack movement.
- Preserve the original attack area, wave timers, troop requirements and movement commands.
- Keep the invalid-route crash protection in place.

## 2026-10-18 · 01:30 CEST — Reject invalid walking routes safely

Walking routes now reject invalid steps before accessing map data. This guards the path that crashed while a villager requested its next direction.

- Reject empty route results and invalid direction/count entries instead of consuming stale steps.
- Check route-buffer indices and map bounds before committing movement.
- Use the existing failed-path handling so villagers can respond through their normal task logic.

## 2026-10-18 · 01:25 CEST — Start on the primary monitor

The game now opens on your Windows primary monitor, even when the mouse pointer is on another screen.

- Applied the default to both borderless fullscreen and windowed startup.

## 2026-10-17 · 03:35 CEST — AI collectors work at resource flags

Computer villages can now harvest known nearby deposits without tying each collector to a warehouse. Dedicated warehouse carriers bring the resources back from the collection flags, helping economies recover when their starting stock runs out.

- Let collectors leave harvested goods near their work flags for carriers to pick up.
- Update collection sites even when the warehouse has none of the required resource left.
- Assign and reuse qualified warehouse carriers while respecting existing worker-slot limits and active construction assignments.
- Pause collection when the destination is full and resume it as capacity becomes available.
- Preserve existing save layouts and use normal harvesting, hauling and pathfinding.

## 2026-10-17 · 02:54 CEST — Complete pottery and cake-baking animations

Viking workshops now show the missing parts of two production animations: the potter's oven burns while making bricks, and the baker appears while preparing cake.

- Restored the small oven fire missing from brick production, matching pottery and roof-tile production.
- Restored the baker's missing house-exit and house-entry events so the existing cake-baking animation is visible.
- Kept cake visible through the final carrying step, which previously switched to a bread graphic.
- Preserved the working bread sequence and production durations; repairs target the exact affected original data.

## 2026-10-17 · 02:38 CEST — Stone colors at the stonemason

The Viking stonemason's building animation now keeps rubble, stone blocks and marble in their intended stone colors. A shared accessory color range could previously make the material look bronze or gold.

- Corrected the material colors while examining and shaping stone.
- Applied the proper grey shades when carrying materials through the building animation.
- Preserved villagers' appearance outside these phases, production timing and saved appearance data.

## 2026-10-17 · 02:21 CEST — Clear vehicle action labels

Catapult and other vehicle action buttons now display the correct localized labels and help text. Misassigned text entries previously showed unrelated cargo instructions and an unfilled quantity placeholder.

- Restored the correct labels and tooltips for disembarking passengers, unloading goods and docking ships.
- Applied the correction to the shared vehicle window, using the existing translations for the selected language.

## 2026-10-17 · 02:05 CEST — Natural sleep animations for Viking spearmen

Viking wooden-spear soldiers now lie down, rest and stand up at the intended pace. The original animation was missing most of its resting phase, stretching the movement across the entire sleep action.

- Restored the missing resting frames using the matching iron-spear animation as the reference.
- Kept sleep duration and stamina recovery unchanged.
- Applied the correction when loading the exact affected original sequence, preserving customized and already-corrected animations.

## 2026-10-16 · 20:48 CEST — Rendering without MMX

The remaining legacy MMX rendering paths now use SSE2 throughout, completing the graphics-side cleanup after the runtime replacement.

- Moved packed sprite brightness, transparency and terrain shading calculations to SSE2.
- Preserved the existing integer rounding, color saturation and HUD transparency rules.
- Removed obsolete MMX processor detection and rendering switches.
- Updated renderer names and callers to match their SSE2 implementation.

## 2026-10-16 · 18:15 CEST — A modern foundation for the game

The remaster now uses the current Microsoft C++ runtime and Windows services for its shared system functions. This removes a large compatibility layer inherited from the original executable and simplifies future maintenance.

- Removed the reconstructed legacy runtime, custom small-block heaps and private compiler/math backends.
- Removed dependency on discovering internal functions in old Windows runtime DLLs.
- Moved file access, memory management, text handling and shutdown callbacks to supported runtime services.
- Replaced custom x87 math bridges with standard math functions and moved the remaining MMX blending instructions to SSE2.
- Kept the existing game-data formats, 32-bit Windows target and application initialization order.

## 2026-10-16 · 16:45 CEST — Gameplay, audio and loading fixes

This update addresses inherited problems in education, equipment, transport, file loading and the interface. It also makes custom game data safer to load, while keeping the original game's data formats and the 32-bit Windows target.

### Villagers, transport and navigation

- School training no longer awards unrelated experience for the pupil's previous profession. The selected training still grants its intended skill.
- Equipment unlocks use the equipment list's own length instead of the number of production goods. Soldier and hero equipment checks now use equipment permissions rather than unrelated goods permissions.
- Military profession changes also update villagers who are temporarily detached from the map.
- Water-sector navigation selects its own flood-fill group instead of accidentally reusing the land-sector counter.
- Vehicle reservations start with clean counters when a slot is reused and remain active until their reserved amount is exhausted.
- Vehicle stock changes stay within capacity and the storage limit of each counter. Reservation records track the amount actually applied, and a full reservation list rejects a new reservation without changing future stock.
- Failed walking and vehicle-door searches release reservations rather than leaving goods permanently promised. Food pickup and cargo delivery handle reservation failures before consuming or discarding items.
- Workers arriving after another builder has finished a vehicle can leave the completed construction task correctly.
- An overly restrictive combat distance check now allows the affected military mode to consider targets between 19 and 30 map points away. Weapon minimum ranges still apply.

### Graphics and interface

- Brightness calculations stay within the allocated palette range.
- Empty minimap texture samples use a black fallback instead of dividing by zero.
- Button-caption queries handle missing closing braces, empty output buffers and missing captions safely.
- Links between help-text files now use the requested destination anchor instead of always jumping to anchor zero.
- Missing named text blocks release their loaded buffers. Oversized block names and incomplete text reads are rejected cleanly.
- Valid map-edge markers receive full initialization. Invalid placements outside the map no longer consume marker slots, addressing a possible crash involving markers and AI guides.
- Projectiles and other particles that leave the map are retired so their slots can be reused.

### Audio and loading

- WAV loading finds the actual RIFF format and sample-data chunks, including optional chunks and padding, instead of assuming a fixed sample offset. Invalid or truncated chunks are rejected.
- Sound creation releases temporary resources if buffer creation, allocation or filling fails. Locked sound buffers are unlocked on read/reset failures, and memory-backed audio reads advance their cursor.
- Reloading a sound finds an existing entry even when an earlier preload slot has been freed, avoiding duplicate sound resources.
- Unknown jingle events no longer play the first unrelated jingle.
- Failed sound-buffer restoration returns to the game instead of retrying forever.
- Volume fades start from the current level, reach their target and release their transition slot. Fading to zero stops playback.
- Zero-duration volume transitions apply immediately without dividing by zero.
- Sound-effects volume changes preserve each sound's relative level and stereo position, including during fades. Muting sets the affected sounds to silence.
- Archive fallback keeps using the selected archive across search paths. Disabling search paths now also disables that fallback, and oversized expanded paths are skipped.
- Completed or inactive directory searches return normally instead of entering an endless loop.
- House, vehicle and tribe definitions reject invalid goods, passenger, job and animation indices. Oversized experience prerequisite lists cannot overwrite neighboring records.

## 2026-10-16 · 16:16:28 CEST — Retire the old DirectInput mouse backend

The remaster already uses Windows Raw Input for its active mouse handling. This cleanup removes the unused backend and its remaining build dependencies.

- Removed the legacy DirectInput mouse implementation and export-definition file.
- Removed DirectInput import-library generation, the symbol alias and the unused linker dependency.
- Kept the active mouse handling and cursor behavior unchanged.

## 2026-10-16 · 12:33 CEST — AI takeover for multiplayer maps played solo

Multiplayer maps can define several human starting positions. When one of these maps is launched through single-player mode, the other active human positions now receive AI control rather than remaining idle. This applies generally, including to cooperative maps.

- Kept the local player's starting position under human control.
- Assigned the other active human starting positions to the AI when starting a new multiplayer map in single-player mode.
- Preserved existing player assignments when loading saved games, along with the map's diplomacy and scripted AI controls.

## 2026-10-16 · 12:22 CEST — Weather in 32-bit color

Weather effects now work with the remaster's 32-bit graphics path. Obsolete 16-bit output code has also been removed while retaining support for legacy source assets.

- Ported rain, snow and sandstorm overlays to 32-bit color.
- Preserved weather simulation, effect ordering and the existing detail and zoom controls.
- Removed obsolete 16-bit output fallbacks, shader variants and darkening tables.

## 2026-10-16 · 11:52 CEST — Modern Windows presentation

The remaster replaces the old window-presentation system with DirectX 11 and a modern Windows window. The goal is a comfortable experience on current displays while preserving the original game's appearance.

- Added borderless fullscreen, mouse confinement and Alt+Enter switching to a resizable window.
- Stopped repeated desktop-resolution changes between startup images, videos, loading screens and menus.
- Sorted the resolution list by size and retained only 32-bit modes, without a redundant color-depth label.
- Preserved the aspect ratio of game resolutions such as 4:3, using black bars for unused screen space.
- Scaled the HUD, main menu, cursor and loading bar with the resolution, using the interface at 1024 × 768 as the reference.
- Enlarged the minimap and improved its frame, transparency, shadow and spacing. The narrow inner edge is retained.
- Stretched videos to fill the window and added smoothing when enlarging them.
- Corrected flickering between the game cursor and the Windows arrow.
