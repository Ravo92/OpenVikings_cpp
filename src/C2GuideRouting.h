#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	struct SGuidePointSearch
	{
		std::uint32_t player;
		std::int32_t guideIds[2];
		std::int32_t count;
		std::byte unknown10[4];
		std::uint32_t point;
		std::int32_t radius;
	};

	static_assert(sizeof(SGuidePointSearch) == 28);
	void Guide_GetMovePoint(const void* guide, void* point);
	void GuideManager_l_InitializeScan(void* manager);
	void GuideManager_GetNearestGuideIds(void* manager, const void* point, std::uint32_t radius, SGuidePointSearch& data);
	void GuideManager_GetRawNearestGuideIds(void* manager, const void* point, std::uint32_t radius, SGuidePointSearch& data);
	void GuideManager_l_CleanSearchArrayAndSort(void* manager, SGuidePointSearch& data, const void* point);
	std::int32_t GuideManager_l_GetNextGuideIdNearSourceGuide(void* manager, std::int32_t guideId, std::int32_t otherGuideId);
	std::int32_t GuideManager_GetNextGuideId(void* manager, std::int32_t guideId, std::int32_t otherGuideId);
	std::int32_t GuideManager_GetNextGuideId(void* manager, std::int32_t player, std::uint32_t radius, const void* source, const void* destination, std::int32_t& destinationGuideId, std::int32_t& approachGuideId);
}