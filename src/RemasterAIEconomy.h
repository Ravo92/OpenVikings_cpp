#pragma once

#include <cstdint>

namespace NC2Logic::RemasterAIEconomy
{
	// Derived from live native objects; never serialized or added to original structures.
	struct SWorkforce
	{
		std::uint32_t population = 0, workers = 0, soldiers = 0, children = 0, women = 0, builders = 0;
		std::uint32_t vacantWorkplaces = 0, constructionSites = 0, warehouses = 0, foodVacancies = 0, foodStock = 0;
		std::uint32_t recruitmentVacancies = 0;
	};
	bool IsActive(std::uint32_t player);
	SWorkforce Inspect(std::uint32_t player);
	std::uint32_t WaveSize(std::uint32_t population);
	std::uint32_t MilitaryLimit();
	std::uint32_t WomenLimit();
	void StaffVacantWorkplaces(std::uint32_t player);
	std::uint32_t BuildingFamily(std::uint32_t logic);
	bool CanBuild(std::uint32_t player, std::uint32_t logic);
	bool CanExpand(std::uint32_t player);
	bool IsSurplusSupplyHouse(const void* house);
	bool IsWorkerCommitted(const void* human);
	std::int32_t FindWorker(std::uint32_t player, const void* point, std::uint32_t job, bool civilistsOnly = false);
	void ReleaseExcessBuilders(std::uint32_t player);
	std::int32_t UpdateHomes(void* village);
}
