#pragma once
#include <cstdint>

namespace NC2Logic
{
	bool Vehicle_Attachment_IsInsideVehicle(const void* vehicle);
	void Vehicle_ChangePlayerId(void* vehicle, std::uint32_t player);
	bool Vehicle_Stock_ModifyAmount(void* vehicle, std::uint32_t good, std::int32_t amount, std::uint32_t revision);
	bool Vehicle_Stock_IsFull(const void* vehicle);
	bool Vehicle_Stock_IsFullSoon(const void* vehicle);
	std::uint32_t Vehicle_GetCurrentAtomicAnimationLengthInTicks(const void* vehicle);
	struct SStaticVehicleTypeData;
	bool Vehicle_DoAddNewUserCommand(void* vehicle, const void* command);
	bool Vehicle_l_UserCommands_TryToExecuteRightAway(void* vehicle, const void* command);
	std::uint32_t Vehicle_Stock_GetTotalWantedAmount(const void* vehicle);
	std::uint32_t Vehicle_Stock_GetAmountOfWantedGoods(const void* vehicle, std::uint32_t goodId);
	std::uint32_t Vehicle_Stock_GetFutureAmount(const void* vehicle, std::uint32_t goodId);
	std::uint32_t Vehicle_Stock_GetTotalFutureAmount(const void* vehicle);
	std::uint32_t Vehicle_Stock_GetTotalAmount(const void* vehicle);
	bool Vehicle_Stock_ModifyFutureAmount(void* vehicle, std::uint32_t goodId, std::int32_t delta, std::uint32_t humanRevision);
	bool Vehicle_Door_GetEntryPoint(const void* vehicle, void* point, std::uint32_t requiredSpace);
	bool Vehicle_Door_IsEntryPointOnLand(const void* vehicle, std::uint32_t requiredSpace);
	void Vehicle_GetMapPosition(const void* vehicle, void* point);
	bool Vehicle_Stock_SetWantedAmount(void* vehicle, std::uint32_t good, std::uint32_t wanted);
	void* Vehicle_Passengers_GetAttachedHumanPtr(const void* vehicle, std::uint32_t slot);
	void* Vehicle_Passengers_GetAttachedCommanderPtr(const void* vehicle);
	bool Vehicle_Passengers_IsCarrierAttached(const void* vehicle);
	bool Vehicle_Passengers_MoveOut_h(void* vehicle, std::uint32_t humanId);
	bool Vehicle_Passengers_MoveIn_h(void* vehicle, std::uint32_t humanId);
	bool Vehicle_Passengers_IsCommander(const void* vehicle, std::uint32_t humanId);
	void Vehicle_GetTargetWorkPosition(const void* vehicle, void* point);
	bool Vehicle_IsPriorityCommandInQueue(const void* vehicle);
	void Vehicle_l_Passengers_StopAllIdleTasks(void* vehicle);
	const SStaticVehicleTypeData* Vehicle_GetVehicleTypeDataPtr(const void* vehicle);
	std::int32_t Vehicle_Passengers_GetWorkTraderHumanId(const void* vehicle);
	const void* Vehicle_GetConstMapMicroPointPtr(const void* vehicle);
	const void* Vehicle_GetMapPositionRef(const void* vehicle);
	const std::uint16_t* Human_GetMapPositionRef(const void* human);
	bool Human_GetMapPosition(const void* human, void* point);
	std::uint32_t Vehicle_Stock_GetAmountOfGoods(const void* vehicle, std::uint32_t goodId);
	bool Vehicle_Stock_CanStoreGoodType(const void* vehicle, std::uint32_t goodId);
	std::int32_t Vehicle_l_Stock_GetStockSlotIdForGoodType(const void* vehicle, std::uint32_t goodId);
	std::int32_t Vehicle_Passengers_GetAttachedHumanSlot(const void* vehicle, std::uint32_t humanId);
	std::int32_t Vehicle_Passengers_GetAttachedCommanderId(const void* vehicle);
	std::int32_t Vehicle_Passengers_GetAttachedVehicleSlot(const void* vehicle, std::uint32_t vehicleId);
	bool Vehicle_Passengers_DetachHuman(void* vehicle, std::uint32_t humanId);
	void Vehicle_Passengers_ScanForAttachedCommander(void* vehicle);
	bool Vehicle_Passengers_AttachHuman(void* vehicle, std::uint32_t humanId);
	bool Vehicle_Passengers_CanVehicleBeAttached(const void* vehicle, std::uint32_t vehicleId);
	std::uint32_t Vehicle_Passengers_GetNumberOfAttachedVehicles(const void* vehicle);
	bool Vehicle_Passengers_CanHumanBeAttached(const void* vehicle, std::uint32_t humanId);
	bool Vehicle_Passengers_CanBeCommander(const void* vehicle, const void* human);
	std::uint32_t Vehicle_Passengers_GetNumberOfAttachedHumans(const void* vehicle, bool includeCommander);
}