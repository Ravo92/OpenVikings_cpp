#pragma once
#include "C2AIBuildingAccess.h"
#include "C2MapPathfinderSearch.h"
#include <cstddef>
#include <cstdint>
namespace NC2Logic
{
	// Physical Windows village storage. Slot-view offsets in legacy code include a -8 bias.
	struct SAIStockStorage
	{
		std::uint32_t point, houseId, uniqueId;
	};
	struct SAIHouseManagerStorage
	{
		std::uint8_t hasHome;
		std::byte unresolved01[3];
		std::uint32_t previousHouseCount;
		std::uint8_t allHousesLost;
		std::uint8_t buildAllowed[55];
		std::uint32_t stockCount;
		std::uint8_t stockConstructionPending;
		std::byte unresolved45[3];
		SAIStockStorage stocks[10];
		std::uint32_t defendPointCount;
		std::uint32_t defendPoints[100];
		std::int32_t barrackHouseId;
	};
	struct SAIVillageStorage
	{
		std::uint8_t allocated;
		std::byte unresolved01[3];
		std::uint32_t villageId;
		std::uint32_t playerId;
		std::uint32_t center;
		std::uint32_t radius;
		std::byte unresolved14[8];
		SAIHouseManagerStorage houses;
		std::byte goodsStorage[2640];
		SHouseBuild buildHouse;
		SHouseUpgrade upgradeHouse;
		std::int32_t towerHouseId;
		std::uint32_t type;
		std::int32_t resourceGood;
	};
	struct SAIVillageSlotStorage
	{
		std::uint8_t active;
		std::byte unresolved01[3];
		SAIVillageStorage village;
	};
	struct SAIVillagesStorage
	{
		std::uint32_t count;
		std::uint32_t nextVillageId;
		SAIVillageSlotStorage slots[8];
		std::uint32_t pendingVillageType;
		std::int32_t pendingVillageGood;
		std::uint8_t pending;
		std::uint8_t confirmed;
		std::int16_t pendingX;
		std::int16_t pendingY;
		std::byte unresolved6796[2];
	};

	std::int32_t AIHighLevelHandler_Tool_GetUnusedHuman(const void* handler, const void* point);
	void AIVillageManager_Init(void* village, std::uint32_t type, std::int32_t good, std::uint32_t id, std::uint32_t player, const void* point, std::uint32_t radius);
	void AIVillageManager_DoOneLogicTick(void* village, std::uint32_t tick);
	void AIVillageManager_CheckAndSetGoodData(void* village, const void* resources);
	bool AIVillageManager_Good_GetWorkCenterPoint(const void* village, std::uint32_t good, void* point);
	std::int32_t AIVillageManager_Good_GetWorkHouseId(void* village, std::uint32_t good);
	std::int32_t AIVillageManager_Good_AskForNewHouse(void* village, std::uint32_t good, const void* resources);
	std::int32_t AIVillageManager_Good_GetWorkHumanId(const void* village, const void* point);
	void AIHouseManager_Init(void* manager);
	void AIHouseManager_ClearAllNoHouseCanBeBuildFlags(void* manager);
	bool AIHouseManager_Military_IsBarrackBuild(const void* manager);
	void AIVillagesData_Init(void* villages, std::uint32_t player, const void* point);
	void AIVillagesData_DoOneLogicTick(void* villages, const void* resources, void* goods, std::uint32_t tick);
	void AIVillagesData_l_Village_Add(void* villages, std::uint32_t type, std::int32_t good, std::uint32_t player, const void* point, std::uint32_t radius);
	void AIVillagesData_l_Village_Clear(void* villages, std::int32_t id);
	std::int32_t AIVillagesData_House_BuildHouse(void* villages, std::uint32_t house, bool disableOnFailure, MapSearchCallback callback, std::uintptr_t userData);
	bool AIVillagesData_Military_IsBarrackBuild(const void* villages);
	std::int32_t AIVillagesData_Military_GetBarrackHouseId(const void* villages);
	bool AIVillagesData_l_GetVillageFreePoint(const void* villages, void* point, std::uint32_t radius);
	std::int32_t AIVillagesData_Villages_CreateNew(void* villages, std::uint32_t type, std::uint32_t good, void* resources, void* invalidAreas);
}
