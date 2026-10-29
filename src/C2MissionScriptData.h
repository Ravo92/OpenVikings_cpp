#pragma once

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CIniFileReader;
}

namespace NC2Logic
{
	struct SMissionGoal
	{
		std::uint32_t type;
		std::uint32_t parameters[9];
	};

	struct SResult
	{
		std::uint32_t type;
		std::uint32_t parameters[10];
		char text[32];
		std::uint32_t GetParameter(std::uint32_t index) const;
	};

	static_assert(sizeof(SMissionGoal) == 40);
	static_assert(sizeof(SResult) == 76);
	static_assert(offsetof(SResult, text) == 0x2C);

	// Physical Windows records; unknown bytes and the goal status overlay remain explicit.
	struct SMissionDataStorage
	{
		char debugInfo[30];
		std::byte unresolved1E[6];
		std::uint8_t visible, fulfilled, active;
		std::byte unresolved27;
		std::uint32_t activationTick;
		std::uint8_t initiallyActive;
		std::byte unresolved2D[3];
		std::int32_t descriptionId, goalMatchMode, goalCount;
		SMissionGoal goals[30];
		std::int32_t resultCount;
		SResult results[30];
	};

	struct SMissionManagerStorage
	{
		std::uint32_t currentBriefingId;
		std::uint8_t won, failed, evaluationInterrupted;
		std::byte unresolved07;
		std::uint8_t startSubMap;
		std::byte unresolved09[3];
		std::uint32_t subMapCampaignId, subMapId;
		std::uint8_t returnToParent;
		std::byte unresolved15[3];
		std::int32_t missionCount;
		SMissionDataStorage missions[150];
	};

	std::uint32_t Logic_EntryPoint_GetTribeType(const char* name);
	std::uint32_t Logic_EntryPoint_GetJobType(const char* name);
	std::uint32_t Logic_EntryPoint_GetHouseType(const char* name);
	std::uint32_t Logic_EntryPoint_GetVehicleType(const char* name);
	std::uint32_t Logic_EntryPoint_GetGoodType(const char* name);
	std::uint32_t Logic_EntryPoint_GetDiplomacyState(const char* name);
	void MissionData_SetMissionActiveFlag(void* mission, bool active);
	void __fastcall MissionManager_l_LoadMissionData(void* manager, void* unusedEdx, NXBasics::CIniFileReader* reader);
	void __fastcall MissionManager_l_LoadObjectData(void* manager, void* unusedEdx, NXBasics::CIniFileReader* reader);
	void __fastcall MissionManager_LoadMissionScript(void* manager, void* unusedEdx, NXBasics::CIniFileReader* reader);
}

namespace NC2Logic_MissionTool
{
	std::uint32_t Debug_Tool_Mission_GetGoalType(const char* name);
	std::uint32_t Debug_Tool_Mission_GetResultType(const char* name);
	std::uint32_t Mission_Tool_GetParameter(NXBasics::CIniFileReader& reader, std::uint32_t parameterType);
	void Mission_Tool_ReadGoalData(NXBasics::CIniFileReader& reader, NC2Logic::SMissionGoal& goal);
	void Mission_Tool_ReadResultData(NXBasics::CIniFileReader& reader, NC2Logic::SResult& result);
}