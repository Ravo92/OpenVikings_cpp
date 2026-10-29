#pragma once
#include <cstdint>

namespace NC2Logic
{
	bool Human_Item_IsAmuletEquipped_Strength(const void* human);
	bool Human_Item_IsAmuletEquipped_CriticalHit(const void* human);
	bool Human_Item_IsAmuletEquipped_Defense(const void* human);
	void AIManager_Message_Attacked(void* manager, std::uint32_t player, const void* point);
	void AIManager_Messages_SoldierAttacked(void* manager, std::uint32_t player, std::uint32_t human, const void* target);
	bool House_Military_StartGettingHitByWeapon(void* house, std::uint32_t tribe, std::uint32_t weapon, std::uint32_t attackerId, std::uint32_t attackerRevision, std::uint32_t attackerType, std::uint32_t power);
	bool Vehicle_Military_StartGettingHitByWeapon(void* vehicle, std::uint32_t tribe, std::uint32_t weapon, std::uint32_t attackerId, std::uint32_t attackerRevision, std::uint32_t attackerType, std::uint32_t power);
	bool Human_Military_StartGettingHitByWeapon(void* human, const void* source, std::uint32_t tribe, std::uint32_t weapon, std::uint32_t attackerId, std::uint32_t attackerRevision, std::uint32_t attackerType, std::uint32_t power, bool propagate);
	void Vehicle_Military_GainExperience(const void* vehicle);
	std::uint32_t Human_GetExperienceForEquippedWeapon(const void* human);
	std::int32_t Tool_Military_GetDamage(std::int32_t damage, const void* attacker);
	bool Tool_Misc_GetFleePointFarestAwayFromTargetPointRelativToSourcePoint(const void* source, const void* target, void* result, std::uint32_t radius);
	void Animal_l_Aggressive_GetAngry(void* animal);
	bool Animal_Military_StartGettingHitByWeapon(void* animal, std::uint32_t tribe, std::uint32_t weapon, std::uint32_t attackerId, std::uint32_t attackerRevision, std::uint32_t attackerType, std::uint32_t power, bool propagate);
}