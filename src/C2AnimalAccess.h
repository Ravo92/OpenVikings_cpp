#pragma once
#include <cstdint>

namespace NC2Logic
{
	bool Animal_Misc_CanBeAttackedByHuman(const void* animal, std::int32_t humanId, bool allowOwnAnimals);
	const void* Animal_GetMapPositionRef(const void* animal);
	bool Animal_GetMapPosition(const void* animal, void* point);
	const void* Animal_GetAnimalWorkCenterPointRef(const void* animal);
	std::uint32_t Animal_GetCurrentAtomicAnimationLengthInTicks(const void* animal);
	std::uint32_t Animal_HitPoints_GetMaximum(const void* animal);
	bool Animal_Init(void* animal, std::int32_t id, std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags);
	void Animal_Exit(void* animal);
	void Animal_HouseMisc_Exit(void* animal);
	bool Animal_l_IsAtomicInterruptable(const void* animal);
	bool Animal_l_StartAtomicAnimation(void* animal, std::uint32_t action, std::int32_t direction);
	bool Animal_l_IsLeaderValid(const void* animal);
	bool Animal_IsAttachedToHouse(const void* animal);
	bool Animal_IsAttachedToVehicle(const void* animal);
	void Animal_AttachToHouse(void* animal, std::int32_t houseId);
	bool Animal_AttachToVehicle(void* animal, std::int32_t vehicleId);
	void Animal_l_MoveWorkCenterPoint(void* animal);
	void Animal_GoIntoHouseInteractionMode(void* animal);
	void Animal_l_Aggressive_Check(void* animal);
}