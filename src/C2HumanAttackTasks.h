#pragma once
#include <cstdint>

namespace NC2Logic
{
	// Documentary landscape-target variant; direction/attempt WORDs overlap object revision storage.
	struct SMapPositionAttackTargetStorage
	{
		std::uint32_t type;
		std::int16_t x, y;
		std::int16_t baseDirection, attempts;
	};

	void Human_l_StartTask_Attack(void* human);
	void Human_l_FinishTask_Attack(void* human);
	void Human_l_FailTask_Attack(void* human);
	void Human_l_AttackTarget(void* human, const void* point);
	bool Human_l_IsOtherAttackingHumanOnPoint(const void* human, const void* point);
	bool Human_l_GetNextPointToStartAttack(void* human, void* point);
	bool Human_l_GetDistancePointFromTarget(const void* human, const void* source, const void* target, void* point, std::uint32_t radius);
	bool House_Military_GetNextFreeAttackPoint(const void* house, const void* source, void* point, const void* human, bool outside);
	bool Vehicle_Military_GetAttackPoint(const void* vehicle, const void* source, void* point, const void* human, bool outside);
	bool TargetData_MapPos_StartCheckForAttackPoint(void* target, const void* point);
	bool TargetData_MapPos_NextAttackPoint(void* target);
	void TargetData_MapPos_GetAttackPosition(const void* target, void* point);
}