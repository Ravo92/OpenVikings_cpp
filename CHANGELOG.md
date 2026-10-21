# OpenVikings Remaster Changelog

Gameplay improvements and fixes, grouped by day. Multiplayer changes are developed on the separate `network` branch. Times are local to Vienna, Austria (CEST, UTC+02:00).

## 2026-10-21 · 18:50 CEST

- **AI:**
  - Computer players put available workers into existing workplaces before building more, and plan new warehouses around their workforce and defenses.
  - Computer players build at most two farms, mills, bakeries, wells and beehives each, rather than covering the map with workplaces they cannot staff.
  - Computer players aim to assign twice as many collectors to needed raw materials. Carriers collect their goods, and qualified collectors can move into crafting jobs.
  - Bread production, homes for homeless families and more frequent family planning help computer villages grow, with the same food costs and population limits.
  - Computer players recruit soldiers from spare workers while keeping essential production and transport staffed.
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
