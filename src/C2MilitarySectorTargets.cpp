#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapSectorAttachment.h"
#include "C2MapSectorGridCursor.h"
#include "C2MilitaryTargetSearch.h"
#include "C2ObjectLayout.h"
#include "C2VehicleAccess.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_0044edd5; the clipped last sector is excluded in both native loops.
	bool CGameMapSectorAttachmentManager::CheckForEnemyTarget_Human(std::uint32_t player, const void* point, std::uint32_t minimumDistance, std::uint32_t maximumDistance, void* target, bool ignoreAnimals, bool allowLandscape)
	{
		const SSectorGridCursor center(Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		std::uint16_t boundsPoint[2] = { static_cast<std::uint16_t>(Read<std::uint16_t>(point, NC2Layout::MapCoordinatePairOffset::X) - maximumDistance), static_cast<std::uint16_t>(Read<std::uint16_t>(point, NC2Layout::MapCoordinatePairOffset::Y) - maximumDistance) };
		SSectorGridCursor first(Read<std::uint32_t>(boundsPoint, NC2Layout::MapCoordinatePairOffset::Coordinates));
		boundsPoint[0] = static_cast<std::uint16_t>(boundsPoint[0] + maximumDistance * 2u);
		boundsPoint[1] = static_cast<std::uint16_t>(boundsPoint[1] + maximumDistance * 2u);
		SSectorGridCursor last(Read<std::uint32_t>(boundsPoint, NC2Layout::MapCoordinatePairOffset::Coordinates));
		first.CutInsideMap();
		last.CutInsideMap();
		bool scan = false;
		for (auto row = first.y; row < last.y && !scan; ++row)
		{
			for (auto column = first.x; column < last.x; ++column)
			{
				const auto* cache = mSectorAuxiliaryData + static_cast<std::size_t>(static_cast<std::uint32_t>(row) * static_cast<std::uint32_t>(mGridWidth) + static_cast<std::uint32_t>(column)) * 0xFCu + player * 12u;
				if (Read<std::uint32_t>(cache, NC2Layout::SectorEnemySearchCacheOffset::LastSearchTick) + 12u <= gGameTime.tick || Read<std::uint8_t>(cache, NC2Layout::SectorEnemySearchCacheOffset::FoundTarget) != 0)
				{
					scan = true; break;
				}
			}
		}
		if (!scan) return false;
		// Windows updates the source sector's cache, regardless of which sector triggered the scan.
		auto* cache = mSectorAuxiliaryData + static_cast<std::size_t>(static_cast<std::uint32_t>(center.y) * static_cast<std::uint32_t>(mGridWidth) + static_cast<std::uint32_t>(center.x)) * 0xFCu + player * 12u;
		Write(cache, NC2Layout::SectorEnemySearchCacheOffset::LastSearchTick, gGameTime.tick);
		allowLandscape = allowLandscape && Read<std::uint32_t>(cache, NC2Layout::SectorEnemySearchCacheOffset::LastLandscapeSearchTick) + 60u < gGameTime.tick;
		if (allowLandscape) Write(cache, NC2Layout::SectorEnemySearchCacheOffset::LastLandscapeSearchTick, gGameTime.tick);
		const bool result = l_CheckSectorForEnemyTarget_Human(player, point, minimumDistance, maximumDistance, target, ignoreAnimals, allowLandscape);
		Write<std::uint8_t>(cache, NC2Layout::SectorEnemySearchCacheOffset::FoundTarget, result);
		return result;
	}

	// Original function: FUN_0044ef00; search ordering precedes the random candidate selection in each family.
	bool CGameMapSectorAttachmentManager::l_CheckSectorForEnemyTarget_Human(std::uint32_t player, const void* point, std::uint32_t minimumDistance, std::uint32_t maximumDistance, void* target, bool ignoreAnimals, bool allowLandscape)
	{
		auto setTarget = [&] (std::uint32_t type, std::int32_t id, const void* object)
		{
			Write(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint, id);
			Write(target, NC2Layout::TargetDataOffset::ObjectRevision, Read<std::uint32_t>(object, NC2Layout::ActorIdentityOffset::UniqueId));
			Write(target, NC2Layout::TargetDataOffset::Type, type);
		};
		auto id = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, minimumDistance, maximumDistance, true, 5, 3, true, false);
		if (id != -1)
		{
			setTarget(1, id, CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize); return true;
		}
		id = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, minimumDistance, maximumDistance, true, 5, 3, false, false);
		if (id != -1)
		{
			const auto* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::ElementSize;
			Vehicle_GetMapPositionRef(vehicle);
			setTarget(2, id, vehicle);
			return true;
		}
		id = Tool_GetOneOfNextEnemyHousesInRange(player, point, minimumDistance, maximumDistance, true, 5, 3);
		if (id != -1)
		{
			setTarget(4, id, CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize); return true;
		}
		if (!ignoreAnimals)
		{
			id = Tool_Animals_GetOneOfNextEnemyInRange(player, point, minimumDistance, maximumDistance, true, false, 5, 3);
			if (id != -1)
			{
				setTarget(3, id, GetAnimalPtr(id)); return true;
			}
		}
		id = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, minimumDistance, maximumDistance, false, 5, 3, true, false);
		if (id != -1)
		{
			setTarget(1, id, CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize); return true;
		}
		id = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, minimumDistance, maximumDistance, false, 5, 3, false, false);
		if (id != -1)
		{
			const auto* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::ElementSize;
			Vehicle_GetMapPositionRef(vehicle);
			setTarget(2, id, vehicle);
			return true;
		}
		id = Tool_GetOneOfNextEnemyHousesInRange(player, point, minimumDistance, maximumDistance, false, 5, 3);
		if (id != -1)
		{
			setTarget(4, id, CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize); return true;
		}
		if (allowLandscape && NXBasics::Runtime_TimeSigned32(player) < 20 && Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TypeUnsigned) == 2)
		{
			std::uint32_t landscape = 0;
			if (Tool_GetOneOfNearestEnemyLandscapes(player, point, 0, maximumDistance, true, 5, 3, &landscape))
			{
				Write(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint, MapMigPoint_GetLong(&landscape));
				Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 6);
				return true;
			}
		}
		return false;
	}
}
