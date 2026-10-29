#pragma once
#include <cstdint>

namespace NC2Logic
{
	std::uint32_t Human_DoExecuteUserCommand_BuildGuide(void* human, const void* point);
	std::uint32_t Human_DoExecuteUserCommand_ExploreArea(void* human, const void* point);
	bool Human_FindNotExploredPointNearWorkCenter(const void* human, void* point);
	void Human_l_StartTask_BuildGuide(void* human);
	void Human_l_FinishTask_BuildGuide(void* human);
	void Human_l_StartTask_ExploreArea(void* human);
	void Human_l_FailTask_ExploreArea(void* human);
	void Human_l_FinishTask_ExploreArea(void* human);
	void Human_l_FinishTask_Misc(void* human);
	void Human_l_PerformTaskEvent_Build(void* human);
}