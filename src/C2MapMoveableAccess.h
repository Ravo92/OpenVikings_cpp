#pragma once

#include <cstdint>

namespace NC2Logic
{
	const void* MapMoveable_GetMapPositionRef(const void* moveable);
	class CArrayHandler;
	struct SSectorGridCursor;

	std::int32_t MapMoveableArray_AllocateMapMoveable(CArrayHandler* array, std::uint32_t type, std::int32_t objectId);
	void MapMoveableArray_FreeMapMoveable(CArrayHandler* array, std::int32_t id);
	std::int32_t MapPathfinderManager_AllocatePathfinder(CArrayHandler* manager);
	void MapPathfinderManager_FreePathfinder(CArrayHandler* manager, std::int32_t id);
	void* GetUnsafePathfinderPtr(std::int32_t id);
	void MapPathfinderElement_Init(void* pathfinder);

	bool MapMoveable_Init(void* moveable, std::int32_t id, std::uint32_t type, std::int32_t objectId);
	void MapMoveable_Exit(void* moveable);
	void MapMoveable_l_Misc_FreeAllAllocatedData(void* moveable);
	void MapMoveable_Attachment_AttachToMap(void* moveable, const void* packedMigPoint);
	void MapMoveable_Attachment_StartTurn(void* moveable, std::int32_t direction);
	void MapMoveable_Attachment_GetNextMapPosition(const void* moveable, void* point);
	std::uint32_t MapMoveable_GetUniqueId(const void* moveable);
	bool MapMoveable_Pathfinder_Start(void* moveable, const void* destination, std::uint32_t radius, bool useResistance, bool useSectors, bool useGuides);
	bool MapMoveable_Pathfinder_Start(void* moveable, std::int32_t targetId, std::int32_t distance, std::uint32_t radius, bool useResistance, bool useSectors, bool useGuides);
	bool MapMoveable_Pathfinder_Stop(void* moveable);
	bool MapMoveable_Attachment_PerformTurn(void* moveable);
	void __fastcall MapMoveableArray_DoOneGameTick(void* array, void* unusedEdx);
	void MapMoveable_DoOneGameTick(void* moveable);
	void MapMoveable_DoOneGameTick_Human(void* moveable, void* human);
	void MapMoveable_DoOneGameTick_Animal(void* moveable, void* animal);
	void MapMoveable_DoOneGameTick_Vehicle(void* moveable, void* vehicle);
	void MapMoveable_Attachment_DetachFromMap(void* moveable);
	void MapMoveable_l_Attachment_AttachToGameMap(void* moveable);
	void MapMoveable_l_Attachment_DetachFromGameMap(void* moveable);
	void MapMoveable_Attachment_AttachToSectorMap(void* moveable);
	void MapMoveable_Attachment_DetachFromSectorMap(void* moveable);
	const void* MapMoveable_Pathfinder_GetPathfinderPtr(const void* moveable);

	void Human_Sector_AttachHumanToSector(void* human, const SSectorGridCursor& sector);
	void Human_Sector_DetachHumanFromSector(void* human, const SSectorGridCursor& sector);
	void Animal_Sector_AttachAnimalToSector(void* animal, const SSectorGridCursor& sector);
	void Animal_Sector_DetachAnimalFromSector(void* animal, const SSectorGridCursor& sector);
	void Vehicle_Sector_AttachVehicleToSector(void* vehicle, const SSectorGridCursor& sector);
	void Vehicle_Sector_DetachVehicleFromSector(void* vehicle, const SSectorGridCursor& sector);
}