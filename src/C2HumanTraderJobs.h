#pragma once
#include <cstdint>

namespace NC2Logic
{
	std::uint32_t Human_l_Trader_GetNextGoodFromVehicle(void* human);
	std::uint32_t Human_l_Trader_GetNextGoodForNextHouse(void* human);
	bool Human_l_Trader_GoodForCurrentHouse(void* human, std::uint32_t good);
	bool Human_l_Trader_GoodForNextHouse(void* human, std::uint32_t good);
	void Human_l_StartTask_ExecuteJob_Trader(void* human);
	bool Human_l_Trader_MoveVehicleNearHouse(void* human, void* house);
	bool Human_l_Trader_FetchForWork(void* human, void* house);
	std::uint32_t Human_l_Trader_Foreign_CheckGoods(void* human, void* house, const void* agreement, std::uint32_t* excessGood);
}