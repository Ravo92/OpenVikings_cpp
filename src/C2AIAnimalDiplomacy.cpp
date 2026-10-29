#include "C2AIAnimalDiplomacy.h"
#include "C2AIMilitaryAccess.h"
#include "C2AIObjectAccess.h"
#include "C2AIResourceAccess.h"
#include "C2AnimalAccess.h"
#include "C2AnimalIterator.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2HumanIterator.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapSectorAttachment.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_00429453; quantities and requested flags are independent.
	bool AIAnimalTypeCounts_IsRequested(const void* counts, std::uint32_t type)
	{
		for (std::int32_t i = 0; i < Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::Count); ++i)
		{
			if (Read<std::uint32_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstTribeType + i * NC2Layout::RecordSize::AIAnimalCount) == type)
				return Read<std::uint8_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstRequestedFlag + i * NC2Layout::RecordSize::AIAnimalCount) != 0 && Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstQuantity + i * NC2Layout::RecordSize::AIAnimalCount) < NC2Runtime::AIAnimalCatcherLimits::DesiredQuantity;
		}

		return false;
	}

	// Original function: FUN_0042948e.
	bool AIAnimalTypeCounts_IsKnownOrSatisfied(const void* counts, std::uint32_t type)
	{
		for (std::int32_t i = 0; i < Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::Count); ++i)
		{
			if (Read<std::uint32_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstTribeType + i * NC2Layout::RecordSize::AIAnimalCount) == type)
				return Read<std::uint8_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstRequestedFlag + i * NC2Layout::RecordSize::AIAnimalCount) != 0 || Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstQuantity + i * NC2Layout::RecordSize::AIAnimalCount) >= NC2Runtime::AIAnimalCatcherLimits::DesiredQuantity;
		}

		return false;
	}

	// Original function: FUN_0042942c.
	bool AIAnimalCatcher_IsType10Available(const void* catcher)
	{
		const auto* counts = static_cast<const std::byte*>(catcher) + NC2Layout::AIAnimalCatcherOffset::Counts;

		return AIAnimalTypeCounts_IsKnownOrSatisfied(counts, 10) && !AIAnimalTypeCounts_IsRequested(counts, 10);
	}

	// Original function: FUN_00455e20.
	bool AIAnimalCatcher_IsType19Available(const void* catcher)
	{
		const auto* counts = static_cast<const std::byte*>(catcher) + NC2Layout::AIAnimalCatcherOffset::Counts;

		return AIAnimalTypeCounts_IsKnownOrSatisfied(counts, 19) && !AIAnimalTypeCounts_IsRequested(counts, 19);
	}

	namespace
	{
		// Original function: FUN_0045618a.
		void AIAnimalTypeCounts_AddType(void* counts, std::uint32_t type)
		{
			const auto i = Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::Count);

			if (i >= NC2Runtime::AIAnimalCatcherLimits::Types)
			{

				return;
			}

			Write<std::uint32_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstQuantity + i * NC2Layout::RecordSize::AIAnimalCount, 0);
			Write<std::uint8_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstRequestedFlag + i * NC2Layout::RecordSize::AIAnimalCount, 1);
			Write(counts, NC2Layout::AIAnimalCountsOffset::FirstTribeType + i * NC2Layout::RecordSize::AIAnimalCount, type);
			Write(counts, NC2Layout::AIAnimalCountsOffset::Count, i + 1);
		}

		// Original function: FUN_004561b5.
		void AIAnimalTypeCounts_Increment(void* counts, std::uint32_t type)
		{
			for (std::int32_t i = 0; i < Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::Count); ++i)
			{
				if (Read<std::uint32_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstTribeType + i * NC2Layout::RecordSize::AIAnimalCount) == type)
					Write(counts, NC2Layout::AIAnimalCountsOffset::FirstQuantity + i * NC2Layout::RecordSize::AIAnimalCount, Read<std::uint32_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstQuantity + i * NC2Layout::RecordSize::AIAnimalCount) + 1);
			}
		}

		// Original function: FUN_004561d6.
		bool AIAnimalTypeCounts_AreAllSatisfied(const void* counts)
		{
			for (std::int32_t i = 0; i < Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::Count); ++i)
			{
				if (Read<std::uint8_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstRequestedFlag + i * NC2Layout::RecordSize::AIAnimalCount) != 0 && Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstQuantity + i * NC2Layout::RecordSize::AIAnimalCount) < NC2Runtime::AIAnimalCatcherLimits::DesiredQuantity)
					return false;
			}

			return true;
		}

		// Original function: FUN_004561ff; mark before testing the quantity.
		bool AIAnimalTypeCounts_MarkRequested(void* counts, std::uint32_t type)
		{
			for (std::int32_t i = 0; i < Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::Count); ++i)
			{
				if (Read<std::uint32_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstTribeType + i * NC2Layout::RecordSize::AIAnimalCount) == type)
				{
					Write<std::uint8_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstRequestedFlag + i * NC2Layout::RecordSize::AIAnimalCount, 1);
					return Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstQuantity + i * NC2Layout::RecordSize::AIAnimalCount) < NC2Runtime::AIAnimalCatcherLimits::DesiredQuantity;
				}
			}

			return false;
		}
	}

	// Original function: FUN_00455e47.
	void AIAnimalCatcher_Init(void* catcher)
	{
		auto* counts = static_cast<std::byte*>(catcher) + NC2Layout::AIAnimalCatcherOffset::Counts;
		AIAnimalTypeCounts_AddType(counts, 10);
		AIAnimalTypeCounts_AddType(counts, 19);
		AIScout_Init(catcher);
		Write<std::int32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::MilitaryPointIndex, -1);
		Write<std::uint32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::Target, 0);
	}

	// Original function: FUN_00455e72.
	void AIAnimalCatcher_DoOneLogicTick(void* catcher, std::uint32_t player, const void* center, void* military)
	{
		auto* counts = static_cast<std::byte*>(catcher) + NC2Layout::AIAnimalCatcherOffset::Counts;

		for (std::int32_t i = 0; i < Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::Count); ++i)
		{
			Write<std::uint32_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstQuantity + i * NC2Layout::RecordSize::AIAnimalCount, 0);
		}

		CAnimalIterator owned;

		for (auto id = owned.GetFirstSectorAnimalId(player, 0, 10000, false, false); id != -1; id = owned.GetNextSectorAnimalId())
		{
			AIAnimalTypeCounts_Increment(counts, Read<std::uint32_t>(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::ElementSize, NC2Layout::AnimalOffset::TribeType));
		}

		if (AIAnimalTypeCounts_AreAllSatisfied(counts))
		{
			if (AIScout_IsValid(catcher))
			{
				if (AIScout_IsNearPos(catcher, center))
				{
					AIScout_Clear(catcher);
				}
				else
				{
					AIScout_MoveToPos(catcher, center);
				}
			}
			else if (Read<std::int32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::MilitaryPointIndex) != -1)
			{
				AIMilitaryManager_MilitaryPoint_Clear(military, Read<std::int32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::MilitaryPointIndex));
				Write<std::int32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::MilitaryPointIndex, -1);
			}

			return;
		}

		if (!AIScout_IsValid(catcher))
		{
			if (Read<std::int32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::MilitaryPointIndex) != -1)
			{
				AIMilitaryManager_MilitaryPoint_Clear(military, Read<std::int32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::MilitaryPointIndex));
				Write<std::int32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::MilitaryPointIndex, -1);
			}

			Write<std::uint8_t>(catcher, NC2Layout::AIAnimalCatcherOffset::HumanRequestedFlag, 1);
			return;
		}

		if (!AIScout_CheckAndChangeJob(catcher))
			return;

		auto* target = static_cast<std::byte*>(catcher) + NC2Layout::AIAnimalCatcherOffset::Target;

		if (TargetData_IsValid(target))
		{
			if (TargetData_GetPlayerId(target) != 20)
			{
				Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 0);
				return;
			}

			if (Read<std::int32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::MilitaryPointIndex) == -1)
			{
				std::uint32_t scoutTarget[3]{ 1, Read<std::uint32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::HumanId), Read<std::uint32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::HumanRevision) };
				Write(catcher, NC2Layout::AIAnimalCatcherOffset::MilitaryPointIndex, AIMilitaryManager_MilitaryPoint_Add(military, 1, 0, scoutTarget, 40, 1, 1));
			}

			const auto* human = CHumanArray::mStaticVars + Read<std::uint32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::HumanId) * CHumanArray::ElementSize;
			const auto* position = Human_GetMapPositionRef(human);

			if (AIMilitaryManager_Military_GetCivilianDangerValueForPosition(position, player) > 10)
			{
				AIScout_MoveToPos(catcher, Human_GetMapPositionRef(human));
				return;
			}

			std::uint32_t destination = 0;
			TargetData_GetMapPosition(target, &destination);

			if (AIMilitaryManager_Military_GetCivilianDangerValueForPosition(&destination, player) > 20)
			{
				AIMilitaryManager_MilitaryPoint_Add(military, 2, 0, target, 40, 1, 2);
			}
			else
			{
				AIScout_MoveToPos(catcher, &destination);
			}

			return;
		}

		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(center), NC2Layout::MapMicroPointOffset::ContinentId);

		for (std::int32_t i = 0; i < Read<std::int32_t>(counts, NC2Layout::AIAnimalCountsOffset::Count); ++i)
		{
			Write<std::uint8_t>(counts, NC2Layout::AIAnimalCountsOffset::FirstRequestedFlag + i * NC2Layout::RecordSize::AIAnimalCount, 0);
		}

		const auto* human = CHumanArray::mStaticVars + Read<std::uint32_t>(catcher, NC2Layout::AIAnimalCatcherOffset::HumanId) * CHumanArray::ElementSize;
		const auto* position = Human_GetMapPositionRef(human);
		const void* best = nullptr;
		std::int32_t bestScore = 10000;
		CAnimalIterator animals;

		for (auto id = animals.GetFirstAnimalId(); id != -1; id = animals.GetNextAnimalId())
		{
			const auto* animal = CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::ElementSize;
			const auto owner = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId);

			if (owner != 20 && GameControl_Diplomacy_GetState(gGameControl, player, owner) != 3)
				continue;

			if (!AIAnimalTypeCounts_MarkRequested(counts, Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType)))
				continue;

			const auto* animalPosition = Animal_GetMapPositionRef(animal);

			if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(animalPosition), NC2Layout::MapMicroPointOffset::ContinentId) != continent)
				continue;

			const auto danger = AIMilitaryManager_Military_GetCivilianDangerValueForPosition(animalPosition, player);
			const auto distance = VE_HexagonDirection_GetDistance(Read<std::uint16_t>(animalPosition, NC2Layout::MapCoordinatePairOffset::X), Read<std::uint16_t>(animalPosition, NC2Layout::MapCoordinatePairOffset::Y), Read<std::uint16_t>(position, NC2Layout::MapCoordinatePairOffset::X), Read<std::uint16_t>(position, NC2Layout::MapCoordinatePairOffset::Y));
			const auto score = static_cast<std::int32_t>(static_cast<std::uint32_t>(danger) + static_cast<std::uint32_t>(distance / 5));

			if (score > bestScore)
				continue;

			bestScore = score;
			best = animal;
		}

		if (best)
		{
			Write(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPoint, Read<std::uint32_t>(best, NC2Layout::AnimalOffset::AnimalId));
			Write(target, NC2Layout::TargetDataOffset::ObjectRevision, Read<std::uint32_t>(best, NC2Layout::AnimalOffset::UniqueId));
			Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 3);
		}
	}

	// Original function: FUN_00453dff.
	void AIDiplomacy_DoOneLogicTick(void* diplomacy, std::uint32_t player, const void* villages)
	{
		if (Read<std::uint32_t>(diplomacy, NC2Layout::AIDiplomacyOffset::PlayerCursor) == 20)
			Write<std::uint32_t>(diplomacy, NC2Layout::AIDiplomacyOffset::PlayerCursor, 0);

		const auto other = Read<std::int32_t>(diplomacy, NC2Layout::AIDiplomacyOffset::PlayerCursor);

		if (static_cast<std::uint32_t>(other) != player && other < 20 && Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(other) * NC2Layout::RecordSize::GameControlPlayerSigned) != 0)
		{
			const auto theirs = GameControl_Diplomacy_GetState(gGameControl, other, player);
			const auto ours = GameControl_Diplomacy_GetState(gGameControl, player, other);
			const auto counter = NC2Layout::AIDiplomacyOffset::IntrusionCounts + static_cast<std::uint32_t>(other) * NC2Layout::RecordSize::AIDiplomacyCounter;

			if (ours == 1)
			{
				if (theirs != 1)
				{
					GameControl_Diplomacy_SetState(gGameControl, nullptr, player, other, theirs);
				}
				else
				{
					Write<std::uint32_t>(diplomacy, counter, 0);
				}
			}
			else if (ours == 2)
			{
				if (theirs == 3)
				{
					GameControl_Diplomacy_SetState(gGameControl, nullptr, player, other, 3);
				}
				else
				{
					CHumanIterator humans;
					auto id = humans.GetFirstSectorHumanId(other, true);

					for (; id != -1; id = humans.GetNextSectorHumanId())
					{
						if (AIVillagesData_GetVillageId(villages, Human_GetMapPositionRef(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize)) != -1)
							break;
					}

					if (id == -1)
					{
						Write<std::uint32_t>(diplomacy, counter, 0);
					}
					else
					{
						const auto count = Read<std::uint32_t>(diplomacy, counter) + 1;
						Write(diplomacy, counter, count);

						if (count > 5)
							GameControl_Diplomacy_SetState(gGameControl, nullptr, player, other, 3);
					}
				}
			}
		}

		Write(diplomacy, NC2Layout::AIDiplomacyOffset::PlayerCursor, Read<std::uint32_t>(diplomacy, NC2Layout::AIDiplomacyOffset::PlayerCursor) + 1);
	}
}