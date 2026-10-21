# OpenVikings Remaster Changelog

News and changes from the `remaster` branch and its separate `network` development branch. Times are local to Vienna, Austria; the entries below use CEST (UTC+02:00).

## 2026-10-21 · 11:35 CEST — Keep Saracen woodcutters' heads visible

- Use a complete original Saracen head animation during axe swings when the selected head variant lacks those frames, including selection and depth sorting.

## 2026-10-21 · 11:11 CEST — Native civilization figures and cargo palettes

- Restore original civilization soldier figures and complete native animations as the default; generated draft models remain available with `-remaster_sprites:2` while faithful weapon additions are developed.
- Apply the native clay material palette when Byzantine inhabitants carry clay without a vehicle.

## 2026-10-21 · 11:11 CEST — Egyptian settlement music

- Give Egyptian skirmish settlements the requested AddOn Arabs2 Standard, Wealthy and Danger music set.

## 2026-10-21 · 10:21 CEST — Longer swords without shields

- Replace the Byzantine and Egyptian draft swordsmen with longer blades and two-handed grips, removing their shields from every custom standing, walking and attack frame.

## 2026-10-21 · 09:41 CEST — Civilization longsword sprites

The experimental remaster asset pack now includes longsword soldiers for the civilizations without original longsword models.

- Add Byzantine, Saracen and Egyptian standing, walking and attack sprites with player colors, including previously Viking-looking units owned by those civilizations.
- Keep the original Viking and Frank longsword graphics, and use each civilization's native sword animations for missing poses.
- Preserve weapon values, attack events and saved unit data; the shared pack also retains the Saracen longbow sprites.

## 2026-10-21 · 07:32 CEST — Experimental Saracen longbow sprites

Saracen longbow soldiers can use a first set of custom sprites from the separate remaster asset pack.

- Add player-colored clothing and standing, walking and shooting poses, including existing Viking-looking longbow soldiers owned by a Saracen player.
- Use original Saracen bow animations for missing poses and retain existing weapons, combat timing and saved unit data.
- Copy `remaster.lib` beside the executable when building; original graphics remain available when the optional pack is absent or disabled.

## 2026-10-21 · 00:32 CEST — Named civilization building roles

- Replace numeric building choices with named house-type enums while preserving their existing values and behavior.

## 2026-10-21 · 00:14 CEST — Real wells and beehives for every civilization

- Use the original well model for every supported civilization in skirmish and multiplayer, retaining water production and civilization ownership.
- Use the original beehive model for every civilization too, retaining honey production.

## 2026-10-20 · 23:43 CEST — More space for skirmish map names

Balance the map browser and player controls with a tighter map preview.

- Widen the map list and move dimensions to the Map preview heading.
- Fit and center the visible preview artwork without its uneven transparent padding.
- Reduce the civilization column and give the recovered space to map names.

## 2026-10-20 · 23:35 CEST — Separate game setup responsibilities

Make the recent skirmish and multiplayer additions easier to maintain together.

- Give offline skirmish its own menu and keep shared player settings, startup choices and civilization profiles independent of the lobby protocol.
- Isolate automatic network synchronization policy from the simulation clock while retaining existing behavior and file formats.

## 2026-10-20 · 23:19 CEST — Clearer skirmish configuration

Keep map information and player controls together with less repetition.

- Show map titles and dimensions together in the map list, retaining full titles in tooltips.
- Remove the redundant You/Computer column from local skirmish; choose the human start slot through Control.
- Anchor the fill button at the bottom of the player panel.
- Use Computer instead of AI in the lobby and skirmish controls and help text.

## 2026-10-20 · 23:03 CEST — Lighter menu panels

Give skirmish information the translucent appearance of the original map-selection screen.

- Replace large brown panel fills with a darkened view of the scenery and a single frame.
- Draw map previews directly over that background, retaining transparent parchment edges.
- Remove the description's inner frame and show scrollbars only when content overflows.
- Remove the repeated Missions heading inside the mission list.

## 2026-10-20 · 20:13 CEST — Integrated skirmish map browser

Choose a map and configure players together on the skirmish screen.

- Add a scrollable map list, the original map preview and framed areas for descriptions and settings.
- Use the larger original menu font, narrower player-name fields and layouts suited to small and large screens.
- Keep complete descriptions accessible through scrolling and retain the selected map when adjusting players.
- Show fixed player colors at full brightness and explain when the map prevents changing them.

