#include "C2AIMovementMessages.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMilitaryJobs.h"
#include "C2HumanNeeds.h"
#include "C2HumanWalking.h"
#include "GameProfile.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapPathfinderSearch.h"
#include "C2ObjectLayout.h"
#include "C2ObjectMemberConstructors.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	// Original function: FUN_0042c9c9; include a pending pathfinder start.
	bool Human_IsWalking(const void* human)
	{
		if (NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 1)
			return true;

		const auto* moveable = gMapMoveableArrayStorage.entries + NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;

		return NC2Lifetime::Read<std::uint32_t>(MapMoveable_Pathfinder_GetPathfinderPtr(moveable), NC2Layout::MapPathfinderOffset::SearchState) == 1;
	}

	using namespace NC2Lifetime;

	namespace
	{
		struct FreeMapPoints
		{
			FreeMapPoints()
			{
				SearchNearbyMapPoints_ConstructMembers(this);
			}

			std::uint32_t count = 0;
			std::uint32_t points[8];
		};

		struct VehicleFreeMapPoints
		{
			std::uint32_t minimumDistance;
			FreeMapPoints candidates;
		};

		bool IsFreeForStationaryMoveables(const void* point)
		{
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
				return false;

			for (auto id = static_cast<std::int32_t>(Read<std::int16_t>(micro, NC2Layout::MapMicroPointOffset::FirstMapMoveableId)); id != -1;)
			{
				const auto* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;
				const auto state = Read<std::uint32_t>(MapMoveable_Pathfinder_GetPathfinderPtr(moveable), NC2Layout::MapPathfinderOffset::SearchState);

				if (state == 0 || state == 3)
					return false;

				id = Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::NextCellMoveableId);
			}

			return true;
		}

		// Original function: FUN_0045d374; the second accepted point stops the search.
		bool __cdecl slHumanSearchNextFreeMapPointFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			if (!IsFreeForStationaryMoveables(point))
				return false;

			auto& candidates = *static_cast<FreeMapPoints*>(callback->userData);
			candidates.points[candidates.count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			return candidates.count > 1;
		}

		// Original function: FUN_0045d45e; compare the path search distance before occupancy.
		bool __cdecl slHumanSearchNextVehicleFreeMapPointFunction(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<VehicleFreeMapPoints*>(callback->userData);

			if (callback->distance < data.minimumDistance || !IsFreeForStationaryMoveables(point))
				return false;

			data.candidates.points[data.candidates.count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			return data.candidates.count > 1;
		}
	}

	// Original function: FUN_0045d315; blocked traversal is allowed for finding an escape point.
	bool Human_FindNextFreeMapPoint(const void* human, void* point)
	{
		FreeMapPoints data;
		CMapPathfinderElement::SearchOverMap(Human_GetMapPositionRef(human), 40, false, slHumanSearchNextFreeMapPointFunction, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));

		if (data.count == 0)
			return false;

		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, data.points[Game_Random_GetNumber() % data.count]);

		return true;
	}

	// Original function: FUN_0045d3f9.
	bool Human_FindNextVehicleFreeMapPoint(const void* human, void* point, std::uint32_t minimumDistance)
	{
		VehicleFreeMapPoints data{ minimumDistance, { } };
		CMapPathfinderElement::SearchOverMap(Human_GetMapPositionRef(human), 40, true, slHumanSearchNextVehicleFreeMapPointFunction, &data, true, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId));

		if (data.candidates.count == 0)
			return false;

		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, data.candidates.points[Game_Random_GetNumber() % data.candidates.count]);

		return true;
	}

	// Original function: FUN_0045d196.
	bool Human_l_Item_IsGoodTypeEquipped(const void* human, std::uint32_t good)
	{
		for (std::uint32_t slot = 0; slot < 4; ++slot)
		{
			const auto offset = NC2Layout::HumanOffset::InventorySlots + slot * NC2Layout::RecordSize::HumanInventorySlot;
			const auto equipped = Read<std::uint32_t>(human, offset);

			if (equipped != 0 && Read<std::int32_t>(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition) > 0 && equipped == good)
				return true;
		}

		return false;
	}

	// Original function: FUN_0045d176.
	bool Human_Item_IsAmuletEquipped_Speed(const void* human)
	{

		return Human_l_Item_IsGoodTypeEquipped(human, 55);
	}

	// Original function: FUN_004739e1; preserve 32-bit wrapping before the signed clamp.
	std::int32_t Human_WalkSpeed_GetMoveSpeed(const void* human, std::uint32_t resistance)
	{
		auto duration = resistance * 2u + 4u;

		if (Human_HasEquippedShoes(human))
			duration -= 2u;

		if (Human_Item_IsAmuletEquipped_Speed(human))
			duration -= 2u;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
			++duration;

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Stamina) <= 2000)
			duration += 2u;

		const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

		if (tribe == 3 && job == 32)
			duration -= 2u;

		if (tribe == 7)
			duration -= 2u;

		if (job == 1 || job == 2)
		{
			duration *= 2u;
		}
		else if (job == 3 || job == 4)
		{
			duration += 2u;
		}

		if (!Human_IsJobHero(human))
		{
			const auto* weapons = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[tribe]);
			const auto* armor = static_cast<const SStaticArmorTypeData*>(gStaticArmorTypeData);
			const auto weight = static_cast<std::uint32_t>(weapons[Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon)].weight) + static_cast<std::uint32_t>(armor[Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor)].weight);
			duration += weight >> 1;
		}

		const auto flags = Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags);

		if ((flags & NXBasics::BitMask::Bit9Unsigned) != 0)
			duration *= 2u;

		if ((flags & NXBasics::BitMask::Bit17) != 0)
			duration -= 2u;

		const auto signedDuration = NXBasics::Runtime_TimeSigned32(duration);

		return signedDuration < 3 ? 3 : signedDuration;
	}

	// Original function: FUN_004739af.
	void Human_WalkSpeed_Update(void* human, std::uint32_t resistance)
	{
		void* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
		const auto duration = Human_WalkSpeed_GetMoveSpeed(human, resistance);
		Write(moveable, NC2Layout::MapMoveableOffset::WalkingTicksPerStep, duration);
		Write<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::TurningTicksPerDirection, 1);
		Write(moveable, NC2Layout::MapMoveableOffset::WalkingProgressPerTick, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(duration) + 9999u) / duration);
	}

	// Original function: FUN_0044057d.
	void GameAssistantSingleHumanData_System_Init(void* data)
	{
		NXBasics::Runtime_FillMemory(data, 0, NC2Layout::RecordSize::HumanAssistantData);
		Write<std::uint8_t>(data, NC2Layout::HumanAssistantDataOffset::InitializedFlag, 1);
	}

	// Original function: FUN_00440595; first free slot or first strictly earliest expiry.
	void GameAssistantSingleHumanData_CommandHistory_Add(void* data, std::uint32_t command, std::uint32_t parameter1, std::uint32_t parameter2, std::uint32_t duration)
	{
		GameAssistantSingleHumanData_CommandHistory_Validate(data);
		std::uint32_t index = 0;

		while (index < NC2Runtime::HumanAssistantCommandHistory::Capacity && Read<std::uint32_t>(data, NC2Layout::HumanAssistantDataOffset::CommandHistory + index * NC2Layout::RecordSize::HumanAssistantCommandHistory) != 0)
			++index;

		if (index == NC2Runtime::HumanAssistantCommandHistory::Capacity)
		{
			std::uint32_t earliest = NXBasics::IntegerRange::UInt32Maximum;
			index = NXBasics::IntegerRange::UInt32Maximum;

			for (std::uint32_t candidate = 0; candidate < NC2Runtime::HumanAssistantCommandHistory::Capacity; ++candidate)
			{
				const auto offset = NC2Layout::HumanAssistantDataOffset::CommandHistory + candidate * NC2Layout::RecordSize::HumanAssistantCommandHistory;
				const auto expiry = Read<std::uint32_t>(data, offset + NC2Layout::HumanAssistantCommandHistoryOffset::StartTick) + Read<std::uint32_t>(data, offset + NC2Layout::HumanAssistantCommandHistoryOffset::DurationTicks);

				if (expiry < earliest)
				{
					earliest = expiry;
					index = candidate;
				}
			}
		}
		// Keep the native -1 sentinel if all ten expiries equal UINT32_MAX.
		const auto offset = NC2Layout::HumanAssistantDataOffset::CommandHistory + index * NC2Layout::RecordSize::HumanAssistantCommandHistory;
		Write(data, offset, command);
		Write(data, offset + NC2Layout::HumanAssistantCommandHistoryOffset::Parameter1, parameter1);
		Write(data, offset + NC2Layout::HumanAssistantCommandHistoryOffset::Parameter2, parameter2);
		Write(data, offset + NC2Layout::HumanAssistantCommandHistoryOffset::StartTickUnsigned, gGameTime.tick);
		Write(data, offset + NC2Layout::HumanAssistantCommandHistoryOffset::DurationTicksUnsigned, duration * NC2Runtime::HumanAssistantCommandHistory::TicksPerDurationUnit);
		Write<std::uint8_t>(data, NC2Layout::HumanAssistantDataOffset::HasCommandHistoryFlag, 1);
	}

	// Original function: FUN_0044061a.
	bool GameAssistantSingleHumanData_CommandHistory_IsInArray(void* data, std::uint32_t command)
	{
		GameAssistantSingleHumanData_CommandHistory_Validate(data);

		if (Read<std::uint8_t>(data, NC2Layout::HumanAssistantDataOffset::HasCommandHistoryFlag) == 0)
			return false;

		for (std::uint32_t index = 0; index < NC2Runtime::HumanAssistantCommandHistory::Capacity; ++index)
		{
			const auto stored = Read<std::uint32_t>(data, NC2Layout::HumanAssistantDataOffset::CommandHistory + index * NC2Layout::RecordSize::HumanAssistantCommandHistory);

			if (stored != 0 && stored == command)
				return true;
		}

		return false;
	}

	// Original function: FUN_0044064c.
	bool GameAssistantSingleHumanData_CommandHistory_IsInArray(void* data, std::uint32_t command, std::uint32_t parameter1, std::uint32_t parameter2)
	{
		GameAssistantSingleHumanData_CommandHistory_Validate(data);

		if (Read<std::uint8_t>(data, NC2Layout::HumanAssistantDataOffset::HasCommandHistoryFlag) == 0)
			return false;

		for (std::uint32_t index = 0; index < NC2Runtime::HumanAssistantCommandHistory::Capacity; ++index)
		{
			const auto offset = NC2Layout::HumanAssistantDataOffset::CommandHistory + index * NC2Layout::RecordSize::HumanAssistantCommandHistory;
			const auto stored = Read<std::uint32_t>(data, offset);

			if (stored != 0 && stored == command && Read<std::uint32_t>(data, offset + NC2Layout::HumanAssistantCommandHistoryOffset::Parameter1) == parameter1 && Read<std::uint32_t>(data, offset + NC2Layout::HumanAssistantCommandHistoryOffset::Parameter2) == parameter2)
				return true;
		}

		return false;
	}

	// Original function: FUN_004406c4; expiry uses unsigned time addition/comparison.
	void GameAssistantSingleHumanData_CommandHistory_Validate(void* data)
	{
		if (Read<std::uint8_t>(data, NC2Layout::HumanAssistantDataOffset::HasCommandHistoryFlag) == 0)
			return;

		bool changed = false;

		for (std::uint32_t index = 0; index < NC2Runtime::HumanAssistantCommandHistory::Capacity; ++index)
		{
			const auto offset = NC2Layout::HumanAssistantDataOffset::CommandHistory + index * NC2Layout::RecordSize::HumanAssistantCommandHistory;

			if (Read<std::uint32_t>(data, offset) != 0 && Read<std::uint32_t>(data, offset + NC2Layout::HumanAssistantCommandHistoryOffset::StartTick) + Read<std::uint32_t>(data, offset + NC2Layout::HumanAssistantCommandHistoryOffset::DurationTicks) <= gGameTime.tick)
			{
				Write<std::uint32_t>(data, offset, 0);
				changed = true;
			}
		}

		if (!changed)
		{

			return;
		}

		bool any = false;

		for (std::uint32_t index = 0; index < NC2Runtime::HumanAssistantCommandHistory::Capacity; ++index)
		{
			if (Read<std::uint32_t>(data, NC2Layout::HumanAssistantDataOffset::CommandHistory + index * NC2Layout::RecordSize::HumanAssistantCommandHistory) != 0)
			{
				any = true;
				break;
			}
		}

		Write<std::uint8_t>(data, NC2Layout::HumanAssistantDataOffset::HasCommandHistoryFlag, any ? 1 : 0);
	}

	// Original function: FUN_00440690.
	bool GameAssistantSingleHumanData_CommandHistory_IsEquipCommandInArray(void* data)
	{
		GameAssistantSingleHumanData_CommandHistory_Validate(data);

		if (Read<std::uint8_t>(data, NC2Layout::HumanAssistantDataOffset::HasCommandHistoryFlag) == 0)
			return false;

		for (std::uint32_t index = 0; index < NC2Runtime::HumanAssistantCommandHistory::Capacity; ++index)
		{
			const auto type = Read<std::int32_t>(data, NC2Layout::HumanAssistantDataOffset::CommandHistory + index * NC2Layout::RecordSize::HumanAssistantCommandHistory);

			if (type >= 79 && type <= 87)
				return true;
		}

		return false;
	}

	// Original function: FUN_0042eec9.
	void Animal_l_DetachLeader(void* animal)
	{
		const auto id = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId);

		if (id == NXBasics::IntegerRange::UInt32Maximum)
		{

			return;
		}

		void* leader = CAnimalArray::mStaticVars + id * CAnimalArray::RuntimeElementSize;

		if (Read<std::uint8_t>(leader, NC2Layout::AnimalOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::UniqueId) == Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderRevision))
			Write(leader, NC2Layout::AnimalOffset::GroupSize, Read<std::uint32_t>(leader, NC2Layout::AnimalOffset::GroupSize) - 1u);

		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::LeaderId, NXBasics::IntegerRange::UInt32Maximum);
	}

	// Original function: FUN_0042f63f; transfer updates pathfinder ownership and sector lists.
	void Animal_ChangePlayerId(void* animal, std::uint32_t player)
	{
		void* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
		MapMoveable_Attachment_DetachFromSectorMap(moveable);
		Write<std::uint32_t>(animal, NC2Layout::AnimalOffset::AttachmentTarget, 0);
		Write(animal, NC2Layout::AnimalOffset::PlayerId, player);
		void* path = GetUnsafePathfinderPtr(Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::PathfinderId));
		Write<std::uint32_t>(path, NC2Layout::MapPathfinderOffset::CurrentGuideId, NXBasics::IntegerRange::UInt32Maximum);
		Write<std::uint32_t>(path, NC2Layout::MapPathfinderOffset::SourceGuideId, NXBasics::IntegerRange::UInt32Maximum);
		Write(path, NC2Layout::MapPathfinderOffset::PlayerId, player);
		MapMoveable_Attachment_AttachToSectorMap(moveable);
		Animal_l_DetachLeader(animal);
	}

	// Original function: FUN_0044dc14; catchable animals become the exploring player's animals.
	void Map_AnimalTool_ChangePlayerId(std::uint32_t player, const void* point)
	{
		if (!MapMigPoint_IsInsideMap(point))
		{

			return;
		}

		CMapMoveableIterator iterator;

		for (auto id = iterator.GetFirstMapMoveableId(point); id != -1; id = iterator.GetNextMapMoveableId())
		{
			const void* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

			if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) != 2)
				continue;

			void* animal = CAnimalArray::mStaticVars + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId) * CAnimalArray::RuntimeElementSize;
			const auto tribe = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType);
			const auto* data = static_cast<const SStaticAnimalTypeData*>(gStaticAnimalTypeData) + tribe - NRemaster::GetFirstAnimalTribe();

			if (data->catchable == 0)
				continue;

			const auto owner = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId);

			if (owner == 20 || GameControl_Diplomacy_GetState(gGameControl, static_cast<std::int32_t>(player), static_cast<std::int32_t>(owner)) == 3)
				Animal_ChangePlayerId(animal, player);
		}
	}

	// Original function: FUN_0044dbbb; the radius excludes its outermost ring.
	void Map_AnimalTool_ChangePlayerId(std::uint32_t player, std::uint32_t point, std::uint32_t radius)
	{
		Map_AnimalTool_ChangePlayerId(player, &point);

		for (std::uint32_t ring = 1; ring < radius; ++ring)
		{
			MapMigPoint_MoveCursorUnwrapped(point, 4);

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				for (std::uint32_t step = 0; step < ring; ++step)
				{
					MapMigPoint_MoveCursorUnwrapped(point, direction);
					Map_AnimalTool_ChangePlayerId(player, &point);
				}
			}
		}
	}

	// Original function: FUN_0043ae66; in-place ring traversal and byte continent identity.
	bool Tool_Map_GetNotBlockedPosition(void* point, bool avoidHouses)
	{
		const void* micro = GameMap_GetConstMapMicroPointPtr(point);

		if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0 && (!avoidHouses || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 2))
			return true;

		const auto continent = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId);
		bool found = false;

		for (std::uint32_t ring = 1; ring < 10 && !found; ++ring)
		{
			MapMigPoint_MoveCursorUnwrapped(point, 4, 1);

			for (std::uint32_t direction = 0; direction < 6 && !found; ++direction)
			{
				for (std::uint32_t step = 0; step < ring; ++step)
				{
					micro = GameMap_GetConstMapMicroPointPtr(point);

					if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) == 0 && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId) == continent && (!avoidHouses || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 2))
					{
						found = true;
						break;
					}

					MapMigPoint_MoveCursorUnwrapped(point, direction, 1);
				}
			}
		}

		return found;
	}

	// Original function: FUN_0043aeff; only players 0..9 receive first-contact messages.
	void Tool_Map_CheckUserInformationAboutEnemies(std::uint32_t player, const void* point)
	{
		if (player >= 16)
		{

			return;
		}

		const auto seen = Read<std::uint16_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ExplorationMask);

		for (std::uint32_t other = 0; other < 10; ++other)
		{
			if (other == player || (seen & (1u << other)) == 0)
				continue;

			void* otherData = static_cast<std::byte*>(gGameControl) + other * NC2Layout::RecordSize::GameControlPlayer;

			if (Read<std::uint8_t>(otherData, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0 || Read<std::uint8_t>(otherData, NC2Layout::GameControlPlayerOffset::PlayerSeenFlagsUnsigned + player) != 0)
				continue;

			Write<std::uint8_t>(otherData, NC2Layout::GameControlPlayerOffset::PlayerSeenFlagsUnsigned + player, 1);
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x29, true, other, player, MapMigPoint_GetLong(point));
			const void* ownData = static_cast<const std::byte*>(gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;

			if (Read<std::uint8_t>(ownData, NC2Layout::GameControlPlayerOffset::SuppressPlayerSeenMessagesUnsigned + other) != 0)
				continue;

			std::byte message[772];
			ToUserMessage_Construct(message);
			Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 0x3C);
			Write(message, NC2Layout::ToUserMessageOffset::PlayerId, other);
			Write(message, NC2Layout::ToUserMessageOffset::RelatedPlayerId, player);
			Write(message, NC2Layout::ToUserMessageOffset::MapPositionX, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
			ToUserMessage_FinishMessage(message);
			NC2LogicExternals::External_Misc_SendToUserMessage(message);
		}
	}

	// Original function: FUN_00462cd5; civilian and military destination continuations are connected.
	bool Human_AI_DoNewMapPositionReached(void* human, bool destinationReached)
	{
		Write(human, NC2Layout::HumanOffset::MapPositionReachedCount, Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapPositionReachedCount) + 1u);
		const void* position = Human_GetMapPositionRef(human);
		const void* micro = GameMap_GetConstMapMicroPointPtr(position);
		GameMap_Exploration_DoExplore(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), position, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ExplorationRadius));

		if (Human_HasEquippedShoes(human))
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x24, true, MapMigPoint_GetLong(position), 5, 100);

		if (Human_IsAgeAdult(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) != 5 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & 2u) == 0)
		{
			const void* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
			auto next = Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates);
			MapMigPoint_MoveCursorUnwrapped(next, Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0 ? Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::Direction) : 2u);
			std::uint32_t macroPoint = 0;
			MapMagPoint_ConstructFromMig(&macroPoint, position, false);
			const void* macro = GameMap_GetConstMapMacroPointPtr(Read<std::int16_t>(&macroPoint, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&macroPoint, NC2Layout::MapCoordinatePairOffset::Y));
			const auto kind = Read<std::uint8_t>(macro, NC2Layout::MapMacroPointOffset::PatternA);

			if (kind == 7 || (kind == 8 && Read<std::uint8_t>(macro, NC2Layout::MapMacroPointOffset::PatternB) == 8))
			{
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x26, true, kind == 7 ? 2u : 3u, MapMigPoint_GetLong(position), MapMigPoint_GetLong(&next));
			}
			else if (kind == 4)
			{
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x24, true, MapMigPoint_GetLong(position), 5, 100);
			}
		}

		if (Human_IsJobScout(human))
			Map_AnimalTool_ChangePlayerId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates), 3);

		auto resistance = (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) >> 3) & 15u;
		Human_WalkSpeed_Update(human, resistance);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
			resistance *= 2u;

		const auto shoes = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ShoeCondition);

		if (shoes != 0)
		{
			if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit12) == 0)
			{
				const auto remaining = NXBasics::Runtime_TimeSigned32(shoes - resistance);
				Write<std::int32_t>(human, NC2Layout::HumanOffset::ShoeCondition, remaining < 0 ? 0 : remaining > 10000 ? 10000 : remaining);
			}
		}
		else if (!Human_AI_IsIgnoringNeeds(human))
		{
			Write(human, NC2Layout::HumanOffset::Food, Read<std::uint32_t>(human, NC2Layout::HumanOffset::Food) - resistance);
		}

		if (!destinationReached)
			return Human_AI_DoNewMapPositionReached_DestinationNotReached(human);

		Human_AI_DoNewMapPositionReached_DestinationReached(human);

		return true;
	}

	// Original function: FUN_00463043; AI self-defence consumes the found target before civilian transitions.
	bool Human_AI_DoNewMapPositionReached_DestinationNotReached(void* human)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return true;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter) == 0 && Human_IsRefreshingInWorld(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapPositionReachedCount) % 10u == 0)
		{
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::UrgentNeedsCheckedFlag, 0);
			Human_AI_DoCheckForUrgentNeeds(human);
		}

		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 7 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == 0) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 13)
			return false;

		Human_DoExecuteAllPriorityUserCommandsWhichDoNotInterrupt(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 23 && Human_UserControl_IsPriorityCommandInQueue(human))
		{
			Human_CurrentTask_DoInterrupt(human);
			return false;
		}

		if (!Human_IsJobSoldier(human) && !Human_IsJobHero(human))
			return true;

		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapPositionReachedCount) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId)) % 10u != 0)
			return true;

		const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);

		if (task != 14 && task != 15 && !(Human_GetPlayerType(human) == 2 && task == 4))
			return true;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter) != 0 && Human_IsRefreshingInWorld(human))
			return true;

		const auto mode = Read<std::uint32_t>(human, NC2Layout::HumanOffset::MilitaryMode);
		const auto* primary = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTarget;

		if (mode == 3)
		{
			if (task == 15 && !TargetData_IsValid(primary))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
				return false;
			}

			return true;
		}

		if ((mode != 1 && mode != 2) || TargetData_IsValid(primary))
			return true;

		std::uint32_t target[3]{ };
		const auto* source = mode == 2 && Human_GetPlayerType(human) != 2 ? static_cast<const void*>(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter) : Human_GetMapPositionRef(human);

		if (Human_l_CheckForEnemyTarget(human, source, 0, 18, target))
		{
			if (Human_GetPlayerType(human) == 2)
			{
				AIManager_Message_StartSelfDefence(gAIManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), target);
				return true;
			}

			for (std::uint32_t index = 0; index < 3; ++index)
			{
				Write(human, NC2Layout::HumanOffset::SecondaryMilitaryTargetUnsigned + index * NC2Layout::RecordSize::MilitaryTargetValueUnsigned, target[index]);
			}

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
			return false;
		}

		if (mode == 2 && task == 15)
		{
			auto point = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter);
			auto* secondary = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::SecondaryMilitaryTarget;

			if (TargetData_IsValid(secondary))
				TargetData_GetMapPosition(secondary, &point);

			const auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(human, NC2Layout::HumanOffset::WorkCenterX), Read<std::int16_t>(human, NC2Layout::HumanOffset::WorkCenterY), Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y)));

			if (distance > 40)
			{
				Write<std::uint32_t>(secondary, NC2Layout::TargetDataOffset::Type, 0);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
				return false;
			}
		}

		return true;
	}
}