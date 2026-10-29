#pragma once

#include <cstdint>

namespace NC2E2
{
	std::int32_t StaticDataGroups_FindIndex(const void* groupData, const char* name);
	std::int32_t StaticDataGroups_AddName(void* groupData, const char* name);
}