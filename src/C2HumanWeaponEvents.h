#pragma once
#include <cstdint>

namespace NC2Logic
{
	void Human_l_PerformTaskEvent_Attack(void* human);
	void Human_Military_ShootArrowAtPosition(void* human, std::uint32_t position);
}