#pragma once

namespace NC2Logic
{
	bool Animal_l_Military_CheckForEnemy(void* animal);
	void Animal_l_StartTask_Attack(void* animal);
	bool Animal_l_Military_GetNextPointToStartAttack(const void* animal, void* point);
	bool Animal_l_Military_IsOtherAttackingAnimalOnPoint(const void* animal, const void* point);
	void Animal_l_Military_AttackTarget(void* animal, const void* point);
}