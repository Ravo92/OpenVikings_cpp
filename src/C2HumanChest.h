#pragma once
#include <cstdint>

namespace NC2Logic
{
	std::uint32_t Human_DoExecuteUserCommand_OpenChest(void* human, const void* point);
	void Human_l_StartTask_OpenChest(void* human);
	void Human_l_PerformTaskEvent_OpenChest(void* human);
	bool Tool_MapChest_UseChest(std::uint32_t humanId, const void* point);
	void Tool_TechTree_EnableGoodProduction(std::uint32_t player, std::uint32_t tribe, std::uint32_t good);
	std::uint32_t Tool_MapChest_GetMapChestTypeFromRandomType(std::uint32_t type);
	void Tool_MapChest_SetRandomChestOnRandomPosition(std::uint32_t mask);
	void Tool_MapChest_SetRandomChestOnPosition(std::uint32_t mask, const void* point);
}