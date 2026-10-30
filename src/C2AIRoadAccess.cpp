#include "C2AIRoadAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapPathfinderSearch.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "C2StaticHouseTypeData.h"
#include "C2WallRoadConstruction.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_00456241.
	void AIRoadManager_Init(void* manager)
	{
		Write<std::int32_t>(manager, NC2Layout::AIRoadManagerOffset::PreviousHouseId, -1);
	}

	// Original function: FUN_00456245; process one production house after outstanding roads finish.
	void AIRoadManager_DoOneLogicTick(void* manager, std::uint32_t player, const void* resources)
	{
		const auto previous = Read<std::int32_t>(manager, NC2Layout::AIRoadManagerOffset::PreviousHouseId);

		if (previous != -1 && Read<std::uint8_t>(CHouseArray::mStaticVars + static_cast<std::uint32_t>(previous) * CHouseArray::RuntimeElementSize, NC2Layout::HouseOffset::AllocatedFlag) == 0)
			Write<std::int32_t>(manager, NC2Layout::AIRoadManagerOffset::PreviousHouseId, -1);

		if (Read<std::uint32_t>(RoadBuildManager_GetRoadBuildDataPtr(gRoadBuildManager, player), NC2Layout::BuildPlayerOffset::PointCount) != 0)
			return;

		CHouseIterator houses;

		for (auto id = houses.GetFirstHouseId(player); id != -1; id = houses.GetNextHouseId())
		{
			const auto cursor = Read<std::int32_t>(manager, NC2Layout::AIRoadManagerOffset::PreviousHouseId);

			if (cursor != -1)
			{
				if (cursor <= id)
					Write<std::int32_t>(manager, NC2Layout::AIRoadManagerOffset::PreviousHouseId, -1);

				continue;
			}

			const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
				continue;

			const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);

			if (type == 1 || type == 7 || type == 8 || type == 9)
				continue;

			const auto good = Read<std::uint32_t>(resources, NC2Layout::AIResourceManagerOffset::HouseGoods + type * NC2Layout::RecordSize::AIResourceIndexValue);

			if (Read<std::uint8_t>(resources, NC2Layout::AIResourceManagerOffset::FirstGoodCheckTerrainFlag + good * NC2Layout::RecordSize::AIResourceGood) != 0)
				continue;

			const auto* source = house + NC2Layout::HouseOffset::DoorEntryPoint;
			CHouseIterator nearby;

			for (auto otherId = nearby.GetFirstHouseId(player, source, 20); otherId != -1; otherId = nearby.GetNextHouseId())
			{
				if (otherId == id)
					continue;

				const auto* other = CHouseArray::mStaticVars + static_cast<std::uint32_t>(otherId) * CHouseArray::RuntimeElementSize;
				const auto otherType = Read<std::uint32_t>(other, NC2Layout::HouseOffset::LogicType);
				const auto& data = gStaticHouseTypeData[otherType];

				for (std::uint32_t stock = 0; stock < data.stockCount; ++stock)
				{
					if (otherType != 1 && otherType != 7 && otherType != 8 && otherType != 9 && (static_cast<std::uint32_t>(data.stock[stock].goodType) != good || data.stock[stock].isOutputStock == 0))
						continue;

					std::byte pathfinder[132];
					// The original point-only constructor is followed by a complete initialization.
					Write<std::uint16_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentPositionY, 0);
					Write<std::uint16_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentPosition, 0);
					Write<std::uint16_t>(pathfinder, NC2Layout::MapPathfinderOffset::DestinationPositionY, 0);
					Write<std::uint16_t>(pathfinder, NC2Layout::MapPathfinderOffset::DestinationPosition, 0);
					MapPathfinderElement_Init(pathfinder);
					RoadBuildManager_SetRoadBuildFlag(gRoadBuildManager, nullptr, player, source);
					MapPathfinderElement_StartPathfinder(pathfinder, source, other + NC2Layout::HouseOffset::DoorEntryPoint, 20, true, false, false, true);
					auto point = Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates);

					for (auto direction = MapPathfinderElement_GetNextDirection(pathfinder); direction >= 0; direction = MapPathfinderElement_GetNextDirection(pathfinder))
					{
						MapMigPoint_MoveCursorUnwrapped(point, direction);
						RoadBuildManager_SetRoadBuildFlag(gRoadBuildManager, nullptr, player, &point);
					}
				}
			}

			Write(manager, NC2Layout::AIRoadManagerOffset::PreviousHouseId, id);

			return;
		}
	}
}