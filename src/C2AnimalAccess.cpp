#include "C2AnimalAccess.h"
#include "C2AnimalIterator.h"
#include "GameProfile.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanInitialization.h"
#include "C2HumanWalking.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapSectorAttachment.h"
#include "C2MapSectorGridCursor.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2StaticAtomicAnimationData.h"
#include "C2StaticTribeTypeData.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		void* Animal(std::int32_t id)
		{
			return CAnimalArray::mStaticVars + static_cast<std::intptr_t>(id) * CAnimalArray::ElementSize;
		}
		void* Moveable(const void* animal)
		{
			return gMapMoveableArrayStorage.entries + Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
		}
		const SStaticAnimalTypeData& Type(const void* animal)
		{
			return static_cast<const SStaticAnimalTypeData*>(gStaticAnimalTypeData)[Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType) - NRemaster::GetFirstAnimalTribe()];
		}
	}

	// Original function: FUN_0044710b; neutral animals bypass diplomacy, owned animals do so only on selection.
	bool Animal_Misc_CanBeAttackedByHuman(const void* animal, std::int32_t humanId, bool allowOwnAnimals)
	{
		if (Type(animal).cannotBeAttacked != 0) return false;
		const auto player = Read<std::int32_t>(animal, NC2Layout::AnimalOffset::PlayerId);
		if (player == 20) return true;
		const auto humanPlayer = Read<std::int32_t>(CHumanArray::mStaticVars + static_cast<std::uint32_t>(humanId) * CHumanArray::ElementSize, NC2Layout::HumanOffset::PlayerId);
		if (player == humanPlayer && allowOwnAnimals) return true;
		return Logic_EntryPoint_GetDiplomacyState(humanPlayer, player) == 3;
	}

	// Original function: FUN_0040ad95; the manager owns allocation and callbacks.
	std::int32_t Logic_EntryPoint_Add_Animal(std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags)
	{
		return CAnimalArray::sTheObjectPtr->AllocateAnimal(player, tribe, job, point, missionCheck, flags);
	}

	// Original function: FUN_0041c2a9; only non-neutral requests evict a neutral animal.
	std::int32_t CAnimalArray::AllocateAnimal(std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags)
	{
		if (!MapMigPoint_IsInsideMap(point) || MapMigPoint_IsBorderPoint(point)) return -1;
		auto id = Element_Allocate();
		if (id == -1)
		{
			if (player == 20) return -1;
			CAnimalIterator iterator;
			const auto neutral = iterator.GetFirstAnimalId(20);
			if (neutral == -1) return -1;
			FreeAnimal(neutral);
			id = Element_Allocate();
		}
		if (!Animal_Init(Animal(id), id, player, tribe, job, point, missionCheck, flags))
		{
			FreeAnimal(id); return -1;
		}
		CCallbackManager::sTheObjectPtr->ExecuteCallback(6, true, id, 1, 0);
		return id;
	}

	// Original function: FUN_0041c358; notify before exiting the object.
	void CAnimalArray::FreeAnimal(std::int32_t id)
	{
		auto* animal = Animal(id);
		if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag) == 0 || !Element_Free(id, true)) return;
		if (Read<std::int32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId) != -1) CCallbackManager::sTheObjectPtr->ExecuteCallback(6, false, id, 0, 0);
		Animal_Exit(animal);
	}

	// Original function: FUN_0042dce9; retain random-call and initial-atomic order.
	bool Animal_Init(void* animal, std::int32_t id, std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags)
	{
		if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag) != 0) return false;
		std::memset(animal, 0, CAnimalArray::ElementSize);
		Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag, 1);
		Write(animal, NC2Layout::AnimalOffset::AnimalId, id);
		Write(animal, NC2Layout::AnimalOffset::UniqueId, Game_UniqueId_Generate());
		Write<std::int32_t>(animal, NC2Layout::AnimalOffset::LeaderId, -1);
		Write(animal, NC2Layout::AnimalOffset::PlayerId, player);
		Write(animal, NC2Layout::AnimalOffset::TribeType, tribe);
		Write(animal, NC2Layout::AnimalOffset::AnimalType, job);
		const auto moveableId = MapMoveableArray_AllocateMapMoveable(gMapMoveableArrayStorage.manager, 2, id);
		Write(animal, NC2Layout::AnimalOffset::MapMoveableId, moveableId);
		if (moveableId == -1) return false;
		auto* moveable = Moveable(animal);
		MapMoveable_Attachment_AttachToMap(moveable, point);
		const auto start = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition);
		Write(animal, NC2Layout::AnimalOffset::BirthPoint, start);
		Write(animal, NC2Layout::AnimalOffset::StayPoint, start);
		Write(animal, NC2Layout::AnimalOffset::WorkCenter, start);
		Write(animal, NC2Layout::AnimalOffset::Hitpoints, Animal_HitPoints_GetMaximum(animal));
		Write(animal, NC2Layout::AnimalOffset::Food, Game_Random_GetNumber() % 400u);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::AgeCounter, 0);
		auto* path = GetUnsafePathfinderPtr(Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId));
		const auto speed = Type(animal).moveSpeed;
		Write(moveable, NC2Layout::MapMoveableOffset::WalkingTicksPerStep, speed);
		Write(moveable, NC2Layout::MapMoveableOffset::WalkingProgressPerTick, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(speed) + 9999u) / speed);
		Write(path, NC2Layout::MapPathfinderOffset::PlayerId, player);
		Write<std::int32_t>(path, NC2Layout::MapPathfinderOffset::CurrentGuideId, -1);
		Write<std::int32_t>(path, NC2Layout::MapPathfinderOffset::SourceGuideId, -1);
		Animal_l_StartAtomicAnimation(animal, 2, -1);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 1);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 1);
		Write<std::int32_t>(animal, NC2Layout::AnimalOffset::LeaderId, -1);
		Write(animal, NC2Layout::AnimalOffset::MissionCheckId, missionCheck);
		// This value is saved and checksummed; a countdown use is not established.
		const auto initialRandomValue = Game_Random_GetNumber() % 3u;
		Write(animal, NC2Layout::AnimalOffset::MiscFlags, flags);
		Write(animal, 0x30, initialRandomValue);
		Animal_l_Aggressive_Check(animal);
		return true;
	}

	// Original function: FUN_0042de20; preserve cadaver editing and callback ownership.
	void Animal_Exit(void* animal)
	{
		if (Animal_l_IsLeaderValid(animal)) Animal_l_DetachLeader(animal);
		if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag) == 0) return;
		const auto moveableId = Read<std::int32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId);
		auto size = static_cast<std::uint32_t>(Type(animal).maximumCadaverSize);
		if (Read<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDeathStatisticsFlag) == 0 && moveableId != -1 && size != 0 && Read<std::uint8_t>(Moveable(animal), NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0)
		{
			const auto* point = Animal_GetMapPositionRef(animal);
			if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::EnvironmentalFoodValue) == NC2Runtime::AnimalFoodValue::SlaughteredBits)
			{
				const auto engine = NC2LogicExternals::External_Misc_GetEngineCadaverLandscapeId(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType), 79);
				if (engine != -1)
				{
					Logic_EntryPoint_Edit_Start();
					if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 48) size >>= 1;
					Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(point, 79, size, engine, 0, -1);
					Logic_EntryPoint_Edit_Stop();
				}
			}
			else
			{
				size /= 3u;
				if (size == 0) size = 1;
				Logic_EntryPoint_Edit_Start();
				if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 48) size >>= 1;
				const auto engine = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(44, true);
				Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(point, 44, size, engine, 0, -1);
				Logic_EntryPoint_Edit_Stop();
			}
		}
		if (moveableId != -1) MapMoveableArray_FreeMapMoveable(gMapMoveableArrayStorage.manager, moveableId);
		Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::AllocatedFlag, 0);
	}

	// Original function: FUN_0042e82c; unsupported atomic types return false.
	bool Animal_l_StartAtomicAnimation(void* animal, std::uint32_t action, std::int32_t direction)
	{
		if (action == 1 || !Animal_l_IsAtomicInterruptable(animal)) return false;
		auto job = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType);
		std::uint32_t animation;
		for (;;)
		{
			const auto* data = static_cast<const std::byte*>(gStaticJobTypeData) + job * NC2Layout::RecordSize::JobType;
			if (Read<std::uint8_t>(data, NC2Layout::StaticJobTypeOffset::AllowedAtomicsUnsigned + action) == 0) return false;
			animation = gStaticTribeTypeData[Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType)].atomicIds[job][action];
			if (animation != NXBasics::IntegerRange::UInt32Maximum) break;
			job = Read<std::uint32_t>(data, NC2Layout::StaticJobTypeOffset::BaseJob);
			if (job == 0) return false;
		}
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationFrame, 0);
		Write(animal, NC2Layout::AnimalOffset::AtomicAnimationStartCount, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationStartCount) + 1u);
		Write(animal, NC2Layout::AnimalOffset::AtomicAnimationId, animation);
		Write(animal, NC2Layout::AnimalOffset::AtomicAction, action);
		Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationRunningFlag, 1);
		const auto preferred = gStaticAtomicAnimationData[animation].startDirection;
		if (preferred != -1) MapMoveable_Attachment_StartTurn(Moveable(animal), preferred);
		else if (direction != -1) MapMoveable_Attachment_StartTurn(Moveable(animal), direction);
		return true;
	}

	// Original function: FUN_0042e9ae; no active animation is interruptible.
	bool Animal_l_IsAtomicInterruptable(const void* animal)
	{
		if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAction) == 1) return Read<std::uint8_t>(Moveable(animal), NC2Layout::MapMoveableOffset::WalkingFlag) == 0;
		return Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationRunningFlag) == 0 || gStaticAtomicAnimationData[Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationId)].interruptable != 0;
	}

	// Original function: FUN_0042ebc1; leader attachment types must agree.
	bool Animal_l_IsLeaderValid(const void* animal)
	{
		const auto id = Read<std::int32_t>(animal, NC2Layout::AnimalOffset::LeaderId);
		if (id == -1) return false;
		const auto* leader = Animal(id);
		if (Read<std::uint8_t>(leader, NC2Layout::AnimalOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::UniqueId) != Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderRevision) || Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::PlayerId) != Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId) || Animal_IsAttachedToVehicle(leader)) return false;
		if (!Animal_IsAttachedToHouse(animal)) return !Animal_IsAttachedToHouse(leader);
		return Animal_IsAttachedToHouse(leader) && TargetData_GetUniqueId(static_cast<const std::byte*>(leader) + NC2Layout::AnimalOffset::AttachmentTarget) == TargetData_GetUniqueId(static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::AttachmentTarget);
	}

	// Original function: FUN_0042f2ad.
	bool Animal_IsAttachedToHouse(const void* animal)
	{
		const auto* target = static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::AttachmentTarget; return TargetData_GetTargetType(target) == 4 && TargetData_IsValid(target);
	}
	// Original function: FUN_0042f2d1.
	bool Animal_IsAttachedToVehicle(const void* animal)
	{
		const auto* target = static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::AttachmentTarget; return TargetData_GetTargetType(target) == 2 && TargetData_IsValid(target);
	}

	// Original function: FUN_0042f2f5; even an unallocated house triggers center update.
	void Animal_AttachToHouse(void* animal, std::int32_t houseId)
	{
		const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(houseId) * CHouseArray::ElementSize;
		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0)
		{
			Write(animal, NC2Layout::AnimalOffset::AttachedObjectId, houseId);
			Write(animal, NC2Layout::AnimalOffset::AttachedObjectRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::AttachmentTarget, 4);
			Animal_l_DetachLeader(animal);
		}
		Animal_l_MoveWorkCenterPoint(animal);
	}

	// Original function: FUN_0042f331; a walking animal keeps its task and reports failure after attachment.
	bool Animal_AttachToVehicle(void* animal, std::int32_t vehicleId)
	{
		const auto* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(vehicleId) * CVehicleArray::ElementSize;
		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) != 0)
		{
			Write(animal, NC2Layout::AnimalOffset::AttachedObjectId, vehicleId);
			Write(animal, NC2Layout::AnimalOffset::AttachedObjectRevision, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::AttachmentTarget, 2);
		}
		if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAction) == 1) return false;
		if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask) != 9)
		{
			Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0); Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 9);
		}
		return true;
	}

	// Original function: FUN_0042f376.
	void Animal_l_MoveWorkCenterPoint(void* animal)
	{
		if (Animal_IsAttachedToVehicle(animal)) return;
		if (Animal_IsAttachedToHouse(animal))
		{
			const auto id = TargetData_GetObjectId(static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::AttachmentTarget);
			Write(animal, NC2Layout::AnimalOffset::WorkCenter, Read<std::uint32_t>(CHouseArray::mStaticVars + id * CHouseArray::ElementSize, NC2Layout::HouseOffset::DoorEntryPoint));
			return;
		}
		const auto player = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId);
		if (player != 20)
		{
			const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Animal_GetMapPositionRef(animal)), NC2Layout::MapMicroPointOffset::ContinentId);
			CHouseIterator iterator;
			for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId())
			{
				const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
				if (House_GetHouseMainType(house) != 1 || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0 || Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition), NC2Layout::MapMicroPointOffset::ContinentId) != continent) continue;
				Write(animal, NC2Layout::AnimalOffset::AttachedObjectId, id);
				Write(animal, NC2Layout::AnimalOffset::AttachedObjectRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
				Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::AttachmentTarget, 4);
				Animal_l_DetachLeader(animal);
				return;
			}
		}
		if (Type(animal).ignoreHouses != 0) return;
		const auto* manager = CGameMapSectorAttachmentManager::sTheObjectPtr;
		const auto occupied = [manager, player] (const SSectorGridCursor& cursor, bool stopAtFirst) -> std::uint32_t
		{
			const auto index = static_cast<std::uint32_t>(cursor.y) * static_cast<std::uint32_t>(manager->GetGridWidth()) + static_cast<std::uint32_t>(cursor.x);
			const auto* sector = manager->GetSectorData() + static_cast<std::size_t>(index) * 0x540u;
			std::uint32_t total = 0;
			for (std::uint32_t other = 0; other < 20; ++other)
			{
				if (other == player || Read<std::uint8_t>(gGameControl, other * NC2Layout::RecordSize::GameControlPlayer) == 0) continue;
				const auto first = Read<std::uint32_t>(sector, other * NC2Layout::RecordSize::SectorPlayerUnsigned + NC2Layout::SectorPlayerOffset::CivilianHouseCount), second = Read<std::uint32_t>(sector, other * NC2Layout::RecordSize::SectorPlayerUnsigned + NC2Layout::SectorPlayerOffset::MilitaryHouseCount);
				if (stopAtFirst && (first != 0 || second != 0)) return 1;
				total += first + second;
			}
			return total;
		};
		const SSectorGridCursor center(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::WorkCenter));
		if (occupied(center, true) == 0) return;
		const SSectorGridCursor active(Read<std::uint32_t>(Animal_GetMapPositionRef(animal), NC2Layout::MapCoordinatePairOffset::Coordinates));
		auto best = active;
		auto minimum = occupied(active, false);
		for (std::uint32_t direction = 0; direction < 8; ++direction)
		{
			auto candidate = center;
			candidate.MoveCursorUnwrapped(direction);
			if (!candidate.IsInsideMap()) continue;
			const auto count = occupied(candidate, false);
			if (count < minimum)
			{
				best = candidate; minimum = count;
			}
		}
		Write(animal, NC2Layout::AnimalOffset::WorkCenter, static_cast<std::uint32_t>(static_cast<std::uint16_t>(best.x * 20)) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(best.y * 20)) << 16));
		auto* point = static_cast<std::byte*>(animal) + NC2Layout::AnimalOffset::WorkCenter;
		if (MapMigPoint_IsBorderPoint(point)) MapMigPoint_ConstructOffset(point, point, 10, 10, false);
		Tool_Map_GetNotBlockedPosition(point, true);
	}

	// Original function: FUN_00461521.
	void Animal_GoIntoHouseInteractionMode(void* animal)
	{
		if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask) == 10) return;
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::CurrentTask, 10);
	}

	// Original function: FUN_004471ca; aggression changes sector-list membership.
	void Animal_l_Aggressive_Check(void* animal)
	{
		if (Type(animal).aggressive != 0 && Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 49)
		{
			if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AggressiveFlag) == 0)
			{
				MapMoveable_Attachment_DetachFromSectorMap(Moveable(animal));
				Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::AggressiveFlag, 1);
				MapMoveable_Attachment_AttachToSectorMap(Moveable(animal));
			}
		}
		else if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AggressionClearDelay) != 0) Write(animal, NC2Layout::AnimalOffset::AggressionClearDelay, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AggressionClearDelay) - 1u);
		else if (Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AggressiveFlag) != 0)
		{
			MapMoveable_Attachment_DetachFromSectorMap(Moveable(animal));
			Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::AggressiveFlag, 0);
			MapMoveable_Attachment_AttachToSectorMap(Moveable(animal));
		}
	}

	// Original function: FUN_0042e9e8.
	std::uint32_t Animal_GetCurrentAtomicAnimationLengthInTicks(const void* animal)
	{
		return NC2Lifetime::Read<std::uint32_t>(gStaticAtomicAnimationData, NC2Lifetime::Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AtomicAnimationId) * NC2Layout::RecordSize::AtomicAnimation + NC2Layout::AtomicAnimationOffset::LengthInTicks);
	}
	// Original function: FUN_0042f6a2; forced house exit only clears the hidden byte.
	void Animal_HouseMisc_Exit(void* animal)
	{
		Write<std::uint8_t>(animal, NC2Layout::AnimalOffset::InsideHouseFlag, 0);
	}

	// Original function: FUN_0042f6aa; animal type IDs index the native biased table.
	std::uint32_t Animal_HitPoints_GetMaximum(const void* animal)
	{
		const auto offset = (NC2Lifetime::Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType) - NRemaster::GetFirstAnimalTribe()) * 0x40u + (NC2Lifetime::Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 48 ? 0x34u : 0x38u);
		return NC2Lifetime::Read<std::uint32_t>(gStaticAnimalTypeData, offset);
	}
	// Original function: FUN_0042e9fc; the native helper copies the position without allocation or attachment checks.
	bool Animal_GetMapPosition(const void* animal, void* point)
	{
		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(Moveable(animal), NC2Layout::MapMoveableOffset::MapPosition));
		return true;
	}

	// Original function: FUN_0042ef84; the native work-center reference is the leader's +2Ch point.
	const void* Animal_GetAnimalWorkCenterPointRef(const void* animal)
	{
		return static_cast<const std::byte*>(Animal(Read<std::int32_t>(animal, NC2Layout::AnimalOffset::LeaderId))) + NC2Layout::AnimalOffset::BirthPoint;
	}

	// Original function: FUN_0042ea17
	const void* Animal_GetMapPositionRef(const void* animal)
	{
		std::uint32_t index;
		std::memcpy(&index, static_cast<const std::byte*>(animal) + NC2Layout::AnimalOffset::MapMoveableId, sizeof(index));

		const std::uint32_t baseAddress = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(gMapMoveableArrayStorage.entries));
		const std::uint32_t positionAddress = baseAddress + index * 0x58u + 0x12u;

		return reinterpret_cast<const void*>(static_cast<std::uintptr_t>(positionAddress));
	}
}
