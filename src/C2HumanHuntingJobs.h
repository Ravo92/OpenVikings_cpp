#pragma once
#include <cstdint>

namespace NC2Logic
{
	void Human_l_StartTask_ExecuteJob_Hunter(void* human);
	void Human_l_StartTask_ExecuteJob_Hunter_Collect(void* human);
	void Human_l_StartTask_ExecuteJob_Hunter_TryToCollect(void* human);
	void Human_l_StartTask_ExecuteJob_Hunter_Harvest(void* human);
	void* Human_FindAnimalForHunting(const void* human);
	bool Human_FindCadaverForHarvesting(const void* human, void* realPosition, void* workPosition, std::uint32_t* good, const void* source);
}