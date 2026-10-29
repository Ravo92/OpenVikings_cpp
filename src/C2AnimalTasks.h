#pragma once
#include <cstdint>

namespace NC2Logic
{
	void Animal_DoUpdateConditions(void* animal);
	void Animal_l_Speed_Fast(void* animal);
	void Animal_DoUpdateAI(void* animal);
	bool Animal_DoNewMapPositionReached(void* animal, bool finalPosition);
	void Animal_DoPathfinderFailed(void* animal);
	void Animal_DoPathfinderStopped(void* animal);
	bool Animal_l_StartAtomicWalk(void* animal, const void* point);
	bool Animal_l_StartAtomicWalk(void* animal, std::int32_t moveableId, std::uint32_t distance);
	void Animal_l_StartTask_InteractWithHouse(void* animal);
	void Animal_l_StartTask_InteractWithVehicle(void* animal);
}