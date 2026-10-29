#pragma once
#include <cstdint>

namespace NC2Logic
{
	const char* StringAccess_GetGoodNamePtr(std::uint32_t type, std::uint32_t amount);
	const char* StringAccess_GetTribeNamePtr(std::uint32_t type, std::uint8_t variant);
	const char* StringAccess_GetJobNamePtr(std::uint32_t type, std::uint32_t amount);
	const char* StringAccess_GetHouseNamePtr(std::uint32_t type, std::uint32_t amount);
	const char* StringAccess_GetJobExperienceNamePtr(std::uint32_t type);
	const char* StringAccess_GetVehicleTypeNamePtr(std::uint32_t type);
	const char* StringAccess_GetVehicleTaskNamePtr(std::uint32_t type);
	const char* StringAccess_GetMilitaryModeNamePtr(std::uint32_t type);
	const char* StringAccess_GetMiscStringPtr(std::uint32_t type);
	const char* StringAccess_GetHouseNamePtr(std::uint32_t type, std::uint32_t developmentStage, std::uint32_t amount);
}