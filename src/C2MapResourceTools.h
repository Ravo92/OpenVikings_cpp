#pragma once
#include <cstdint>

namespace NC2Logic
{
	std::uint32_t Tool_CountGoodsInVehicles(std::uint32_t player, std::uint32_t vehicleType, std::uint32_t good, std::uint32_t point, std::uint32_t radius);
	std::int32_t Tool_AddGoodsToMapPoint(std::uint32_t good, std::int32_t amount, const void* point, bool useHouses, std::uint32_t player);
	std::int32_t Tool_AddGoodsToMapArea(std::uint32_t good, std::int32_t amount, std::uint32_t point, std::uint32_t radius, bool useHouses, std::uint32_t player);
	std::int32_t Tool_RemoveGoodsFromMapPoint(std::uint32_t good, std::int32_t amount, const void* point, bool useHouses, std::uint32_t player);
	std::int32_t Tool_RemoveGoodsFromMapArea(std::uint32_t good, std::int32_t amount, std::uint32_t point, std::uint32_t radius, bool useHouses, std::uint32_t player);
}