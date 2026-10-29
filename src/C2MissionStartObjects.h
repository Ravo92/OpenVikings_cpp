#pragma once
#include "C2HumanWorkCommands.h"

#include "C2HouseAccess.h"
#include <cstdint>

namespace NC2Logic
{
	bool Logic_EntryPoint_IsPlayerInGame(std::int32_t player);
	bool Logic_EntryPoint_IsPlayerNotInMultiPlayerGame(std::int32_t player);
	std::int32_t Logic_EntryPoint_House_GetOnPosition(const void* point);
	void Logic_EntryPoint_House_Remove(const void* point);
	void Logic_EntryPoint_FreeMapMoveable(std::uint32_t id);
	void Logic_EntryPoint_Guide_Set(std::uint32_t player, const void* point, std::uint32_t flags);
	std::int32_t Logic_EntryPoint_Add_Human(std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags);
	std::int32_t Logic_EntryPoint_Add_Animal(std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags);
	std::int32_t Logic_EntryPoint_Add_Vehicle(std::uint32_t player, std::uint32_t tribe, std::uint32_t type, const void* point, std::uint32_t missionCheck);
	int Logic_EntryPoint_House_Set(unsigned int player, unsigned int engineType, unsigned int stage, const void* point, bool completed, unsigned int missionCheck);
	std::int32_t MissionCheck_GetFirstHumanIdWithMissionCheckId(std::uint32_t missionCheck);
	std::int32_t MissionCheck_GetNextHumanIdWithMissionCheckId(std::uint32_t missionCheck, std::int32_t previous);
	std::int32_t MissionCheck_GetFirstVehicleIdWithMissionCheckId(std::uint32_t missionCheck);
	std::int32_t MissionCheck_GetNextVehicleIdWithMissionCheckId(std::uint32_t missionCheck, std::int32_t previous);
	std::int32_t MissionCheck_GetFirstHouseIdWithMissionCheckId(std::uint32_t missionCheck);
	std::int32_t MissionCheck_GetNextHouseIdWithMissionCheckId(std::uint32_t missionCheck, std::int32_t previous);
	std::int32_t MissionCheck_GetFirstAnimalIdWithMissionCheckId(std::uint32_t missionCheck);
	std::int32_t MissionCheck_GetNextAnimalIdWithMissionCheckId(std::uint32_t missionCheck, std::int32_t previous);
	std::int32_t MissionCheck_GetNumberOfAnimalsWithMissionCheckId(std::uint32_t missionCheck);
	bool Human_IsSexFemale(const void* human);
	bool Human_IsAbleToMarry(const void* human);
	bool Human_IsMarried(const void* human);
	bool Human_IsHavingChild(const void* human);
	void Human_Production_SetCounter(void* human, std::uint32_t good, std::uint32_t counter);

	bool House_Inhabitants_AttachHuman(void* house, std::uint32_t human, std::uint32_t attachmentMask);
	bool House_Inhabitants_DetachHuman(void* house, std::uint32_t human, std::uint32_t attachmentMask);
	std::uint32_t Human_DoExecuteUserCommand_AttachHomeHouse(void* human, std::int32_t house, std::uint32_t revision);
	std::uint32_t Human_VehicleMisc_Enter(void* human);
	bool Human_IsAbleToProduceGoodNow(const void* human, std::uint32_t good);
	void Human_UpdateCurrentJobType(void* human);
	bool Vehicle_Stock_ModifyAmount(void* vehicle, std::uint32_t good, std::int32_t amount, std::uint32_t revision);
}

namespace NC2LogicExternals
{
	std::int32_t External_HouseType_GetEngineId(const char* name);
	void External_Human_SetName(std::uint32_t human, std::uint32_t name);
	void External_Human_SetRandomPalettePatch(std::uint32_t human, const char* palette);
}