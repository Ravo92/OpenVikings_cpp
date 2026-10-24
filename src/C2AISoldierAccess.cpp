#include "RemasterAILog.h"
#include "C2AIHumanRequests.h"
#include "C2AIMilitaryAccess.h"
#include "C2AIMovementMessages.h"
#include "C2AISoldierAccess.h"
#include "C2AITaskAccess.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanWalking.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapPathfinderSearch.h"
#include "C2MilitaryTargetSearch.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "C2VehicleIterator.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"
#include "RemasterAIEconomy.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		std::byte* Soldier(void* handler, std::uint32_t i)
		{
			return static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::Soldiers + i * NC2Layout::RecordSize::AISoldier;
		}
		std::byte* Human(std::uint32_t id)
		{
			return CHumanArray::mStaticVars + id * CHumanArray::ElementSize;
		}
		void CountWeapon(void* handler, const void* human)
		{
			const auto* weapon = static_cast<const std::byte*>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)]) + Human_Equipment_GetWeaponType(human) * 0xC4;
			const auto offset = NC2Layout::AISinglePlayerOffset::WeaponCategoryCounts + Read<std::uint32_t>(weapon, NC2Layout::StaticWeaponTypeOffset::MainType) * 4;
			Write(handler, offset, Read<std::uint32_t>(handler, offset) + 1);
		}
		void RefreshNeeds(void* human)
		{
			// Windows constant region 004F3B38..004F3B77: maxima 10000, critical levels 1000.
			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) < 1000) Write<std::int32_t>(human, NC2Layout::HumanOffset::Food, 10000);
			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Stamina) < 1000) Write<std::int32_t>(human, NC2Layout::HumanOffset::Stamina, 10000);
		}
		struct SRemasterAttackApproach
		{
			std::uint32_t target, radius, point;
		};
		bool __cdecl slRemasterReachableAttackApproach(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& approach = *static_cast<SRemasterAttackApproach*>(callback->userData);
			if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0) return false;
			if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(&approach.target, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&approach.target, NC2Layout::MapCoordinatePairOffset::Y))) > approach.radius) return false;
			approach.point = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);
			return true;
		}
	}
	// Original function: FUN_00472c02.
	void AISingleSoldier_Init(void* soldier, std::uint32_t id)
	{
		NXBasics::Runtime_FillMemory(soldier, 0, NC2Layout::RecordSize::AISoldier);
		const auto* human = Human(id);
		Write(soldier, NC2Layout::AISoldierOffset::HumanId, id);
		Write(soldier, NC2Layout::AISoldierOffset::HumanUniqueId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription, 0);
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
		Write(soldier, NC2Layout::AISoldierOffset::WeaponType, Human_Equipment_GetWeaponType(human));
		Write(soldier, NC2Layout::AISoldierOffset::ArmorType, Human_Equipment_GetArmorType(human));
		Write<std::int32_t>(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex, -1);
	}
	// Original function: FUN_00472c55.
	void AISingleSoldier_Tool_StopVAAndVCTask(void* soldier)
	{
		auto* human = Human(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId));
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription, 0);
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
		const auto* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveableSigned;
		std::uint32_t point = 0;
		MapMoveable_Attachment_GetNextMapPosition(moveable, &point);
		Human_DoAddNewUserCommand(human, 4, Read<std::uint8_t>(human, NC2Layout::HumanOffset::PlayerId), 0, 0, point, 0);
		Write<std::int32_t>(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex, -1);
		Write<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::UsesDefaultTask, 0);
	}
	// Original function: FUN_004593ed.
	void AISinglePlayerHandler_l_SingleSoldiers_RecreateListCompletly(void* handler)
	{
		if (RemasterAIEconomy::IsActive(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId)))
		{
			// Existing valid records contain the saved membership and equipment reservations of a wave.
			AISinglePlayerHandler_l_SingleSoldiers_UpdateList(handler); return;
		}
		NXBasics::Runtime_FillMemory(static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::SoldierCount, 0, 0x41A4);
		NXBasics::Runtime_FillMemory(static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::SoldierSelfDefenceOrigins, 0, 0x320);
		AISinglePlayerHandler_l_SingleSoldiers_UpdateList(handler);
	}
	// Original function: FUN_00459422.
	void AISinglePlayerHandler_l_SingleSoldiers_UpdateList(void* handler)
	{
		NXBasics::Runtime_FillMemory(static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::WeaponCategoryCounts, 0, 32);
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount);)
		{
			auto* soldier = Soldier(handler, i); const auto* human = Human(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId));
			const bool valid = Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) != 0 && (Human_IsJobSoldier(human) || Human_IsJobHero(human)) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) == Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanUniqueId) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) == Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId) && !Human_IsAttachedToWorkHouse(human) && !Human_IsAttachedToVehicle(human) && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 32u) == 0;
			if (valid)
			{
				CountWeapon(handler, human); ++i; continue;
			}
			const auto remaining = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount) - i - 1;
			if (remaining != 0)
			{
				NXBasics::Runtime_MoveMemory(soldier, soldier + NC2Layout::RecordSize::AISoldier, remaining * NC2Layout::RecordSize::AISoldier);
				auto* point = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::SoldierSelfDefenceOrigins + i * 4;
				NXBasics::Runtime_MoveMemory(point, point + 4, remaining * 4);
			}
			Write(handler, NC2Layout::AISinglePlayerOffset::SoldierCount, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount) - 1);
		}
		CHumanIterator iterator;
		for (auto id = iterator.GetFirstSectorHumanId(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), true); id != -1; id = iterator.GetNextSectorHumanId())
		{
			auto count = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount);
			if (count >= 200) return;
			auto* human = Human(id);
			if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 32u) != 0 || Human_IsAttachedToWorkHouse(human) || Human_IsAttachedToVehicle(human) || AISinglePlayerHandler_Misc_Human_CheckRequestsForCreature(handler, id)) continue;
			std::uint32_t i = 0;
			while (i < count && Read<std::uint32_t>(Soldier(handler, i), NC2Layout::AISoldierOffset::HumanId) != static_cast<std::uint32_t>(id)) ++i;
			if (i != count) continue;
			if (Human_IsJobSoldier(human))
			{
				std::byte command[20]{ };
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 41);
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, Read<std::uint8_t>(human, NC2Layout::HumanOffset::PlayerId));
				Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1, 3);
				Human_DoAddNewUserCommand(human, command);
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 42); Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1, 0); Human_DoAddNewUserCommand(human, command);
			}
			AISingleSoldier_Init(Soldier(handler, count), id);
			CountWeapon(handler, human); Write(handler, NC2Layout::AISinglePlayerOffset::SoldierCount, count + 1);
		}
	}
	// Original function: FUN_0045969f.
	void AISinglePlayerHandler_l_SingleSoldiers_ClearInvalidTask(void* handler)
	{
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount); ++i)
		{
			auto* soldier = Soldier(handler, i);
			if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription) == 0 || Read<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::UsesDefaultTask) != 0) continue;
			const auto count = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::CombinedTaskCount);
			std::uint32_t task = 0;
			for (; task < count; ++task)
			{
				const auto main = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::FirstCombinedTaskMainIndices + task * NC2Layout::RecordSize::AICombinedTask);
				if (AITaskSimpleDescription_IsEqualTo(static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::FirstMainTaskDescription + main * NC2Layout::RecordSize::AIMainTask, soldier + NC2Layout::AISoldierOffset::TaskDescription))
				{
					Write(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex, task); break;
				}
			}
			if (task == count || (Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::CombinedTasks + task * NC2Layout::RecordSize::AICombinedTask) == 2 && Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::FirstCombinedTaskPhase + task * NC2Layout::RecordSize::AICombinedTask) == 0)) AISingleSoldier_Tool_StopVAAndVCTask(soldier);
		}
	}
	// Original function: FUN_0045aac6.
	bool AISinglePlayerHandler_l_SingleSoldiers_Let_GotoPosition(void* handler, std::uint32_t slot, const void* destination, std::uint32_t radius)
	{
		auto* soldier = Soldier(handler, slot); auto* human = Human(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId));
		auto point = Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates);
		if (!Tool_Map_GetNotBlockedPosition(&point, Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription) == 1)) return false;
		const auto* position = Human_GetMapPositionRef(human);
		bool reachableApproach = false;
		if (radius != 0 && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription) == 2 && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::Moving)
		{
			// Retry within the task's accepted radius instead of another isolated free tile.
			SRemasterAttackApproach approach{ Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates), radius, point };
			if (CMapPathfinderElement::SearchOverMap(position, NC2Runtime::PathfinderSearchLimits::MaximumRadiusUnsigned, true, slRemasterReachableAttackApproach, &approach, false, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId)))
			{
				point = approach.point;
				reachableApproach = true;
			}
		}
		const void* distanceTarget = reachableApproach ? destination : &point;
		if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(position[0], position[1], Read<std::int16_t>(distanceTarget, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(distanceTarget, NC2Layout::MapCoordinatePairOffset::Y))) >= radius)
		{
			Human_DoAddNewUserCommand(human, 4, Read<std::uint8_t>(human, NC2Layout::HumanOffset::PlayerId), 0, 0, MapMigPoint_GetLong(&point), 0);
			Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Moving);
			Write(soldier, NC2Layout::AISoldierOffset::MovementDestination, Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates));
			Write(soldier, NC2Layout::AISoldierOffset::MovementRadius, radius);
		}
		else Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
		return true;
	}
	// Original function: FUN_0045aee6.
	void AISinglePlayerHandler_l_SingleSoldiers_RefreshAllEnergyAndCondition(void* handler)
	{
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
		CHumanIterator humans;
		for (auto id = humans.GetFirstSectorHumanId(player, true); id != -1; id = humans.GetNextSectorHumanId()) RefreshNeeds(Human(id));
		CVehicleIterator vehicles;
		for (auto id = vehicles.GetFirstSectorVehicleId(player, 0, 10000, true); id != -1; id = vehicles.GetNextSectorVehicleId())
		{
			auto* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::ElementSize;
			if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0) RefreshNeeds(Vehicle_Passengers_GetAttachedCommanderPtr(vehicle));
		}
	}
	// Original function: FUN_00473377.
	void AISinglePlayerHandler_l_Vehicles_RecreateListCompletly(void* handler)
	{
		Write<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::VehicleCount, 0);
		AISinglePlayerHandler_l_Vehicles_UpdateList(handler);
	}
	// Original function: FUN_00473383.
	void AISinglePlayerHandler_l_Vehicles_UpdateList(void* handler)
	{
		auto* entries = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::Vehicles;
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::VehicleCount);)
		{
			auto* entry = entries + i * NC2Layout::RecordSize::AIVehicle;
			const auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::VehicleId) * CVehicleArray::ElementSize;
			if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId) == Read<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::VehicleUniqueId) && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId) == Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId))
			{
				++i; continue;
			}
			const auto remaining = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::VehicleCount) - i - 1;
			if (remaining != 0) NXBasics::Runtime_MoveMemory(entry, entry + 28, remaining * NC2Layout::RecordSize::AIVehicle);
			Write(handler, NC2Layout::AISinglePlayerOffset::VehicleCount, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::VehicleCount) - 1);
		}
		CVehicleIterator iterator;
		for (auto id = iterator.GetFirstSectorVehicleId(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), 0, 10000, true); id != -1; id = iterator.GetNextSectorVehicleId())
		{
			const auto count = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::VehicleCount);
			std::uint32_t i = 0;
			while (i < count && Read<std::uint32_t>(entries + i * NC2Layout::RecordSize::AIVehicle, NC2Layout::AIVehicleOffset::VehicleId) != static_cast<std::uint32_t>(id)) ++i;
			if (i != count) continue;
			auto* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::ElementSize;
			std::byte command[20]{ };
			Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 111);
			Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::PlayerId));
			Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
			Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
			Write<std::uint32_t>(command, NC2Layout::UserNetworkCommandOffset::Parameter1, 3);
			Vehicle_DoAddNewUserCommand(vehicle, command);
			auto* entry = entries + count * NC2Layout::RecordSize::AIVehicle;
			Write(entry, NC2Layout::AIVehicleOffset::VehicleId, id);
			Write(entry, NC2Layout::AIVehicleOffset::VehicleUniqueId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
			Write<std::int32_t>(entry, NC2Layout::AIVehicleOffset::TaskIndex, -1);
			Write<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::AttackActive, 0);
			Write(handler, NC2Layout::AISinglePlayerOffset::VehicleCount, count + 1); if (count + 1 == 30) return;
		}
	}
	// Original function: FUN_004734de.
	void AISinglePlayerHandler_l_Vehicles_CheckForCommander(void* handler)
	{
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::VehicleCount); ++i)
		{
			auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::Vehicles + i * NC2Layout::RecordSize::AIVehicle) * CVehicleArray::ElementSize;
			if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0) continue;
			auto* human = AISinglePlayerHandler_l_SingleSoldier_GetBestSoldier(handler, Vehicle_GetMapPositionRef(vehicle), 20);
			if (human != nullptr) Human_DoAddNewUserCommand(human, 38, Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
		}
	}

	namespace
	{
		std::uint32_t Distance(const void* a, const void* b)
		{
			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::Y)));
		}
	}
	// Original function: FUN_0045a832.
	bool AISinglePlayerHandler_l_SingleSoldiers_CheckAttack(void* handler, std::uint32_t slot, const void* point, std::uint32_t radius, bool houses, bool civilians, bool restrictCandidates, bool defence)
	{
		const auto* human = Human(Read<std::uint32_t>(Soldier(handler, slot), NC2Layout::AISoldierOffset::HumanId));
		const auto* weapon = static_cast<const std::byte*>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)]) + Human_Equipment_GetWeaponType(human) * 0xC4;
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
		const auto limit = restrictCandidates ? 1u : 5u, distance = restrictCandidates ? 0u : 3u;
		const auto attackMoveable = [&] (bool humans, bool secondList)
		{
			const auto id = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, 0, radius, secondList, limit, distance, humans, false);
			if (id == -1) return false;
			return humans ? AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHuman(handler, slot, id, defence) : AISinglePlayerHandler_l_SingleSoldiers_Let_AttackVehicle(handler, slot, id, defence);
		};
		if (Read<std::uint8_t>(weapon, NC2Layout::StaticWeaponTypeOffset::HasMunition) != 0)
		{
			if (attackMoveable(false, true) || attackMoveable(true, true)) return true;
		}
		else if (attackMoveable(true, true) || attackMoveable(false, true)) return true;
		if (houses)
		{
			const auto id = Tool_GetOneOfNextEnemyHousesInRange(player, point, 0, radius, true, restrictCandidates ? 1u : 3u, restrictCandidates ? 0u : 5u);
			if (id != -1 && AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHouse(handler, slot, id, defence)) return true;
		}
		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit13Signed) == 0)
		{
			const auto id = Tool_Animals_GetOneOfNextEnemyInRange(player, point, 0, radius, true, false, limit, distance);
			if (id != -1 && AISinglePlayerHandler_l_SingleSoldiers_Let_AttackAnimal(handler, slot, id, defence)) return true;
		}
		if (civilians && (attackMoveable(true, false) || attackMoveable(false, false))) return true;
		if (houses && civilians)
		{
			const auto id = Tool_GetOneOfNextEnemyHousesInRange(player, point, 0, radius, false, limit, restrictCandidates ? 0u : 10u);
			if (id != -1 && AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHouse(handler, slot, id, defence)) return true;
		}
		return false;
	}
	// Original function: FUN_00459765.
	void AISinglePlayerHandler_l_SingleSoldier_CheckFormOrAttack(void* handler, std::uint32_t taskIndex)
	{
		auto* task = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasks + taskIndex * NC2Layout::RecordSize::AICombinedTask;
		std::uint32_t target = 0;
		AITaskSimpleDescription_Tool_GetMapPosition(static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::FirstMainTaskDescription + Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskIndices) * NC2Layout::RecordSize::AIMainTask, &target);
		if (Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::TaskType) != 2) return;
		const auto allocated = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::AllocatedSoldierCounts + taskIndex * NC2Layout::RecordSize::AITaskIndexValue);
		if (Remaster_AIMilitary_IsWaveTask(handler, task))
		{
			// Launch once; separation en route must never send the expedition back home.
			if (Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase) == 2) return;
			const auto wanted = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers);
			if (allocated < wanted) { RemasterAILog::Event(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), "wave_wait", static_cast<std::int32_t>(taskIndex), "allocated=%u wanted=%u reason=missing_soldiers", allocated, wanted); return; }
			std::uint32_t gathered = 0, archers = 0;
			for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount); ++i)
			{
				const auto* soldier = Soldier(handler, i);
				if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierAssignedFlags + i) == 0 || Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex) != taskIndex) continue;
				const auto* human = Human(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId));
				if (!Human_HasEquippedWeapon(human) || Distance(Human_GetMapPositionRef(human), task + NC2Layout::AICombinedTaskOffset::FormationPosition) > 8) continue;
				++gathered; const auto weapon = Human_Equipment_GetWeaponType(human);
				if (weapon == 16 || weapon == 17) ++archers;
			}
			RemasterAILog::Event(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), "wave_gathering", static_cast<std::int32_t>(taskIndex), "allocated=%u wanted=%u gathered=%u required_gathered=%u archers=%u", allocated, wanted, gathered, (wanted * 4 + 4) / 5, archers);
			if (gathered >= (wanted * 4 + 4) / 5 && archers >= Remaster_AIMilitary_WantedArchers(handler, wanted))
			{
				Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase, 2);
				Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers, 1);
			}
			return;
		}
		std::uint32_t minimum, spread;
		if (Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase) == 2)
		{
			minimum = allocated / 3; spread = allocated * 3;
		}
		else
		{
			minimum = allocated >> 1; spread = allocated * 2;
			Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase, 2);
		}
		if (allocated <= 1) return;
		std::uint32_t nearTarget = 0;
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount); ++i)
		{
			if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierAssignedFlags + i) == 0 || Read<std::uint32_t>(Soldier(handler, i), NC2Layout::AISoldierOffset::CombinedTaskIndex) != taskIndex) continue;
			if (Distance(Human_GetMapPositionRef(Human(Read<std::uint32_t>(Soldier(handler, i), NC2Layout::AISoldierOffset::HumanId))), &target) <= Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::Radius)) ++nearTarget;
		}
		if (nearTarget >= minimum) return;
		std::uint32_t center = 0, nearest = 10000;
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount); ++i)
		{
			auto* soldier = Soldier(handler, i);
			if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierAssignedFlags + i) == 0 || Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex) != taskIndex) continue;
			const auto* position = Human_GetMapPositionRef(Human(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId)));
			const auto distance = Distance(soldier + NC2Layout::AISoldierOffset::TaskMapPoint, position);
			if (distance < nearest)
			{
				center = Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates); nearest = distance;
			}
		}
		std::uint32_t together = 0;
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount); ++i)
		{
			auto* soldier = Soldier(handler, i);
			if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierAssignedFlags + i) == 0 || Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex) != taskIndex) continue;
			if (Distance(Human_GetMapPositionRef(Human(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId))), &center) <= spread) ++together;
		}
		if (together < minimum)
		{
			Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase, 1);
			Write(task, NC2Layout::AICombinedTaskOffset::FormationRadius, allocated / 3);
		}
	}
	// Original function: FUN_0045998e.
	void AISinglePlayerHandler_l_SingleSoldiers_UpdateAttachedTasks(void* handler)
	{
		const auto soldiers = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount);
		if (soldiers == 0) return;
		const bool remaster = RemasterAIEconomy::IsActive(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId));
		NXBasics::Runtime_FillMemory(static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::TaskAllocationEligible, 0, 0xA00);
		const auto tasks = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::CombinedTaskCount);
		for (std::uint32_t i = 0; i < tasks; ++i)
		{
			auto* task = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasks + i * NC2Layout::RecordSize::AICombinedTask;
			if (!Remaster_AIMilitary_IsWaveTask(handler, task)) continue;
			Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers, 1);
			if (Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase) != 2) continue;
			const auto* description = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::FirstMainTaskDescription + Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskIndices) * NC2Layout::RecordSize::AIMainTask;
			bool survivors = false;
			for (std::uint32_t j = 0; j < soldiers; ++j)
				if (Human_HasEquippedWeapon(Human(Read<std::uint32_t>(Soldier(handler, j), NC2Layout::AISoldierOffset::HumanId))) && AITaskSimpleDescription_IsEqualTo(Soldier(handler, j) + NC2Layout::AISoldierOffset::TaskDescription, description)) { survivors = true; break; }
			if (!survivors)
			{
				const auto size = RemasterAIEconomy::WaveSize(RemasterAIEconomy::Inspect(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId)).population);
				Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase, 1);
				Write<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers, 1);
				Write(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers, size);
				for (std::uint32_t j = 0; j < Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskCount); ++j)
				{
					auto* main = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::MainTasks + Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskIndices + j * 4) * NC2Layout::RecordSize::AIMainTask;
					Write(main, NC2Layout::AIMainTaskOffset::MinimumSoldiers, size);
					Write(main, NC2Layout::AIMainTaskOffset::MaximumSoldiers, size);
				}
			}
		}
		double remaining = static_cast<double>(soldiers);
		const auto combined = [&] (std::uint32_t i)
		{
			return static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasks + i * NC2Layout::RecordSize::AICombinedTask;
		};
		const auto round = [] (double value)
		{
			if (!(value >= 0.0)) value = 0.0; return static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(value + 0.5));
		};
		const auto candidates = [&] (const void* description, const void* point)
		{
			const void* wave = nullptr;
			for (std::uint32_t i = 0; i < tasks; ++i)
				if (description == static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::FirstMainTaskDescription + Read<std::uint32_t>(combined(i), NC2Layout::AICombinedTaskOffset::MainTaskIndices) * NC2Layout::RecordSize::AIMainTask && Remaster_AIMilitary_IsWaveTask(handler, combined(i))) { wave = combined(i); break; }
			Write<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateCount, 0);
			for (std::uint32_t i = 0; i < soldiers; ++i)
			{
				if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierAssignedFlags + i) != 0) continue;
				const auto* soldier = Soldier(handler, i); const auto* human = Human(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId));
				if (Human_IsJobHero(human) && Read<std::uint32_t>(description, NC2Layout::AITaskDescriptionOffset::Type) == 2) continue;
				if (remaster && (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::ChangingEquipment || Human_UserControl_IsCommandTypeInQueue(human, 81) || Human_UserControl_IsCommandTypeInQueue(human, 82))) continue;
				if (remaster && Read<std::uint32_t>(description, NC2Layout::AITaskDescriptionOffset::Type) == 2 && !Human_HasEquippedWeapon(human)) continue;
				if (wave != nullptr && Read<std::uint32_t>(wave, NC2Layout::AICombinedTaskOffset::AttackPhase) == 2 && !AITaskSimpleDescription_IsEqualTo(soldier + NC2Layout::AISoldierOffset::TaskDescription, description)) continue;
				std::uint32_t position = 0; Human_GetMapPosition(human, &position);
				auto score = Distance(point, &position);
				if (!AITaskSimpleDescription_IsEqualTo(static_cast<const std::byte*>(soldier) + NC2Layout::AISoldierOffset::TaskDescription, description)) score += 10000;
				if (!remaster && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::ArmorType) == 0) score += 10000;
				if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::WeaponType) == 0) score += 10000;
				const auto count = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateCount);
				std::uint32_t at = 0;
				while (at < count&& Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateScores + at * NC2Layout::RecordSize::AISoldierScoreEntry) <= score) ++at;
				for (std::uint32_t end = count; end > at; --end)
				{
					Write(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateSlots + end * NC2Layout::RecordSize::AISoldierScoreEntry, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateSlots + (end - 1) * NC2Layout::RecordSize::AISoldierScoreEntry));
					Write(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateScores + end * NC2Layout::RecordSize::AISoldierScoreEntry, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateScores + (end - 1) * NC2Layout::RecordSize::AISoldierScoreEntry));
				}
				Write(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateSlots + at * NC2Layout::RecordSize::AISoldierScoreEntry, i);
				Write(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateScores + at * NC2Layout::RecordSize::AISoldierScoreEntry, score);
				Write(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateCount, count + 1);
			}
			if (wave != nullptr && Read<std::uint32_t>(wave, NC2Layout::AICombinedTaskOffset::AttackPhase) == 1)
			{
				// Select the same 60:40 mix even when all nearby soldiers happen to be melee fighters.
				const auto count = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateCount);
				std::uint32_t ordered[200]{}; bool used[200]{}; std::uint32_t archers = 0;
				for (std::uint32_t i = 0; i < count; ++i)
				{
					std::uint32_t selected = count;
					const bool wantBow = archers < Remaster_AIMilitary_WantedArchers(handler, i + 1);
					for (std::uint32_t j = 0; j < count; ++j)
					{
						if (used[j]) continue;
						const auto slot = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateSlots + j * NC2Layout::RecordSize::AISoldierScoreEntry);
						const auto weapon = Human_Equipment_GetWeaponType(Human(Read<std::uint32_t>(Soldier(handler, slot), NC2Layout::AISoldierOffset::HumanId)));
						if (selected == count) selected = j;
						if ((weapon == 16 || weapon == 17) == wantBow) { selected = j; break; }
					}
					used[selected] = true; ordered[i] = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateSlots + selected * NC2Layout::RecordSize::AISoldierScoreEntry);
					const auto weapon = Human_Equipment_GetWeaponType(Human(Read<std::uint32_t>(Soldier(handler, ordered[i]), NC2Layout::AISoldierOffset::HumanId)));
					if (weapon == 16 || weapon == 17) ++archers;
				}
				for (std::uint32_t i = 0; i < count; ++i) Write(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateSlots + i * NC2Layout::RecordSize::AISoldierScoreEntry, ordered[i]);
			}
		};
		std::byte firstCommand[20]{ }; Write<std::uint8_t>(firstCommand, NC2Layout::UserNetworkCommandOffset::Opcode, 41);
		std::byte secondCommand[20]{ }; Write<std::uint8_t>(secondCommand, NC2Layout::UserNetworkCommandOffset::Opcode, 41);
		const auto assign = [&] (std::uint32_t taskId, const void* description, std::uint32_t count, void* command)
		{
			auto* task = combined(taskId);
			const auto available = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateCount);
			if (count > available) count = available;
			Write(handler, NC2Layout::AISinglePlayerOffset::AllocatedSoldierCounts + taskId * NC2Layout::RecordSize::AITaskIndexValue, count);
			for (std::uint32_t i = 0; i < count; ++i)
			{
				const auto slot = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateSlots + i * NC2Layout::RecordSize::AISoldierScoreEntry); auto* soldier = Soldier(handler, slot);
				if (!AITaskSimpleDescription_IsEqualTo(soldier + NC2Layout::AISoldierOffset::TaskDescription, description))
				{
					NXBasics::Runtime_CopyMemory(soldier + NC2Layout::AISoldierOffset::TaskDescription, description, NC2Layout::RecordSize::AITaskDescription);
					const auto type = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription);
					if (type == 2) Write(soldier, NC2Layout::AISoldierOffset::TaskRadius, Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::Radius));
					if (type == 1 || type == 2)
					{
						auto* human = Human(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId));
						Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, Read<std::uint8_t>(human, NC2Layout::HumanOffset::PlayerId));
						Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, type == 1 ? 2u : Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskMilitaryMode));
						Human_DoAddNewUserCommand(human, command);
					}
					Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
					Write(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex, taskId);
				}
				Write<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::UsesDefaultTask, 0);
				Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierAssignedFlags + slot, 1);
			}
			if (Remaster_AIMilitary_IsWaveTask(handler, task) && Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase) == 1)
				for (std::uint32_t i = 0; i < soldiers; ++i)
				{
					auto* soldier = Soldier(handler, i);
					if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierAssignedFlags + i) != 0 || !AITaskSimpleDescription_IsEqualTo(soldier + NC2Layout::AISoldierOffset::TaskDescription, description)) continue;
					// A previous gathering selection is not automatically a member of the launched wave.
					if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::ChangingEquipment)
					{
						Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription, 0);
						Write<std::int32_t>(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex, -1);
					}
					else AISingleSoldier_Tool_StopVAAndVCTask(soldier);
				}
			AISinglePlayerHandler_l_SingleSoldier_CheckFormOrAttack(handler, taskId);
		};
		for (;;)
		{
			double weight = 0.0;
			for (std::uint32_t i = 0; i < tasks; ++i)
			{
				const auto* task = combined(i);
				if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationFinished + i) != 0) continue;
				if (Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::TaskType) != 2 && (Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers) == 0 || Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers) == 0)) continue;
				weight += Read<double>(task, NC2Layout::AICombinedTaskOffset::PriorityPercent);
				Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationWeighted + i, 1);
			}
			if (weight == 0.0) break;
			bool allocated = false;
			for (std::uint32_t i = 0; i < tasks; ++i)
			{
				if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationFinished + i) != 0 || Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationWeighted + i) == 0) continue;
				const auto* task = combined(i); auto count = round(static_cast<double>(soldiers) * Read<double>(task, NC2Layout::AICombinedTaskOffset::PriorityPercent) / weight);
				const auto maximum = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers);
				if (maximum != 0 && count > maximum) count = maximum;
				if (count < Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers)) continue;
				Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationEligible + i, 1);
				Write(handler, NC2Layout::AISinglePlayerOffset::AllocatedSoldierCounts + i * NC2Layout::RecordSize::AITaskIndexValue, count); allocated = true; remaining -= static_cast<double>(count);
			}
			if (!allocated) break;
			for (std::uint32_t i = 0; i < tasks; ++i)
			{
				if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationFinished + i) != 0 || Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationEligible + i) == 0) continue;
				const auto* description = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::FirstMainTaskDescription + Read<std::uint32_t>(combined(i), NC2Layout::AICombinedTaskOffset::MainTaskIndices) * NC2Layout::RecordSize::AIMainTask;
				std::uint32_t point = 0;
				if (!AITaskSimpleDescription_Tool_GetMapPosition(description, &point)) continue;
				candidates(description, &point); assign(i, description, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::AllocatedSoldierCounts + i * NC2Layout::RecordSize::AITaskIndexValue), firstCommand);
				Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationFinished + i, 1);
			}
		}
		for (std::uint32_t i = 0; i < tasks; ++i)
		{
			if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationFinished + i) != 0) continue;
			const auto* task = combined(i); auto count = round(Read<double>(task, NC2Layout::AICombinedTaskOffset::PriorityPercent) * Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers) * 0.01);
			const auto maximum = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers);
			if (maximum != 0 && count > maximum) count = maximum;
			if (count >= Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers)) Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationEligible + i, 1);
		}
		std::uint32_t totalPriority = 0;
		for (std::uint32_t i = 0; i < tasks; ++i) if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationFinished + i) == 0 && Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationEligible + i) != 0) totalPriority += Read<std::uint32_t>(combined(i), NC2Layout::AICombinedTaskOffset::Priority);
		const auto priorityFactor = 100.0 / static_cast<std::int32_t>(totalPriority);
		for (std::uint32_t i = 0; i < tasks; ++i) if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationFinished + i) == 0 && Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationEligible + i) != 0) Write(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationPriorityPercent + i * NC2Layout::RecordSize::AISoldierScoreEntry, static_cast<double>(Read<std::int32_t>(combined(i), NC2Layout::AICombinedTaskOffset::Priority)) * priorityFactor);
		for (std::uint32_t i = 0; i < tasks; ++i)
		{
			if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationFinished + i) != 0 || Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationEligible + i) == 0) continue;
			const auto* task = combined(i); auto count = round(remaining * Read<double>(handler, NC2Layout::AISinglePlayerOffset::TaskAllocationPriorityPercent + i * NC2Layout::RecordSize::AISoldierScoreEntry) * 0.01);
			const auto maximum = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers), minimum = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MinimumSoldiers);
			if (maximum != 0 && count > maximum) count = maximum;
			if (count < minimum) continue;
			const auto* description = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::FirstMainTaskDescription + Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskIndices) * NC2Layout::RecordSize::AIMainTask;
			std::uint32_t point = 0;
			if (!AITaskSimpleDescription_Tool_GetMapPosition(description, &point)) continue;
			candidates(description, &point);
			if (Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateCount) < count) count = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCandidateCount);
			if (count < minimum) continue;
			if (maximum != 0 && count > maximum) count = maximum;
			Write(handler, NC2Layout::AISinglePlayerOffset::AllocatedSoldierCounts + i * NC2Layout::RecordSize::AITaskIndexValue, count);
			if (count != 0) assign(i, description, count, secondCommand);
		}
		if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::DefaultSoldierPositionEnabled) == 0) return;
		Tool_Map_GetNotBlockedPosition(static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::DefaultSoldierPosition, true);
		for (std::uint32_t i = 0; i < soldiers; ++i)
		{
			auto* soldier = Soldier(handler, i);
			if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription) != 0) continue;
			auto* human = Human(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId));
			if (AISingleSoldier_Tool_IsSelfDefending(soldier) && Distance(static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::SoldierSelfDefenceOrigins + i * 4, Human_GetMapPositionRef(human)) < 25) continue;
			if (Read<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::UsesDefaultTask) != 0) continue;
			NXBasics::Runtime_FillMemory(soldier + NC2Layout::AISoldierOffset::TaskDescription, 0, NC2Layout::RecordSize::AITaskDescription);
			Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription, 1);
			Write(soldier, NC2Layout::AISoldierOffset::TaskMapPoint, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::DefaultSoldierPosition));
			Write(soldier, NC2Layout::AISoldierOffset::TaskRadius, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::DefaultSoldierRadius));
			Write<std::int32_t>(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex, -1);
			Write<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::UsesDefaultTask, 1);
			Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
			Write<std::uint8_t>(secondCommand, NC2Layout::UserNetworkCommandOffset::PlayerId, Read<std::uint8_t>(human, NC2Layout::HumanOffset::PlayerId));
			Write<std::uint32_t>(secondCommand, NC2Layout::UserNetworkCommandOffset::Parameter1, 2);
			Human_DoAddNewUserCommand(human, secondCommand);
		}
	}

	// Original function: FUN_0045a2d4.
	void AISinglePlayerHandler_l_SingleSoldiers_WorkOnAttachedTasks(void* handler)
	{
		const bool remaster = RemasterAIEconomy::IsActive(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId));
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount); ++i)
		{
			auto* soldier = Soldier(handler, i); auto* human = Human(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId));
			const auto position = Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates);
			const auto type = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription);
			if (remaster)
			{
				if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::ChangingEquipment)
				{
					const auto atomic = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);
					if (Human_UserControl_IsCommandTypeInQueue(human, 81) || Human_UserControl_IsCommandTypeInQueue(human, 82) || (atomic == 16 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) <= 1)) continue;
					if (!Human_IsDoingNothing(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == 1) continue;
					Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
					Write(soldier, NC2Layout::AISoldierOffset::WeaponType, Human_Equipment_GetWeaponType(human));
					Write(soldier, NC2Layout::AISoldierOffset::ArmorType, Human_Equipment_GetArmorType(human));
				}
				if (!Human_HasEquippedWeapon(human))
				{
					// Older saves can already contain fist fighters on expeditions. Bring them home.
					if (Human_UserControl_IsPriorityCommandInQueue(human) || Human_UserControl_IsCommandTypeInQueue(human, 81) || Human_UserControl_IsCommandTypeInQueue(human, 82)) continue;
					if (type == 2) AISingleSoldier_Tool_StopVAAndVCTask(soldier);
					const auto atomic = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);
					if (atomic == 16) continue;
					const auto* home = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::DefaultSoldierPosition;
					if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::DefaultSoldierPositionEnabled) != 0 && Distance(&position, home) > 8 && (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) != SwitchIds::AISoldierAction::Moving || (atomic != 2 && atomic != 4 && atomic != 14 && atomic != 15))) AISinglePlayerHandler_l_SingleSoldiers_Let_GotoPosition(handler, i, home, 8);
					continue;
				}
			}
			if (type == 0)
			{
				AISinglePlayerHandler_l_SingleSoldiers_CheckAttack(handler, i, &position, 25, true, false, true, true); continue;
			}
			const auto busyWalking = [&] ()
			{
				const auto atomic = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask); return atomic == 2 || atomic == 4 || atomic == 14 || atomic == 15;
			};
			auto state = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction);
			if (state == SwitchIds::AISoldierAction::Moving)
			{
				if (type == 2)
				{
					const auto* task = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasks + Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex) * NC2Layout::RecordSize::AICombinedTask;
					const auto phase = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase);
					if (phase == 2)
					{
						if (Distance(soldier + NC2Layout::AISoldierOffset::MovementDestination, soldier + NC2Layout::AISoldierOffset::TaskMapPoint) > 10) Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
						else if (Distance(&position, soldier + NC2Layout::AISoldierOffset::MovementDestination) <= Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::MovementRadius))
						{
							if (Remaster_AIMilitary_IsWaveTask(handler, task)) Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
							else AISingleSoldier_Tool_StopVAAndVCTask(soldier);
						}
						// Queuing a retry can interrupt idle and reset the human task status.
						// Keep its reachable destination instead of falling through to another command.
						else if (!busyWalking() && AISinglePlayerHandler_l_SingleSoldiers_Let_GotoPosition(handler, i, soldier + NC2Layout::AISoldierOffset::MovementDestination, Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::MovementRadius))) continue;
					}
					else if (phase == 1)
					{
						if (Distance(soldier + NC2Layout::AISoldierOffset::MovementDestination, task + NC2Layout::AICombinedTaskOffset::FormationPosition) > 10 || Distance(&position, task + NC2Layout::AICombinedTaskOffset::FormationPosition) <= Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::FormationRadius)) Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
						else if (!busyWalking() && AISinglePlayerHandler_l_SingleSoldiers_Let_GotoPosition(handler, i, task + NC2Layout::AICombinedTaskOffset::FormationPosition, Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::FormationRadius))) continue;
					}
				}
				else if (Distance(&position, soldier + NC2Layout::AISoldierOffset::MovementDestination) <= Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::MovementRadius)) AISingleSoldier_Tool_StopVAAndVCTask(soldier);
				else if (!busyWalking()) AISinglePlayerHandler_l_SingleSoldiers_Let_GotoPosition(handler, i, soldier + NC2Layout::AISoldierOffset::MovementDestination, Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::MovementRadius));
			}
			else if (state == SwitchIds::AISoldierAction::ChangingEquipment)
			{
				if ((Human_Equipment_GetWeaponType(human) == Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::WeaponType) && Human_Equipment_GetArmorType(human) == Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::ArmorType)) || Human_IsDoingNothing(human)) Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
			}
			if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::Attacking)
			{
				if (TargetData_IsValid(soldier + NC2Layout::AISoldierOffset::AttackTarget))
				{
					if (AISinglePlayerHandler_l_SingleSoldier_CheckIfIsInTaskRange(handler, i, true)) continue;
					if (AISingleSoldier_Tool_IsSelfDefending(soldier) && Distance(static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::SoldierSelfDefenceOrigins + i * 4, Human_GetMapPositionRef(human)) < 25) continue;
				}
				Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
			}
			if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) != SwitchIds::AISoldierAction::Waiting && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == 1) continue;
			if (AISingleSoldier_Tool_IsSelfDefending(soldier)) continue;
			Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Waiting);
			// Reload the description: stopping a completed movement can clear the task above.
			const auto currentType = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription);
			if (currentType == 2)
			{
				const auto target = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskMapPoint);
				const auto* task = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasks + Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex) * NC2Layout::RecordSize::AICombinedTask;
				const auto phase = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase);
				if (phase == 2)
				{
					const auto houseId = Logic_EntryPoint_House_GetOnPosition(&target);
					if (houseId != -1)
					{
						const auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(houseId) * CHouseArray::ElementSize;
						if (GameControl_Diplomacy_GetState(gGameControl, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId)) == 3 && AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHouse(handler, i, houseId, false)) continue;
					}
					if (Distance(&position, &target) <= Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskRadius))
					{
						if (AISinglePlayerHandler_l_SingleSoldiers_CheckAttack(handler, i, &position, 25, true, false, false, true)) continue;
						if (AISinglePlayerHandler_l_SingleSoldiers_CheckAttack(handler, i, &position, 25, true, true, false, true)) continue;
						if (AISinglePlayerHandler_l_SingleSoldiers_CheckAttack(handler, i, &target, Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskRadius), true, true, false, false)) continue;
					}
					const auto radius = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskRadius) >> 1;
					if (Distance(&position, &target) > radius) AISinglePlayerHandler_l_SingleSoldiers_Let_GotoPosition(handler, i, &target, radius);
				}
				else if (phase == 1)
				{
					if (Distance(&position, task + NC2Layout::AICombinedTaskOffset::FormationPosition) <= Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::FormationRadius))
					{
						if (!Remaster_AIMilitary_IsWaveTask(handler, task)) AISinglePlayerHandler_l_SingleSoldiers_CheckAttack(handler, i, &position, 25, true, false, false, true);
					}
					else AISinglePlayerHandler_l_SingleSoldiers_Let_GotoPosition(handler, i, task + NC2Layout::AICombinedTaskOffset::FormationPosition, Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::FormationRadius));
				}
			}
			else if (currentType == 1)
			{
				const auto target = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskMapPoint);
				if (Distance(&position, &target) <= Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskRadius))
				{
					if (AISinglePlayerHandler_l_SingleSoldiers_CheckAttack(handler, i, &position, 25, true, false, false, true)) continue;
					if (AISinglePlayerHandler_l_SingleSoldiers_CheckAttack(handler, i, &target, Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskRadius), true, true, false, false)) continue;
				}
				if (Distance(&position, &target) > 10) AISinglePlayerHandler_l_SingleSoldiers_Let_GotoPosition(handler, i, &target, Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskRadius) >> 1);
			}
		}
	}
}
