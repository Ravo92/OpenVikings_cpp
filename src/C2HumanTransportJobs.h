#pragma once
#include <cstdint>

namespace NC2Logic
{
	void Human_l_StartTask_MoveInsideVehicle(void* human);
	void Human_l_FinishTask_MoveInsideVehicle(void* human);
	void Human_l_StartTask_ExecuteJob_Carrier(void* human);
	bool Human_l_StartTask_ExecuteJob_Carrier_Flush_Bring(void* human);
	bool Human_l_StartTask_ExecuteJob_Carrier_FlushVehicle(void* human);
	bool Human_l_StartTask_ExecuteJob_Carrier_Vehicle(void* human);
	void Human_l_StartTask_ExecuteJob_Carrier_Collect(void* human);
	void Human_l_StartTask_ExecuteJob_Carrier_TryToCollect(void* human);
	void Human_l_StartTask_ExecuteJob_Carrier_FlushStock(void* human);

	bool Human_IsBringingCarriedGood(const void* human);
}