#pragma once
#include <cstdint>

namespace NC2Logic
{
	void GameAssistantManager_l_Assistant_DoWork(void* manager, std::int32_t player, std::uint32_t tick);
	void GameAssistantManager_l_Human_Command_Add(void* manager, std::uint32_t human, std::uint32_t command, std::uint32_t parameter1, std::uint32_t parameter2, std::uint32_t duration);
	std::int32_t GameAssistantManager_ls_Tool_FindWomanToGetChild(std::uint32_t player);
	bool GameAssistantManager_ls_Tool_GetHQPosition(std::uint32_t player, void* point);
	bool GameAssistantManager_ls_Tool_GetCenteredStockPosition(std::uint32_t player, void* point);
}