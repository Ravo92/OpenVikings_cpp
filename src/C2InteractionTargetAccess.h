#pragma once
#include <cstdint>

namespace NC2Logic
{
	bool InteractionTargetData_IsValid(const void* target);
	bool TargetData_l_IsValid(const void* target);
	bool TargetData_IsValid(const void* target);
	std::uint32_t TargetData_GetTargetType(const void* target);
	std::uint32_t TargetData_GetObjectId(const void* target);
	std::uint32_t TargetData_GetUniqueId(const void* target);
	void TargetData_GetMapPosition(const void* target, void* point);
	std::int32_t TargetData_GetPlayerId(const void* target);

	void TargetData_SetMapPosition(void* target, const void* point);
}