## 2026-10-20 · 19:10 CEST — Names for other players' buildings

Show the purpose of visible buildings belonging to other players across all civilizations.

- Display localized building names on hover and beside the preview in the General panel.
- Include production, residential and military buildings alongside headquarters and warehouses.

## 2026-10-20 · 18:58 CEST — Missions and local skirmish setup

Split the old Free Play selection into dedicated missions and configurable games against the AI.

- Add **Missions** and **Skirmish** (German: **Missionen** and **Gefecht**) while retaining available tutorials, campaigns and saved games.
- Configure local games with the multiplayer lobby's control/civilization dropdowns, player colors and AI filling.
- Assign the human player automatically, allow another start slot, and show map information and difficulty choices.
- Start locally and preserve the chosen human slot when loading new skirmish saves.
- Apply difficulty benefits to the human's chosen slot and retain normal AI resource gathering and construction costs.

## 2026-10-20 · 18:21 CEST — Complete AI civilization building roles

Non-Viking AI civilizations can use the full economic and military building purposes in new multiplayer games.

- Use each civilization's own available models, including farms, mills, bakeries and barracks; reuse suitable local models for missing purposes without changing their original role.
- Retain ordinary construction costs and enable the missing production chains instead of treating their goods as already produced.
- Show localized purpose names for visible foreign buildings.
- Restore session building profiles when loading new saves; these saves and multiplayer sessions require matching updated network builds.

## 2026-10-20 · 18:21 CEST — AI resources and soldier equipment

AI supply now depends on gathering, production and transport instead of the periodic five-unit stock mechanism.

- Remove the periodic stock gift and free initial AI warehouse construction goods in multiplayer.
- Let idle unarmed soldiers fetch an eligible available weapon through the normal pickup and equipment process.
- Preserve active and queued player orders when checking for weapons.

## 2026-10-20 · 18:21 CEST — Preserve the main-menu village

Prevent automatic AI demolition from dismantling the village shown behind the main menu.

- Suppress both random deletion and clearing a building to make room for a warehouse while the main menu is active.
- Retain the normal warehouse placement fallback during gameplay.

## 2026-10-20 · 15:39 CEST — Automatic synchronization and connection graphs

The multiplayer network window now shows connection history and chooses synchronization frequency automatically.

- Show a latency graph, current RTT, variation and connection status for each remote player, with pages for larger sessions.
- Measure the actual route between clients through the host and mark missing replies instead of showing a false zero latency.
- Display the active cycle mode and target interval; replace manual frequency controls with an automatic status page.
- Adapt to recent latency and variation, increasing frequency after stable measurements while preserving the normal simulation pace.
- Require matching current network builds for the new connection measurements.

## 2026-10-20 · 15:39 CEST — Lobby dropdown spacing

Give player-control and civilization captions more room inside their fields.

- Add consistent padding between text and the left frame.
- Keep long captions clear of the separate arrow area.

## 2026-10-20 · 15:14 CEST — Player control dropdown and arrow styling

Use matching dropdowns for player control and AI civilization in the multiplayer lobby.

- Replace the separate Human/AI/Off buttons with one control selector; unavailable map/savegame choices remain disabled.
- Draw dropdown arrows in a separate area at the right edge instead of appending a letter to the caption.
- Keep the existing mouse/keyboard selection and cancellation behavior, skipping disabled choices with arrow keys.

## 2026-10-20 · 15:02 CEST — Civilization dropdown in the lobby

Choose an AI civilization directly from its player row instead of opening a separate selection screen.

- Show the available civilizations and the map default in a dropdown using the game's button style.
- Support mouse selection, arrow keys and Enter; Escape or an outside click closes the list without changing the selection.
- Open upward near the bottom of the player table, keeping the list above the chat area.

## 2026-10-20 · 14:47 CEST — Civilization buildings and Egyptian AI storage

Apply the chosen AI civilization to compatible starting buildings and recognize Egypt's large main warehouse.

- Replace starting headquarters and other matching buildings before placement, using the selected civilization's own doors, footprint and building data.
- Use native warehouse counterparts for civilizations without a Viking headquarters; retain the original if no suitable building fits.
- Include large warehouses in AI resource delivery, warehouse construction and village placement.
- Apply building replacement when starting a new map; existing saved buildings are preserved.

