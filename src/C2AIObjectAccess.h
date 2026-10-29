#pragma once
#include <cstdint>

namespace NC2Logic
{
	void AIScout_Init(void* scout);
	void AIScout_Set(void* scout, std::int32_t human);
	bool AIScout_IsObjectUsed(const void* scout, const void* target);
	bool AIScout_IsValid(void* scout);
	bool AIScout_IsValidCheck(const void* scout);
	void AIScout_Clear(void* scout);
	bool AIScout_CheckAndChangeJob(const void* scout);
	bool AIScout_IsNearPos(const void* scout, const void* point);
	void AIScout_MoveToPos(const void* scout, const void* point);
	bool AIGoodManager_Producer_IsHouseValid(void* producer);
	bool AIGoodManager_Producer_IsHumanValid(void* producer);
	bool AIGoodManager_Producer_IsHumanValidCheck(const void* producer);
	void AIGoodManager_Producer_Clear(void* producer);
	std::uint8_t AIGoodManager_IsProducingGoodRealy(const void* manager, std::uint32_t good);
	void AIGoodManager_StopProduction(void* manager, std::uint32_t good);
	void AIGoodManager_ImportThisGoodType(void* manager, std::uint32_t good);
	void AIGoodManager_SetTraderBringFlag(void* manager, std::uint32_t good, bool active);
	std::uint32_t AIGoodManager_GetNumberOfUsedHumans(const void* manager);
	bool AITradeManager_IsObjectUsed(const void* manager, const void* target);
	bool AIGoodManager_IsObjectUsed(const void* manager, const void* target);
	bool AIVillageManager_IsObjectUsed(const void* village, const void* target);
	bool AIVillagesData_IsObjectUsed(const void* villages, const void* target);
	bool AIHighLevelHandler_IsObjectUsed(const void* handler, const void* target);
}