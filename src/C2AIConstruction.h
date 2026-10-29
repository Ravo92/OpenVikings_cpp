#pragma once
#include <cstdint>

namespace NC2Logic
{
	void __stdcall ConstructOriginalMemberArray(void* array, std::uint32_t stride, std::uint32_t count, void(__thiscall* constructor)(void*));
	void* MapFishManager_ConstructMembers(void* manager);
	void* AISinglePlayerHandler_Construct(void* handler);
	void* AIHighLevelHandler_Vars_Construct(void* handler);
	void* AIGuideManager_Construct(void* manager);
}