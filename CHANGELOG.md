# OpenVikings Remaster Changelog

Changes on `remaster` and its separate `network` development branch, grouped by day. Times are local to Vienna, Austria (CEST, UTC+02:00).

## 2026-10-21 · 12:38 CEST

- Prioritize staffing existing AI production, families and defense before expanding settlements; reduce excessive construction crews and tie warehouse expansion to workforce and defense needs.
- Limit autonomous AI construction to two farms, mills, bakeries, wells and beehives per player, including upgrades and construction sites.
- Target two collectors per needed raw material, using collection flags and warehouse carriers, and let qualified collectors advance into crafts.
- Prioritize bread production, housing for homeless families and more frequent family planning while retaining food costs and population limits.
- Recruit soldiers from surplus workers without disrupting production, logistics or existing troops.
- Use the original well and beehive models for all supported civilizations, retaining water and honey production.
- Replace numeric civilization building choices with named house-type enums.
- Use original civilization figures and complete native animations for unsupported soldier weapon appearances, preserving actual weapons and combat values.
- Keep Saracen woodcutters' heads visible during axe swings and apply clay material colors to clay carried by Byzantine inhabitants.
- Use the AddOn Arabs2 Standard, Wealthy and Danger music set for Egyptian settlements.
- Recover obstructed falling trees, including those abandoned in older saves. When a trunk cannot fit, move its full wood quantity to a nearby safe pile and remove the tree; if no position is available, retain the final animation frame until placement is possible without replaying the fall or losing wood.

## 2026-10-20 · 23:43 CEST

- Separate Missions and Skirmish in single-player map selection while retaining tutorials, campaigns and saved games.
- Integrate a wide, scrollable map list, centered preview, dimensions beside the Map preview heading, descriptions and difficulty settings into Skirmish.
- Use translucent framed panels and the original menu font; remove redundant headings, player-name columns and nested description frames.
- Narrow civilization fields to leave more room for map names, place the fill button at the bottom of the player panel, and display fixed map colors clearly with an explanation of their restrictions.
- Use padded control and civilization dropdowns in Skirmish and multiplayer, with separate arrows, mouse and keyboard navigation, and Computer labels.
- Assign connected players automatically and allow filling available slots with computer players. In local Skirmish, allow choosing the human start position and apply difficulty benefits to that slot while retaining map restrictions and saved assignments.
- Give non-Viking computer players civilization-specific starting buildings and economic and military production chains, reusing suitable native models for missing building purposes.
- Include Egyptian large warehouses in resource delivery and settlement planning, and restore session building profiles from new saves.
- Display localized purpose names for visible buildings across civilizations, both on hover and in the General panel.
- Remove periodic five-unit AI stock gifts and free initial warehouse construction goods so computer players depend on natural resources, production and transport.
- Let idle unarmed soldiers collect suitable available weapons through normal equipment handling without replacing active orders.
- Protect the main-menu village from automatic demolition.
- Adapt multiplayer synchronization automatically to measured latency and variation while preserving simulation speed. Show per-player latency graphs, RTT, variation, connection status and the current cycle mode.
- Synchronize civilization choices consistently at startup, handle IPv6 and long address lists safely, acknowledge transfer cancellation, and report send-queue failures without disconnecting healthy forwarding peers. Participants need matching updated network builds.
- Separate offline Skirmish, shared player settings and automatic synchronization policy in the implementation.
- Add Die Saga's launcher and resource profile, campaign artwork and progress markers for Nordland, Wunder and Saga, and both user-campaign menus. Support original archives and unpacked campaign folders, keeping one entry per map GUID and preferring folders when both forms exist. Compatibility remains experimental.

## 2026-10-19 · 23:56 CEST

- Support UTF-8 keyboard, IME and clipboard input, character-safe editing, savegame-editor names and map text, with DirectWrite shaping and Windows font fallback for additional characters.
- Render fallback text upright. Font coverage depends on installed fonts; translations are not included.
- Preserve Unicode player and session names, chat and rename records on the network branch.
- Use Cultures 2's own menu scenery, text tables, animal selection, music directory and campaign map, with its ten mission buttons and unlocked sections tied to campaign progress.
- Start selected Cultures 2 maps directly without the later games' difficulty screen, and retain each game's own campaign identities.
- Decode Cultures 2's IMA-ADPCM effects with their recorded lengths and original 44,100 Hz playback rate.
- Restore Cultures 2's Letters and Extras for everyone page with its four assistance options, and omit unsupported difficulty text from its options window.
- Play BIK intro and mission videos through the installation's `binkw32.dll`, and restore the framed loading bar with neutral grey shading and a moving highlight.
- Blend isolated red speckles in Cultures 2's enlarged minimap frame into its brown shading.
- Make the first loading window visible in the taskbar and restore normal foreground activation and input focus.

## 2026-10-18 · 18:45 CEST

