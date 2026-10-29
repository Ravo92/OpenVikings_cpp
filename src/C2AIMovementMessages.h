#pragma once
#include <cstdint>

namespace NC2Logic
{
	void AIManager_Message_NoWayFound(void* manager, std::uint32_t moveable, const void* target);
	void AIManager_Message_NoWayFoundToEnemy(void* manager, std::uint32_t moveable, const void* target);
	void AIManager_Message_StartSelfDefence(void* manager, std::uint32_t player, std::uint32_t human, const void* target);
	void AISinglePlayerHandler_Messages_NoWayFound(void* handler, const void* source, const void* target);
	bool AISinglePlayerHandler_l_Messages_GetNewValidPosition(void* handler, const void* source, void* target);
	void AISinglePlayerHandler_Messages_Human_NoWayFoundToEnemy(void* handler, void* human, const void* target);
	void AISinglePlayerHandler_Messages_Soldier_StartSelfDefence(void* handler, std::uint32_t human, const void* target);
	std::int32_t AISinglePlayerHandler_l_SingleSoldier_GetSlotIdByCreatureId(const void* handler, std::uint32_t human);
	bool AISinglePlayerHandler_l_SingleSoldier_CheckIfIsInTaskRange(const void* handler, std::uint32_t slot, bool defence);
	bool AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHuman(void* handler, std::uint32_t slot, std::uint32_t target, bool defence);
	bool AISinglePlayerHandler_l_SingleSoldiers_Let_AttackAnimal(void* handler, std::uint32_t slot, std::uint32_t target, bool defence);
	bool AISinglePlayerHandler_l_SingleSoldiers_Let_AttackVehicle(void* handler, std::uint32_t slot, std::uint32_t target, bool defence);
	bool AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHouse(void* handler, std::uint32_t slot, std::uint32_t target, bool defence);
	void AISinglePlayerHandler_l_SingleSoldier_StartSelfDefenceMode(void* handler, std::uint32_t slot);
	void __fastcall AISinglePlayerHandler_Messages_SoldierAttacked(void* handler, void* unusedEdx, std::uint32_t human, const void* target);
	bool AISingleSoldier_Tool_IsSelfDefending(const void* soldier);

	bool AISinglePlayerHandler_l_Vehicles_Let_AttackHuman(void* handler, std::uint32_t slot, std::uint32_t target, bool defence);
	bool AISinglePlayerHandler_l_Vehicles_Let_AttackVehicle(void* handler, std::uint32_t slot, std::uint32_t target, bool defence);
	bool AISinglePlayerHandler_l_Vehicles_Let_AttackHouse(void* handler, std::uint32_t slot, std::uint32_t target);
	bool AISinglePlayerHandler_l_Vehicles_CheckForEnemys(void* handler, std::uint32_t slot, const void* point, std::uint32_t radius, bool houses, bool civilians, bool restrictCandidates, bool defence);
	void AISinglePlayerHandler_l_Vehicles_WorkOnAttachedTasks(void* handler);
	bool AISinglePlayerHandler_l_Vehicles_IsSubTaskTargetValid(const void* handler, std::uint32_t slot);
	bool AISinglePlayerHandler_l_Soldiers_AttachToTower(void* handler, std::int32_t houseId);
	void AISinglePlayerHandler_l_Soldiers_AttachToTowers(void* handler);
	void* AISinglePlayerHandler_l_SingleSoldier_GetBestSoldier(void* handler, const void* point, std::uint32_t radius);
	void __fastcall AISinglePlayerHandler_Messages_Attacked(void* handler, void* unusedEdx, const void* point);
}