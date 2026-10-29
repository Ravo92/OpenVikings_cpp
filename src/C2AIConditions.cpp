#include "C2AIConditions.h"
#include "C2AnimalAccess.h"
#include "C2GameControlNetwork.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2PlayerStatisticAccess.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_0042a520.
	bool AIManager_EntryPoint_Condition_Set(void* manager, std::uint32_t player, std::uint32_t index, const void* condition)
	{
		void* handler = Read<void*>(manager, player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned);

		return handler != nullptr && AISinglePlayerHandler_Condition_Set(handler, index, condition);
	}

	// Original function: FUN_0042a564.
	bool AIManager_EntryPoint_SetUnitLimit(void* manager, std::uint32_t player, std::uint32_t limit)
	{
		void* handler = Read<void*>(manager, player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned);

		if (handler == nullptr)
			return false;

		Write(handler, NC2Layout::AISinglePlayerOffset::UnitLimit, limit);

		return true;
	}

	// Original function: FUN_0042a582.
	bool AIManager_EntryPoint_SetMaxUnitLimit(void* manager, std::uint32_t player, std::uint32_t limit)
	{
		void* handler = Read<void*>(manager, player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned);

		if (handler == nullptr)
			return false;

		Write(handler, NC2Layout::AISinglePlayerOffset::MaxUnitLimit, limit);

		return true;
	}

	// Original function: FUN_0042a5a0; existing mission-action entry belongs to this condition block.
	bool AIManager_EntryPoint_Condition_ExternalActivate_SetFlag(void* manager, std::uint32_t player, std::uint32_t index, bool active)
	{
		void* handler = Read<void*>(manager, player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned);

		return handler != nullptr && AISinglePlayerHandler_EntryPoint_Condition_ExternalActivate_SetFlag(handler, index, active);
	}

	// Original function: FUN_0042b5ed; only NOT and XOR constrain the number of operands.
	bool AIConditionToConditionCheck_Tool_CheckNumberOfConditions(const void* check)
	{
		const auto operation = Read<std::uint32_t>(check, NC2Layout::AIConditionCheckOffset::Operation);

		if (operation == SwitchIds::AIConditionOperation::Not)
			return Read<std::uint32_t>(check, NC2Layout::AIConditionCheckOffset::OperandCount) == 1;

		if (operation == SwitchIds::AIConditionOperation::Xor)
			return Read<std::uint32_t>(check, NC2Layout::AIConditionCheckOffset::OperandCount) == 2;

		return true;
	}

	// Original function: FUN_0042b60c.
	void AICondition_Init(void* condition, std::uint32_t type, std::uint8_t flag)
	{
		NXBasics::Runtime_FillMemory(condition, 0, NC2Layout::RecordSize::AICondition);
		Write(condition, NC2Layout::AIConditionOffset::Type, type);
		Write(condition, NC2Layout::AIConditionOffset::KeepActive, flag);
		Write<std::uint32_t>(condition, NC2Layout::AIConditionOffset::ActivatedTick, NXBasics::IntegerRange::UInt32Maximum);
		Write<std::uint32_t>(condition, NC2Layout::AIConditionOffset::DeactivatedTick, NXBasics::IntegerRange::UInt32Maximum);
		Write(condition, NC2Layout::AIConditionOffset::CreatedTick, gGameTime.tick);
	}

	// Original function: FUN_0042b63e; the stored flag may inhibit a transition away from active.
	bool AICondition_SetCondition(void* condition, std::uint8_t active)
	{
		const auto previous = Read<std::uint8_t>(condition, NC2Layout::AIConditionOffset::Active);

		if (previous == active || (previous != 0 && Read<std::uint8_t>(condition, NC2Layout::AIConditionOffset::KeepActive) != 0))
			return false;

		Write(condition, NC2Layout::AIConditionOffset::Active, active);
		Write(condition, active != 0 ? NC2Layout::AIConditionOffset::ActivatedTickUnsigned : NC2Layout::AIConditionOffset::DeactivatedTickUnsigned, gGameTime.tick);

		return true;
	}

	// Original function: FUN_00458dc4; preserve the raw active byte for XOR conditions.
	std::uint8_t AISinglePlayerHandler_Condition_IsActive(const void* handler, std::uint32_t index)
	{
		if (index == NC2Runtime::AIConditionReference::AlwaysActive)
			return 1;

		if (index == NC2Runtime::AIConditionReference::AlwaysInactive || index >= NC2Runtime::AIConditionLimits::Slots || Read<std::uint32_t>(handler, index * NC2Layout::RecordSize::AIConditionUnsigned + NC2Layout::AISinglePlayerOffset::FirstConditionTypeUnsigned) == 0)
			return 0;

		return Read<std::uint8_t>(handler, index * NC2Layout::RecordSize::AIConditionUnsigned + NC2Layout::AISinglePlayerOffset::ConditionsUnsigned);
	}

	namespace
	{
		// Entire Windows DWORD region 00500F44..00500F7B, indexed by condition type.
		constexpr std::uint32_t gAIConditionCheckIntervals[] = { 0, 1, 1, 1, 1, 1, 10, 10, 1, 1, 1, 1, 1, NXBasics::IntegerRange::UInt32Maximum };
	}

	// Original function: FUN_00458df6; do at most ten passes, repeating while a condition changes.
	bool AISinglePlayerHandler_Condition_RecheckAll(void* handler, std::uint32_t tick)
	{
		bool anyChanged = false;

		for (std::uint32_t pass = 0; pass < NC2Runtime::AIConditionLimits::RecheckPasses; ++pass)
		{
			bool changed = false;
			std::uint32_t index = 0;
			do
			{
				void* condition = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::ConditionsUnsigned + index * NC2Layout::RecordSize::AIConditionUnsigned;
				const auto type = Read<std::uint32_t>(condition, NC2Layout::AIConditionOffset::Type);

				if (type != 0 && (tick == 0 || tick % gAIConditionCheckIntervals[type] == 0))
					changed |= AISinglePlayerHandler_l_Condition_CheckForConditionChange(handler, condition);

				++index;
			} while (index <= Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::LastConditionIndex));
			anyChanged |= changed;

			if (!changed)
				break;
		}

		return anyChanged;
	}

	// Original function: FUN_00458e5f; all twelve Windows jump-table branches at 00459260 are retained.
	bool AISinglePlayerHandler_l_Condition_CheckForConditionChange(void* handler, void* condition)
	{
		const auto type = Read<std::uint32_t>(condition, NC2Layout::AIConditionOffset::Type);
		std::uint8_t active = 0;

		switch (type)
		{
			case SwitchIds::AICondition::True:
				active = 1;
				break;
			case SwitchIds::AICondition::GameTime:
				active = gGameTime.tick >= Read<std::uint32_t>(condition, NC2Layout::AIConditionGameTimeOffset::RequiredTick);
				break;
			case SwitchIds::AICondition::LogicalCombination:
			{
				const auto operation = Read<std::uint32_t>(condition, NC2Layout::AIConditionLogicalCombinationOffset::Operation);

				if (operation == SwitchIds::AIConditionOperation::Xor)
				{
					const auto first = AISinglePlayerHandler_Condition_IsActive(handler, Read<std::uint32_t>(condition, NC2Layout::AIConditionLogicalCombinationOffset::OperandIds));
					active = first ^ AISinglePlayerHandler_Condition_IsActive(handler, Read<std::uint32_t>(condition, NC2Layout::AIConditionLogicalCombinationOffset::SecondOperandId));
				}
				else if (operation == SwitchIds::AIConditionOperation::Not)
				{
					active = AISinglePlayerHandler_Condition_IsActive(handler, Read<std::uint32_t>(condition, NC2Layout::AIConditionLogicalCombinationOffset::OperandIds)) == 0;
				}
				else if (operation == SwitchIds::AIConditionOperation::And || operation == SwitchIds::AIConditionOperation::Or)
				{
					std::uint32_t index = 0;
					const auto count = Read<std::uint32_t>(condition, NC2Layout::AIConditionLogicalCombinationOffset::OperandCount);

					while (index < count)
					{
						const auto value = AISinglePlayerHandler_Condition_IsActive(handler, Read<std::uint32_t>(condition, NC2Layout::AIConditionLogicalCombinationOffset::OperandIdsUnsigned + index * NC2Layout::RecordSize::AIConditionOperandUnsigned));

						if ((operation == SwitchIds::AIConditionOperation::And && value == 0) || (operation == SwitchIds::AIConditionOperation::Or && value != 0))
							break;

						++index;
					}

					active = operation == SwitchIds::AIConditionOperation::And ? index == count : index != count;
				}
				else
				{
					return false;
				}

				break;
			}
			case SwitchIds::AICondition::TimeSinceConditionChange:
			{
				const auto index = Read<std::uint32_t>(condition, NC2Layout::AIConditionTimeSinceConditionChangeOffset::ConditionId);
				const void* reference = static_cast<const std::byte*>(handler) + index * NC2Layout::RecordSize::AIConditionUnsigned + NC2Layout::AISinglePlayerOffset::ConditionsUnsigned;

				if (Read<std::uint32_t>(reference, NC2Layout::AIConditionOffset::Type) != 0)
				{
					const auto time = Read<std::uint32_t>(reference, Read<std::uint8_t>(condition, NC2Layout::AIConditionTimeSinceConditionChangeOffset::WatchActive) != 0 ? NC2Layout::AIConditionOffset::ActivatedTickUnsigned : NC2Layout::AIConditionOffset::DeactivatedTickUnsigned);

					if (time != NXBasics::IntegerRange::UInt32Maximum)
						active = gGameTime.tick >= time + Read<std::uint32_t>(condition, NC2Layout::AIConditionTimeSinceConditionChangeOffset::DelayTicks);
				}

				break;
			}
			case SwitchIds::AICondition::Diplomacy:
				active = static_cast<std::uint32_t>(GameControl_Diplomacy_GetState(gGameControl, Read<std::int32_t>(condition, NC2Layout::AIConditionDiplomacyOffset::FirstPlayerId), Read<std::int32_t>(condition, NC2Layout::AIConditionDiplomacyOffset::SecondPlayerId))) == Read<std::uint32_t>(condition, NC2Layout::AIConditionDiplomacyOffset::State);
				break;
			case SwitchIds::AICondition::MapMoveableNearPosition:
			{
				std::uint32_t center;
				MapMigPoint_Construct(&center, Read<std::int16_t>(condition, NC2Layout::AIConditionMapMoveableNearPositionOffset::MapX), Read<std::int16_t>(condition, NC2Layout::AIConditionMapMoveableNearPositionOffset::MapY), false);
				CMapMoveableIterator iterator;

				for (auto id = iterator.GetFirstMapMoveableId(); id != -1; id = iterator.GetNextMapMoveableId())
				{
					const void* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;
					const auto moveableType = Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType);
					std::uint32_t player = 20, position = 0;
					std::uint8_t military = 0;

					if (moveableType == SwitchIds::MapMoveableType::Human)
					{
						const void* human = GetHumanPtr(Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId));
						player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
						military = Human_IsJobSoldier(human);
						position = Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapMigPointOffset::Coordinates);
					}
					else if (moveableType == SwitchIds::MapMoveableType::Animal)
					{
						const void* animal = GetAnimalPtr(Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId));
						player = Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::PlayerId);

						if (player == 20)
							continue;

						if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::AnimalType) == 49)
							military = Read<std::uint8_t>(animal, NC2Layout::AnimalOffset::AggressiveFlag);

						position = Read<std::uint32_t>(Animal_GetMapPositionRef(animal), NC2Layout::MapMigPointOffset::Coordinates);
					}
					else if (moveableType == SwitchIds::MapMoveableType::Vehicle)
					{
						const void* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId) * CVehicleArray::ElementSize;
						player = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId);
						military = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::JobType) == 54;
						position = Read<std::uint32_t>(Vehicle_GetMapPositionRef(vehicle), NC2Layout::MapMigPointOffset::Coordinates);
					}
					else if (moveableType == SwitchIds::MapMoveableType::Marker)
					{
						continue;
					}

					if (player == NXBasics::IntegerRange::UInt32Maximum)
						continue;

					const auto owner = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), requestedPlayer = Read<std::uint32_t>(condition, NC2Layout::AIConditionMapMoveableNearPositionOffset::PlayerId);

					if (player == owner && requestedPlayer != owner)
						continue;

					if (requestedPlayer != 20 && requestedPlayer != player)
						continue;

					if (Read<std::uint8_t>(condition, NC2Layout::AIConditionMapMoveableNearPositionOffset::OpponentsOnly) != 0 && GameControl_Diplomacy_GetState(gGameControl, owner, player) != 3)
						continue;

					if (Read<std::uint8_t>(condition, NC2Layout::AIConditionMapMoveableNearPositionOffset::MilitaryOnly) != 0 && military == 0)
						continue;

					if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(static_cast<std::uint16_t>(position), static_cast<std::uint16_t>(position >> 16), static_cast<std::uint16_t>(center), static_cast<std::uint16_t>(center >> 16))) < Read<std::uint32_t>(condition, NC2Layout::AIConditionMapMoveableNearPositionOffset::Radius))
					{
						active = 1;
						break;
					}
				}

				break;
			}
			case SwitchIds::AICondition::HouseNearPosition:
			{
				std::uint32_t center;
				MapMigPoint_Construct(&center, Read<std::int16_t>(condition, NC2Layout::AIConditionHouseNearPositionOffset::MapX), Read<std::int16_t>(condition, NC2Layout::AIConditionHouseNearPositionOffset::MapY), false);
				CHouseIterator iterator;
				const auto player = Read<std::uint32_t>(condition, NC2Layout::AIConditionHouseNearPositionOffset::PlayerId);
				auto id = player == 20 ? iterator.GetFirstHouseId() : iterator.GetFirstHouseId(player);

				for (; id != -1; id = iterator.GetNextHouseId())
				{
					const void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
					const auto housePlayer = Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId);

					if (player == 20 && housePlayer == Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId))
						continue;

					if (Read<std::uint8_t>(condition, NC2Layout::AIConditionHouseNearPositionOffset::CompletedOnly) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3)
						continue;

					const auto houseType = Read<std::uint32_t>(condition, NC2Layout::AIConditionHouseNearPositionOffset::HouseType);

					if (houseType != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) != houseType)
						continue;

					if (Read<std::uint8_t>(condition, NC2Layout::AIConditionHouseNearPositionOffset::OpponentsOnly) != 0 && GameControl_Diplomacy_GetState(gGameControl, Read<std::int32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), housePlayer) != 3)
						continue;

					if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::uint16_t>(house, NC2Layout::HouseOffset::MapPositionX), Read<std::uint16_t>(house, NC2Layout::HouseOffset::MapPositionY), static_cast<std::uint16_t>(center), static_cast<std::uint16_t>(center >> 16))) < Read<std::uint32_t>(condition, NC2Layout::AIConditionHouseNearPositionOffset::Radius))
					{
						active = 1;
						break;
					}
				}

				break;
			}
			case SwitchIds::AICondition::ExternalFlag:
				active = Read<std::uint8_t>(gGameControl, Read<std::uint32_t>(condition, NC2Layout::AIConditionExternalFlagOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer + Read<std::uint32_t>(condition, NC2Layout::AIConditionExternalFlagOffset::SeenPlayerId) + NC2Layout::GameControlPlayerOffset::PlayerSeenFlagsUnsigned);
				break;
			case SwitchIds::AICondition::PlayerHasNoHumans:
				if (gGameTime.tick <= NC2Runtime::AIConditionLimits::PlayerDeathGraceTicks)
					return false;

				active = Read<std::uint16_t>(PlayerStatisticManager_GetCurrentStatistics(gPlayerStatisticManager, Read<std::uint32_t>(condition, NC2Layout::AIConditionPlayerHasNoHumansOffset::PlayerId)), NC2Layout::PlayerStatisticSampleOffset::HumanCount) == 0;
				break;
			case SwitchIds::AICondition::SoldierCount:
				active = Read<std::uint16_t>(PlayerStatisticManager_GetCurrentStatistics(gPlayerStatisticManager, Read<std::uint32_t>(condition, NC2Layout::AIConditionSoldierCountOffset::PlayerId)), NC2Layout::PlayerStatisticSampleOffset::SoldierCount) >= Read<std::uint32_t>(condition, NC2Layout::AIConditionSoldierCountOffset::MinimumCount);
				break;
			case SwitchIds::AICondition::ActivatedFlag:
				active = Read<std::uint8_t>(condition, NC2Layout::AIConditionActivatedFlagOffset::Flag);
				break;
			case SwitchIds::AICondition::Periodic:
				if (Read<std::uint8_t>(condition, NC2Layout::AIConditionOffset::Active) != 0)
				{
					active = Read<std::uint32_t>(condition, NC2Layout::AIConditionOffset::ActivatedTick) + Read<std::uint32_t>(condition, NC2Layout::AIConditionPeriodicOffset::ActiveTicks) > gGameTime.tick;
				}
				else if (Read<std::uint32_t>(condition, NC2Layout::AIConditionOffset::ActivatedTick) != NXBasics::IntegerRange::UInt32Maximum)
				{
					active = Read<std::uint32_t>(condition, NC2Layout::AIConditionOffset::DeactivatedTick) + Read<std::uint32_t>(condition, NC2Layout::AIConditionPeriodicOffset::InactiveTicks) < gGameTime.tick;
				}
				else
				{
					active = Read<std::uint32_t>(condition, NC2Layout::AIConditionOffset::CreatedTick) + Read<std::uint32_t>(condition, NC2Layout::AIConditionPeriodicOffset::InitialDelayTicks) < gGameTime.tick;
				}

				break;
			default:
				return false;
		}

		return AICondition_SetCondition(condition, active);
	}

	// Original function: FUN_00459290; invalid parameters create a disabled type-13 record, occupied slots reject replacement.
	bool AISinglePlayerHandler_Condition_Set(void* handler, std::uint32_t index, const void* condition)
	{
		if (index >= NC2Runtime::AIConditionLimits::Slots)
			return false;

		bool valid = true;
		const auto type = Read<std::uint32_t>(condition, NC2Layout::AIConditionOffset::Type);
		auto playerIsValid = [](std::uint32_t player)
		{

			return NXBasics::Runtime_TimeSigned32(player) < 20 && Read<std::uint8_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer) != 0;
		};

		if (type == SwitchIds::AICondition::LogicalCombination)
		{
			const auto count = Read<std::uint32_t>(condition, NC2Layout::AIConditionLogicalCombinationOffset::OperandCount);

			for (std::uint32_t operand = 0; operand < count; ++operand)
			{
				const auto reference = Read<std::uint32_t>(condition, NC2Layout::AIConditionLogicalCombinationOffset::OperandIdsUnsigned + operand * NC2Layout::RecordSize::AIConditionOperandUnsigned);

				if (reference >= NC2Runtime::AIConditionLimits::Slots && reference != NC2Runtime::AIConditionReference::AlwaysActive && reference != NC2Runtime::AIConditionReference::AlwaysInactive && reference != NC2Runtime::AIConditionReference::UnresolvedOperand)
					return false;
			}

			valid = AIConditionToConditionCheck_Tool_CheckNumberOfConditions(static_cast<const std::byte*>(condition) + NC2Layout::AIConditionOffset::Payload);
		}
		else if (type == SwitchIds::AICondition::TimeSinceConditionChange)
		{
			if (Read<std::uint32_t>(condition, NC2Layout::AIConditionTimeSinceConditionChangeOffset::ConditionId) >= NC2Runtime::AIConditionLimits::Slots)
				return false;
		}
		else if (type == SwitchIds::AICondition::Diplomacy)
		{
			// Windows still checks the second player after finding the first invalid.
			valid = playerIsValid(Read<std::uint32_t>(condition, NC2Layout::AIConditionDiplomacyOffset::FirstPlayerId));

			if (!playerIsValid(Read<std::uint32_t>(condition, NC2Layout::AIConditionDiplomacyOffset::SecondPlayerId)))
				valid = false;
		}
		else if (type == SwitchIds::AICondition::MapMoveableNearPosition || type == SwitchIds::AICondition::HouseNearPosition)
		{
			const auto player = Read<std::uint32_t>(condition, NC2Layout::AIConditionNearPositionOffset::PlayerId);

			if (player != 20)
				valid = playerIsValid(player);
		}
		else if (type == SwitchIds::AICondition::PlayerHasNoHumans)
		{
			valid = playerIsValid(Read<std::uint32_t>(condition, NC2Layout::AIConditionPlayerHasNoHumansOffset::PlayerId));
		}
		else if (type == SwitchIds::AICondition::SoldierCount)
		{
			valid = playerIsValid(Read<std::uint32_t>(condition, NC2Layout::AIConditionSoldierCountOffset::PlayerId));
		}

		void* target = static_cast<std::byte*>(handler) + index * NC2Layout::RecordSize::AIConditionUnsigned + NC2Layout::AISinglePlayerOffset::ConditionsUnsigned;

		if (Read<std::uint32_t>(target, NC2Layout::AIConditionOffset::Type) != 0)
			return false;

		if (!valid)
		{
			AICondition_Init(target, SwitchIds::AICondition::Disabled, 0);
		}
		else
		{
			// Native REP MOVSD copies forward, including when records overlap.

			for (std::uint32_t offset = 0; offset < NC2Layout::RecordSize::AICondition; offset += NC2Layout::RecordSize::AIConditionOperand)
			{
				Write(target, offset, Read<std::uint32_t>(condition, offset));
			}
		}

		if (index > Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::LastConditionIndex))
			Write(handler, NC2Layout::AISinglePlayerOffset::LastConditionIndex, index);

		return true;
	}

	// Original function: FUN_004593c6; only ExternalActivate condition records accept a flag.
	bool AISinglePlayerHandler_EntryPoint_Condition_ExternalActivate_SetFlag(void* handler, std::uint32_t index, bool active)
	{
		if (index >= NC2Runtime::AIConditionLimits::Slots || Read<std::uint32_t>(handler, index * NC2Layout::RecordSize::AIConditionUnsigned + NC2Layout::AISinglePlayerOffset::FirstConditionTypeUnsigned) != SwitchIds::AICondition::ActivatedFlag)
			return false;

		Write<std::uint8_t>(handler, index * NC2Layout::RecordSize::AIConditionUnsigned + NC2Layout::AISinglePlayerOffset::FirstActivatedConditionFlagUnsigned, active);

		return true;
	}
}