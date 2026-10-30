# OpenVikings Remaster Changelog

## 2026-10-29 · 22:33 CET

Game launchers use the original executable names.

- Cultures 2 uses `Cultures2.exe`.
- EightWonders and Northland use `Game.exe`.

## 2026-10-22 · 18:57 CEST

Computer villages manage construction and housing more effectively.

- Computer players give new homes priority after their first farm.
- Small computer villages use one builder, leaving more villagers available for gathering and production.
- Computer players use building materials already in their warehouses when planning construction.
- Villagers trapped inside construction sites can walk out.
- Reduced the size of the Egyptian headquarters pyramid.
- Cultures 2 movies no longer require `binkw32.dll`, and music no longer requires the separate `gedx8musicdrv.dll`.

## 2026-10-21 · 19:58 CEST

Computer opponents build more sustainable settlements and coordinate their armies.

- Computer players prioritize staffing existing workplaces, food production and housing before expanding or recruiting more soldiers.
- Computer villages limit farms, mills, bakeries, wells and beehives to two each and assign more workers to gathering raw materials.
- Computer families balance women and employed civilian workers to support population growth.
- Computer armies gather before attacking and aim for roughly 60% melee fighters and 40% archers. Easier difficulties have smaller armies.
- Destroyed computer warehouses leave their actual stored goods behind instead of a fixed food reward.
- Doubled hunters' search distance for live prey.
- Added civilization-specific headquarters for new Skirmish and multiplayer games: pyramids, castles, cathedrals and palaces.
- Added an archer appearance and an Arabian soundtrack for Egyptian settlements.
- Fixed an original artwork issue that left some Saracen woodcutters without heads while chopping.
- Felled trees blocked by rocks no longer remain permanently stuck.

## 2026-10-20 · 23:43 CEST

A new Skirmish menu makes games against computer opponents easier to set up.

- Separated story missions from Skirmish maps in the single-player menu.
- Added map previews, starting-position selection and difficulty selection to Skirmish setup.
- Added dropdowns for player type and civilization, plus a button to fill open positions with computer opponents.
- Improved menu readability with clearer map descriptions, previews and player lists.
- Computer opponents use their civilization's buildings and must gather, produce and transport their own supplies instead of receiving free resources.
- Added a multiplayer connection display showing each player's delay and connection stability.
- Added experimental Die Saga support, including its campaigns and user campaigns loaded from unpacked folders.

## 2026-10-19 · 23:56 CEST

More languages and another Cultures game are supported.

- Added Unicode support for names and messages, including multiplayer chat and the savegame editor. Available characters depend on installed Windows fonts; no new translations are included.
- Added experimental Cultures 2 support with its own menus, campaign map, animals, music and movies.

## 2026-10-18 · 18:45 CEST

New multiplayer connections and savegame tools expand how you can play.

- Added experimental LAN and direct-address multiplayer without DirectPlay. All players need the same OpenVikings version.
- Added a savegame editor for villager names, needs, health, equipment, experience and building inventories, with undo and redo.
- Added map export from saves, including terrain, buildings, villagers, animals, vehicles and signposts. Exported maps start a new game and do not retain mission progress or scripts.
- Music initialization no longer holds up menus and game startup.
- Automatic builder recruitment checks whether construction sites are reachable, reducing repeated assignments to inaccessible sites.

## 2026-10-17 · 03:35 CEST

Village life receives improvements to gathering and workshop presentation.

- Computer-controlled collectors leave gathered goods at their work flags for warehouse carriers and resume gathering when storage space becomes available.
- Improved Viking wooden-spearman sleep animations and the appearance of stone and marble in stonemason animations.
- Added the potter's oven fire during brick production and the baker's complete cake-making animation.

## 2026-10-16 · 20:48 CEST

Modern display options make the game more comfortable to play on current Windows PCs.

- Added borderless fullscreen and resizable windows, with Alt+Enter switching and mouse confinement.
- Menus, the interface, the cursor and loading screens scale with your resolution while preserving their proportions.
- Added a larger minimap and improved fullscreen movie presentation.
- Computer players take over other active human starting positions when you play multiplayer maps alone.
- Schools teach the selected skill instead of awarding experience for the pupil's previous job.
- Builders stop working on vehicles that another worker has already completed.
- Soldiers consider more valid targets within their weapon's permitted range.
