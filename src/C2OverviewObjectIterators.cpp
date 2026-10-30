#include "C2GameMapAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapSectorAttachment.h"
#include "C2MapSectorGridCursor.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2VehicleAccess.h"
#include "XCrtFileAccess.h"

#include <cstring>

namespace NC2Logic
{
	template <typename T>
	static T Iterator_Read(const void* data, std::size_t offset)
	{
		T value;
		std::memcpy(&value, static_cast<const std::byte*>(data) + offset, sizeof(value));

		return value;
	}

	static const void* Iterator_Entry(const void* data, std::int32_t index, std::uint32_t stride)
	{

		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(data) + static_cast<std::size_t>(static_cast<std::uint32_t>(index)) * stride);
	}

	static std::int32_t Iterator_Increment(std::int32_t value)
	{

		return NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(value) + 1u);
	}

	// Original function: FUN_0042b73f.
	CHouseIterator::CHouseIterator()
	{
		l_ConstructNonVirtual();
		current = -1;
	}

	// Original function: FUN_0042b74f; flags/filter fields remain uninitialized.
	void CHouseIterator::l_ConstructNonVirtual()
	{
		center[1] = 0;
		center[0] = 0;

		for (auto& coordinate : sectorCoordinates)
		{
			coordinate = 0;
		}
	}

	// Original function: FUN_0042b76e.
	std::int32_t CHouseIterator::GetFirstHouseId()
	{
		flags = 0;
		current = l_GetNext(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(CHouseArray::sTheObjectPtr->mFirstScanIndex) - 1u));

		return current;
	}

	// Original function: FUN_0042b788.
	std::int32_t CHouseIterator::GetFirstHouseId(std::uint32_t playerId)
	{
		player = playerId;
		flags = 1;
		current = l_GetNext(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(CHouseArray::sTheObjectPtr->mFirstScanIndex) - 1u));

		return current;
	}

	// Original function: FUN_0042b7ae; all-house iteration retains its radius filter.
	std::int32_t CHouseIterator::GetFirstHouseId(std::uint32_t playerId, const void* point, std::uint32_t radiusValue)
	{
		player = playerId;
		flags = 3;
		std::memcpy(center, point, 4);
		radius = radiusValue;
		current = l_GetNext(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(CHouseArray::sTheObjectPtr->mFirstScanIndex) - 1u));

		return current;
	}

	// Original function: FUN_0042b7e4; the type filter uses the engine-house ID.
	std::int32_t CHouseIterator::GetFirstHouseId(std::uint32_t playerId, const void* point, std::uint32_t radiusValue, std::uint32_t engineType)
	{
		player = playerId;
		flags = 7;
		std::memcpy(center, point, 4);
		radius = radiusValue;
		houseType = engineType;
		current = l_GetNext(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(CHouseArray::sTheObjectPtr->mFirstScanIndex) - 1u));

		return current;
	}

	// Original function: FUN_0042b821; the boolean selects the second sector head.
	std::int32_t CHouseIterator::GetFirstSectorHouseId(std::uint32_t playerId, const void* point, std::uint32_t radiusValue, bool useSecondList)
	{
		player = playerId;
		std::memcpy(center, point, 4);
		radius = radiusValue;
		auto lowerPoint = NC2Lifetime::Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
		NC2Lifetime::Write<std::uint16_t>(&lowerPoint, NC2Layout::MapCoordinatePairOffset::X, center[0] - radiusValue);
		NC2Lifetime::Write<std::uint16_t>(&lowerPoint, NC2Layout::MapCoordinatePairOffset::Y, center[1] - radiusValue);
		SSectorGridCursor lower(lowerPoint);
		lower.CutInsideMap();
		auto upperPoint = lowerPoint;
		NC2Lifetime::Write<std::uint16_t>(&upperPoint, NC2Layout::MapCoordinatePairOffset::X, NC2Lifetime::Read<std::uint16_t>(&upperPoint, NC2Layout::MapCoordinatePairOffset::X) + radiusValue * 2u);
		NC2Lifetime::Write<std::uint16_t>(&upperPoint, NC2Layout::MapCoordinatePairOffset::Y, NC2Lifetime::Read<std::uint16_t>(&upperPoint, NC2Layout::MapCoordinatePairOffset::Y) + radiusValue * 2u);
		SSectorGridCursor upper(upperPoint);
		upper.CutInsideMap();
		sectorCoordinates[0] = lower.x;
		sectorCoordinates[1] = lower.y;
		sectorCoordinates[2] = upper.x;
		sectorCoordinates[3] = upper.y;
		sectorCoordinates[4] = lower.x;
		sectorCoordinates[5] = lower.y;
		flags = useSecondList ? 1u : 0u;
		current = l_GetNextSectorHouseId(-1);

		return current;
	}

	// Original function: FUN_0042b8c0.
	std::int32_t CHouseIterator::GetNextSectorHouseId()
	{
		if (current != -1)
			current = l_GetNextSectorHouseId(current);

		return current;
	}

	// Original function: FUN_0042b98a; preserve sector/list order and unsigned radius.
	std::int32_t CHouseIterator::l_GetNextSectorHouseId(std::int32_t previous)
	{
		using namespace NC2Lifetime;

		for (;;)
		{
			std::int32_t id;

			if (previous == -1)
			{
				const auto* manager = CGameMapSectorAttachmentManager::sTheObjectPtr;
				const auto cell = sectorCoordinates[5] * manager->GetGridWidth() + sectorCoordinates[4];
				id = Read<std::int32_t>(manager->GetSectorData(), (cell * NC2Layout::SectorLimits::PlayerSlotsUnsigned + player) * NC2Layout::RecordSize::SectorPlayerUnsigned + ((flags & 1u) != 0 ? NC2Layout::SectorPlayerOffset::MilitaryHouseHeadUnsigned : NC2Layout::SectorPlayerOffset::CivilianHouseHeadUnsigned));
			}
			else
			{
				id = Read<std::int32_t>(CHouseArray::mStaticVars, static_cast<std::uint32_t>(previous) * CHouseArray::RuntimeElementSize + NC2Layout::HouseOffset::NextSectorHouseIdUnsigned);
			}

			previous = id;

			if (id != -1)
			{
				const void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

				if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(house, NC2Layout::HouseOffset::MapPositionX), Read<std::int16_t>(house, NC2Layout::HouseOffset::MapPositionY), center[0], center[1])) <= radius)
					return id;
			}
			else
			{
				++sectorCoordinates[4];

				if (static_cast<std::int32_t>(sectorCoordinates[4]) > static_cast<std::int32_t>(sectorCoordinates[2]))
				{
					sectorCoordinates[4] = sectorCoordinates[0];
					++sectorCoordinates[5];

					if (static_cast<std::int32_t>(sectorCoordinates[5]) > static_cast<std::int32_t>(sectorCoordinates[3]))
					{

						return -1;
					}
				}
			}
		}
	}

	// Original function: FUN_0042b8dc.
	std::int32_t CHouseIterator::GetNextHouseId()
	{
		if (current == -1)
			return -1;

		current = l_GetNext(current);

		return current;
	}

	// Original function: FUN_0042b8f8; all three original filter bits are retained.
	std::int32_t CHouseIterator::l_GetNext(std::int32_t previous)
	{
		for (std::int32_t index = Iterator_Increment(previous); index <= CHouseArray::sTheObjectPtr->mLastScanIndex; index = Iterator_Increment(index))
		{
			const void* house = Iterator_Entry(CHouseArray::mStaticVars, index, CHouseArray::ElementSize);

			if (Iterator_Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0)
				continue;

			if ((flags & 1u) != 0 && Iterator_Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) != player)
				continue;

			if ((flags & 2u) != 0 && static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Iterator_Read<std::int16_t>(house, NC2Layout::HouseOffset::MapPositionX), Iterator_Read<std::int16_t>(house, NC2Layout::HouseOffset::MapPositionY), center[0], center[1])) > radius)
				continue;

			if ((flags & 4u) != 0 && Iterator_Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType) != houseType)
				continue;

			return index;
		}

		return -1;
	}

	// Original function: FUN_0042dbdf.
	CGuideIterator::CGuideIterator()
	{
		center[1] = 0;
		center[0] = 0;
		current = -1;
	}

	// Original function: FUN_0042dbf0.
	std::int32_t CGuideIterator::GetFirstGuideId(std::uint32_t playerId)
	{
		player = playerId;
		flags = 1;
		current = l_GetNext(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(CGuideArray::sTheObjectPtr->mFirstScanIndex) - 1u));

		return current;
	}

	// Original function: FUN_0042DC16.
	std::int32_t CGuideIterator::GetFirstGuideId(std::uint32_t playerId, const void* point, std::uint32_t distance)
	{
		player = playerId;
		flags = 3;
		std::memcpy(center, point, sizeof(center));
		radius = distance;
		current = l_GetNext(NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(CGuideArray::sTheObjectPtr->mFirstScanIndex) - 1u));

		return current;
	}

	// Original function: FUN_0042dc4c.
	std::int32_t CGuideIterator::GetNextGuideId()
	{
		if (current == -1)
			return -1;

		current = l_GetNext(current);

		return current;
	}

	// Original function: FUN_0042dc68.
	std::int32_t CGuideIterator::l_GetNext(std::int32_t previous)
	{
		for (std::int32_t index = Iterator_Increment(previous); index <= CGuideArray::sTheObjectPtr->mLastScanIndex; index = Iterator_Increment(index))
		{
			const void* guide = Iterator_Entry(CGuideArray::mStaticVars, index, CGuideArray::ElementSize);

			if (Iterator_Read<std::uint8_t>(guide, NC2Layout::GuideOffset::AllocatedFlag) == 0)
				continue;

			if ((flags & 1u) != 0 && Iterator_Read<std::uint32_t>(guide, NC2Layout::GuideOffset::PlayerId) != player)
				continue;

			if ((flags & 2u) != 0 && static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Iterator_Read<std::int16_t>(guide, NC2Layout::GuideOffset::MapPositionX), Iterator_Read<std::int16_t>(guide, NC2Layout::GuideOffset::MapPositionY), center[0], center[1])) > radius)
				continue;

			return index;
		}

		return -1;
	}

	// Original function: FUN_00434d30.
	CMapMoveableIterator::CMapMoveableIterator() : flags(0), current(-1)
	{
	}

	// Original function: FUN_00434d3a; an unused first slot ends iteration immediately.
	std::int32_t CMapMoveableIterator::GetFirstMapMoveableId()
	{
		flags = 1;
		current = gMapMoveableArrayStorage.manager->mFirstScanIndex;

		if (Iterator_Read<std::uint8_t>(Iterator_Entry(gMapMoveableArrayStorage.entries, current, 0x58), NC2Layout::MapMoveableOffset::AllocatedFlag) == 0)
			current = -1;

		return current;
	}

	// Original function: FUN_00434d65; caller supplies an inside-map position.
	std::int32_t CMapMoveableIterator::GetFirstMapMoveableId(const void* point)
	{
		flags = 2;
		current = Iterator_Read<std::int16_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::FirstMapMoveableId);

		if (current < -1)
			current = -1;

		return current;
	}

	// Original function: FUN_00434d8f; failed iteration clears flags, retaining current.
	std::int32_t CMapMoveableIterator::GetNextMapMoveableId()
	{
		if (flags == 2)
		{
			current = Iterator_Read<std::int32_t>(Iterator_Entry(gMapMoveableArrayStorage.entries, current, 0x58), NC2Layout::MapMoveableOffset::NextCellMoveableId);

			if (current != -1)
				return current;
		}
		else if (flags == 1)
		{
			current = Iterator_Increment(current);

			while (current <= gMapMoveableArrayStorage.manager->mLastScanIndex)
			{
				if (Iterator_Read<std::uint8_t>(Iterator_Entry(gMapMoveableArrayStorage.entries, current, 0x58), NC2Layout::MapMoveableOffset::AllocatedFlag) != 0)
					return current;

				current = Iterator_Increment(current);
			}
		}

		flags = 0;

		return -1;
	}
}