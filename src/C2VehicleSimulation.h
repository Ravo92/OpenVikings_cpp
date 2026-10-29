#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Physical Windows fragments; existing byte access and ownership remain unchanged.
	struct SVehiclePassengerReferenceStorage
	{
		std::uint32_t objectId, objectRevision;
		std::uint8_t inside;
		std::byte padding09[3];
	};

	struct SVehicleMilitaryTargetStorage
	{
		std::uint32_t type, objectIdOrMapPosition, objectRevision;
	};

	struct SVehicleMilitaryStateStorage
	{
		SVehicleMilitaryTargetStorage targets[2];
		std::uint32_t activeTargetIndex, militaryMode, carrierVehicleId, carrierVehicleRevision;
		std::uint8_t insideVehicle;
		std::byte padding29[3];
	};
	struct SStaticAtomicAnimationEventData;
	void Vehicle_DoUpdateConditions(void* vehicle);
	void Vehicle_DoUpdateAI(void* vehicle);
	void Vehicle_CurrentTask_DoPerform(void* vehicle);
	bool Vehicle_DoNewMapPositionReached(void* vehicle, bool destinationReached);
	void Vehicle_DoPathfinderFailed(void* vehicle);
	bool Vehicle_IsTaskOrAttachmentActive(const void* vehicle);
	void Vehicle_l_Error_NoCommander(const void* vehicle);
	void Vehicle_WalkSpeed_Update(void* vehicle, std::uint32_t terrainSpeed);
	bool Vehicle_DoExecuteNextUserCommand(void* vehicle);
	bool Vehicle_DoExecuteUserCommand(void* vehicle, const void* command);
	bool Vehicle_l_UserCommands_DetachFromVehicle(void* vehicle);
	bool Vehicle_DoExecuteUserCommand_Goto(void* vehicle, const void* point);
	bool Vehicle_DoExecuteUserCommand_Dock(void* vehicle, const void* point);
	bool Vehicle_l_Passengers_MoveIn(void* vehicle);
	bool Vehicle_Passengers_AttachVehicle(void* vehicle, std::uint32_t id);
	bool Vehicle_Passengers_MoveIn_v(void* vehicle, std::uint32_t id);
	bool Vehicle_Passengers_CanVehicleMoveIn(const void* vehicle, std::uint32_t id);
	void Vehicle_Passengers_ExtractPassengerSlot(void* vehicle, void* slot);
	bool Vehicle_Attachment_EnterVehicle(void* vehicle);
	bool Vehicle_l_StartAtomicDock(void* vehicle);
	bool Vehicle_l_StartAtomicUnload(void* vehicle, bool includeCommander);
	void Vehicle_l_ResetAtomic(void* vehicle);
	void Vehicle_l_PerformTaskEvent(void* vehicle, const SStaticAtomicAnimationEventData* event);
	bool Vehicle_l_Military_CheckForEnemys(void* vehicle, const void* point, std::uint32_t minimumDistance, std::uint32_t maximumDistance);
	bool Vehicle_l_Military_AttackTask(void* vehicle);
	bool Vehicle_l_Military_ScanForAttackPoint(void* vehicle, const void* source, void* point);
	void Vehicle_l_StartAtomicAttachToVehicle(void* vehicle);
}