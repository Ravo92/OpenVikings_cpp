#pragma once
#include "C2AITaskAccess.h"
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Physical Windows records, separate from the owning player handler.
	struct SAISingleSoldierStorage
	{
		std::uint32_t humanId, humanUniqueId, weaponType, armorType;
		std::uint8_t usesDefaultTask;
		std::byte padding[3];
		SAITaskSimpleDescriptionStorage task;
		std::uint32_t currentAction, combinedTaskIndex, movementDestination, movementRadius;
		std::uint8_t selfDefence;
		std::byte targetPadding[3];
		std::uint32_t attackTarget[3];
	};

	struct SAISingleVehicleStorage
	{
		std::uint32_t vehicleId, vehicleUniqueId, taskIndex, attackActive, attackTarget[3];
	};

	static_assert(sizeof(SAISingleSoldierStorage) == 84);
	static_assert(sizeof(SAISingleVehicleStorage) == 28);

	void AISingleSoldier_Init(void* soldier, std::uint32_t human);
	void AISingleSoldier_Tool_StopVAAndVCTask(void* soldier);
	void AISinglePlayerHandler_l_SingleSoldiers_RecreateListCompletly(void* handler);
	void AISinglePlayerHandler_l_SingleSoldiers_UpdateList(void* handler);
	void AISinglePlayerHandler_l_SingleSoldiers_ClearInvalidTask(void* handler);
	void AISinglePlayerHandler_l_SingleSoldier_CheckFormOrAttack(void* handler, std::uint32_t task);
	void AISinglePlayerHandler_l_SingleSoldiers_UpdateAttachedTasks(void* handler);
	void AISinglePlayerHandler_l_SingleSoldiers_WorkOnAttachedTasks(void* handler);
	bool AISinglePlayerHandler_l_SingleSoldiers_CheckAttack(void* handler, std::uint32_t slot, const void* point, std::uint32_t radius, bool houses, bool civilians, bool restrictCandidates, bool defence);
	bool AISinglePlayerHandler_l_SingleSoldiers_Let_GotoPosition(void* handler, std::uint32_t slot, const void* point, std::uint32_t radius);
	void AISinglePlayerHandler_l_SingleSoldiers_RefreshAllEnergyAndCondition(void* handler);
	void AISinglePlayerHandler_l_Vehicles_RecreateListCompletly(void* handler);
	void AISinglePlayerHandler_l_Vehicles_UpdateList(void* handler);
	void AISinglePlayerHandler_l_Vehicles_CheckForCommander(void* handler);
}