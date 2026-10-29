#pragma once
#include <cstdint>

void __fastcall InGameGuiManager_l_Selection_ActionButtons_BringUp(void* manager);
std::uint32_t __fastcall InGameGuiManager_l_Selection_ActionButtons_GetChecksum(void* manager);

namespace NC2InGameGuiManager
{
	bool ls_Tool_SendSimpleHumanNetworkCommand(std::uint32_t action, std::int32_t id, std::uint32_t first, std::uint32_t second);
	bool HumanActionButtons_HandleCommand(std::uint32_t action);
}