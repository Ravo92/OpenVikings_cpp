#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2E2
{
	// Windows macro-map storage; section arrays and manager entries retain owned allocation.
	struct SMacroMapSectionStorage
	{
		std::uint32_t count;
		void* entries;
	};

	struct SMacroMapNameStorage
	{
		char name[40];
		std::int32_t staticDataId;
	};

	struct SMacroMapPatternStorage
	{
		std::uint16_t x, y;
		std::uint8_t triangleA;
		std::byte padding05[3];
		std::uint32_t nameIndex;
	};

	struct SMacroMapTransitionStorage
	{
		std::uint16_t x, y;
		std::uint32_t layer;
		std::uint8_t triangleA;
		std::byte padding09[3];
		std::uint32_t nameIndex;
	};

	struct SMacroMapLandscapeStorage
	{
		std::uint16_t x, y;
		std::uint32_t nameIndex, valency;
		std::int32_t playerId;
	};

	struct SMacroMapByteValueStorage
	{
		std::uint16_t x, y;
		std::uint8_t value;
		std::byte padding05;
	};

	struct SMacroMapCreatureStorage
	{
		std::uint16_t x, y;
		std::int32_t type;
		std::uint32_t tribe, player;
	};

	struct SMacroMapHouseStorage
	{
		std::uint32_t player;
		std::uint16_t x, y;
		std::int32_t type;
		std::uint32_t stage;
		std::uint8_t completed;
		std::byte padding11[3];
	};

	struct SMacroMapFishSwarmStorage
	{
		std::uint16_t x, y;
		std::uint32_t fishCount;
	};

	struct SMacroMapCaptureUsageStorage
	{
		std::uint32_t count;
		std::int32_t nameIndex;
	};

	struct SMacroMapStorage
	{
		std::uint8_t initialized;
		char filenameRegion[261];
		std::uint16_t width, height, centerX, centerY;
		std::byte padding10E[2];
		SMacroMapSectionStorage patternNames, patterns, transitionNames, transitions, landscapeNames, landscapes, heights, vertexColors, roads, humans, animals, houses, fishSwarms;
	};

	struct SMacroMapManagerStorage
	{
		std::int32_t count;
		SMacroMapStorage** entries;
	};

	static_assert(sizeof(SMacroMapSectionStorage) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SMacroMapSectionStorage, entries) == sizeof(void*));
	static_assert(sizeof(SMacroMapManagerStorage) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(sizeof(SMacroMapStorage) == (sizeof(void*) == 8 ? 480 : 376));
	static_assert(offsetof(SMacroMapStorage, patternNames) == 272);
	static_assert(sizeof(SMacroMapNameStorage) == 44);
	static_assert(sizeof(SMacroMapPatternStorage) == 12);
	static_assert(sizeof(SMacroMapTransitionStorage) == 16);
	static_assert(sizeof(SMacroMapLandscapeStorage) == 16);
	static_assert(sizeof(SMacroMapByteValueStorage) == 6);
	static_assert(sizeof(SMacroMapCreatureStorage) == 16);
	static_assert(sizeof(SMacroMapHouseStorage) == 20);
	static_assert(sizeof(SMacroMapFishSwarmStorage) == 8);

	using MacroMapPositionCallback = bool(__cdecl*)(const void* point, std::uint32_t triangle);
	void* MacroMap_Construct(void* map, const char* filename);
	void MacroMap_Destruct(void* map);
	void MacroMap_l_FreeAllData(void* map);
	bool MacroMap_LoadFromIniFile(void* map, const char* filename);
	void MacroMap_CreateFromGameMap(void* map, const char* filename, bool includeCreatures, std::int16_t minX, std::int16_t minY, std::int16_t maxX, std::int16_t maxY, bool mirrorX, bool mirrorY);
	void MapIO_NewMap(void* map, bool enableTime, std::int16_t width, std::int16_t height, bool initializeAllPlayers);
	void MapIO_RebuildCurrentMapViaMacroMap(std::int16_t width, std::int16_t height);
	void EngineMap_PostLoad_RebuildIfRequired();
	void MacroMap_SetPatternToGameMap(void* map, const void* root, MacroMapPositionCallback callback);
	void MacroMap_SetTransitionsToGameMap(void* map, const void* root, MacroMapPositionCallback callback);
	void MacroMap_SetLandscapesToGameMap(void* map, const void* root, MacroMapPositionCallback callback, bool preserveWalkingBlock);
	void MacroMap_SetHeightsToGameMap(void* map, const void* root, std::int32_t mode, MacroMapPositionCallback callback);
	void MacroMap_SetVertexColorsToGameMap(void* map, const void* root, MacroMapPositionCallback callback);
	void MacroMap_SetRoadsToGameMap(void* map, const void* root, MacroMapPositionCallback callback);
	void MacroMap_SetHumansToGameMap(void* map, const void* root);
	void MacroMap_SetAnimalsToGameMap(void* map, const void* root);
	void MacroMap_SetHousesToGameMap(void* map, const void* root);
	void MacroMap_SetFishSwarmsToGameMap(void* map, const void* root);
	std::int32_t Engine2d_EntryPoint_Edit_SetHouse(std::uint32_t player, std::int32_t engineType, std::uint32_t stage, const void* point, bool completed);
	void* __fastcall MacroMapManager_Construct(void* manager, void* unusedEdx);
	void MacroMapManager_RescanMacroMaps(void* manager);
	void MacroMapManager_l_FreeAllMacroMaps(void* manager);
}