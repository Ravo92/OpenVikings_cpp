#include "C2AIMovementMessages.h"
#include "C2AIMilitaryAccess.h"
#include "C2AIBuildingAccess.h"
#include "C2GameControlNetwork.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HouseUserControl.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2HumanUserControl.h"
#include "C2HumanInitialization.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2InteractionTargetAccess.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapSectorGridCursor.h"
#include "C2MapWalkSectorManager.h"
#include "C2MilitaryTargetSearch.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2StaticHouseTypeData.h"
#include "C2VehicleAccess.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"
#include "RemasterAIEconomy.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		std::uint32_t TowerArcherQuota(const void* handler, std::int32_t houseId, std::uint32_t job, std::uint32_t wanted)
		{
			const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
			if (!RemasterAIEconomy::IsActive(player) || (job != SwitchIds::HumanJob::SoldierBowShort && job != SwitchIds::HumanJob::SoldierBowLong)) return wanted;
			const auto* house = GetHousePtr(houseId);
			// Defend an actually threatened tower; a stale defence-mode flag is not a threat.
			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DefenceModeFlag) != 0 && AI_IsEnemyObjectAround(player, House_Door_GetEntryPointRef(house), 30)) return wanted;
			auto wave = RemasterAIEconomy::WaveSize(RemasterAIEconomy::Inspect(player).population);
			for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::CombinedTaskCount); ++i)
			{
				const auto* task = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasks + i * NC2Layout::RecordSize::AICombinedTask;
				if (Remaster_AIMilitary_IsWaveTask(handler, task)) { wave = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MaximumSoldiers); break; }
			}
			std::uint32_t archers = 0, sameJob = 0;
			CHumanIterator humans;
			for (auto id = humans.GetFirstHumanId(player); id != -1; id = humans.GetNextHumanId())
			{
				const auto* human = GetHumanPtr(id);
				if (!Human_IsJobSoldier(human) || Human_IsAttachedToVehicle(human)) continue;
				const auto weapon = Human_Equipment_GetWeaponType(human);
				if (weapon != SwitchIds::WeaponType::ShortBow && weapon != SwitchIds::WeaponType::LongBow) continue;
				++archers;
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == job) ++sameJob;
			}
			const auto reserve = Remaster_AIMilitary_WantedArchers(handler, wave);
			auto budget = archers > reserve ? archers - reserve : 0u;
			if (budget > archers / 3u) budget = archers / 3u;
			budget = archers != 0 ? budget * sameJob / archers : 0;
			// Stable house-ID order prevents released archers from entering another empty tower.
			CHouseIterator houses;
			for (auto id = houses.GetFirstHouseId(player); budget != 0 && id != -1; id = houses.GetNextHouseId())
			{
				if (id >= houseId) continue;
				const auto* other = GetHousePtr(id);
				const auto logic = Read<std::uint32_t>(other, NC2Layout::HouseOffset::LogicType);
				if ((logic != 40 && logic != 41) || Read<std::uint32_t>(other, NC2Layout::HouseOffset::ConstructionState) != 3 || Read<std::uint8_t>(other, NC2Layout::HouseOffset::DemolitionFlag) != 0) continue;
				const auto* definition = House_GetHouseTypeDataPtr(other);
				for (std::int32_t i = 0; i < definition->workerCount; ++i)
					if (static_cast<std::uint32_t>(definition->workers[i].jobType) == job) { --budget; break; }
			}
			return budget != 0 && wanted != 0 ? 1u : 0u;
		}
	}

	// Original function: FUN_0045b0ed.
	bool AISinglePlayerHandler_l_Soldiers_AttachToTower(void* handler, std::int32_t houseId)
	{
		auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(houseId) * CHouseArray::ElementSize;
		const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);
		if ((type != 40 && type != 41) || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3) return false;
		const auto* definition = House_GetHouseTypeDataPtr(house);
		// Windows initializes the sixteen job flags 27..42 used by the tower definitions.
		std::byte local[68];
		NXBasics::Runtime_FillMemory(local + 0x2C, 0, 16);
		auto* flags = local + 0x11;
		for (std::int32_t i = 0; i < definition->workerCount; ++i)
		{
			const auto job = definition->workers[i].jobType;
			if (job == 24) continue;
			const auto wanted = TowerArcherQuota(handler, houseId, static_cast<std::uint32_t>(job), Read<std::uint8_t>(house, NC2Layout::HouseOffset::DefenceModeFlag) != 0 ? definition->workers[i].maximumWorkers : 1);
			const auto attached = static_cast<std::uint32_t>(House_Inhabitants_GetNumberOfAttachedWorkersOfPassedJob(house, job));
			if (attached < wanted)
			{
				flags[job] = std::byte{ 1 }; continue;
			}
			if (attached <= wanted) continue;
			for (std::uint32_t slot = 0; slot < 20; ++slot)
			{
				const auto* inhabitant = House_Inhabitants_GetAttachedHumanSlotPtrBySlotId(house, slot);
				if (Read<std::uint8_t>(inhabitant, NC2Layout::HouseAttachmentOffset::Flags) == 0) continue;
				auto* human = CHumanArray::mStaticVars + Read<std::uint32_t>(inhabitant, NC2Layout::HouseAttachmentOffset::HumanId) * CHumanArray::ElementSize;
				if (Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType) != job) continue;
				Human_DoAddNewUserCommand(human, 29, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
				break;
			}
		}
		bool changed = false;
		CHumanIterator iterator;
		for (auto id = iterator.GetFirstSectorHumanId(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), Read<std::uint32_t>(House_Door_GetEntryPointRef(house), NC2Layout::MapMigPointOffset::Coordinates), 20, true); id != -1; id = iterator.GetNextSectorHumanId())
		{
			auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
			if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 32u) != 0 || Human_IsAttachedToWorkHouse(human) || Human_IsAttachedToVehicle(human) || !Human_IsJobSoldier(human)) continue;
			if (RemasterAIEconomy::IsActive(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId)))
			{
				if (Human_UserControl_IsPriorityCommandInQueue(human) || Human_UserControl_IsCommandTypeInQueue(human, 28) || Human_UserControl_IsCommandTypeInQueue(human, 29) || Human_UserControl_IsCommandTypeInQueue(human, 81) || Human_UserControl_IsCommandTypeInQueue(human, 82)) continue;
				bool expedition = false;
				for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount); ++i)
				{
					const auto* soldier = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::Soldiers + i * NC2Layout::RecordSize::AISoldier;
					if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId) == static_cast<std::uint32_t>(id) && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanUniqueId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::TaskDescription) == 2) { expedition = true; break; }
				}
				if (expedition) continue;
			}
			const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
			if (flags[job] == std::byte{ 0 }) continue;
			Human_DoAddNewUserCommand(human, 28, Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId), Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
			flags[job] = std::byte{ 0 }; changed = true;
		}
		return changed;
	}
	// Original function: FUN_0045b058.
	void AISinglePlayerHandler_l_Soldiers_AttachToTowers(void* handler)
	{
		auto cursor = Read<std::int32_t>(handler, NC2Layout::AISinglePlayerOffset::TowerAttachmentCursor);
		if (cursor != -1 && Read<std::uint8_t>(CHouseArray::mStaticVars + static_cast<std::uint32_t>(cursor) * CHouseArray::ElementSize, NC2Layout::HouseOffset::AllocatedFlag) == 0) Write<std::int32_t>(handler, NC2Layout::AISinglePlayerOffset::TowerAttachmentCursor, -1);
		std::uint32_t origin;
		MapMigPoint_SetPosition(&origin, 0, 0, false);
		CHouseIterator iterator;
		for (auto id = iterator.GetFirstHouseId(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), &origin, 10000); id != -1; id = iterator.GetNextHouseId())
		{
			cursor = Read<std::int32_t>(handler, NC2Layout::AISinglePlayerOffset::TowerAttachmentCursor);
			if (cursor == -1)
			{
				if (AISinglePlayerHandler_l_Soldiers_AttachToTower(handler, id))
				{
					Write(handler, NC2Layout::AISinglePlayerOffset::TowerAttachmentCursor, id); return;
				}
			}
			else if (id >= cursor) Write<std::int32_t>(handler, NC2Layout::AISinglePlayerOffset::TowerAttachmentCursor, -1);
		}
	}
	// Original function: FUN_0045b295; the iterator and native flags determine candidate eligibility.
	void* AISinglePlayerHandler_l_SingleSoldier_GetBestSoldier(void* handler, const void* point, std::uint32_t radius)
	{
		void* best = nullptr;
		std::uint32_t distance = 10000;
		CHumanIterator iterator;
		for (auto id = iterator.GetFirstSectorHumanId(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates), radius, true); id != -1; id = iterator.GetNextSectorHumanId())
		{
			auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
			if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 32u) != 0 || Human_IsAttachedToWorkHouse(human) || Human_IsAttachedToVehicle(human)) continue;
			const auto* position = Human_GetMapPositionRef(human);
			const auto candidate = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::Coordinates), Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y)));
			if (candidate < distance)
			{
				distance = candidate; best = human;
			}
		}
		return best;
	}
	// Original function: FUN_00473102; Windows radius 004F4308 is 40.
	void __fastcall AISinglePlayerHandler_Messages_Attacked(void* handler, void* /*unusedEdx*/, const void* point)
	{
		Write(handler, NC2Layout::AISinglePlayerOffset::LastAttackedTick, gGameTime.tick);
		CHouseIterator iterator;
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
		for (auto id = iterator.GetFirstSectorHouseId(player, point, 40, true); id != -1; id = iterator.GetNextSectorHouseId())
		{
			auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
			if (!House_CanEnableDefenceMode(house)) continue;
			House_SetDefenceModeFlag(house, true); AISinglePlayerHandler_l_Soldiers_AttachToTower(handler, id);
		}
		for (auto id = iterator.GetFirstSectorHouseId(player, point, 40, false); id != -1; id = iterator.GetNextSectorHouseId())
		{
			auto* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
			if (House_CanEnableDefenceMode(house)) House_SetDefenceModeFlag(house, true);
		}
	}
	namespace
	{
		// Complete native static region 00554D4C..00554D57: last point, initialization byte/padding, last tick.
		struct SNoWayMessageState
		{
			std::uint32_t point; std::uint8_t initialized; std::byte padding[3]; std::uint32_t tick;
		};
		static_assert(sizeof(SNoWayMessageState) == 12);
		SNoWayMessageState gNoWayMessageState{ };
		// Original function: FUN_00472F48; the original empty point destructor returns no value.
		int __cdecl NoWayMessagePoint_OnExit()
		{
			return 0;
		}
		void* Soldier(void* handler, std::uint32_t slot)
		{
			return static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::SoldiersUnsigned + slot * NC2Layout::RecordSize::AISoldierUnsigned;
		}
		const void* Soldier(const void* handler, std::uint32_t slot)
		{
			return static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::SoldiersUnsigned + slot * NC2Layout::RecordSize::AISoldierUnsigned;
		}
		std::uint32_t Distance(const void* a, const void* b)
		{
			return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(a, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(b, NC2Layout::MapCoordinatePairOffset::Y)));
		}
	}

	// Original function: FUN_0045B02A; only the single-soldier leader id is searched.
	std::int32_t AISinglePlayerHandler_l_SingleSoldier_GetSlotIdByCreatureId(const void* handler, std::uint32_t human)
	{
		const auto count = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::SoldierCount);
		for (std::uint32_t slot = 0; slot < count; ++slot) if (Read<std::uint32_t>(Soldier(handler, slot), NC2Layout::AISoldierOffset::HumanId) == human) return NXBasics::Runtime_TimeSigned32(slot);
		return -1;
	}

	// Original function: FUN_0045B346; unsigned distances and native 3/2 defence margins are retained.
	bool AISinglePlayerHandler_l_SingleSoldier_CheckIfIsInTaskRange(const void* handler, std::uint32_t slot, bool defence)
	{
		const auto* soldier = Soldier(handler, slot);
		const auto* position = Human_GetMapPositionRef(GetHumanPtr(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId))));
		if (Read<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::UsesDefaultTask) != 0)
		{
			const auto range = defence ? 25u : Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::DefaultSoldierRadius) * 8u;
			return Distance(position, static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::DefaultSoldierPosition) <= range;
		}
		const auto attachment = Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CombinedTaskIndex);
		if (attachment == NXBasics::IntegerRange::UInt32Maximum) return true;
		const auto* task = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::CombinedTasksUnsigned + attachment * NC2Layout::RecordSize::AICombinedTaskUnsigned;
		const auto* main = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::MainTasksUnsigned + Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::MainTaskIndices) * NC2Layout::RecordSize::AIMainTaskUnsigned;
		const auto type = Read<std::uint32_t>(main, NC2Layout::AIMainTaskOffset::Description);
		if (type == 0) return true;
		std::uint32_t range, center;
		if (type == 2 && Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::AttackPhase) == 1)
		{
			range = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::FormationRadius);
			center = Read<std::uint32_t>(task, NC2Layout::AICombinedTaskOffset::FormationPosition);
		}
		else
		{
			MapMigPoint_SetPosition(&center, Read<std::int16_t>(main, NC2Layout::AIMainTaskOffset::PrimaryPosition), Read<std::int16_t>(main, NC2Layout::AIMainTaskOffset::PrimaryPositionY), false);
			range = Read<std::uint32_t>(main, NC2Layout::AIMainTaskOffset::Radius);
		}
		if (defence) range = (range * 3u) >> 1;
		return Distance(&center, position) <= range;
	}

	// Original function: FUN_0045B4B6.
	void AISinglePlayerHandler_l_SingleSoldier_StartSelfDefenceMode(void* handler, std::uint32_t slot)
	{
		auto* soldier = Soldier(handler, slot);
		const auto* human = GetHumanPtr(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId)));
		Write<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::SelfDefence, 1);
		Write(handler, NC2Layout::AISinglePlayerOffset::SoldierSelfDefenceOriginsUnsigned + slot * NC2Layout::RecordSize::MapMigPointUnsigned, Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapMigPointOffset::Coordinates));
	}

	// Original function: FUN_0045AB88.
	bool AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHuman(void* handler, std::uint32_t slot, std::uint32_t target, bool defence)
	{
		auto* soldier = Soldier(handler, slot);
		const auto* enemy = CHumanArray::mStaticVars + target * CHumanArray::ElementSize;
		if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::Attacking && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTarget) == 1 && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTargetUniqueId) == Read<std::uint32_t>(enemy, NC2Layout::HumanOffset::UniqueId) && Read<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::SelfDefence) == static_cast<std::uint8_t>(defence)) return true;
		auto* human = const_cast<void*>(GetHumanPtr(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId))));
		const auto player = Read<std::uint32_t>(enemy, NC2Layout::HumanOffset::PlayerId);
		GameControl_Diplomacy_SetState(gGameControl, nullptr, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), player, 3);
		alignas(4) std::byte command[20]{ };
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 43);
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)));
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, target);
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, Read<std::uint32_t>(enemy, NC2Layout::HumanOffset::UniqueId));
		Human_DoAddNewUserCommand(human, command);
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Attacking);
		Write(soldier, NC2Layout::AISoldierOffset::AttackTargetId, target);
		Write(soldier, NC2Layout::AISoldierOffset::AttackTargetUniqueId, Read<std::uint32_t>(enemy, NC2Layout::HumanOffset::UniqueId));
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTarget, 1);
		if (defence) AISinglePlayerHandler_l_SingleSoldier_StartSelfDefenceMode(handler, slot);
		else Write<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::SelfDefence, 0);
		return true;
	}

	// Original function: FUN_0045AC58.
	bool AISinglePlayerHandler_l_SingleSoldiers_Let_AttackAnimal(void* handler, std::uint32_t slot, std::uint32_t target, bool defence)
	{
		auto* soldier = Soldier(handler, slot);
		const auto* enemy = CAnimalArray::mStaticVars + target * CAnimalArray::ElementSize;
		if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::Attacking && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTarget) == 3 && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTargetUniqueId) == Read<std::uint32_t>(enemy, NC2Layout::AnimalOffset::UniqueId) && Read<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::SelfDefence) == static_cast<std::uint8_t>(defence)) return true;
		auto* human = const_cast<void*>(GetHumanPtr(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId))));
		const auto player = Read<std::uint32_t>(enemy, NC2Layout::AnimalOffset::PlayerId);
		if (player != 20) GameControl_Diplomacy_SetState(gGameControl, nullptr, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), player, 3);
		alignas(4) std::byte command[20]{ };
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 45);
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)));
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, target);
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, Read<std::uint32_t>(enemy, NC2Layout::AnimalOffset::UniqueId));
		Human_DoAddNewUserCommand(human, command);
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Attacking);
		Write(soldier, NC2Layout::AISoldierOffset::AttackTargetId, target);
		Write(soldier, NC2Layout::AISoldierOffset::AttackTargetUniqueId, Read<std::uint32_t>(enemy, NC2Layout::AnimalOffset::UniqueId));
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTarget, 3);
		if (defence) AISinglePlayerHandler_l_SingleSoldier_StartSelfDefenceMode(handler, slot);
		else Write<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::SelfDefence, 0);
		return true;
	}

	// Original function: FUN_0045AD35.
	bool AISinglePlayerHandler_l_SingleSoldiers_Let_AttackVehicle(void* handler, std::uint32_t slot, std::uint32_t target, bool defence)
	{
		auto* soldier = Soldier(handler, slot);
		const auto* enemy = CVehicleArray::mStaticVars + target * CVehicleArray::ElementSize;
		if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::Attacking && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTarget) == 2 && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTargetUniqueId) == Read<std::uint32_t>(enemy, NC2Layout::VehicleOffset::UniqueId) && Read<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::SelfDefence) == static_cast<std::uint8_t>(defence)) return true;
		auto* human = const_cast<void*>(GetHumanPtr(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId))));
		const auto player = Read<std::uint32_t>(enemy, NC2Layout::VehicleOffset::PlayerId);
		GameControl_Diplomacy_SetState(gGameControl, nullptr, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), player, 3);
		alignas(4) std::byte command[20]{ };
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 46);
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)));
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, target);
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, Read<std::uint32_t>(enemy, NC2Layout::VehicleOffset::UniqueId));
		Human_DoAddNewUserCommand(human, command);
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Attacking);
		Write(soldier, NC2Layout::AISoldierOffset::AttackTargetId, target);
		Write(soldier, NC2Layout::AISoldierOffset::AttackTargetUniqueId, Read<std::uint32_t>(enemy, NC2Layout::VehicleOffset::UniqueId));
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTarget, 2);
		if (defence) AISinglePlayerHandler_l_SingleSoldier_StartSelfDefenceMode(handler, slot);
		else Write<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::SelfDefence, 0);
		return true;
	}

	// Original function: FUN_0045AE0F.
	bool AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHouse(void* handler, std::uint32_t slot, std::uint32_t target, bool defence)
	{
		auto* soldier = Soldier(handler, slot);
		const auto* enemy = CHouseArray::mStaticVars + target * CHouseArray::ElementSize;
		if (Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::Attacking && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTarget) == 4 && Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTargetUniqueId) == Read<std::uint32_t>(enemy, NC2Layout::HouseOffset::UniqueId) && Read<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::SelfDefence) == static_cast<std::uint8_t>(defence)) return true;
		auto* human = const_cast<void*>(GetHumanPtr(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::HumanId))));
		const auto player = Read<std::uint32_t>(enemy, NC2Layout::HouseOffset::PlayerId);
		GameControl_Diplomacy_SetState(gGameControl, nullptr, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), player, 3);
		alignas(4) std::byte command[20]{ };
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 44);
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)));
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, target);
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, Read<std::uint32_t>(enemy, NC2Layout::HouseOffset::UniqueId));
		Human_DoAddNewUserCommand(human, command);
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction, SwitchIds::AISoldierAction::Attacking);
		Write(soldier, NC2Layout::AISoldierOffset::AttackTargetId, target);
		Write(soldier, NC2Layout::AISoldierOffset::AttackTargetUniqueId, Read<std::uint32_t>(enemy, NC2Layout::HouseOffset::UniqueId));
		Write<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::AttackTarget, 4);
		if (defence) AISinglePlayerHandler_l_SingleSoldier_StartSelfDefenceMode(handler, slot);
		else Write<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::SelfDefence, 0);
		return true;
	}

	// Original function: FUN_00472CC0.
	bool AISingleSoldier_Tool_IsSelfDefending(const void* soldier)
	{
		return Read<std::uint32_t>(soldier, NC2Layout::AISoldierOffset::CurrentAction) == SwitchIds::AISoldierAction::Attacking && Read<std::uint8_t>(soldier, NC2Layout::AISoldierOffset::SelfDefence) != 0 && TargetData_IsValid(static_cast<const std::byte*>(soldier) + NC2Layout::AISoldierOffset::AttackTarget);
	}

	// Original function: FUN_00472CDE; the throttle is shared across all player handlers.
	void AISinglePlayerHandler_Messages_NoWayFound(void* handler, const void* source, const void* target)
	{
		if ((gNoWayMessageState.initialized & 1u) == 0)
		{
			gNoWayMessageState.initialized |= 1u;
			gNoWayMessageState.point = 0;
			NXBasics::Runtime_AtExit(NoWayMessagePoint_OnExit);
		}
		if (gNoWayMessageState.point == Read<std::uint32_t>(target, NC2Layout::MapMigPointOffset::Coordinates) && gNoWayMessageState.tick + 12u > gGameTime.tick) return;
		for (std::uint32_t index = 0; index <= Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::LastMainTaskIndex); ++index)
		{
			auto* task = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::MainTasksUnsigned + index * NC2Layout::RecordSize::AIMainTaskUnsigned;
			const auto type = Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::Description);
			if (type != 1 && type != 2) continue;
			std::uint32_t point;
			MapMigPoint_SetPosition(&point, Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::PrimaryPosition), Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::PrimaryPositionY), false);
			if (point == Read<std::uint32_t>(target, NC2Layout::MapMigPointOffset::Coordinates) && (type == 1 || Read<std::uint8_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionPendingFlag) != 1 || Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionMode) == 1))
			{
				const auto sourceContinent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::ContinentId);
				if (sourceContinent == Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(target), NC2Layout::MapMicroPointOffset::ContinentId) && AISinglePlayerHandler_l_Messages_GetNewValidPosition(handler, source, &point))
				{
					Write<std::uint8_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionPendingFlag, 1);
					Write<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionMode, type == 1 ? 3u : 1u);
					Write(task, NC2Layout::AIMainTaskOffset::NoWaySourcePoint, Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates));
					Write(task, NC2Layout::AIMainTaskOffset::NoWayTargetPoint, Read<std::uint32_t>(target, NC2Layout::MapMigPointOffset::Coordinates));
					Write(task, NC2Layout::AIMainTaskOffset::PrimaryPosition, point);
				}
			}
			// For a type-2 task an incompatible pending mode skips the second endpoint too.
			else if (type == 2 && point == Read<std::uint32_t>(target, NC2Layout::MapMigPointOffset::Coordinates) && Read<std::uint8_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionPendingFlag) == 1 && Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionMode) != 1) continue;
			if (type != 2) continue;
			MapMigPoint_Construct(&point, Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::SecondaryPosition), Read<std::int16_t>(task, NC2Layout::AIMainTaskOffset::SecondaryPositionY), false);
			if (point != Read<std::uint32_t>(target, NC2Layout::MapMigPointOffset::Coordinates) || (Read<std::uint8_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionPendingFlag) == 1 && Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionMode) != 2)) continue;
			const auto sourceContinent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::ContinentId);
			if (sourceContinent != Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(target), NC2Layout::MapMicroPointOffset::ContinentId) || !AISinglePlayerHandler_l_Messages_GetNewValidPosition(handler, source, &point)) continue;
			Write<std::uint8_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionPendingFlag, 1);
			Write<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionMode, 2);
			Write(task, NC2Layout::AIMainTaskOffset::NoWaySourcePoint, Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates));
			Write(task, NC2Layout::AIMainTaskOffset::NoWayTargetPoint, Read<std::uint32_t>(target, NC2Layout::MapMigPointOffset::Coordinates));
			Write(task, NC2Layout::AIMainTaskOffset::SecondaryPosition, point);
		}
		gNoWayMessageState.point = Read<std::uint32_t>(target, NC2Layout::MapMigPointOffset::Coordinates);
		gNoWayMessageState.tick = gGameTime.tick;
	}

	// Original function: FUN_00472F49; preserve the cumulative square cursor and first successful ring.
	bool AISinglePlayerHandler_l_Messages_GetNewValidPosition(void* handler, const void* source, void* target)
	{
		(void) handler;
		const SSectorGridCursor origin(Read<std::uint32_t>(source, NC2Layout::MapMigPointOffset::Coordinates));
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(source), NC2Layout::MapMicroPointOffset::ContinentId);
		if (CMapWalkSectorManager::GetLandSectorDataPtr(origin)->continentId != continent) return false;
		SSectorGridCursor cursor(Read<std::uint32_t>(target, NC2Layout::MapMigPointOffset::Coordinates));
		std::uint32_t bestDistance = 10000, bestPoint = 0;
		for (std::uint32_t radius = 2; radius < 10; radius += 2)
		{
			cursor.MoveCursorUnwrapped(5);
			for (std::uint32_t direction = 0; direction < 8; direction += 2)
				for (std::uint32_t step = 0; step < radius; ++step)
				{
					cursor.MoveCursorUnwrapped(direction);
					if (!cursor.IsInsideMap() || !CMapWalkSectorManager::WayPossible(origin, cursor, 0, true, 20)) continue;
					const auto* point = &CMapWalkSectorManager::GetLandSectorDataPtr(cursor)->basePoints[0];
					const auto distance = Distance(point, source);
					if (distance < bestDistance)
					{
						bestDistance = distance; bestPoint = Read<std::uint32_t>(point, NC2Layout::MapMigPointOffset::Coordinates);
					}
				}
			if (bestDistance != 10000)
			{
				Write(target, NC2Layout::MapMigPointOffset::Coordinates, bestPoint); return true;
			}
		}
		return false;
	}

	// Original function: FUN_004731CD.
	void AISinglePlayerHandler_Messages_Human_NoWayFoundToEnemy(void* handler, void* human, const void* target)
	{
		auto point = Read<std::uint32_t>(target, NC2Layout::MapMigPointOffset::Coordinates);
		if (!AISinglePlayerHandler_l_Messages_GetNewValidPosition(handler, Human_GetMapPositionRef(human), &point)) return;
		const auto player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), revision = Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId);
		Human_DoAddNewUserCommand(human, 4, static_cast<std::uint8_t>(player), id, revision, MapMigPoint_GetLong(&point), 0);
	}

	// Original function: FUN_0047322D.
	void AISinglePlayerHandler_Messages_Soldier_StartSelfDefence(void* handler, std::uint32_t human, const void* target)
	{
		const auto slot = AISinglePlayerHandler_l_SingleSoldier_GetSlotIdByCreatureId(handler, human);
		if (slot == -1 || AISingleSoldier_Tool_IsSelfDefending(Soldier(handler, static_cast<std::uint32_t>(slot))) || !AISinglePlayerHandler_l_SingleSoldier_CheckIfIsInTaskRange(handler, static_cast<std::uint32_t>(slot), true)) return;
		switch (TargetData_GetTargetType(target))
		{
			case SwitchIds::TargetType::Human: AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHuman(handler, slot, TargetData_GetObjectId(target), true); break;
			case SwitchIds::TargetType::Vehicle: AISinglePlayerHandler_l_SingleSoldiers_Let_AttackVehicle(handler, slot, TargetData_GetObjectId(target), true); break;
			case SwitchIds::TargetType::Animal: AISinglePlayerHandler_l_SingleSoldiers_Let_AttackAnimal(handler, slot, TargetData_GetObjectId(target), true); break;
			case SwitchIds::TargetType::House: AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHouse(handler, slot, TargetData_GetObjectId(target), true); break;
		}
	}

	// Original function: FUN_004732D2; attacked soldiers outside their task range start self defence.
	void __fastcall AISinglePlayerHandler_Messages_SoldierAttacked(void* handler, void* unusedEdx, std::uint32_t human, const void* target)
	{
		(void) unusedEdx;
		const auto slot = AISinglePlayerHandler_l_SingleSoldier_GetSlotIdByCreatureId(handler, human);
		if (slot == -1 || AISingleSoldier_Tool_IsSelfDefending(Soldier(handler, static_cast<std::uint32_t>(slot))) || AISinglePlayerHandler_l_SingleSoldier_CheckIfIsInTaskRange(handler, static_cast<std::uint32_t>(slot), true)) return;
		switch (TargetData_GetTargetType(target))
		{
			case SwitchIds::TargetType::Human: AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHuman(handler, slot, TargetData_GetObjectId(target), true); break;
			case SwitchIds::TargetType::Vehicle: AISinglePlayerHandler_l_SingleSoldiers_Let_AttackVehicle(handler, slot, TargetData_GetObjectId(target), true); break;
			case SwitchIds::TargetType::Animal: AISinglePlayerHandler_l_SingleSoldiers_Let_AttackAnimal(handler, slot, TargetData_GetObjectId(target), true); break;
			case SwitchIds::TargetType::House: AISinglePlayerHandler_l_SingleSoldiers_Let_AttackHouse(handler, slot, TargetData_GetObjectId(target), true); break;
		}
	}

	// Original function: FUN_004622D6; notifications apply only to human moveables.
	void AIManager_Message_NoWayFound(void* manager, std::uint32_t moveable, const void* target)
	{
		const auto* entry = gMapMoveableArrayStorage.entries + moveable * NC2Layout::RecordSize::MapMoveable;
		if (Read<std::uint32_t>(entry, NC2Layout::MapMoveableOffset::ObjectType) != 1) return;
		const auto* human = GetHumanPtr(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(entry, NC2Layout::MapMoveableOffset::ObjectId)));
		if (auto* handler = Read<void*>(manager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned)) AISinglePlayerHandler_Messages_NoWayFound(handler, Human_GetMapPositionRef(human), target);
	}

	// Original function: FUN_00462361.
	void AIManager_Message_NoWayFoundToEnemy(void* manager, std::uint32_t moveable, const void* target)
	{
		const auto* entry = gMapMoveableArrayStorage.entries + moveable * NC2Layout::RecordSize::MapMoveable;
		if (Read<std::uint32_t>(entry, NC2Layout::MapMoveableOffset::ObjectType) != 1) return;
		auto* human = const_cast<void*>(GetHumanPtr(NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(entry, NC2Layout::MapMoveableOffset::ObjectId))));
		if (auto* handler = Read<void*>(manager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned)) AISinglePlayerHandler_Messages_Human_NoWayFoundToEnemy(handler, human, target);
	}

	// Original function: FUN_00462398.
	void AIManager_Message_StartSelfDefence(void* manager, std::uint32_t player, std::uint32_t human, const void* target)
	{
		if (auto* handler = Read<void*>(manager, player * NC2Layout::RecordSize::AIPlayerHandlerPointerUnsigned)) AISinglePlayerHandler_Messages_Soldier_StartSelfDefence(handler, human, target);
	}
	// Original function: FUN_004736e2.
	bool AISinglePlayerHandler_l_Vehicles_Let_AttackHuman(void* handler, std::uint32_t slot, std::uint32_t target, bool defence)
	{
		(void) defence; // The Windows argument is unused.
		auto* entry = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::VehiclesUnsigned + slot * NC2Layout::RecordSize::AIVehicleUnsigned;
		auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::VehicleId) * CVehicleArray::ElementSize;
		auto* enemy = CHumanArray::mStaticVars + target * CHumanArray::ElementSize;
		GameControl_Diplomacy_SetState(gGameControl, nullptr, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), Read<std::uint32_t>(enemy, NC2Layout::HumanOffset::PlayerId), 3);
		std::byte command[20]{ };
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 104);
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId)));
		Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
		Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, target);
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, Read<std::uint32_t>(enemy, NC2Layout::HumanOffset::UniqueId));
		Vehicle_DoAddNewUserCommand(vehicle, command);
		Write<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::AttackActive, 1);
		Write(entry, NC2Layout::AIVehicleOffset::AttackTargetUniqueId, Read<std::uint32_t>(enemy, NC2Layout::HumanOffset::UniqueId));
		Write(entry, NC2Layout::AIVehicleOffset::AttackTargetId, target);
		Write<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::AttackTarget, 1);
		return true;
	}

	// Original function: FUN_00473788.
	bool AISinglePlayerHandler_l_Vehicles_Let_AttackVehicle(void* handler, std::uint32_t slot, std::uint32_t target, bool defence)
	{
		(void) defence; // The Windows argument is unused.
		auto* entry = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::VehiclesUnsigned + slot * NC2Layout::RecordSize::AIVehicleUnsigned;
		auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::VehicleId) * CVehicleArray::ElementSize;
		auto* enemy = CVehicleArray::mStaticVars + target * CVehicleArray::ElementSize;
		GameControl_Diplomacy_SetState(gGameControl, nullptr, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), Read<std::uint32_t>(enemy, NC2Layout::VehicleOffset::PlayerId), 3);
		std::byte command[20]{ };
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 46);
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId)));
		Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
		Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, target);
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, Read<std::uint32_t>(enemy, NC2Layout::VehicleOffset::UniqueId));
		Vehicle_DoAddNewUserCommand(vehicle, command);
		Write<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::AttackActive, 1);
		Write(entry, NC2Layout::AIVehicleOffset::AttackTargetUniqueId, Read<std::uint32_t>(enemy, NC2Layout::VehicleOffset::UniqueId));
		Write(entry, NC2Layout::AIVehicleOffset::AttackTargetId, target);
		Write<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::AttackTarget, 2);
		return true;
	}

	// Original function: FUN_00473833.
	bool AISinglePlayerHandler_l_Vehicles_Let_AttackHouse(void* handler, std::uint32_t slot, std::uint32_t target)
	{
		auto* entry = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::VehiclesUnsigned + slot * NC2Layout::RecordSize::AIVehicleUnsigned;
		auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::VehicleId) * CVehicleArray::ElementSize;
		auto* enemy = CHouseArray::mStaticVars + target * CHouseArray::ElementSize;
		GameControl_Diplomacy_SetState(gGameControl, nullptr, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), Read<std::uint32_t>(enemy, NC2Layout::HouseOffset::PlayerId), 3);
		std::byte command[20]{ };
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 44);
		Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId)));
		Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
		Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, target);
		Write(command, NC2Layout::UserNetworkCommandOffset::Parameter2, Read<std::uint32_t>(enemy, NC2Layout::HouseOffset::UniqueId));
		Vehicle_DoAddNewUserCommand(vehicle, command);
		Write<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::AttackActive, 1);
		Write(entry, NC2Layout::AIVehicleOffset::AttackTargetUniqueId, Read<std::uint32_t>(enemy, NC2Layout::HouseOffset::UniqueId));
		Write(entry, NC2Layout::AIVehicleOffset::AttackTargetId, target);
		Write<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::AttackTarget, 4);
		return true;
	}

	// Original function: FUN_0047354b; retain Windows target search order and all four flags.
	bool AISinglePlayerHandler_l_Vehicles_CheckForEnemys(void* handler, std::uint32_t slot, const void* point, std::uint32_t radius, bool houses, bool civilians, bool restrictCandidates, bool defence)
	{
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
		const auto limit = restrictCandidates ? 1u : 5u, threshold = restrictCandidates ? 0u : 3u;
		auto target = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, 0, radius, true, limit, threshold, false, false);
		if (target != -1 && AISinglePlayerHandler_l_Vehicles_Let_AttackVehicle(handler, slot, static_cast<std::uint32_t>(target), defence)) return true;
		target = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, 0, radius, true, limit, threshold, true, false);
		if (target != -1 && AISinglePlayerHandler_l_Vehicles_Let_AttackHuman(handler, slot, static_cast<std::uint32_t>(target), defence)) return true;
		if (houses)
		{
			target = Tool_GetOneOfNextEnemyHousesInRange(player, point, 0, radius, true, restrictCandidates ? 1u : 3u, restrictCandidates ? 0u : 5u);
			if (target != -1 && AISinglePlayerHandler_l_Vehicles_Let_AttackHouse(handler, slot, static_cast<std::uint32_t>(target))) return true;
		}
		if (civilians)
		{
			target = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, 0, radius, false, limit, threshold, true, false);
			if (target != -1 && AISinglePlayerHandler_l_Vehicles_Let_AttackHuman(handler, slot, static_cast<std::uint32_t>(target), defence)) return true;
			target = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, point, 0, radius, false, limit, threshold, false, false);
			if (target != -1 && AISinglePlayerHandler_l_Vehicles_Let_AttackVehicle(handler, slot, static_cast<std::uint32_t>(target), defence)) return true;
		}
		if (houses && civilians)
		{
			target = Tool_GetOneOfNextEnemyHousesInRange(player, point, 0, radius, false, limit, restrictCandidates ? 0u : 10u);
			if (target != -1 && AISinglePlayerHandler_l_Vehicles_Let_AttackHouse(handler, slot, static_cast<std::uint32_t>(target))) return true;
		}
		return false;
	}

	// Original function: FUN_004738dd.
	void AISinglePlayerHandler_l_Vehicles_WorkOnAttachedTasks(void* handler)
	{
		for (std::uint32_t slot = 0; slot < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::VehicleCount); ++slot)
		{
			const auto* entry = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::VehiclesUnsigned + slot * NC2Layout::RecordSize::AIVehicleUnsigned;
			const auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::VehicleId) * CVehicleArray::ElementSize;
			const auto* point = Vehicle_GetMapPositionRef(vehicle);
			if (Read<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::TaskIndex) == NXBasics::IntegerRange::UInt32Maximum && !AISinglePlayerHandler_l_Vehicles_IsSubTaskTargetValid(handler, slot)) AISinglePlayerHandler_l_Vehicles_CheckForEnemys(handler, slot, point, 30, true, false, true, true);
		}
	}

	// Original function: FUN_00473944.
	bool AISinglePlayerHandler_l_Vehicles_IsSubTaskTargetValid(const void* handler, std::uint32_t slot)
	{
		const auto* entry = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::VehiclesUnsigned + slot * NC2Layout::RecordSize::AIVehicleUnsigned;
		const auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::VehicleId) * CVehicleArray::ElementSize;
		const auto* target = entry + NC2Layout::AIVehicleOffset::AttackTarget;
		if (Read<std::uint32_t>(entry, NC2Layout::AIVehicleOffset::AttackActive) != 1 || !TargetData_IsValid(target)) return false;
		return Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PrimaryMilitaryTargetRevision) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectRevision) && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PrimaryMilitaryTargetValue) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition) && Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PrimaryMilitaryTarget) == Read<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type);
	}

}
