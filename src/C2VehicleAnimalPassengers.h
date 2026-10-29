#pragma once
#include <cstdint>

namespace NC2Logic
{
	bool Vehicle_Passengers_IsDraggingAnimal(const void* vehicle, std::uint32_t animalId, std::uint32_t revision);
	bool Vehicle_Passengers_IsHavingDraggingAnimal(const void* vehicle);
	bool Vehicle_Passengers_TryToGetDraggingAnimal(void* vehicle);
	bool Vehicle_Passengers_AnimalMoveIn(void* vehicle, std::uint32_t animalId);
}