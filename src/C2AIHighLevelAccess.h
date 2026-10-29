#pragma once
#include <cstdint>

namespace NC2Logic
{
	void AIHighLevelHandler_DoOneGameTick(void* handler, std::uint32_t tick);
	bool AIHighLevelHandler_l_CreateVillages(void* handler);
	std::int32_t AIHighLevelHandler_l_Humans_CheckShoes(void* handler, std::int32_t previous);
	void AIHighLevelHandler_l_Villages_EnhanceVillages(void* handler);
	void AIHighLevelHandler_l_Villages_CheckForNeedments(void* handler);
	void AIHighLevelHandler_l_Trader_Setup(void* handler);
	void AIHighLevelHandler_l_DestroyRandom(void* handler, std::uint32_t tick);
}