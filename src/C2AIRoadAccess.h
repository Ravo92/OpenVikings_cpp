#pragma once
#include <cstdint>

namespace NC2Logic
{
	void AIRoadManager_Init(void* manager);
	void AIRoadManager_DoOneLogicTick(void* manager, std::uint32_t player, const void* resources);
}