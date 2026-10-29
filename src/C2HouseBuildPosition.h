#pragma once

#include <cstdint>

namespace NC2Logic
{
	struct SMapPathfinderManagerSearchOverMapCallbackData;
	void Map_HouseBuildForbiddenFlagTool_SetArea(std::uint32_t point, std::int32_t radius, std::uint8_t forbidden);
	bool __cdecl Tool_HouseBuildPointSearchEvaluationFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* data);
	bool Tool_House_IsWorkPointOnContinent(const void* point, std::int32_t engineType, std::uint32_t stage, std::uint32_t continent, void* workPoint);
	bool Tool_House_IsAllowedToBeBuild(std::uint32_t player, std::int32_t engineType, std::uint32_t stage, std::uint32_t point, bool ignoreTerrain, bool checkBioPattern);
	bool Tool_Human_IsAnyOfPlayerAround(std::uint32_t player, const void* point, std::int32_t radius, bool includeVehicles);
	bool Remaster_House_IsConstructionTrap(const void* point);
	void Remaster_House_PrepareConstructionEscape(const void* point);
}