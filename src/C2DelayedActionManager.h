#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct SDelayedAction
	{
		std::uint8_t allocated;
		std::byte padding01[3];
		std::uint32_t elapsedTicks, delayTicks, type, attackerType, attackerId, attackerRevision;
		std::uint32_t targetPosition, weapon, tribe, player, sourcePosition;
	};

	struct SDelayedHitObjectArray
	{
		std::uint32_t count;

		struct Entry
		{
			std::uint32_t type, id;
		} entries[100];
	};

	static_assert(sizeof(SDelayedAction) == 48);
	static_assert(offsetof(SDelayedAction, targetPosition) == 0x1C);
	static_assert(sizeof(SDelayedHitObjectArray) == 804);
	void __fastcall DelayedActionManager_System_DoOneGameTick(void* manager, void* unusedEdx);
	bool DelayedActionManager_DelayedAction_AddWeaponHit(void* manager, std::uint32_t delay, std::uint32_t attackerType, std::uint32_t attackerId, const void* point, std::uint32_t weapon);
	void DelayedActionManager_l_DelayedAction_Execute(void* manager, const SDelayedAction* action);
	void DelayedActionManager_FillHitObjectList(void* manager, const void* point, const SDelayedAction* action, SDelayedHitObjectArray* targets);
	bool DelayedActionManager_HitTargetsInList(void* manager, std::uint32_t count, const SDelayedAction* action, const SDelayedHitObjectArray* targets);
	bool House_IsPositionInHouse(const void* house, const void* point);
	bool Vehicle_IsPositionInsideVehicle(const void* vehicle, const void* point);
	bool Tool_HitLandscapeOnPoint(std::uint32_t player, std::uint32_t tribe, std::uint32_t weapon, const void* point);
}