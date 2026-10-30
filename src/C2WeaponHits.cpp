#include "C2AIMovementMessages.h"
#include "C2AnimalAccess.h"
#include "C2AnimalIterator.h"
#include "C2GameControlNetwork.h"
#include "C2HappinessEvents.h"
#include "C2HouseAccess.h"
#include "C2HouseUserControl.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanWalking.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MilitaryDamage.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// The attacked-point and high-level notification bodies remain isolated link boundaries.
	extern void __fastcall AISinglePlayerHandler_Messages_Attacked(void* handler, void* unusedEdx, const void* point);
	extern void __fastcall AIHighLevelHandler_Message_Soldier_Attacked(void* handler, void* unusedEdx, const void* target);

	namespace
	{
		std::uint32_t HalfForDifficulty(std::uint32_t amount, std::uint32_t player)
		{
			if (Read<std::uint32_t>(gGameControl, NC2Layout::GameControlOffset::DifficultyLevel) != 0 && Remaster_GameControl_Player_HasDifficultyBenefits(gGameControl, player))
				return static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(amount) / 2);

			return amount;
		}

		void RecordKill(const void* attacker, std::uint32_t offset)
		{
			if (!TargetData_IsValid(attacker))
				return;

			const auto player = TargetData_GetPlayerId(attacker);

			if (player >= 20 || Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer) == 0)
			{

				return;
			}

			const auto field = static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer + offset;
			Write(gGameControl, field, Read<std::uint32_t>(gGameControl, field) + 1u);
		}

		std::uint32_t Distance(const void* source, const void* target)
		{

			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(target, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(target, NC2Layout::MapCoordinatePairOffset::Y)));
		}
	}

	// Original function: FUN_00462316.
	void AIManager_Message_Attacked(void* manager, std::uint32_t player, const void* point)
	{
		if (auto* handler = Read<void*>(manager, player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned))
			AISinglePlayerHandler_Messages_Attacked(handler, nullptr, point);
	}

	// Original function: FUN_0046232d; single-player notification precedes high-level notification.
	void AIManager_Messages_SoldierAttacked(void* manager, std::uint32_t player, std::uint32_t human, const void* target)
	{
		if (auto* handler = Read<void*>(manager, player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned))
		{
			AISinglePlayerHandler_Messages_SoldierAttacked(handler, nullptr, human, target);
			auto* high = Read<void*>(manager, player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned + NC2Layout::AIManagerOffset::HighLevelHandlersUnsigned);

			if (Read<std::uint8_t>(high, NC2Layout::AIHighLevelOffset::Enabled) != 0)
				AIHighLevelHandler_Message_Soldier_Attacked(high, nullptr, target);
		}
	}

	// Original function: FUN_0045d17e.
	bool Human_Item_IsAmuletEquipped_Defense(const void* human)
	{

		return Human_l_Item_IsGoodTypeEquipped(human, 53);
	}

	// Original function: FUN_00434781; difficulty adjustment precedes attacker bonuses for houses.
	bool House_Military_StartGettingHitByWeapon(void* house, std::uint32_t tribe, std::uint32_t weapon, std::uint32_t attackerId, std::uint32_t attackerRevision, std::uint32_t attackerType, std::uint32_t power)
	{
		const auto& data = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[tribe])[weapon];
		auto damage = static_cast<std::uint32_t>(data.damageValues[7]) * 200u / (200u - (power > 100u ? 100u : power));
		const auto player = Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId);
		damage = HalfForDifficulty(damage, player);

		if (damage == 0)
			return false;

		if (data.soundTypesHit[7] != 0)
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x17, true, data.soundTypesHit[7], Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition), 0);

		if (NXBasics::Runtime_TimeSigned32(player) < 20 && Read<std::uint32_t>(gGameControl, player * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::TypeUnsigned) == 2 && House_CanEnableDefenceMode(house))
			AIManager_Message_Attacked(gAIManager, player, static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint);

		const std::uint32_t attacker[3] = { attackerType, attackerId, attackerType == 6 ? 0u : attackerRevision };
		damage = static_cast<std::uint32_t>(Tool_Military_GetDamage(NXBasics::Runtime_TimeSigned32(damage), attacker));
		House_HitPoints_ModifyValue(house, NXBasics::Runtime_TimeSigned32(0u - damage));

		if (Read<std::int32_t>(house, NC2Layout::HouseOffset::Hitpoints) <= 0)
			RecordKill(attacker, NC2Layout::GameControlPlayerOffset::HousesDestroyed);

		HappinessManager_Messages_HouseAttacked(gHappinessManager, house);

		if (attackerType != 1)
			return true;

		const auto* human = CHumanArray::mStaticVars + attackerId * CHumanArray::RuntimeElementSize;

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) != attackerRevision)
			return false;

		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x28, true, player, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 0);
		alignas(4) std::byte message[772];
		ToUserMessage_Construct(message);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 49);
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, player);
		Write(message, NC2Layout::ToUserMessageOffset::HouseId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
		Write(message, NC2Layout::ToUserMessageOffset::HouseUniqueId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
		Write(message, NC2Layout::ToUserMessageOffset::HouseItems, Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType));
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);

		return true;
	}

	// Original function: FUN_0044272a; unlike houses, vehicles apply attacker bonuses first.
	bool Vehicle_Military_StartGettingHitByWeapon(void* vehicle, std::uint32_t tribe, std::uint32_t weapon, std::uint32_t attackerId, std::uint32_t attackerRevision, std::uint32_t attackerType, std::uint32_t power)
	{
		const auto& data = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[tribe])[weapon];
		auto damage = static_cast<std::uint32_t>(data.damageValues[6]) * 200u / (200u - (power > 100u ? 100u : power));

		if (damage == 0)
			return false;

		std::uint32_t attacker[3]{ };

		if (attackerType == 6)
		{
			std::uint32_t point;
			MapMigPoint_AssignLong(&point, attackerId);
			TargetData_SetMapPosition(attacker, &point);
		}
		else
		{
			attacker[1] = attackerId;
			attacker[2] = attackerRevision;
		}

		attacker[0] = attackerType;
		damage = static_cast<std::uint32_t>(Tool_Military_GetDamage(NXBasics::Runtime_TimeSigned32(damage), attacker));
		const auto player = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId);
		damage = HalfForDifficulty(damage, player);

		if (data.soundTypesHit[6] != 0)
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x17, true, data.soundTypesHit[6], MapMigPoint_GetLong(Vehicle_GetMapPositionRef(vehicle)), 0);

		Write(vehicle, NC2Layout::VehicleOffset::Hitpoints, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::Hitpoints) - damage);

		if (Read<std::int32_t>(vehicle, NC2Layout::VehicleOffset::Hitpoints) <= 0)
			RecordKill(attacker, NC2Layout::GameControlPlayerOffset::VehiclesDestroyed);

		if (attackerType != 1)
			return true;

		const auto* human = CHumanArray::mStaticVars + attackerId * CHumanArray::RuntimeElementSize;

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) != attackerRevision)
			return false;

		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x28, true, player, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 0);
		alignas(4) std::byte message[772];
		ToUserMessage_Construct(message);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 52);
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, player);
		Write(message, NC2Layout::ToUserMessageOffset::VehicleId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
		Write(message, NC2Layout::ToUserMessageOffset::VehicleUniqueId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
		Write(message, NC2Layout::ToUserMessageOffset::EquipmentItems, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType));
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);

		return true;
	}

	// Original function: FUN_00443ee9; zero-damage notifications retain native propagation and RNG order.
	bool Human_Military_StartGettingHitByWeapon(void* human, const void* source, std::uint32_t tribe, std::uint32_t weapon, std::uint32_t attackerId, std::uint32_t attackerRevision, std::uint32_t attackerType, std::uint32_t power, bool propagate)
	{
		const auto& armor = static_cast<const SStaticArmorTypeData*>(gStaticArmorTypeData)[Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedArmor)];
		const auto& data = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[tribe])[weapon];
		auto damage = static_cast<std::uint32_t>(data.damageValues[armor.materialType]) * 200u / (200u - (power > 100u ? 100u : power));
		const auto* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
		auto facing = Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0 ? Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::Direction) : 2u;

		if (facing == 7)
		{
			facing = 2;
		}
		else if (facing == 6)
		{
			facing = 5;
		}

		const auto direction = VE_HexagonDirection_GetDirectionFromPositions(Read<std::int16_t>(moveable, NC2Layout::MapMoveableOffset::MapPosition), Read<std::int16_t>(moveable, NC2Layout::MapMoveableOffset::MapPositionY), Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(source, NC2Layout::MapCoordinatePairOffset::Y), false);
		auto difference = NXBasics::Runtime_TimeSigned32(direction - facing);

		if (difference < 0)
			difference = -difference;

		if (difference > 2)
		{
			damage = damage * 150u / 100u;
		}
		else if (difference > 1)
		{
			damage = damage * 125u / 100u;
		}

		damage -= static_cast<std::uint32_t>(armor.blockingValue);
		const std::uint32_t attacker[3] = { attackerType, attackerId, attackerType == 6 ? 0u : attackerRevision };
		damage = static_cast<std::uint32_t>(Tool_Military_GetDamage(NXBasics::Runtime_TimeSigned32(damage), attacker));

		if (Human_Item_IsAmuletEquipped_Defense(human))
			damage = static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(damage) / 2);

		const auto player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
		damage = HalfForDifficulty(damage, player);

		if (NXBasics::Runtime_TimeSigned32(damage) > 0)
		{
			if (NXBasics::Runtime_TimeSigned32(0u - damage) > 0 || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 8u) == 0)
				Write(human, NC2Layout::HumanOffset::Hitpoints, Read<std::uint32_t>(human, NC2Layout::HumanOffset::Hitpoints) - damage);

			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints) <= 0 && Human_l_Item_GetItemSlotIndex(human, 5) == -1)
				RecordKill(attacker, Human_IsJobSoldier(human) ? NC2Layout::GameControlPlayerOffset::SoldiersKilledUnsigned : NC2Layout::GameControlPlayerOffset::CiviliansKilledUnsigned);
			// Windows 0041AE3E initializes the two positions only; other fields are assigned by this caller.
			// The final borrowed human pointer uses native storage; the preceding six DWORDs stay fixed-width.
			alignas(void*) std::byte effect[NC2Layout::DirectionalParticleCallbackOffset::Human + sizeof(human)];
			Write(effect, NC2Layout::DirectionalParticleCallbackOffset::MapPoint, MapMigPoint_GetLong(Human_GetMapPositionRef(human)));
			Write(effect, NC2Layout::DirectionalParticleCallbackOffset::DirectionPoint, MapMigPoint_GetLong(source));
			Write(effect, NC2Layout::DirectionalParticleCallbackOffset::PlayerId, player);
			Write(effect, NC2Layout::DirectionalParticleCallbackOffset::Human, human);
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x27, true, reinterpret_cast<std::uintptr_t>(effect), damage, 0);

			if (data.soundTypesHit[armor.materialType] != 0)
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x17, true, data.soundTypesHit[armor.materialType], MapMigPoint_GetLong(Human_GetMapPositionRef(human)), 1);

			if (TargetData_IsValid(attacker))
			{
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x28, true, player, TargetData_GetPlayerId(attacker), 0);
				Human_UserMessage_SendToUserMessage(human, 46);
			}

			if (Human_GetPlayerType(human) == 2)
				AIManager_Message_Attacked(gAIManager, player, Human_GetMapPositionRef(human));
		}

		if (!TargetData_IsValid(attacker))
			return false;

		std::uint32_t attackerPosition = 0, previousPosition = 10000u | (10000u << 16);
		TargetData_GetMapPosition(attacker, &attackerPosition);

		if ((Human_IsJobSoldier(human) || Human_IsJobHero(human)) && TargetData_GetPlayerId(attacker) != NXBasics::Runtime_TimeSigned32(player))
		{
			auto allowed = NXBasics::Runtime_TimeSigned32(Human_GetMilitaryMode(human)) > 0 && NXBasics::Runtime_TimeSigned32(Human_GetMilitaryMode(human)) <= 2 && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 4u) == 0;

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 4 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapPositionReachedCount) < 10u)
				allowed = false;

			const auto enemy = TargetData_GetPlayerId(attacker);

			if (enemy < 20 && Read<std::uint8_t>(gGameControl, static_cast<std::uint32_t>(enemy) * NC2Layout::RecordSize::GameControlPlayer) != 0 && GameControl_Diplomacy_GetState(gGameControl, enemy, player) != 3 && GameControl_Diplomacy_GetState(gGameControl, player, enemy) != 3)
				allowed = false;

			auto* secondary = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::SecondaryMilitaryTarget;

			if (allowed && TargetData_IsValid(secondary))
			{
				TargetData_GetMapPosition(secondary, &previousPosition);

				if (Distance(Human_GetMapPositionRef(human), &previousPosition) <= Distance(Human_GetMapPositionRef(human), &attackerPosition))
					allowed = false;
			}
			else if (allowed && Human_GetPlayerType(human) == 2)
			{
				AIManager_Messages_SoldierAttacked(gAIManager, player, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), attacker);
			}
			// Native comparison deliberately checks type/revision without comparing the object id.

			if (allowed && !(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTargetRevision) == attacker[2] && Read<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget) == attacker[0]))
			{
				for (std::uint32_t index = 0; index < 3; ++index)
				{
					Write(secondary, index * NC2Layout::RecordSize::MilitaryTargetValueUnsigned, attacker[index]);
				}

				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 14)
					Human_DoAddNewUserCommand(human, 48, static_cast<std::uint8_t>(player), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
			}
		}
		else if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 4u) == 0 && !TargetData_IsValid(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTarget) && !Human_IsDefendingHouse(human, -1) && Tool_Misc_GetFleePointFarestAwayFromTargetPointRelativToSourcePoint(Human_GetMapPositionRef(human), &attackerPosition, &previousPosition, 10))
		{
			alignas(4) std::byte command[20]{ };
			Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 4);
			Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(player));
			Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, previousPosition);
			Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter2, propagate ? 0u : 1u);
			Human_DoAddNewUserCommand(human, command);
		}

		if (propagate && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit15) == 0)
		{
			CHumanIterator humans;

			for (std::uint32_t group = 0; group < 2; ++group)
			{
				for (auto id = humans.GetFirstSectorHumanId(player, MapMigPoint_GetLong(Human_GetMapPositionRef(human)), group == 0 ? 40 : 20, group == 0); id != -1; id = humans.GetNextSectorHumanId())
				{
					if (static_cast<std::uint32_t>(id) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId))
						Human_Military_StartGettingHitByWeapon(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize, Human_GetMapPositionRef(human), tribe, 0, attackerId, attackerRevision, attackerType, 0, false);
				}
			}

			CAnimalIterator animals;

			for (std::uint32_t group = 0; group < 2; ++group)
			{
				for (auto id = animals.GetFirstSectorAnimalId(player, MapMigPoint_GetLong(Human_GetMapPositionRef(human)), group == 0 ? 40 : 20, true, false); id != -1; id = animals.GetNextSectorAnimalId())
				{
					Animal_Military_StartGettingHitByWeapon(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::RuntimeElementSize, tribe, 0, attackerId, attackerRevision, attackerType, 0, false);
				}
			}
		}

		return NXBasics::Runtime_TimeSigned32(damage) > 0;
	}

	// Original function: FUN_00443108; the commander gains category-specific weapon experience.
	void Vehicle_Military_GainExperience(const void* vehicle)
	{
		if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0)
			Human_GainExperienceForWeapon(Vehicle_Passengers_GetAttachedCommanderPtr(vehicle), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType) == 5 ? 21u : 0u, 1);
	}
}