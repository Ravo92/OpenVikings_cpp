#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapSectorAttachment.h"
#include "C2ObjectLayout.h"
#include "C2VehicleAccess.h"
#include "C2VehicleIterator.h"

namespace NC2Logic
{
	// Original function: FUN_00440ccc; no player filter is installed.
	std::int32_t CVehicleIterator::GetFirstVehicleId()
	{
		flags = 0;
		currentObjectId = l_GetNext(CVehicleArray::sTheObjectPtr->mFirstScanIndex - 1);
		return currentObjectId;
	}

	// Original function: FUN_00440ce6; this player scan does not initialize a radius filter.
	std::int32_t CVehicleIterator::GetFirstVehicleId(std::uint32_t player)
	{
		selection = player;
		flags = 1;
		currentObjectId = l_GetNext(CVehicleArray::sTheObjectPtr->mFirstScanIndex - 1);
		return currentObjectId;
	}

	// Original function: FUN_00440d0c; array iteration is distinct from sector iteration.
	std::int32_t CVehicleIterator::GetFirstVehicleId(std::uint32_t player, const void* point, std::int32_t searchRadius)
	{
		selection = player;
		flags = 3;
		packedCenter = NC2Lifetime::Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
		radius = searchRadius;
		currentObjectId = l_GetNext(CVehicleArray::sTheObjectPtr->mFirstScanIndex - 1);
		return currentObjectId;
	}

	// Original function: FUN_00440dfd.
	std::int32_t CVehicleIterator::GetNextVehicleId()
	{
		if (currentObjectId == -1) return -1;
		currentObjectId = l_GetNext(currentObjectId);
		return currentObjectId;
	}

	// Original function: FUN_00440e19; retain the live occupied-range upper bound.
	std::int32_t CVehicleIterator::l_GetNext(std::int32_t previous)
	{
		using namespace NC2Lifetime;
		for (auto id = previous + 1; id <= CVehicleArray::sTheObjectPtr->mLastScanIndex; ++id)
		{
			const void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::ElementSize;
			if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) == 0 || ((flags & 1u) != 0 && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId) != selection)) continue;
			if ((flags & 2u) != 0)
			{
				const void* point = Vehicle_GetMapPositionRef(vehicle);
				if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y), static_cast<std::int16_t>(packedCenter), static_cast<std::int16_t>(packedCenter >> 16))) > static_cast<std::uint32_t>(radius)) continue;
			}
			return id;
		}
		return -1;
	}

	// Original function: FUN_00440d42; corners wrap to WORD before sector conversion.
	std::int32_t CVehicleIterator::GetFirstSectorVehicleId(std::uint32_t player, std::uint32_t point, std::int32_t searchRadius, bool useSecondList)
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
		flags = useSecondList ? 1u : 0u;
		currentObjectId = l_GetNextSectorVehicleId(-1);
		return currentObjectId;
	}

	// Original function: FUN_00440de1.
	std::int32_t CVehicleIterator::GetNextSectorVehicleId()
	{
		if (currentObjectId == -1) return -1;
		currentObjectId = l_GetNextSectorVehicleId(currentObjectId);
		return currentObjectId;
	}

	// Original function: FUN_00440ea1; sector chains retain list order without allocation filtering.
	std::int32_t CVehicleIterator::l_GetNextSectorVehicleId(std::int32_t previous)
	{
		using namespace NC2Lifetime;
		for (;;)
		{
			if (previous == -1)
			{
				const auto* manager = CGameMapSectorAttachmentManager::sTheObjectPtr;
				const auto index = static_cast<std::uint32_t>(manager->GetGridWidth()) * static_cast<std::uint32_t>(currentSector.y) + static_cast<std::uint32_t>(currentSector.x);
				previous = Read<std::int32_t>(manager->GetSectorData() + (static_cast<std::size_t>(index) * NC2Layout::SectorLimits::PlayerSlotsUnsigned + selection) * NC2Layout::RecordSize::SectorPlayerUnsigned, ((flags & 1u) != 0 ? NC2Layout::SectorPlayerOffset::MilitaryVehicleHeadUnsigned : NC2Layout::SectorPlayerOffset::CivilianVehicleHeadUnsigned));
			}
			else previous = Read<std::int32_t>(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(previous) * CVehicleArray::ElementSize, NC2Layout::VehicleOffset::NextSectorVehicleId);
			if (previous != -1)
			{
				const auto* point = Vehicle_GetMapPositionRef(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(previous) * CVehicleArray::ElementSize);
				if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y), static_cast<std::int16_t>(packedCenter), static_cast<std::int16_t>(packedCenter >> 16))) <= static_cast<std::uint32_t>(radius)) return previous;
			}
			else
			{
				++currentSector.x;
				if (currentSector.x <= lastSector.x) continue;
				++currentSector.y;
				currentSector.x = firstSector.x;
				if (currentSector.y > lastSector.y) return -1;
			}
		}
	}
}
