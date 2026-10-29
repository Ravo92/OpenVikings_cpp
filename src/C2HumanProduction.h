#pragma once
#include <cstdint>

namespace NC2Logic
{
	void Human_l_StartTask_ExecuteJob_Woman(void* human);
	void Human_l_StartTask_ExerciseJob(void* human);
	void Human_l_JobTraining_Train(void* human, std::uint32_t amount);
	void Human_l_JobTraining_Exercise(void* human, std::uint32_t amount);
	void Human_l_StartTask_ExecuteJob_Tool_FillHome_BringAway(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_FillHome_Fetch(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_FillHome_TryToFetch(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_GoodProducer(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_FillStock_Fetch(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_FlushStock_Fetch(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_FlushStock_BringAway(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_TryToProduce(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_CollectForProduction(void* human);
	bool Human_l_StartTask_ExecuteJob_Tool_CollectWantedGood(void* human);
	void Human_l_StartTask_ExecuteJob_Tool_Produce(void* human);
	void Human_l_PerformTaskEvent_Produce(void* human);
	void Human_l_PerformTaskEvent_Produce_Good(void* human);
}