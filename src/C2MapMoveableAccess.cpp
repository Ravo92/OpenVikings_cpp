#include "C2AnimalAccess.h"
#include "C2AnimalTasks.h"
#include "C2GameMapAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanTaskAccess.h"
#include "C2HumanWalking.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapPathfinderSearch.h"
#include "C2MapSectorAttachment.h"
#include "C2MapSectorGridCursor.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "C2VehicleSimulation.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

#include <cstddef>
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;
	// Original function: FUN_0047dbcb; callers pass a CMapMoveable receiver.
	const void* MapMoveable_GetMapPositionRef(const void* moveable)
	{
		return static_cast<const std::byte*>(moveable) + NC2Layout::MapMoveableOffset::MapPosition;
	}

	namespace
	{
		std::byte* GetSector(const SSectorGridCursor& sector)
		{
			auto* manager = CGameMapSectorAttachmentManager::sTheObjectPtr;
			const auto index = static_cast<std::uint32_t>(sector.y) * static_cast<std::uint32_t>(manager->GetGridWidth()) + static_cast<std::uint32_t>(sector.x);
			return manager->GetSectorData() + static_cast<std::size_t>(index) * 0x540u;
		}
	}

	// Original function: FUN_0040bcbf.
	std::int32_t MapMoveableArray_AllocateMapMoveable(CArrayHandler* array, std::uint32_t type, std::int32_t objectId)
	{
		const auto id = array->Element_Allocate();
		if (id == -1) return -1;
		if (!MapMoveable_Init(gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable, id, type, objectId))
		{
			MapMoveableArray_FreeMapMoveable(array, id);
			return -1;
		}
		return id;
	}

	// Original function: FUN_0040bd00. Element_Free leaves the allocation flag intact.
	void MapMoveableArray_FreeMapMoveable(CArrayHandler* array, std::int32_t id)
	{
		void* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag) != 0 && array->Element_Free(id, true)) MapMoveable_Exit(moveable);
	}

	// Original function: FUN_0040BD33; deletion follows each individual tick.
	void __fastcall MapMoveableArray_DoOneGameTick(void* array, void* /*unusedEdx*/)
	{
		auto* handler = static_cast<CArrayHandler*>(array);
		for (auto id = handler->mFirstScanIndex; id <= handler->mLastScanIndex; ++id)
		{
			void* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;
			if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag) == 0) continue;
			MapMoveable_DoOneGameTick(moveable);
			if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::RemovalRequestedFlag) != 0) Logic_EntryPoint_FreeMapMoveable(static_cast<std::uint32_t>(id));
		}
	}

	// Original function: FUN_0043902B.
	void MapMoveable_DoOneGameTick(void* moveable)
	{
		const auto id = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId);
		switch (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType))
		{
			case SwitchIds::MapMoveableType::Human: MapMoveable_DoOneGameTick_Human(moveable, CHumanArray::mStaticVars + id * CHumanArray::ElementSize); return;
			case SwitchIds::MapMoveableType::Animal: MapMoveable_DoOneGameTick_Animal(moveable, CAnimalArray::mStaticVars + id * CAnimalArray::ElementSize); return;
			case SwitchIds::MapMoveableType::Vehicle: MapMoveable_DoOneGameTick_Vehicle(moveable, CVehicleArray::mStaticVars + id * CVehicleArray::ElementSize); return;
		}
	}

	// Original function: FUN_00439079; preserve its distinct native boundary.
	void MapMoveable_DoOneGameTick_Human(void* moveable, void* human)
	{
		Human_AI_DoUpdateConditions(human);
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0)
		{
			if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag) != 0 && !MapMoveable_Attachment_PerformTurn(moveable)) return;
			const auto id = Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId);
			if (id != -1)
			{
				auto* pathfinder = GetUnsafePathfinderPtr(id);
				if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0)
				{
					Write(moveable, NC2Layout::MapMoveableOffset::WalkingProgress, Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress) + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgressPerTick));
					if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag) == 0 && Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress) >= NC2Runtime::WalkingProgress::RelocationThreshold)
					{
						Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag, 1);
						MapMoveable_l_Attachment_DetachFromGameMap(moveable);
						auto point = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition);
						MapMigPoint_MoveCursorUnwrapped(point, Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingDirection));
						Write(moveable, NC2Layout::MapMoveableOffset::MapPosition, point);
						MapMoveable_l_Attachment_AttachToGameMap(moveable);
						const SSectorGridCursor sector(Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition));
						const auto previous = Read<SSectorGridCursor>(moveable, NC2Layout::MapMoveableOffset::SectorPosition);
						if (sector.x != previous.x || sector.y != previous.y)
						{
							MapMoveable_Attachment_DetachFromSectorMap(moveable);
							MapMoveable_Attachment_AttachToSectorMap(moveable);
						}
					}
					if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress) >= NC2Runtime::WalkingProgress::FullStep) Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag, 0);
					return;
				}
				auto direction = MapPathfinderElement_GetNextDirection(pathfinder);
				if (direction == NC2Runtime::PathfinderDirectionResult::Deferred) return;
				if (direction >= 0)
				{
					if (!Human_AI_DoNewMapPositionReached(human, false))
					{
						MapMoveable_Pathfinder_Stop(moveable);
						direction = -3;
					}
					if (direction != -3)
					{
						const auto turning = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningTicksPerDirection);
						const auto speed = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgressPerTick);
						Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag, 1);
						Write(moveable, NC2Layout::MapMoveableOffset::WalkingDirection, direction);
						Write(moveable, NC2Layout::MapMoveableOffset::WalkingProgress, speed);
						Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag, 0);
						if (turning == 0) Write(moveable, NC2Layout::MapMoveableOffset::Direction, direction);
						else if (direction != Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::Direction))
						{
							Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag, 1);
							Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningElapsedTicks, 0);
							Write(moveable, NC2Layout::MapMoveableOffset::TurnTargetDirection, direction);
						}
					}
				}
				else if (direction == NC2Runtime::PathfinderDirectionResult::Arrived) Human_AI_DoNewMapPositionReached(human, true);
				else if (direction == NC2Runtime::PathfinderDirectionResult::Failed) Human_AI_DoPathfinderFailed(human);
				else if (direction == NC2Runtime::PathfinderDirectionResult::Stopped) Human_AI_DoPathfinderStopped(human);
			}
		}
		Human_AI_DoUpdateAI(human);
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0) Tool_Map_CheckUserInformationAboutEnemies(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), static_cast<std::byte*>(moveable) + NC2Layout::MapMoveableOffset::MapPosition);
		GameAssistantSingleHumanData_CommandHistory_Validate(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::AssistantData);
	}

	// Original function: FUN_004391EB; preserve its distinct native boundary.
	void MapMoveable_DoOneGameTick_Animal(void* moveable, void* animal)
	{
		Animal_DoUpdateConditions(animal);
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0)
		{
			if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag) != 0 && !MapMoveable_Attachment_PerformTurn(moveable)) return;
			const auto id = Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId);
			if (id != -1)
			{
				auto* pathfinder = GetUnsafePathfinderPtr(id);
				if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0)
				{
					Write(moveable, NC2Layout::MapMoveableOffset::WalkingProgress, Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress) + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgressPerTick));
					if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag) == 0 && Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress) >= NC2Runtime::WalkingProgress::RelocationThreshold)
					{
						Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag, 1);
						MapMoveable_l_Attachment_DetachFromGameMap(moveable);
						auto point = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition);
						MapMigPoint_MoveCursorUnwrapped(point, Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingDirection));
						Write(moveable, NC2Layout::MapMoveableOffset::MapPosition, point);
						MapMoveable_l_Attachment_AttachToGameMap(moveable);
						const SSectorGridCursor sector(Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition));
						const auto previous = Read<SSectorGridCursor>(moveable, NC2Layout::MapMoveableOffset::SectorPosition);
						if (sector.x != previous.x || sector.y != previous.y)
						{
							MapMoveable_Attachment_DetachFromSectorMap(moveable);
							MapMoveable_Attachment_AttachToSectorMap(moveable);
						}
					}
					if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress) >= NC2Runtime::WalkingProgress::FullStep) Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag, 0);
					return;
				}
				auto direction = MapPathfinderElement_GetNextDirection(pathfinder);
				if (direction == NC2Runtime::PathfinderDirectionResult::Deferred) return;
				if (direction >= 0)
				{
					if (!Animal_DoNewMapPositionReached(animal, false))
					{
						MapMoveable_Pathfinder_Stop(moveable);
						direction = -3;
					}
					if (direction != -3)
					{
						const auto turning = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningTicksPerDirection);
						const auto speed = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgressPerTick);
						Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag, 1);
						Write(moveable, NC2Layout::MapMoveableOffset::WalkingDirection, direction);
						Write(moveable, NC2Layout::MapMoveableOffset::WalkingProgress, speed);
						Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag, 0);
						if (turning == 0) Write(moveable, NC2Layout::MapMoveableOffset::Direction, direction);
						else if (direction != Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::Direction))
						{
							Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag, 1);
							Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningElapsedTicks, 0);
							Write(moveable, NC2Layout::MapMoveableOffset::TurnTargetDirection, direction);
						}
					}
				}
				else if (direction == NC2Runtime::PathfinderDirectionResult::Arrived) Animal_DoNewMapPositionReached(animal, true);
				else if (direction == NC2Runtime::PathfinderDirectionResult::Failed) Animal_DoPathfinderFailed(animal);
				else if (direction == NC2Runtime::PathfinderDirectionResult::Stopped) Animal_DoPathfinderStopped(animal);
			}
		}
		Animal_DoUpdateAI(animal);
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0) Tool_Map_CheckUserInformationAboutEnemies(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId), static_cast<std::byte*>(moveable) + NC2Layout::MapMoveableOffset::MapPosition);
	}

	// Original function: FUN_00439352; preserve its distinct native boundary.
	void MapMoveable_DoOneGameTick_Vehicle(void* moveable, void* vehicle)
	{
		Vehicle_DoUpdateConditions(vehicle);
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0)
		{
			if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag) != 0 && !MapMoveable_Attachment_PerformTurn(moveable)) return;
			const auto id = Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId);
			if (id != -1)
			{
				auto* pathfinder = GetUnsafePathfinderPtr(id);
				if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0)
				{
					Write(moveable, NC2Layout::MapMoveableOffset::WalkingProgress, Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress) + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgressPerTick));
					if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag) == 0 && Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress) >= NC2Runtime::WalkingProgress::RelocationThreshold)
					{
						Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag, 1);
						MapMoveable_l_Attachment_DetachFromGameMap(moveable);
						auto point = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition);
						MapMigPoint_MoveCursorUnwrapped(point, Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingDirection));
						Write(moveable, NC2Layout::MapMoveableOffset::MapPosition, point);
						MapMoveable_l_Attachment_AttachToGameMap(moveable);
						const SSectorGridCursor sector(Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition));
						const auto previous = Read<SSectorGridCursor>(moveable, NC2Layout::MapMoveableOffset::SectorPosition);
						if (sector.x != previous.x || sector.y != previous.y)
						{
							MapMoveable_Attachment_DetachFromSectorMap(moveable);
							MapMoveable_Attachment_AttachToSectorMap(moveable);
						}
					}
					if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgress) >= NC2Runtime::WalkingProgress::FullStep) Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag, 0);
					return;
				}
				auto direction = MapPathfinderElement_GetNextDirection(pathfinder);
				if (direction == NC2Runtime::PathfinderDirectionResult::Deferred) return;
				if (direction >= 0)
				{
					if (!Vehicle_DoNewMapPositionReached(vehicle, false))
					{
						MapMoveable_Pathfinder_Stop(moveable);
						direction = -3;
					}
					if (direction != -3)
					{
						const auto turning = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningTicksPerDirection);
						const auto speed = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingProgressPerTick);
						Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag, 1);
						Write(moveable, NC2Layout::MapMoveableOffset::WalkingDirection, direction);
						Write(moveable, NC2Layout::MapMoveableOffset::WalkingProgress, speed);
						Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag, 0);
						if (turning == 0) Write(moveable, NC2Layout::MapMoveableOffset::Direction, direction);
						else if (direction != Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::Direction))
						{
							Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag, 1);
							Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningElapsedTicks, 0);
							Write(moveable, NC2Layout::MapMoveableOffset::TurnTargetDirection, direction);
						}
					}
				}
				else if (direction == NC2Runtime::PathfinderDirectionResult::Arrived) Vehicle_DoNewMapPositionReached(vehicle, true);
				else if (direction == NC2Runtime::PathfinderDirectionResult::Failed) Vehicle_DoPathfinderFailed(vehicle);
			}
		}
		Vehicle_DoUpdateAI(vehicle);
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0) Tool_Map_CheckUserInformationAboutEnemies(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId), static_cast<std::byte*>(moveable) + NC2Layout::MapMoveableOffset::MapPosition);
	}

	// Original function: FUN_0041cef8.
	std::int32_t MapPathfinderManager_AllocatePathfinder(CArrayHandler* manager)
	{
		const auto id = manager->Element_Allocate();
		if (id != -1) MapPathfinderElement_Init(GetUnsafePathfinderPtr(id));
		return id;
	}

	// Original function: FUN_0041cf1e. Windows shares the one-byte exit body with FUN_00430ca4.
	void MapPathfinderManager_FreePathfinder(CArrayHandler* manager, std::int32_t id)
	{
		void* pathfinder = GetUnsafePathfinderPtr(id);
		if (Read<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::AllocatedFlag) != 0 && manager->Element_Free(id, true)) Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::AllocatedFlag, 0);
	}

	// Original function: FUN_00438e09.
	bool MapMoveable_Init(void* moveable, std::int32_t id, std::uint32_t type, std::int32_t objectId)
	{
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag) != 0) return false;
		std::memset(moveable, 0, NC2Layout::RecordSize::MapMoveableSigned);
		Write<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::NextCellMoveableId, -1);
		Write<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::MoveableId, id);
		Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag, 1);
		Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType, type);
		Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::Direction, 2);
		const auto pathfinderId = MapPathfinderManager_AllocatePathfinder(static_cast<CArrayHandler*>(gMapPathfinderManager));
		Write<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId, pathfinderId);
		if (pathfinderId == -1)
		{
			MapMoveable_l_Misc_FreeAllAllocatedData(moveable);
			return false;
		}
		if (type >= 1u && type <= 4u) Write<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId, objectId);
		return true;
	}

	// Original function: FUN_00438e7d.
	void MapMoveable_Exit(void* moveable)
	{
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag) == 0) return;
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0) MapMoveable_Attachment_DetachFromMap(moveable);
		MapMoveable_l_Misc_FreeAllAllocatedData(moveable);
		Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag, 0);
	}

	// Original function: FUN_00438e9c.
	void MapMoveable_l_Misc_FreeAllAllocatedData(void* moveable)
	{
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag) == 0) return;
		const auto id = Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId);
		if (id == -1) return;
		MapPathfinderManager_FreePathfinder(static_cast<CArrayHandler*>(gMapPathfinderManager), id);
		Write<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId, -1);
	}

	// Original function: FUN_00438ebe. The source point is copied after detachment.
	void MapMoveable_Attachment_AttachToMap(void* moveable, const void* packedMigPoint)
	{
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0) MapMoveable_Attachment_DetachFromMap(moveable);
		const auto point = Read<std::uint32_t>(packedMigPoint, NC2Layout::MapCoordinatePairOffset::Coordinates);
		Write<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::NextCellMoveableId, -1);
		Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition, point);
		Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag, 1);
		MapMoveable_l_Attachment_AttachToGameMap(moveable);
		MapMoveable_Attachment_AttachToSectorMap(moveable);
	}

	// Original function: FUN_00438eef.
	void MapMoveable_Attachment_DetachFromMap(void* moveable)
	{
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0) return;
		MapMoveable_l_Attachment_DetachFromGameMap(moveable);
		MapMoveable_Attachment_DetachFromSectorMap(moveable);
		Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag, 0);
	}

	// Original function: FUN_00438F6A. An unrelocated walking object predicts its next point.
	void MapMoveable_Attachment_GetNextMapPosition(const void* moveable, void* point)
	{
		auto packed = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition);
		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, packed);
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0 && Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingPositionAdvancedFlag) == 0)
		{
			MapMigPoint_MoveCursorUnwrapped(packed, Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::WalkingDirection));
			Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, packed);
		}
	}

	// Original function: FUN_00438F8C.
	bool MapMoveable_Pathfinder_Start(void* moveable, const void* destination, std::uint32_t radius, bool useResistance, bool useSectors, bool useGuides)
	{
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0) return false;
		MapPathfinderElement_StartPathfinder(GetUnsafePathfinderPtr(Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId)), static_cast<std::byte*>(moveable) + NC2Layout::MapMoveableOffset::MapPosition, destination, radius, useResistance, useSectors, useGuides, false);
		return true;
	}

	// Original function: FUN_00438FC4.
	bool MapMoveable_Pathfinder_Start(void* moveable, std::int32_t targetId, std::int32_t distance, std::uint32_t radius, bool useResistance, bool useSectors, bool useGuides)
	{
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0) return false;
		MapPathfinderElement_StartPathfinder(GetUnsafePathfinderPtr(Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId)), static_cast<std::byte*>(moveable) + NC2Layout::MapMoveableOffset::MapPosition, targetId, distance, radius, useResistance, useSectors, useGuides, false);
		return true;
	}

	// Original function: FUN_00438FFF.
	bool MapMoveable_Pathfinder_Stop(void* moveable)
	{
		const auto id = Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId);
		if (id == -1 || Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0) return false;
		MapPathfinderElement_StopPathfinder(GetUnsafePathfinderPtr(id));
		return true;
	}

	// Original function: FUN_0043961A. Revisions belong to the creature/marker, not its map slot.
	std::uint32_t MapMoveable_GetUniqueId(const void* moveable)
	{
		const auto id = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId);
		switch (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType))
		{
			case SwitchIds::MapMoveableType::Human: return Read<std::uint32_t>(CHumanArray::mStaticVars + id * CHumanArray::ElementSize, NC2Layout::HumanOffset::UniqueId);
			case SwitchIds::MapMoveableType::Animal: return Read<std::uint32_t>(CAnimalArray::mStaticVars + id * CAnimalArray::ElementSize, NC2Layout::AnimalOffset::UniqueId);
			case SwitchIds::MapMoveableType::Vehicle: return Read<std::uint32_t>(CVehicleArray::mStaticVars + id * CVehicleArray::ElementSize, NC2Layout::VehicleOffset::UniqueId);
			case SwitchIds::MapMoveableType::Marker: return Read<std::uint32_t>(static_cast<const std::byte*>(gMarkerEntries) + id * 0x18u, NC2Layout::MarkerOffset::UniqueId);
			default: return NXBasics::IntegerRange::UInt32Maximum;
		}
	}

	// Original function: FUN_00439021.
	const void* MapMoveable_Pathfinder_GetPathfinderPtr(const void* moveable)
	{
		return GetUnsafePathfinderPtr(Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId));
	}

	// Original function: FUN_004394ad. The micro-point stores a signed 16-bit list head.
	void MapMoveable_l_Attachment_AttachToGameMap(void* moveable)
	{
		void* point = GameMap_GetMapMicroPointPtr(static_cast<const std::byte*>(moveable) + NC2Layout::MapMoveableOffset::MapPosition);
		const auto id = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MoveableId);
		const auto previous = Read<std::int16_t>(point, NC2Layout::MapMicroPointOffset::FirstMapMoveableId);
		Write<std::uint16_t>(point, NC2Layout::MapMicroPointOffset::FirstMapMoveableId, static_cast<std::uint16_t>(id));
		Write<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::NextCellMoveableId, previous);
	}

	// Original function: FUN_004394ca. An absent micro-list entry is ignored.
	void MapMoveable_l_Attachment_DetachFromGameMap(void* moveable)
	{
		void* point = GameMap_GetMapMicroPointPtr(static_cast<const std::byte*>(moveable) + NC2Layout::MapMoveableOffset::MapPosition);
		auto current = static_cast<std::int32_t>(Read<std::int16_t>(point, NC2Layout::MapMicroPointOffset::FirstMapMoveableId));
		if (current == -1) return;
		const auto id = Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::MoveableId);
		if (current == id)
		{
			Write<std::uint16_t>(point, NC2Layout::MapMicroPointOffset::FirstMapMoveableId, static_cast<std::uint16_t>(Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::NextCellMoveableId)));
			return;
		}
		while (current != -1)
		{
			void* previous = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(current) * NC2Layout::RecordSize::MapMoveable;
			current = Read<std::int32_t>(previous, NC2Layout::MapMoveableOffset::NextCellMoveableId);
			if (current == id)
			{
				Write<std::int32_t>(previous, NC2Layout::MapMoveableOffset::NextCellMoveableId, Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::NextCellMoveableId));
				return;
			}
		}
	}

	// Original function: FUN_00439516. Type 4 has no sector-list attachment.
	void MapMoveable_Attachment_AttachToSectorMap(void* moveable)
	{
		const auto type = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType);
		if (type < 1u || type > 3u) return;
		const SSectorGridCursor sector(Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition));
		Write<SSectorGridCursor>(moveable, NC2Layout::MapMoveableOffset::SectorPosition, sector);
		const auto id = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId);
		if (type == 1) Human_Sector_AttachHumanToSector(CHumanArray::mStaticVars + id * CHumanArray::ElementSize, sector);
		else if (type == 2) Animal_Sector_AttachAnimalToSector(CAnimalArray::mStaticVars + id * CAnimalArray::ElementSize, sector);
		else Vehicle_Sector_AttachVehicleToSector(CVehicleArray::mStaticVars + id * CVehicleArray::ElementSize, sector);
	}

	// Original function: FUN_004395c3. Detachment uses the stored sector coordinates.
	void MapMoveable_Attachment_DetachFromSectorMap(void* moveable)
	{
		const auto type = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType);
		if (type < 1u || type > 3u) return;
		const auto sector = Read<SSectorGridCursor>(moveable, NC2Layout::MapMoveableOffset::SectorPosition);
		const auto id = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId);
		if (type == 1) Human_Sector_DetachHumanFromSector(CHumanArray::mStaticVars + id * CHumanArray::ElementSize, sector);
		else if (type == 2) Animal_Sector_DetachAnimalFromSector(CAnimalArray::mStaticVars + id * CAnimalArray::ElementSize, sector);
		else Vehicle_Sector_DetachVehicleFromSector(CVehicleArray::mStaticVars + id * CVehicleArray::ElementSize, sector);
	}

	// Original function: FUN_0044186d. No ID or allocation check is performed.
	void* GetUnsafePathfinderPtr(std::int32_t id)
	{
		return static_cast<std::byte*>(gMapPathfinderEntries) + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapPathfinder;
	}

	// Original function: FUN_00447804.
	void MapPathfinderElement_Init(void* pathfinder)
	{
		std::memset(pathfinder, 0, NC2Layout::RecordSize::MapPathfinderSigned);
		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::RequiredClearance, 0);
		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchState, NC2Runtime::PathfinderSearchState::Stopped);
		Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::PlayerId, -1);
		Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SourceGuideId, -1);
		Write<std::int32_t>(pathfinder, NC2Layout::MapPathfinderOffset::CurrentGuideId, -1);
		Write<std::uint8_t>(pathfinder, NC2Layout::MapPathfinderOffset::AllocatedFlag, 1);
		Write<std::uint32_t>(pathfinder, NC2Layout::MapPathfinderOffset::SearchRadius, NC2Runtime::PathfinderSearchLimits::InitialRadius);
	}

	// Original function: FUN_0042ea28. Animals update the count before linking neighbours.
	void Animal_Sector_AttachAnimalToSector(void* animal, const SSectorGridCursor& sector)
	{
		auto* data = GetSector(sector);
		const bool special = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 0x31 && Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AggressiveFlag) != 0;
		const auto offset = (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId) << 6) + (special ? 0x38u : 0x30u);
		const auto previous = Read<std::int32_t>(data, offset);
		const auto id = Read<std::int32_t>(animal, NC2Layout::AnimalOffset::AnimalId);
		Write<std::int32_t>(data, offset, id);
		Write<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count, Read<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count) + 1u);
		Write<std::int32_t>(animal, NC2Layout::AnimalOffset::NextSectorAnimalId, previous);
		if (previous != -1) Write<std::int32_t>(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(previous) * CAnimalArray::ElementSize, NC2Layout::AnimalOffset::PreviousSectorAnimalId, id);
		Write<std::int32_t>(animal, NC2Layout::AnimalOffset::PreviousSectorAnimalId, -1);
	}

	// Original function: FUN_0042eac4. Windows does not repair the next node's previous ID on non-head removal.
	void Animal_Sector_DetachAnimalFromSector(void* animal, const SSectorGridCursor& sector)
	{
		auto* data = GetSector(sector);
		const bool special = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 0x31 && Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AggressiveFlag) != 0;
		const auto offset = (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId) << 6) + (special ? 0x38u : 0x30u);
		auto current = Read<std::int32_t>(data, offset);
		Write<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count, Read<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count) - 1u);
		const auto id = Read<std::int32_t>(animal, NC2Layout::AnimalOffset::AnimalId);
		const auto next = Read<std::int32_t>(animal, NC2Layout::AnimalOffset::NextSectorAnimalId);
		if (current == id)
		{
			Write<std::int32_t>(data, offset, next);
			if (next != -1) Write<std::int32_t>(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(next) * CAnimalArray::ElementSize, NC2Layout::AnimalOffset::PreviousSectorAnimalId, -1);
			return;
		}
		for (;;)
		{
			void* previous = CAnimalArray::mStaticVars + static_cast<std::uint32_t>(current) * CAnimalArray::ElementSize;
			current = Read<std::int32_t>(previous, NC2Layout::AnimalOffset::NextSectorAnimalId);
			if (current == id)
			{
				Write<std::int32_t>(previous, NC2Layout::AnimalOffset::NextSectorAnimalId, next);
				return;
			}
		}
	}

	// Original function: FUN_004375b9.
	void Vehicle_Sector_AttachVehicleToSector(void* vehicle, const SSectorGridCursor& sector)
	{
		auto* data = GetSector(sector);
		const auto offset = (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId) << 6) + (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::JobType) == 0x36 ? 0x20u : 8u);
		const auto previous = Read<std::int32_t>(data, offset);
		const auto id = Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId);
		Write<std::int32_t>(data, offset, id);
		Write<std::int32_t>(vehicle, NC2Layout::VehicleOffset::NextSectorVehicleId, previous);
		if (previous != -1) Write<std::int32_t>(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(previous) * CVehicleArray::ElementSize, NC2Layout::VehicleOffset::PreviousSectorVehicleId, id);
		Write<std::int32_t>(vehicle, NC2Layout::VehicleOffset::PreviousSectorVehicleId, -1);
		Write<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count, Read<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count) + 1u);
	}

	// Original function: FUN_00437689. The original traversal assumes membership in this list.
	void Vehicle_Sector_DetachVehicleFromSector(void* vehicle, const SSectorGridCursor& sector)
	{
		auto* data = GetSector(sector);
		const auto offset = (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId) << 6) + (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::JobType) == 0x36 ? 0x20u : 8u);
		auto current = Read<std::int32_t>(data, offset);
		const auto id = Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId);
		const auto next = Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::NextSectorVehicleId);
		if (current == id)
		{
			Write<std::int32_t>(data, offset, next);
			if (next != -1) Write<std::int32_t>(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(next) * CVehicleArray::ElementSize, NC2Layout::VehicleOffset::PreviousSectorVehicleId, -1);
		}
		else
		{
			for (;;)
			{
				void* previous = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(current) * CVehicleArray::ElementSize;
				current = Read<std::int32_t>(previous, NC2Layout::VehicleOffset::NextSectorVehicleId);
				if (current == id)
				{
					Write<std::int32_t>(previous, NC2Layout::VehicleOffset::NextSectorVehicleId, next);
					break;
				}
			}
		}
		Write<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count, Read<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count) - 1u);
	}

	// Original function: FUN_00441ac3. Soldiers and heroes share the military list.
	void Human_Sector_AttachHumanToSector(void* human, const SSectorGridCursor& sector)
	{
		auto* data = GetSector(sector);
		const auto offset = (Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) << 6) + (Human_IsJobSoldier(human) || Human_IsJobHero(human) ? 0x18u : 0u);
		const auto previous = Read<std::int32_t>(data, offset);
		const auto id = Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId);
		Write<std::int32_t>(data, offset, id);
		Write<std::int32_t>(human, NC2Layout::HumanOffset::NextSectorHumanId, previous);
		if (previous != -1) Write<std::int32_t>(CHumanArray::mStaticVars + static_cast<std::uint32_t>(previous) * CHumanArray::ElementSize, NC2Layout::HumanOffset::PreviousSectorHumanId, id);
		Write<std::int32_t>(human, NC2Layout::HumanOffset::PreviousSectorHumanId, -1);
		Write<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count, Read<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count) + 1u);
	}

	// Original function: FUN_00441b9a. Non-head removal only updates the predecessor's next ID.
	void Human_Sector_DetachHumanFromSector(void* human, const SSectorGridCursor& sector)
	{
		auto* data = GetSector(sector);
		const auto offset = (Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) << 6) + (Human_IsJobSoldier(human) || Human_IsJobHero(human) ? 0x18u : 0u);
		auto current = Read<std::int32_t>(data, offset);
		const auto id = Read<std::int32_t>(human, NC2Layout::HumanOffset::HumanId);
		const auto next = Read<std::int32_t>(human, NC2Layout::HumanOffset::NextSectorHumanId);
		if (current == id)
		{
			Write<std::int32_t>(data, offset, next);
			if (next != -1) Write<std::int32_t>(CHumanArray::mStaticVars + static_cast<std::uint32_t>(next) * CHumanArray::ElementSize, NC2Layout::HumanOffset::PreviousSectorHumanId, -1);
		}
		else
		{
			for (;;)
			{
				void* previous = CHumanArray::mStaticVars + static_cast<std::uint32_t>(current) * CHumanArray::ElementSize;
				current = Read<std::int32_t>(previous, NC2Layout::HumanOffset::NextSectorHumanId);
				if (current == id)
				{
					Write<std::int32_t>(previous, NC2Layout::HumanOffset::NextSectorHumanId, next);
					break;
				}
			}
		}
		Write<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count, Read<std::uint32_t>(data, offset + NC2Layout::SectorObjectListOffset::Count) - 1u);
	}
	// Original function: FUN_00438f0a.
	void MapMoveable_Attachment_StartTurn(void* moveable, std::int32_t direction)
	{
		if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningTicksPerDirection) == 0) Write(moveable, NC2Layout::MapMoveableOffset::Direction, direction);
		else if (Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::Direction) != direction)
		{
			Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningElapsedTicks, 0);
			Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag, 1);
			Write(moveable, NC2Layout::MapMoveableOffset::TurnTargetDirection, direction);
		}
	}

	// Original function: FUN_00438f2c; Mac Attachment_PerformTurn.
	bool MapMoveable_Attachment_PerformTurn(void* moveable)
	{
		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag) == 0) return true;
		const auto elapsed = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningElapsedTicks) + 1u;
		Write(moveable, NC2Layout::MapMoveableOffset::TurningElapsedTicks, elapsed);
		if (NXBasics::Runtime_TimeSigned32(elapsed) < Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::TurningTicksPerDirection)) return false;
		Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningElapsedTicks, 0);
		const auto direction = VE_HexagonDirection_NextDirection(Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::Direction), Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurnTargetDirection), Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::IncludeVerticalDirectionsFlag) != 0);
		Write(moveable, NC2Layout::MapMoveableOffset::Direction, direction);
		if (direction != Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurnTargetDirection)) return false;
		Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag, 0);
		return true;
	}
}