- Replace DirectPlay on the separate `network` branch with an experimental native Windows TCP/IP transport integrated with the multiplayer menu and game synchronization.
- Support IPv4 LAN discovery and direct hostname, IPv4 or IPv6 discovery, with reliable ordered messages, bounded packets, timeouts and host-controlled admission. Matching builds are required; original Cultures multiplayer, host migration and public matchmaking are outside this implementation.
- Verify map and save downloads before replacing files and acknowledge completion; correct command backups, text conversion, log formatting and transfer progress.
- Share one static engine library and common build settings between game launchers, with separate project intermediates and symbols. Northland's launcher is prepared; its data loading remains unimplemented.
- Initialize DirectMusic and process music loading and playback on a dedicated thread, retaining queued commands and volume settings while startup continues.
- Add a standalone savegame editor for villager names, needs, health, equipment, experience, building inventories and save information, with undo, redo and JSON export while preserving source saves and unknown data.
- Export saved worlds as starting maps with terrain, graphics, fish, players, buildings, stock, inhabitants, animals, vehicles and signposts, including native placement masks, pathfinding data and compressed DAT planes.
- Preserve permanent restrictions separately from unlockable progression in map exports. Mission definitions, mission progress, scripted AI and exact runtime state are not exported.
- Correct bounds and resource ownership in archive tables, paths, map/save packing, clipboard access and audio failure handling; skip unknown audio and retain free slots after rejected villager creation.
- Correct RGB555 blending and handle clock wrap in recovery and refresh timers.
- Reject invalid walking routes before accessing route buffers or map data.
- Recruit builders only for reachable work points on the same landmass, requiring connected signposts for longer automatic journeys.
- Follow signposts in the correct direction and retain worker destinations, fixing repeated failures to deliver workshop materials.
- Retry reachable AI attack approach positions without overwriting the new movement order, restoring stalled northern waves while retaining attack areas, timers and troop requirements.
- Start both fullscreen and windowed games on the Windows primary monitor.

## 2026-10-17 · 03:35 CEST

- Let computer villages harvest nearby deposits at collection flags independently of warehouse stock, with dedicated warehouse carriers bringing goods back. Pause when storage is full and resume when capacity returns, using normal harvesting, hauling and pathfinding.
- Restore complete resting animations for Viking wooden-spear soldiers without changing sleep duration or stamina recovery.
- Use the correct localized vehicle labels and help text for passenger disembarkation, unloading and docking.
- Keep rubble, stone blocks and marble in their intended stone colors during the Viking stonemason's work and carrying animations.
- Show the potter's oven fire during brick production and the baker's complete cake-making sequence, retaining cake through the final carrying step without changing production durations or the working bread sequence.

## 2026-10-16 · 20:48 CEST

- Add DirectX 11 presentation with borderless fullscreen, mouse confinement, Alt+Enter switching and resizable windows without repeated desktop-resolution changes.
- Sort 32-bit resolution choices, preserve game aspect ratios with letterboxing, and scale menus, HUD elements, cursors and loading bars with the display.
- Improve the enlarged minimap's frame, transparency, shadow and spacing, along with fullscreen video and cursor presentation.
- Render rain, snow and sandstorms through the 32-bit graphics path while retaining simulation, ordering, detail and zoom behavior.
- Replace the reconstructed legacy runtime, custom heaps, private compiler and math backends, and x87 bridges with supported Microsoft C++ runtime and Windows services, retaining Win32 and existing game-data formats.
- Replace remaining MMX sprite and terrain rendering with SSE2 while preserving integer rounding, saturation and HUD transparency; remove obsolete MMX detection, 16-bit output paths and the unused DirectInput mouse backend.
- Assign other active human start positions to computer control when launching multiplayer maps solo, preserving the local player, saved assignments, diplomacy and scripted AI.
- Correct school experience awards, equipment unlocks and permissions, and military profession changes for temporarily detached inhabitants.
- Use the correct sector counter for water navigation.
- Respect vehicle stock capacities and reservation limits, release promises after failed routes or door searches, handle failed food pickup and delivery safely, and let workers leave completed vehicle construction tasks.
- Allow the affected military targeting mode to consider targets 19 to 30 map points away while retaining weapon minimum ranges.
- Correct bounds and initialization in palette brightness, empty minimap samples, caption parsing, help links, text loading and map-edge markers; release slots for off-map projectiles.
- Parse WAV RIFF chunks and padding correctly, reject truncated input, and advance memory-backed audio read cursors.
- Release temporary sound resources on failures, avoid duplicate reloads, skip unknown jingles, and return from failed buffer restoration instead of retrying indefinitely.
- Correct volume transitions, including zero-duration fades and muting, preserve relative volume and stereo position, and stop playback when fading to zero.
- Respect archive fallback and search-path settings, terminate completed directory searches normally, and reject invalid building, vehicle, civilization and experience definitions before they overwrite neighboring data.
