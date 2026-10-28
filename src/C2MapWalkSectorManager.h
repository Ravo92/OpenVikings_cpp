#pragma once

#include "C2CallbackManager.h"

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	class CIoHelper;
	struct SSectorGridCursor;

	struct SMapWalkSectorData
	{
		std::uint8_t calculated;
		std::uint8_t connectionFlags;
		std::uint8_t continentId;
		std::byte unknown03;
		std::uint32_t connectionValues[8];
		std::uint32_t basePoints[3];
		std::uint8_t basePointCount;
		std::byte unknown31[3];
	};

	class CMapWalkSectorManager
	{
	public:
		struct SBasePointCandidate
		{
			std::int32_t selectionCount;
			std::uint32_t packedBasePoint;
			std::uint32_t lowBitsValue;
		};

		struct SStaticData
		{
			std::int32_t gridWidth;          // +0x0000: number of sector columns.
			std::int32_t gridHeight;         // +0x0004: number of sector rows.
			SMapWalkSectorData* landSectors; // +0x0008
			SMapWalkSectorData* waterSectors; // +0x000C
			std::uint32_t* calculationMap;   // +0x0010: one DWORD per sector.
			std::byte specialSectorSearchWorkspace[96]; // +0x0014: special sector pathfinder, reconstructed in C2MapPathfinderSectorSearch.
			SBasePointCandidate basePointCandidates[250]; // +0x0074
			std::uint32_t floodFillPoints[8][400];         // +0x0C2C
			std::int32_t floodFillPointCounts[8];          // +0x3E2C
			std::int32_t nextFloodFillPointIndices[8];     // +0x3E4C
			std::uint8_t requiresSectorRecalculation;      // +0x3E6C: incompatible loaded sector data.
			std::byte unknown3E6D[3];
		};

		// Original data: DAT_00554ce0.
		inline static CMapWalkSectorManager* sTheObjectPtr = nullptr;

		// Original storage: 00550E70-00554CDF.
		inline static SStaticData mStaticVars{ };

		// Original function: FUN_0044f16d.
		CMapWalkSectorManager();

		// Original function: FUN_0044f1a2.
		~CMapWalkSectorManager();
		CMapWalkSectorManager(const CMapWalkSectorManager&) = delete;
		CMapWalkSectorManager& operator=(const CMapWalkSectorManager&) = delete;

		// Original function: FUN_0044f232.
		static void AllocateArrays();

		// Original function: FUN_0044f379.
		static void RegisterCallbacks();

		// Original function: FUN_0044f3c9.
		static void l_Sectors_CalculationMap_ClearAll();

		// Original function: FUN_0044f3ea.
		static void l_Sectors_ClearAll();

		static void l_Sectors_RecalculateAll();
		static void l_Sectors_UpdateSector(const SSectorGridCursor& cursor);
		static void l_Sectors_SpecialSectorFloodFill(SMapWalkSectorData& sector, const SSectorGridCursor& cursor);
		static void l_Sectors_GetNextBasePoint(std::uint32_t* points, std::int32_t& count, std::int32_t& nextIndex, std::uint32_t& point);
		static bool __cdecl l_Sectors_Callback(TCallbackTypes type, std::uintptr_t userData, std::uintptr_t param1, std::uintptr_t param2, std::uintptr_t param3);
		static SMapWalkSectorData* GetLandSectorDataPtr(const SSectorGridCursor& cursor);
		static SMapWalkSectorData* GetWaterSectorDataPtr(const SSectorGridCursor& cursor);
		static void l_Sectors_CheckAndSetConnection(SSectorGridCursor cursor, std::uint32_t direction);
		static std::uint32_t Sectors_CalculationMap_GetValue(const SSectorGridCursor& cursor);
		static void l_Sectors_CalculationMap_SetValue(const SSectorGridCursor& cursor, std::uint32_t value);
		static bool WayPossible(const SSectorGridCursor& source, const SSectorGridCursor& destination, std::uint32_t clearance, bool land, std::int32_t player);
		static SMapWalkSectorData* l_GetNextSectorPtr(const SSectorGridCursor& source, const SSectorGridCursor& destination, std::uint32_t clearance, bool land, SMapWalkSectorData*& second, std::uint32_t radius, const void* sourcePoint, std::int32_t player);
		static bool GetNextSectorBasePoint(const void* source, const void* destination, std::uint32_t clearance, bool land, void* firstPoint, void* secondPoint, std::uint32_t radius, std::int32_t player);
		static bool __stdcall AI_Military_GetFormPoint(std::int32_t player, const void* source, const void* destination, void* output);
		static void IO_Savegame_DoSave(CIoHelper& io);
		explicit CMapWalkSectorManager(CIoHelper& io);
		static bool IO_Cleanmap_Load(CIoHelper& io);
	};

	static_assert(sizeof(CMapWalkSectorManager) == 1);
	static_assert(sizeof(CMapWalkSectorManager::SStaticData) == (sizeof(void*) == 8 ? 16000 : 15984));
	static_assert(offsetof(CMapWalkSectorManager::SStaticData, gridWidth) == 0);
	static_assert(offsetof(CMapWalkSectorManager::SStaticData, gridHeight) == 0x04);
	static_assert(sizeof(SMapWalkSectorData) == 52);
	static_assert(offsetof(SMapWalkSectorData, calculated) == 0);
	static_assert(offsetof(SMapWalkSectorData, connectionFlags) == 1);
	static_assert(offsetof(SMapWalkSectorData, continentId) == 0x02);
	static_assert(offsetof(SMapWalkSectorData, connectionValues) == 0x04);
	static_assert(offsetof(SMapWalkSectorData, basePoints) == 0x24);
	static_assert(offsetof(SMapWalkSectorData, basePointCount) == 0x30);
	static_assert(sizeof(CMapWalkSectorManager::SBasePointCandidate) == 12);
	static_assert(offsetof(CMapWalkSectorManager::SBasePointCandidate, packedBasePoint) == 0x04);
	static_assert(offsetof(CMapWalkSectorManager::SBasePointCandidate, lowBitsValue) == 0x08);
	static_assert(offsetof(CMapWalkSectorManager::SStaticData, landSectors) == 0x08);
	static_assert(offsetof(CMapWalkSectorManager::SStaticData, waterSectors) == 8 + sizeof(void*));
	static_assert(offsetof(CMapWalkSectorManager::SStaticData, calculationMap) == 8 + 2 * sizeof(void*));
	static_assert(offsetof(CMapWalkSectorManager::SStaticData, specialSectorSearchWorkspace) == 8 + 3 * sizeof(void*));
	static_assert(offsetof(CMapWalkSectorManager::SStaticData, basePointCandidates) == 104 + 3 * sizeof(void*));
	static_assert(offsetof(CMapWalkSectorManager::SStaticData, floodFillPoints) == 3104 + 3 * sizeof(void*));
	static_assert(offsetof(CMapWalkSectorManager::SStaticData, floodFillPointCounts) == 15904 + 3 * sizeof(void*));
	static_assert(offsetof(CMapWalkSectorManager::SStaticData, nextFloodFillPointIndices) == 15936 + 3 * sizeof(void*));
	static_assert(offsetof(CMapWalkSectorManager::SStaticData, requiresSectorRecalculation) == 15968 + 3 * sizeof(void*));
}
