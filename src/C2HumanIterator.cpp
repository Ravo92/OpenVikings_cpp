#include "C2HumanIterator.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapSectorAttachment.h"
#include "C2ObjectLayout.h"
#include "C2VehicleAccess.h"
#include "XCrtFileAccess.h"

namespace NC2Logic
{
	// Original function: FUN_0042c24a; radius filter without a player filter.
	std::int32_t CHumanIterator::GetFirstHumanId(const void* point, std::int32_t searchRadius)
	{
		flags = 2;
		packedCenter = NC2Lifetime::Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
		radius = searchRadius;
		currentObjectId = l_GetNext(CHumanArray::sTheObjectPtr->mFirstScanIndex - 1);

		return currentObjectId;
	}

	// Original function: FUN_0042c279; corners wrap to WORD before sector conversion.
	std::int32_t CHumanIterator::GetFirstSectorHumanId(std::uint32_t player, std::uint32_t point, std::int32_t searchRadius, bool useSecondList)
	{
		selection = player;
		packedCenter = point;
		radius = searchRadius;
		auto corner = static_cast<std::uint32_t>(static_cast<std::uint16_t>(point - searchRadius)) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>((point >> 16) - searchRadius)) << 16);
		firstSector = SSectorGridCursor(corner);
		const auto diameter = static_cast<std::uint32_t>(searchRadius) * 2u;
		corner = static_cast<std::uint32_t>(static_cast<std::uint16_t>(corner + diameter)) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>((corner >> 16) + diameter)) << 16);
		lastSector = SSectorGridCursor(corner);
		firstSector.CutInsideMap();
		lastSector.CutInsideMap();
		currentSector = firstSector;
		flags = useSecondList ? 3u : 2u;
		currentObjectId = l_GetNextSectorHumanId(-1);

		return currentObjectId;
	}

	// Original function: FUN_0042c31b; the full-grid overload retains the existing center/radius fields.
	std::int32_t CHumanIterator::GetFirstSectorHumanId(std::uint32_t player, bool useSecondList)
	{
		selection = player;
		firstSector = SSectorGridCursor(0, 0);
		lastSector = SSectorGridCursor(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(SSectorGridCursor::sGridWidth) - 1u), NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(SSectorGridCursor::sGridHeight) - 1u));
		flags = useSecondList ? 1u : 0u;
		currentSector = firstSector;
		currentObjectId = l_GetNextSectorHumanId(-1);

		return currentObjectId;
	}

	// Original function: FUN_0042c376.
	std::int32_t CHumanIterator::GetNextSectorHumanId()
	{
		if (currentObjectId == -1)
			return -1;

		currentObjectId = l_GetNextSectorHumanId(currentObjectId);

		return currentObjectId;
	}

	// Original function: FUN_0042c433; sector chains retain list order without allocation filtering.
	std::int32_t CHumanIterator::l_GetNextSectorHumanId(std::int32_t previous)
	{
		using namespace NC2Lifetime;

		for (;;)
		{
			if (previous == -1)
			{
				const auto* manager = CGameMapSectorAttachmentManager::sTheObjectPtr;
				const auto index = static_cast<std::uint32_t>(manager->GetGridWidth()) * static_cast<std::uint32_t>(currentSector.y) + static_cast<std::uint32_t>(currentSector.x);
				previous = Read<std::int32_t>(manager->GetSectorData() + (static_cast<std::size_t>(index) * NC2Layout::SectorLimits::PlayerSlotsUnsigned + selection) * NC2Layout::RecordSize::SectorPlayerUnsigned, ((flags & 1u) != 0 ? NC2Layout::SectorPlayerOffset::MilitaryHumanHeadUnsigned : NC2Layout::SectorPlayerOffset::CivilianHumanHeadUnsigned));
			}
			else
			{
				previous = Read<std::int32_t>(CHumanArray::mStaticVars + static_cast<std::uint32_t>(previous) * CHumanArray::ElementSize, NC2Layout::HumanOffset::NextSectorHumanId);
			}

			if (previous != -1)
			{
				if ((flags & 2u) == 0)
					return previous;

				const auto* point = Human_GetMapPositionRef(CHumanArray::mStaticVars + static_cast<std::uint32_t>(previous) * CHumanArray::ElementSize);

				if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y), static_cast<std::int16_t>(packedCenter), static_cast<std::int16_t>(packedCenter >> 16))) <= static_cast<std::uint32_t>(radius))
					return previous;
			}
			else
			{
				++currentSector.x;

				if (currentSector.x <= lastSector.x)
					continue;

				++currentSector.y;
				currentSector.x = firstSector.x;

				if (currentSector.y > lastSector.y)
				{

					return -1;
				}
			}
		}
	}

	// Original function: FUN_0042c1d4.
	std::int32_t CHumanIterator::GetFirstHumanId()
	{
		flags = 0;
		currentObjectId = l_GetNext(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(CHumanArray::sTheObjectPtr->mFirstScanIndex) - 1u));

		return currentObjectId;
	}

	// Original function: FUN_0042c1ee.
	std::int32_t CHumanIterator::GetFirstHumanId(std::uint32_t player)
	{
		selection = player;
		flags = 1;
		currentObjectId = l_GetNext(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(CHumanArray::sTheObjectPtr->mFirstScanIndex) - 1u));

		return currentObjectId;
	}

	// Original function: FUN_0042c214; the radius overload scans the same array in order.
	std::int32_t CHumanIterator::GetFirstHumanId(std::uint32_t player, const void* point, std::int32_t radiusValue)
	{
		selection = player;
		flags = 3;
		packedCenter = NC2Lifetime::Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
		radius = radiusValue;
		currentObjectId = l_GetNext(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(CHumanArray::sTheObjectPtr->mFirstScanIndex) - 1u));

		return currentObjectId;
	}

	// Original function: FUN_0042c392.
	std::int32_t CHumanIterator::GetNextHumanId()
	{
		if (currentObjectId == -1)
			return -1;

		currentObjectId = l_GetNext(currentObjectId);

		return currentObjectId;
	}

	// Original function: FUN_0042c3ae.
	std::int32_t CHumanIterator::l_GetNext(std::int32_t previous)
	{
		auto index = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(previous) + 1u);

		for (; index <= CHumanArray::sTheObjectPtr->mLastScanIndex; index = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(index) + 1u))
		{
			const void* human = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(CHumanArray::mStaticVars) + static_cast<std::uint32_t>(index) * 0xC58u);

			if (NC2Lifetime::Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0)
				continue;

			if ((flags & 1u) != 0 && NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) != selection)
				continue;

			if ((flags & 2u) != 0)
			{
				const auto* position = Human_GetMapPositionRef(human);

				if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(static_cast<std::int16_t>(position[0]), static_cast<std::int16_t>(position[1]), static_cast<std::int16_t>(packedCenter), static_cast<std::int16_t>(packedCenter >> 16))) > static_cast<std::uint32_t>(radius))
					continue;
			}

			return index;
		}

		return -1;
	}
}