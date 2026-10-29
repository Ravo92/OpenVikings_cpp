#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Physical Windows guide-manager records; the calculation cells are owned.
	struct SAIGuideGridCellStorage
	{
		std::uint8_t hasHouse, hasGuide;
		std::byte padding02[2];
		std::int32_t guideId, houseId;
	};

	struct SAIGuideGridStorage
	{
		std::int32_t width, height;
		SAIGuideGridCellStorage* cells;
	};

	struct SAIGuideManagerStorage
	{
		std::int32_t scoutHumanId;
		std::uint32_t scoutHumanRevision;
		std::uint8_t scoutRequested;
		std::byte padding09[3];
		std::int32_t militaryPointIndex;
		std::uint32_t destination;
		std::uint8_t guidePairSearchExhausted;
		std::byte padding15[3];
		SAIGuideGridStorage calculationGrid;
	};

	void AIGuideManager_CalcVars_CreateGuideCalcArray(void* grid);
	bool AIGuideManager_IsGuideAlreadyBuild(const void* manager, std::uint32_t player, const void* point);
	void AIGuideManager_BuildGuideNearPosition(void* manager, std::uint32_t player, const void* point);
	void AIGuideManager_CheckAndBuildGuides(void* manager, std::uint32_t player, void* military);
	void AIGuideManager_l_CheckForNewGuideBuildPos(void* manager, std::uint32_t player);
}