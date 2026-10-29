#pragma once

#include <cstdint>

namespace NC2Logic
{
	bool Tool_Misc_GetPlayerCenterPosition(std::uint32_t playerId, std::uint32_t* mapPoint);
}

namespace NC2InGameGuiEntryPoints
{
	bool C2IGG_EntryPoint_PlayerCenterPosition_Get(std::uint32_t playerId, std::uint32_t* mapPoint);
	void C2IGG_EntryPoint_MainDisplay_SetToMapPosition(const void* mapPoint);
	void C2IGG_EntryPoint_MainDisplay_SetControllingPlayerCenterPosition();
}