## 2026-10-20 · 14:24 CEST — Built-in multiplayer map startup

Correct a startup message mismatch introduced by the new lobby settings.

- Send the extended lobby configuration when starting a built-in map, as already done after map/save transfers.
- Include AI civilization choices in direct starts so the synchronization screen can advance to loading.

## 2026-10-20 · 14:20 CEST — Multiplayer menu address display

Prevent local network addresses from overwriting menu memory when opening multiplayer.

- Use dynamically sized text for network status and address lists instead of the original 100-byte buffer.
- Allow complete numeric IPv6 addresses in the per-address buffer.

## 2026-10-20 · 14:09 CEST — Clearer multiplayer player setup

The experimental network lobby now has explicit player controls and an AI civilization picker.

- Assign connected players automatically and add AI to ordinary player start slots with **+ AI** or **Fill open slots with AI**.
- Replace the ambiguous **Type** button with **Human**, **AI** and **Off**, showing which choices the map allows.
- Choose the civilization of added AI players; fixed map opponents and existing savegames keep their civilization settings.
- Add compact rows and page navigation for longer player lists.
- Use protocol 3 to synchronize civilization choices; all participants need matching updated builds.

## 2026-10-20 · 12:37 CEST — Network transfer cancellation

Improve transfer failure reporting in the experimental network branch.

- Check transfer completion acknowledgements and notify the other peer when an active transfer is aborted.
- Use network protocol 2 for explicit transfer cancellation; all players need matching updated builds.

## 2026-10-20 · 12:37 CEST — Network send failures

Correct send admission in the experimental network branch.

- Report failed admission to destination send queues and local event queues.
- Keep slow recipients from disconnecting healthy forwarding peers.

## 2026-10-19 · 23:56 CEST — Bring network up to date with remaster

The experimental multiplayer branch now includes the current remaster improvements.

- Include the Die Saga launcher, campaigns and user campaign loading corrections.
- Include the latest Cultures 2 animal, audio, menu, campaign, minimap, movie and loading-bar changes, plus shared taskbar and focus corrections.
- Retain the modern Winsock transport, verified file transfers and Unicode network names and chat.

## 2026-10-19 · 23:47 CEST — Cultures 2 movies and loading bar

Support Cultures 2's BIK intro and mission videos through the original installation's Bink decoder.

- Use the installed `binkw32.dll` for video and audio playback.
- Restore Cultures 2's framed loading bar, its neutral grey shading and moving highlight, scaled to the selected resolution.

## 2026-10-19 · 23:29 CEST — Cultures 2 minimap frame colors

Blend the isolated red speckles in Cultures 2's enlarged minimap frame into its brown shading.

- Adjust only the affected frame color during drawing, keeping its transparency and other interface colors.

## 2026-10-19 · 23:10 CEST — Cultures 2 campaign map

Use Cultures 2's own campaign identity, mission positions and map sections in the campaign menu.

- Select campaign 1 instead of Weltwunder's campaign 3.
- Place the ten mission buttons at their original positions and draw unlocked sections from Cultures 2's menu graphics.
- Keep mission access tied to existing campaign progress.

## 2026-10-19 · 23:09 CEST — Taskbar visibility during startup

Correct the shared window startup so the game can appear in the taskbar from its first loading screen.

- Mark the main window explicitly as a taskbar application window.
- Keep window placement from changing visibility before the normal first show.
- Request foreground activation at normal startup and restore Windows input-focus handling on activation.

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

## 2026-10-18 · 18:45 CEST — A new multiplayer foundation on network

The separate `network` branch introduces an experimental native TCP/IP replacement for DirectPlay, integrated with the multiplayer menu and existing game synchronization.

- Discover IPv4 LAN sessions or search a host by name, IPv4 or IPv6 address.
- Exchange reliable, ordered messages with explicit packet limits, connection timeouts and host-controlled admission.
- Verify map and save transfers before replacing local files, and acknowledge completed downloads to the host.
- Correct command backups, network text conversion, log formatting and transfer progress calculations.
- Use the same OpenVikings build on every computer. This protocol is separate from original Cultures multiplayer; host migration and public matchmaking are outside this version.

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
