#pragma once

#include "C2HumanGoods.h"
#include <cstdint>

namespace NC2Logic
{
	// Physical Windows work-marker record; the map moveable is allocated separately.
	struct SMarkerStorage
	{
		std::uint8_t allocated;
		std::byte padding01[3];
		std::int32_t markerId;
		std::uint32_t uniqueId;
		std::int32_t mapMoveableId;
		std::uint32_t type;
		std::int32_t humanId;
	};

	struct SProductionGoodSelectionStorage
	{
		std::uint8_t mode, enabled[66];
	};

	struct SUnresolvedSevenEntrySelectionStorage
	{
		std::uint8_t mode, enabled[7];
	};

	std::int32_t __fastcall MarkerArray_AllocateMarker(void* array, void* unusedEdx, std::uint32_t type, const void* point, std::uint32_t human);
	void __fastcall MarkerArray_FreeMarker(void* array, void* unusedEdx, std::int32_t id);
	std::int32_t Marker_PlayerId_Get(const void* marker);
	bool Human_Init(void* human, std::int32_t id, std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags);
	void Human_Exit(void* human);
	void Human_Detach(void* human, bool forceVehicleDetach);
	void Human_ChangePlayerId(void* human, std::int32_t player, bool detach);
	bool Human_IsAttachedToRoadPoint(const void* human);
	bool Human_IsAttachedToWallPoint(const void* human);
	bool Human_IsAttachedToBuildSite(const void* human);
	bool Human_WorkCenterMarker_IsVisible(const void* human);
	bool Human_WorkCenterMarker_IsAllowed(const void* human);
	std::int32_t Human_WorkCenterMarker_GetId(const void* human);
	void Human_WorkCenterMarker_Create(void* human);
	void Human_WorkCenterMarker_Remove(void* human);
	bool Human_l_StartAtomicAnimation(void* human, std::uint32_t action, std::int32_t direction);
	bool Human_HouseMisc_Enter(void* human);
	void Human_DetachFromVehicle(void* human);
	void Human_DetachFromHomeHouse(void* human);
	void Human_DetachFromWorkHouse(void* human);
	void Human_DetachFromWorkHouse2(void* human);
	void Human_VehicleMisc_Leave(void* human, const void* point);
	void BuildManager_DetachBuilder(void* manager, std::uint32_t player, const void* point);

	void Logic_EntryPoint_Edit_Start();
	void Logic_EntryPoint_Edit_Stop();

	std::uint32_t Human_DoExecuteUserCommand_DetachWorkHouse(void* human);
	void HappinessManager_Messages_CivilistDied(void* manager, const void* human);
	bool Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(const void* point, std::uint32_t type, std::uint32_t valency, std::int32_t engineId, std::uint32_t parameter, std::int32_t player);
}

namespace NC2LogicExternals
{
	std::int32_t External_Misc_GetEngineCadaverLandscapeId(std::uint32_t tribe, std::uint32_t type);
}