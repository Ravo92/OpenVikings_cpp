#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct SMapAreaRow
	{
		std::int16_t x;
		std::int16_t y;
		std::uint32_t width;
	};

	class CMapArea
	{
	public:
		explicit CMapArea(std::uint32_t numberOfRows);
		~CMapArea();
		CMapArea(const CMapArea&) = delete;
		CMapArea& operator=(const CMapArea&) = delete;
		void UpdateCalculatedVariables();
		void AreaTool_GetRandomPositionDelta(std::int16_t* x, std::int16_t* y) const;
		void WalkBlock_Set(const void* point, bool blocked) const;
		void BuildBlock_Set(const void* point, bool blocked) const;
		bool BuildBlock_Check(const void* point) const;
		bool WalkBlock_Check(const void* point) const;
		bool Check_AreAllPointsOnSameLandContinent(const void* point) const;
		bool Check_AreAllPointsAroundOnBuildAllowedPattern(const void* point) const;
		bool Check_AreAllPointsAroundOnBioRegrowPattern(const void* point) const;
		bool Check_NoGuideOnArea(const void* point) const;
		bool Check_NoHouseBuildForbiddenFlag(const void* point) const;
		void Landscapes_RemoveAll(const void* point) const;
		void Tool_CountGoods(const void* point, std::uint32_t* amounts) const;
		bool Check_CanLandscapeBeSetOnPattern(const void* point, std::uint32_t landscape) const;
		bool AdditionalAttachmentPoints_CheckIfPointsFreeOrToBeFreed(const void* point) const;
		void AdditionalAttachmentPoints_SetToMap(const void* point) const;
		void AdditionalAttachmentPoints_ClearFromMap(const void* point) const;

		std::uint32_t rowCount;
		SMapAreaRow* rows;
		std::uint32_t pointCount;
		std::int16_t minX;
		std::int16_t minY;
		std::int16_t maxX;
		std::int16_t maxY;
	};

	static_assert(sizeof(SMapAreaRow) == 8);
	static_assert(offsetof(SMapAreaRow, width) == 4);
	static_assert(sizeof(CMapArea) == (sizeof(void*) == 8 ? 32 : 20));
	static_assert(offsetof(CMapArea, rowCount) == 0);
	static_assert(offsetof(CMapArea, rows) == (sizeof(void*) == 8 ? 8 : 4));
	static_assert(offsetof(CMapArea, pointCount) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(CMapArea, minX) == (sizeof(void*) == 8 ? 20 : 12));
	static_assert(offsetof(CMapArea, minY) == (sizeof(void*) == 8 ? 22 : 14));
	static_assert(offsetof(CMapArea, maxX) == (sizeof(void*) == 8 ? 24 : 16));
	static_assert(offsetof(CMapArea, maxY) == (sizeof(void*) == 8 ? 26 : 18));
}
