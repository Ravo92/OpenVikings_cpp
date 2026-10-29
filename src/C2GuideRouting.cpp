#include "C2GameMapAccess.h"
#include "C2GuideRouting.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* Guide(std::int32_t id)
		{

			return CGuideArray::mStaticVars + static_cast<std::intptr_t>(id) * CGuideArray::ElementSize;
		}

		std::int32_t Distance(const void* a, const void* b)
		{

			return VE_HexagonDirection_GetDistance(Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::Y));
		}

		// Original function: FUN_0041B360. The search stops at its second owned, strictly nearby guide.
		bool __cdecl slGuideSearch(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != SwitchIds::MapMicroObjectType::Guide)
				return false;

			auto* guide = Guide(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::GuideId));
			auto& data = *static_cast<SGuidePointSearch*>(callback->userData);

			if (Distance(static_cast<std::byte*>(guide) + NC2Layout::GuideOffset::MapPosition, &data.point) >= data.radius || Read<std::uint32_t>(guide, NC2Layout::GuideOffset::PlayerId) != data.player)
				return false;

			data.guideIds[data.count++] = Read<std::int32_t>(guide, NC2Layout::GuideOffset::GuideId);

			return data.count == 2;
		}
	}

	// Original function: FUN_00447677.
	void Guide_GetMovePoint(const void* guide, void* point)
	{
		const auto position = Read<std::uint32_t>(guide, NC2Layout::GuideOffset::MapPosition);
		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, position);
		const auto directions = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::WalkingDirections);

		for (std::uint32_t direction = 0; direction < 6; ++direction)
		{
			if ((directions & (1u << direction)) == 0)
				continue;

			Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, position);
			MapMigPoint_MoveCursorUnwrapped(point, direction, 1);

			if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0)
				break;
		}
	}

	// Original function: FUN_0041B30C.
	void GuideManager_GetNearestGuideIds(void* manager, const void* point, std::uint32_t radius, SGuidePointSearch& data)
	{
		CGuideIterator iterator;

		if (iterator.GetFirstGuideId(data.player, point, radius) == -1)
		{

			return;
		}

		CMapPathfinderElement::SearchOverMap(point, radius, true, slGuideSearch, &data, true, -1);
		GuideManager_l_CleanSearchArrayAndSort(manager, data, point);
	}

	// Original function: FUN_0041B3F3.
	void GuideManager_GetRawNearestGuideIds(void* manager, const void* point, std::uint32_t radius, SGuidePointSearch& data)
	{
		CGuideIterator iterator;

		if (iterator.GetFirstGuideId(data.player, point, radius) != -1)
			CMapPathfinderElement::SearchOverMap(point, radius, true, slGuideSearch, &data, true, -1);
	}

	// Original function: FUN_0041B515. Native ordering prefers the first matching source/target link system.
	std::int32_t GuideManager_GetNextGuideId(void* manager, std::int32_t player, std::uint32_t radius, const void* source, const void* destination, std::int32_t& destinationGuideId, std::int32_t& approachGuideId)
	{
		destinationGuideId = -1;
		SGuidePointSearch targetSearch;
		targetSearch.player = static_cast<std::uint32_t>(player);
		targetSearch.count = 0;
		targetSearch.point = Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates);
		targetSearch.radius = NXBasics::Runtime_TimeSigned32(radius);
		GuideManager_GetNearestGuideIds(manager, destination, radius, targetSearch);

		if (targetSearch.count == 0)
			return -1;

		SGuidePointSearch sourceSearch;
		sourceSearch.player = static_cast<std::uint32_t>(player);
		sourceSearch.count = 0;
		sourceSearch.point = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);
		sourceSearch.radius = NXBasics::Runtime_TimeSigned32(radius);
		GuideManager_GetRawNearestGuideIds(manager, source, radius, sourceSearch);

		if (sourceSearch.count == 0)
			return -1;

		std::int32_t selectedTarget = -1, selectedSource = -1;

		for (std::int32_t target = 0; target < targetSearch.count && selectedTarget == -1; ++target)
		{
			for (std::int32_t candidate = 0; candidate < sourceSearch.count && selectedTarget == -1; ++candidate)
			{
				auto* guide = Guide(sourceSearch.guideIds[candidate]);
				std::uint32_t position = 0;
				Guide_GetMovePoint(guide, &position);

				if (Distance(&position, source) > NXBasics::Runtime_TimeSigned32(radius) || Read<std::uint32_t>(guide, NC2Layout::GuideOffset::LinkSystemId) != Read<std::uint32_t>(Guide(targetSearch.guideIds[target]), NC2Layout::GuideOffset::LinkSystemId))
					continue;

				selectedTarget = targetSearch.guideIds[target];
				selectedSource = sourceSearch.guideIds[candidate];
			}
		}

		if (selectedTarget == -1)
			return -1;

		std::uint32_t position = 0;
		// Windows 0041B649..0041B653 uses the source-side guide (EDI).
		Guide_GetMovePoint(Guide(selectedSource), &position);

		if (selectedTarget == selectedSource)
		{
			if (position == Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates))
				return -1;

			destinationGuideId = selectedSource;
			return selectedTarget;
		}
		// Search backwards from the destination to get a step adjacent to the source.
		const auto next = GuideManager_l_GetNextGuideIdNearSourceGuide(manager, selectedSource, selectedTarget);
		approachGuideId = selectedSource;
		destinationGuideId = selectedTarget;

		for (std::int32_t index = 0; index < sourceSearch.count; ++index)
		{
			if (sourceSearch.guideIds[index] == next)
				return next;
		}

		return position == Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates) ? next : selectedSource;
	}

	// Original function: FUN_0041B6C7.
	std::int32_t GuideManager_GetNextGuideId(void* manager, std::int32_t guideId, std::int32_t otherGuideId)
	{

		return guideId == otherGuideId ? -1 : GuideManager_l_GetNextGuideIdNearSourceGuide(manager, guideId, otherGuideId);
	}

	// Original function: FUN_0041B6E3. Collapse same-system entries by distance, then compact at most two slots.
	void GuideManager_l_CleanSearchArrayAndSort(void* manager, SGuidePointSearch& data, const void* point)
	{
		for (std::int32_t index = data.count - 1; index >= 0; --index)
		{
			if (data.guideIds[index] == -1)
				continue;

			const auto* guide = Guide(data.guideIds[index]);

			for (std::int32_t earlier = 0; earlier < index; ++earlier)
			{
				if (data.guideIds[earlier] == -1 || Read<std::uint32_t>(Guide(data.guideIds[earlier]), NC2Layout::GuideOffset::LinkSystemId) != Read<std::uint32_t>(guide, NC2Layout::GuideOffset::LinkSystemId))
					continue;

				const auto firstDistance = static_cast<std::uint32_t>(Distance(static_cast<const std::byte*>(Guide(data.guideIds[earlier])) + 0x14, point));
				const auto secondDistance = static_cast<std::uint32_t>(Distance(static_cast<const std::byte*>(guide) + NC2Layout::GuideOffset::MapPosition, point));

				if (firstDistance < secondDistance)
				{
					data.guideIds[index] = -1;
				}
				else
				{
					data.guideIds[earlier] = -1;
				}

				--data.count;
				break;
			}
		}

		std::int32_t written = 0;

		for (std::int32_t scanned = 0; scanned < 2 && written < data.count; ++scanned)
		{
			if (data.guideIds[scanned] != -1)
				data.guideIds[written++] = data.guideIds[scanned];
		}
	}

	// Original function: FUN_0041B7E8. Search from the second guide and return the predecessor of the first.
	std::int32_t GuideManager_l_GetNextGuideIdNearSourceGuide(void* manager, std::int32_t guideId, std::int32_t otherGuideId)
	{
		auto* first = Guide(otherGuideId);

		if (Read<std::int32_t>(first, NC2Layout::GuideOffset::GuideId) == guideId)
			return -1;

		GuideManager_l_InitializeScan(manager);
		Write(manager, NC2Layout::GuideManagerOffset::FirstBucketGuides, first);
		Write<std::uint32_t>(manager, NC2Layout::GuideManagerOffset::DistanceBuckets, Read<std::uint32_t>(manager, NC2Layout::GuideManagerOffset::DistanceBuckets) + 1u);
		auto* visited = Read<std::uint8_t*>(manager, NC2Layout::GuideManagerOffset::VisitedGuides);
		visited[Read<std::uint32_t>(first, NC2Layout::GuideOffset::GuideId)] = 1;
		std::uint32_t pending = 1, bucket = 0;

		for (;;)
		{
			const auto offset = NC2Layout::GuideManagerOffset::DistanceBucketsUnsigned + bucket * NC2Layout::RecordSize::GuideDistanceBucketUnsigned;

			for (std::int32_t slot = 0; slot < Read<std::int32_t>(manager, offset); ++slot)
			{
				auto* guide = Read<void*>(manager, offset + NC2Layout::GuideDistanceBucketOffset::GuidesUnsigned + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::GuideIdUnsigned);
				--pending;

				for (std::int32_t link = 0; link < Read<std::int32_t>(guide, NC2Layout::GuideOffset::ConnectionCount); ++link)
				{
					const auto linkOffset = NC2Layout::GuideOffset::ConnectionsUnsigned + static_cast<std::uint32_t>(link) * NC2Layout::RecordSize::GuideConnectionUnsigned;
					const auto id = Read<std::int32_t>(guide, linkOffset + NC2Layout::GuideConnectionOffset::GuideId);

					if (visited[id] == 1)
						continue;

					auto* candidate = Guide(id);

					if (Read<std::int32_t>(candidate, NC2Layout::GuideOffset::GuideId) == guideId)
						return Read<std::int32_t>(guide, NC2Layout::GuideOffset::GuideId);

					const auto targetBucket = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(guide, linkOffset) + bucket) % NC2Runtime::GuideRoutingLimits::DistanceBuckets;
					const auto targetOffset = NC2Layout::GuideManagerOffset::DistanceBucketsUnsigned + static_cast<std::uint32_t>(targetBucket) * NC2Layout::RecordSize::GuideDistanceBucketUnsigned;
					const auto count = Read<std::uint32_t>(manager, targetOffset);
					Write(manager, targetOffset + NC2Layout::GuideDistanceBucketOffset::GuidesUnsigned + count * NC2Layout::RecordSize::GuideIdUnsigned, candidate);
					Write(manager, targetOffset, count + 1u);
					++pending;
					visited[id] = 1;
				}
			}

			if (pending == 0)
			{

				return -1;
			}

			Write<std::uint32_t>(manager, offset, 0);

			if (++bucket == NC2Runtime::GuideRoutingLimits::DistanceBuckets)
				bucket = 0;
		}
	}

	// Original function: FUN_0041B90D.
	void GuideManager_l_InitializeScan(void* manager)
	{
		std::memset(Read<void*>(manager, NC2Layout::GuideManagerOffset::VisitedGuides), 0, static_cast<std::uint32_t>(CGuideArray::sTheObjectPtr->mCapacity));

		for (std::uint32_t bucket = 0; bucket < NC2Runtime::GuideRoutingLimits::DistanceBuckets; ++bucket)
		{
			Write<std::uint32_t>(manager, NC2Layout::GuideManagerOffset::DistanceBucketsUnsigned + bucket * NC2Layout::RecordSize::GuideDistanceBucketUnsigned, 0);
		}
	}
}