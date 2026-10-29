#pragma once

#include <cstdint>

namespace NC2Logic
{
	bool Tool_IsAnyNotMovingVehicleInsidePoint(const void* point);
	bool Tool_IsPointHitByNotMovingVehicle(const void* point, const void* searchPoint);
	bool Vehicle_Init(void* vehicle, std::int32_t id, std::uint32_t player, std::uint32_t tribe, std::uint32_t type, const void* point, std::uint32_t missionCheck);
	void Vehicle_Exit(void* vehicle);
	bool VehicleReference_IsAllocated(const void* reference);
	void Vehicle_ResetUserCommandArray(void* vehicle);
	void Vehicle_l_Passengers_ResetContents(void* vehicle);
	void Vehicle_Passengers_MoveAllOut(void* vehicle, bool includeCommander);
	bool Vehicle_Passengers_IsCommanderInVehicle(const void* vehicle);
	bool Vehicle_Passengers_NeedsDraggingAnimal(const void* vehicle);
	bool Vehicle_Passengers_AllPassengersInside(const void* vehicle);
	bool Vehicle_Passengers_AllVehiclesInside(const void* vehicle);
	bool Vehicle_Passengers_DetachVehicle(void* vehicle, std::uint32_t id);
	bool Vehicle_Passengers_MoveOut_v(void* vehicle, std::uint32_t id);
	void Vehicle_Attachment_DetachFromVehicle(void* vehicle);
	void Vehicle_Attachment_LeaveVehicle(void* vehicle, const void* point);
	std::int32_t Vehicle_l_GetLandContinentIdFromBorder(const void* vehicle, std::uint32_t point);
	void Vehicle_l_GetDoorPoint(void* vehicle, std::uint32_t continent, std::uint32_t point);
	void Vehicle_SetTargetWorkPosition(void* vehicle, const void* point);
	void Vehicle_l_SendAwayRadial(void* vehicle);
	void Vehicle_l_SendAllAway(void* vehicle, const void* point);
	bool Vehicle_GoAway_HitByVehicle(const void* vehicle, const void* point);
	bool Vehicle_l_GoAway_HitByVehicle(const void* vehicle, const void* point, const void* searchPoint);
	bool Vehicle_l_GoAway_GetVehicleFreePos(void* vehicle, void* point);
	bool Vehicle_l_StartAtomicAnimation(void* vehicle, std::uint32_t action, std::int32_t direction);
	bool Vehicle_l_IsAtomicInterruptable(const void* vehicle);
	bool Vehicle_l_StartAtomicWalk(void* vehicle, const void* point, bool force);
	void Vehicle_l_StartAtomicIdle(void* vehicle);
	bool Tool_Vehicle_GetNextMovePointNearPosition(std::uint32_t type, const void* source, void* point, std::uint32_t radius, std::uint32_t minimumDistance);
}