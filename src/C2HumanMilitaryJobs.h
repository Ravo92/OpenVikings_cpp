#pragma once

#include <cstdint>

namespace NC2Logic
{
	bool Human_l_CheckForEnemyTarget(void* human, const void* point, std::uint32_t minimumDistance, std::uint32_t maximumDistance, void* target);
	void Human_l_StartTask_ExecuteJob_Soldier(void* human);
}