#pragma once
#include <cstdint>

namespace NC2Logic
{
	void* AISinglePlayerHandler_l_Misc_Human_AllocateStructure(void* handler);
	std::uint32_t AISinglePlayerHandler_l_Misc_Human_CountRequestsForHouse(const void* handler, std::int32_t house);
	std::uint32_t AISinglePlayerHandler_l_Road_CountConstructorRequest(const void* handler);
	bool AISinglePlayerHandler_Misc_Human_CheckRequestsForCreature(const void* handler, std::int32_t human);
	std::int32_t AISinglePlayerHandler_l_Misc_SearchHuman(void* handler, std::uint32_t target, std::uint32_t type, std::uint32_t radius, bool mapPosition);
	void AISinglePlayerHandler_l_Misc_Human_GetInhabitants(void* handler);
	bool AISinglePlayerHandler_l_Request_TryToGetWallBuilder(void* handler, std::uint32_t point);
}