#pragma once
#include <cstdint>

namespace NC2Logic
{
	void* Human_FindHumanToEnjoy(const void* human);
	void* Human_FindHumanToBless(const void* human);
	void Human_l_StartTask_ExecuteJob_Jester(void* human);
	void Human_l_StartTask_ExecuteJob_Jester_Collect(void* human);
	void Human_l_StartTask_ExecuteJob_Jester_TryToJest(void* human);
	void Human_l_StartTask_ExecuteJob_Jester_Jest(void* human);
	void Human_l_StartTask_ExecuteJob_Druid(void* human);
	void Human_l_StartTask_ExecuteJob_Druid_Priest(void* human);
	void Human_l_StartTask_ExecuteJob_Druid_Healer(void* human);
	void Human_l_StartTask_ExecuteJob_Druid_TryToHeal(void* human);
	void Human_l_StartTask_ExecuteJob_Druid_Heal(void* human);

	void Human_l_StartTask_ExecuteJob_Tool_FillTemple(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_FillTemple_Fetch(void* human);
}