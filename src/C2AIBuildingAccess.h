#pragma once

#include "C2MapPathfinderSearch.h"

namespace NC2Logic
{
	struct SHouseBuild
	{
		std::uint8_t active;
		std::byte padding01[3];
		std::int32_t houseId;
		std::uint32_t uniqueId;
		bool IsValid();
		bool IsValidCheck() const;
		void BuildHouse(std::uint32_t player, const void* point, std::uint32_t engineType, std::uint32_t stage, bool fillConstructionStock);
	};
	struct SHouseUpgrade
	{
		std::uint8_t active;
		std::byte padding01[3];
		std::uint32_t stage;
		std::int32_t houseId;
		std::uint32_t uniqueId;
		bool IsValid();
		void UpgradeHouse(void* house);
	};
	static_assert(sizeof(SHouseBuild) == 12);
	static_assert(sizeof(SHouseUpgrade) == 16);

	bool AIVillage_IsPointInside(const void* village, const void* point);
	bool AIHouseManager_Tower_IsPointInDefendRange(const void* manager, const void* point);
	bool AIHouseManager_Tower_IsHouseInDefendRange(const void* manager, const void* house);
	bool AIGoodManager_IsProducingGood(void* manager, std::uint32_t good, bool includeInputs);
	bool AI_IsEnemyObjectAround(std::uint32_t player, const void* point, std::uint32_t radius);
	std::int32_t Remaster_AI_GetStockHouseId(std::uint32_t tribe, std::uint32_t* stage);
	bool __cdecl slAIVillageManagerSearchBuildPos(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* data);
	bool __cdecl slAITowerManagerSearchBuildPos(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* data);
	bool AIVillageManager_l_House_GetPossibleBuildPosition(void* village, const void* source, std::int32_t engineType, std::uint32_t stage, void* output, bool checkTerrain, MapSearchCallback callback, std::uint32_t userData);
	std::int32_t AIVillageManager_House_BuildHouse(void* village, std::uint32_t logicType, const void* source, bool disableOnFailure, MapSearchCallback callback, std::uint32_t userData);
	std::int32_t AIVillageManager_House_TryToUpgradeHouse(void* village, void* house);
	void AITowerManager_DoOneLogicGameTick(void* manager, std::uint32_t player, void* houseManager, void* village);
}
