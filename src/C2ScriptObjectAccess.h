#pragma once
#include "C2CallbackManager.h"
#include <cstdint>

namespace NC2Logic
{
	CCallbackManager* CallbackManager_GetExistingObjectPtr();
	std::uint32_t __fastcall MapMicroPoint_GetContinent(const void* micro, void* unusedEdx);
	void* __fastcall MapMoveable_GetMarkerPtr(const void* moveable, void* unusedEdx);
	std::uint32_t __fastcall HumanReference_GetHumanId(const void* reference, void* unusedEdx);
	std::uint32_t __fastcall Vehicle_GetMapMoveableId(const void* vehicle, void* unusedEdx);
	std::uint32_t __fastcall Vehicle_GetMissionCheckId(const void* vehicle, void* unusedEdx);
	void __fastcall Vehicle_SetMissionCheckId(void* vehicle, void* unusedEdx, std::uint32_t id);
	bool __fastcall Vehicle_IsDocked(const void* vehicle, void* unusedEdx);
	const void* __fastcall Vehicle_Passengers_GetAttachedHumanSlotPtr(const void* vehicle, void* unusedEdx, std::uint32_t slot);
	bool __fastcall Vehicle_Passengers_IsCommanderAttached(const void* vehicle, void* unusedEdx);
	std::uint32_t __fastcall Vehicle_Passengers_GetNumberOfSlots(const void* vehicle, void* unusedEdx);
	bool Game_GetSuppressDeathStatisticsFlag();
	void Game_SetSuppressDeathStatisticsFlag(bool flag);
	bool Game_GetSuppressVehicleWreckFlag();
	void Game_SetSuppressVehicleWreckFlag(bool flag);
	std::uint32_t __fastcall House_GetMissionCheckId(const void* house, void* unusedEdx);
	void* MarkerArray_GetExistingObjectPtr();
	std::uint32_t __fastcall Human_GetExplorationRange(const void* human, void* unusedEdx);
	std::uint32_t __fastcall ArrayObject_GetId(const void* object, void* unusedEdx);
}