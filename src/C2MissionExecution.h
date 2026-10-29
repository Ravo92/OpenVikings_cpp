#pragma once
#include "C2AIConditions.h"
#include "C2CallbackManager.h"
#include "C2MissionScriptData.h"

namespace NC2Logic
{
	bool __cdecl MissionManager_l_Mission_Callback(TCallbackTypes type, std::uintptr_t userData, std::uintptr_t tick, std::uintptr_t param2, std::uintptr_t param3);
	bool MissionManager_CheckAllMissionGoals(void* manager);
	bool MissionManager_l_CheckMission(void* manager, void* mission, bool executeResults);
	bool MissionManager_l_CheckGoal(void* manager, const SMissionGoal& goal);
	void MissionManager_l_ExecuteResult(void* manager, const SResult& result);
}