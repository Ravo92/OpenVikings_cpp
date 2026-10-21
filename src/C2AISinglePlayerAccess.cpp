#include "C2AIConditions.h"
#include "C2AIHumanRequests.h"
#include "C2AIMovementMessages.h"
#include "C2AISinglePlayerAccess.h"
#include "C2AISoldierAccess.h"
#include "C2AITaskAccess.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HouseBuildPosition.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanFamily.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMessageAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2MapWalkSectorManager.h"
#include "C2MilitaryTargetSearch.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2PlayerCenterPosition.h"
#include "C2PlayerStatisticAccess.h"
#include "C2RuntimeConstants.h"
#include "C2StaticHouseTypeData.h"
#include "C2WallRoadConstruction.h"
#include "SwitchIdentifiers.h"
#include "RemasterAIEconomy.h"
#include "XNumericConstants.h"
#include <initializer_list>

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		bool HasHumanNearby(std::uint32_t player, const void* point)
		{
			CHumanIterator iterator;
			return iterator.GetFirstSectorHumanId(player, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates), NC2Runtime::AISinglePlayerLimits::NearbyHumanRadius, false) != -1;
		}
		bool IsDamagedOwnedLandscape(const void* micro, std::uint32_t player)
		{
			return Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == SwitchIds::MapMicroObjectType::Landscape && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) != 0 && Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId) == static_cast<std::int32_t>(player) && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapeValency) < 100;
		}
	}
	// Original function: FUN_0045b612.
	void AISinglePlayerHandler_l_Misc_CheckAndAddDefaultTasks(void* handler)
	{
		std::uint32_t center = 0;
		Tool_Misc_GetPlayerCenterPosition(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), &center);
		if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::DefaultSoldierPositionEnabled) == 0) AISinglePlayerHandler_EntryPoint_SingleSoldier_SetDefaultPosition(handler, &center, NC2Runtime::AISinglePlayerLimits::DefaultSoldierRadius);
		for (std::uint32_t i = 0; i < NC2Runtime::AISinglePlayerLimits::MainTasks; ++i) if (Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::FirstMainTaskDescription + i * NC2Layout::RecordSize::AIMainTask) != 0) return;
		std::byte task[NC2Layout::RecordSize::AIMainTask]{ };
		AIMainTask_Init(task, 1, 10, 100000);
		Write(task, NC2Layout::AIMainTaskOffset::PrimaryPosition, center);
		Write<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::Radius, NC2Runtime::AISinglePlayerLimits::DefaultMainTaskRadius);
		AISinglePlayerHandler_MainTask_Add(handler, task);
	}
	// Original function: FUN_0045b6ab.
	void AISinglePlayerHandler_l_Women_WorkOn(void* handler)
	{
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
		const bool balanced = RemasterAIEconomy::IsActive(player);
		auto population = balanced ? RemasterAIEconomy::Inspect(player) : RemasterAIEconomy::SWorkforce{};
		auto plannedWorkers = population.workers;
		auto plannedPopulation = population.population;
		auto plannedWomen = population.women;
		if (balanced)
		{
			CHumanIterator humans;
			for (auto id = humans.GetFirstHumanId(player); id != -1; id = humans.GetNextHumanId())
			{
				const auto* human = GetHumanPtr(id);
				if (!Human_IsAgeAdult(human))
				{
					if (Human_IsSexFemale(human)) ++plannedWomen;
					else ++plannedWorkers;
				}
				// A baby already created by the native birth event is in the population above.
				if (Human_IsJobWoman(human) && Human_UserControl_DoesWantToGetChild(human) && !Human_IsHavingChild(human))
				{
					++plannedPopulation;
					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == SwitchIds::HumanTask::GetGirl || Human_UserControl_IsCommandTypeInQueue(human, SwitchIds::UserCommand::GetGirl)) ++plannedWomen;
					else ++plannedWorkers;
				}
			}
		}
		CHumanIterator iterator;
		for (auto id = iterator.GetFirstSectorHumanId(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), false); id != -1; id = iterator.GetNextSectorHumanId())
		{
			auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize;
			if (!Human_IsJobWoman(human) || Human_IsAboutToMarry(human)) continue;
			if (!Human_IsMarried(human))
			{
				Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::MarryAlternate, Read<std::uint8_t>(human, NC2Layout::HumanOffset::PlayerId), 0, 0, 0, 0);
				continue;
			}
			if (!Human_IsAttachedToHomeHouse(human)) continue;
			auto* home = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId) * CHouseArray::ElementSize;
			const auto* stats = PlayerStatisticManager_GetCurrentStatistics(gPlayerStatisticManager, Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId));
			if (stats == nullptr) continue;
			const auto males = Read<std::uint16_t>(stats, NC2Layout::PlayerStatisticSampleOffset::AdultMaleCount);
			const auto soldiers = Read<std::uint16_t>(stats, NC2Layout::PlayerStatisticSampleOffset::SoldierCount);
			const auto workers = balanced ? plannedWorkers : static_cast<std::uint32_t>(males) - soldiers;
			if (workers >= Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::UnitLimit)) continue;
			const auto maximum = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::MaxUnitLimit);
			if (maximum != 0 && (balanced ? plannedPopulation : Read<std::uint16_t>(stats, NC2Layout::PlayerStatisticSampleOffset::HumanCount)) >= maximum) continue;
			if (!Human_IsAbleToGetPregnant(human) || (balanced && Human_UserControl_DoesWantToGetChild(human)) || House_Stock_RequestAmountOfGoods(home, 16, false) < (balanced ? 2u : 3u)) continue;
			auto womenTarget = (plannedWorkers + population.soldiers) / 3;
			if (womenTarget < 4) womenTarget = 4;
			if (balanced && womenTarget > RemasterAIEconomy::WomenLimit()) womenTarget = RemasterAIEconomy::WomenLimit();
			if (balanced ? plannedWomen < womenTarget : static_cast<std::int32_t>(workers) / 3 > Read<std::uint16_t>(stats, NC2Layout::PlayerStatisticSampleOffset::AdultFemaleCount))
			{
				Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::GetGirl, Read<std::uint8_t>(human, NC2Layout::HumanOffset::PlayerId), 0, 0, 0, 0);
				if (!balanced) return;
				++plannedWomen; ++plannedPopulation;
				continue;
			}
			Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::GetBoy, Read<std::uint8_t>(human, NC2Layout::HumanOffset::PlayerId), 0, 0, 0, 0);
			if (balanced) { ++plannedWorkers; ++plannedPopulation; }
		}
	}
	// Original function: FUN_0045b88c.
	void AISinglePlayerHandler_l_BuildWallGates_SaveAllIntoArray(void* handler)
	{
		std::uint32_t point = 0;
		Tool_Misc_GetPlayerCenterPosition(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), &point);
		const auto maximum = GameMap_GetWidth() > GameMap_GetHeight() ? GameMap_GetWidth() : GameMap_GetHeight();
		const auto save = [&] (bool checkCapacity)
		{
			const auto* micro = GameMap_GetConstMapMicroPointPtr(&point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != SwitchIds::MapMicroObjectType::Landscape) return;
			const auto landscape = Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData);
			if (landscape == SwitchIds::WallLandscapeType::Wall && Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId) == static_cast<std::int32_t>(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId)))
			{
				const auto count = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::WallCount);
				if (checkCapacity && count == NC2Runtime::AISinglePlayerLimits::Walls) return;
				Write(handler, NC2Layout::AISinglePlayerOffset::FirstWallPosition + count * NC2Layout::RecordSize::AISingleWall, point);
				if (checkCapacity) Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::FirstWallWorkPendingFlag + count * NC2Layout::RecordSize::AISingleWall, 0);
				Write(handler, NC2Layout::AISinglePlayerOffset::WallCount, count + 1);
			}
			else if (landscape == SwitchIds::WallLandscapeType::GateOpen || landscape == SwitchIds::WallLandscapeType::GateClosed)
			{
				const auto count = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::GateCount);
				if (checkCapacity && count == NC2Runtime::AISinglePlayerLimits::Gates) return;
				Write(handler, NC2Layout::AISinglePlayerOffset::FirstGatePosition + count * NC2Layout::RecordSize::AISingleGate, point);
				// Windows seeds this byte from GateClosed, then stores open/close command values.
				Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::FirstGateStateFlag + count * NC2Layout::RecordSize::AISingleGate, landscape == SwitchIds::WallLandscapeType::GateClosed);
				Write(handler, NC2Layout::AISinglePlayerOffset::GateCount, count + 1);
			}
		};
		// Windows skips capacity checks and wall-request clearing for the center point.
		save(false);
		for (std::uint32_t radius = 1; radius < static_cast<std::uint32_t>(maximum); ++radius)
		{
			MapMigPoint_MoveCursorUnwrapped(&point, 4, 1);
			for (std::uint32_t direction = 0; direction < 6; ++direction)
				for (std::uint32_t i = 0; i < radius; ++i)
				{
					MapMigPoint_MoveCursorUnwrapped(&point, direction, 1);
					if (MapMigPoint_IsInsideMap(&point)) save(true);
				}
		}
	}
	// Original function: FUN_0045ba65.
	void AISinglePlayerHandler_l_BuildWall_CheckForBuildOrRepairWall(void* handler)
	{
		Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::WallWorkPendingFlag, 0);
		if (Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::WallCount) == 0) return;
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
		const auto* stats = PlayerStatisticManager_GetCurrentStatistics(gPlayerStatisticManager, player);
		if (stats == nullptr || Read<std::uint16_t>(stats, NC2Layout::PlayerStatisticSampleOffset::HumanCount) == 0) return;
		std::uint32_t count = 0;
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::WallCount); ++i)
		{
			auto* point = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::FirstWallPosition + i * NC2Layout::RecordSize::AISingleWall;
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			const bool build = (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7Signed) == 0;
			if (!build && !IsDamagedOwnedLandscape(micro, player)) continue;
			if (AISinglePlayerHandler_l_IsEnemySodlierOrVehicleNearPosition(handler, point) || !HasHumanNearby(player, point)) continue;
			Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::WallWorkPendingFlag, 1);
			if (build) WallBuildManager_SetWallBuildFlag(gWallBuildManager, nullptr, player, point);
			Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::FirstWallWorkPendingFlag + i * NC2Layout::RecordSize::AISingleWall, 1);
			if (++count == NC2Runtime::AISinglePlayerLimits::WallOrGateWorkPerPass) return;
		}
	}
	// Original function: FUN_0045bb8c.
	void AISinglePlayerHandler_l_BuildGate_CheckForDamagedGate(void* handler)
	{
		Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::GateRepairPendingFlag, 0);
		if (Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::GateCount) == 0) return;
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
		const auto* stats = PlayerStatisticManager_GetCurrentStatistics(gPlayerStatisticManager, player);
		if (stats == nullptr || Read<std::uint16_t>(stats, NC2Layout::PlayerStatisticSampleOffset::HumanCount) == 0) return;
		std::uint32_t count = 0;
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::GateCount); ++i)
		{
			auto* point = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::FirstGatePosition + i * NC2Layout::RecordSize::AISingleGate;
			if (!IsDamagedOwnedLandscape(GameMap_GetConstMapMicroPointPtr(point), player)) continue;
			if (AISinglePlayerHandler_l_IsEnemySodlierOrVehicleNearPosition(handler, point) || !HasHumanNearby(player, point)) continue;
			Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::GateRepairPendingFlag, 1);
			Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::FirstGateRepairPendingFlag + i * NC2Layout::RecordSize::AISingleGate, 1);
			if (++count == NC2Runtime::AISinglePlayerLimits::WallOrGateWorkPerPass) return;
		}
	}
	// Original function: FUN_0045bc60.
	void AISinglePlayerHandler_l_CheckDefenceMode(void* handler)
	{
		const auto time = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::LastAttackedTick);
		if (time == 0 || time + NC2Runtime::AISinglePlayerLimits::DefenceTimeoutTicks >= gGameTime.tick) return;
		std::uint32_t origin = 0;
		CHouseIterator iterator;
		for (const bool secondList : {true, false})
			for (auto id = iterator.GetFirstSectorHouseId(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), &origin, NC2Runtime::AISinglePlayerLimits::DefenceHouseSearchRadius, secondList); id != -1; id = iterator.GetNextSectorHouseId()) House_SetDefenceModeFlag(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize, false);
		Write<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::LastAttackedTick, 0);
	}
	// Original function: FUN_0045bd18.
	bool AISinglePlayerHandler_l_IsEnemySodlierOrVehicleNearPosition(void* handler, const void* point)
	{
		if (Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), point, 0, NC2Runtime::AISinglePlayerLimits::EnemySearchRadius, true, 1, 0, true, false) != -1) return true;
		return Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId), point, 0, NC2Runtime::AISinglePlayerLimits::EnemySearchRadius, true, 1, 0, false, false) != -1;
	}
	// Original function: FUN_0045bd63.
	void AISinglePlayerHandler_l_OpenCloseGates(void* handler)
	{
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::GateCount); ++i)
		{
			auto* point = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::FirstGatePosition + i * NC2Layout::RecordSize::AISingleGate;
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);
			if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != SwitchIds::MapMicroObjectType::Landscape || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0 || Read<std::int8_t>(micro, NC2Layout::MapMicroPointOffset::LandscapePlayerId) != static_cast<std::int32_t>(player)) continue;
			const bool enemy = AISinglePlayerHandler_l_IsEnemySodlierOrVehicleNearPosition(handler, point);
			const auto flag = Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::FirstGateStateFlag + i * NC2Layout::RecordSize::AISingleGate);
			if ((enemy && flag == 1) || (!enemy && flag == 0))
			{
				MapTool_WallGate_SetState(player, point, !enemy);
				Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::FirstGateStateFlag + i * NC2Layout::RecordSize::AISingleGate, !enemy);
			}
		}
	}
	// Original function: FUN_00473070.
	void AISinglePlayerHandler_l_Messages_TryToRepairAllMainTasks(void* handler)
	{
		for (std::uint32_t i = 0; i <= Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::LastMainTaskIndex); ++i)
		{
			auto* task = static_cast<std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::MainTasks + i * NC2Layout::RecordSize::AIMainTask;
			if (Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::Description) == 0 || Read<std::uint8_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionPendingFlag) == 0) continue;
			std::uint32_t temporary = 0;
			if (!CMapWalkSectorManager::GetNextSectorBasePoint(task + NC2Layout::AIMainTaskOffset::NoWaySourcePoint, task + NC2Layout::AIMainTaskOffset::NoWayTargetPoint, 0, true, &temporary, &temporary, 40, 20)) continue;
			Write<std::uint8_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionPendingFlag, 0);
			const auto type = Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::NoWayCorrectionMode);
			if (type == 1 || type == 3) Write(task, NC2Layout::AIMainTaskOffset::PrimaryPosition, Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::NoWayTargetPoint));
			else if (type == 2) Write(task, NC2Layout::AIMainTaskOffset::SecondaryPosition, Read<std::uint32_t>(task, NC2Layout::AIMainTaskOffset::NoWayTargetPoint));
		}
	}
	// Original function: FUN_0044e49a.
	void AISinglePlayerHandler_l_BuildHouses_CheckForBuildHouse(void* handler)
	{
		if (Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::BuildHouseCount) == 0) return;
		const auto player = Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId);
		const auto* stats = PlayerStatisticManager_GetCurrentStatistics(gPlayerStatisticManager, player);
		if (stats == nullptr || Read<std::uint16_t>(stats, NC2Layout::PlayerStatisticSampleOffset::HumanCount) == 0) return;
		CHouseIterator iterator;
		std::uint32_t building = 0;
		for (auto id = iterator.GetFirstHouseId(player); id != -1; id = iterator.GetNextHouseId()) if (Read<std::uint32_t>(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize, NC2Layout::HouseOffset::ConstructionState) != 3) ++building;
		if (building >= NC2Runtime::AISinglePlayerLimits::UnfinishedHouses) return;
		for (std::uint32_t i = 0; i < Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::BuildHouseCount); ++i)
		{
			const auto* entry = static_cast<const std::byte*>(handler) + NC2Layout::AISinglePlayerOffset::BuildHouseTypes + i * NC2Layout::RecordSize::AIBuildHouse;
			const auto engine = Read<std::uint32_t>(entry, NC2Layout::AIBuildHouseOffset::EngineType), stage = Read<std::uint32_t>(entry, NC2Layout::AIBuildHouseOffset::DevelopmentStage);
			const auto* definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(engine);
			if (Read<std::uint32_t>(definition, NC2Layout::EngineHouseLogicViewOffset::ConstructionGoods + stage * NC2Layout::RecordSize::HouseConstructionGoods) == 0) continue;
			if (iterator.GetFirstHouseId(player, entry + NC2Layout::AIBuildHouseOffset::MapPosition, NC2Runtime::AISinglePlayerLimits::HouseBuildSearchRadius, engine) != -1) continue;
			std::uint32_t data[NC2Runtime::HouseBuildSearchCallbackComponent::Count] = { player, engine, stage, 0 };
			if (!CMapPathfinderElement::SearchOverMap(entry + NC2Layout::AIBuildHouseOffset::MapPosition, NC2Runtime::AISinglePlayerLimits::HouseBuildSearchRadius, false, Tool_HouseBuildPointSearchEvaluationFunction, data, false, player)) continue;
			if (AISinglePlayerHandler_l_IsEnemySodlierOrVehicleNearPosition(handler, data + NC2Runtime::HouseBuildSearchCallbackComponent::ResultPosition) || !HasHumanNearby(player, data + NC2Runtime::HouseBuildSearchCallbackComponent::ResultPosition)) continue;
			Logic_EntryPoint_House_Set(player, engine, stage, data + NC2Runtime::HouseBuildSearchCallbackComponent::ResultPosition, false, Read<std::uint32_t>(entry, NC2Layout::AIBuildHouseOffset::MissionCheckId));
			if (++building >= NC2Runtime::AISinglePlayerLimits::UnfinishedHouses) return;
		}
	}
	// Original function: FUN_0045b4f4; the native cadence and call order are preserved.
	void AISinglePlayerHandler_WorkOnAI(void* handler, std::uint32_t tick)
	{
		if (Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::Enabled) == 0) return;
		const auto initialize = Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::InitializationPendingFlag);
		if (initialize != 0 || Read<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::MainTasksChanged) != 0)
		{
			if (initialize != 0)
			{
				AISinglePlayerHandler_l_Misc_CheckAndAddDefaultTasks(handler);
				AISinglePlayerHandler_l_BuildHouses_SaveAllHousesIntoArray(handler);
				AISinglePlayerHandler_l_BuildWallGates_SaveAllIntoArray(handler);
			}
			AISinglePlayerHandler_l_SingleSoldiers_RecreateListCompletly(handler);
			AISinglePlayerHandler_l_Vehicles_RecreateListCompletly(handler);
			AISinglePlayerHandler_MainTask_RecheckAll(handler);
			Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::InitializationPendingFlag, 0);
			Write<std::uint8_t>(handler, NC2Layout::AISinglePlayerOffset::MainTasksChanged, 0);
		}
		AISinglePlayerHandler_l_Soldiers_AttachToTowers(handler);
		AISinglePlayerHandler_l_SingleSoldiers_UpdateList(handler);
		AISinglePlayerHandler_l_Vehicles_UpdateList(handler);
		AISinglePlayerHandler_l_Vehicles_CheckForCommander(handler);
		if (AISinglePlayerHandler_Condition_RecheckAll(handler, tick)) AISinglePlayerHandler_MainTask_RecheckAll(handler);
		AISinglePlayerHandler_l_SingleSoldiers_ClearInvalidTask(handler);
		if (tick % NC2Runtime::AISinglePlayerLimits::SoldierTaskUpdateInterval == 0)
		{
			AISinglePlayerHandler_l_CheckDefenceMode(handler);
			AISinglePlayerHandler_l_SingleSoldiers_UpdateAttachedTasks(handler);
		}
		AISinglePlayerHandler_l_SingleSoldiers_WorkOnAttachedTasks(handler);
		AISinglePlayerHandler_l_Vehicles_WorkOnAttachedTasks(handler);
		if (tick % NC2Runtime::AISinglePlayerLimits::EnergyRepairAndFamilyInterval == 0)
		{
			AISinglePlayerHandler_l_SingleSoldiers_RefreshAllEnergyAndCondition(handler);
			AISinglePlayerHandler_l_Messages_TryToRepairAllMainTasks(handler);
		}
		if (tick % NC2Runtime::AISinglePlayerLimits::HouseBuildInterval == 0) AISinglePlayerHandler_l_BuildHouses_CheckForBuildHouse(handler);
		if (tick % NC2Runtime::AISinglePlayerLimits::ConstructionAndInhabitantsInterval == 0)
		{
			AISinglePlayerHandler_l_BuildWall_CheckForBuildOrRepairWall(handler);
			AISinglePlayerHandler_l_BuildGate_CheckForDamagedGate(handler);
			AISinglePlayerHandler_l_Misc_Human_GetInhabitants(handler);
		}
		const auto familyInterval = RemasterAIEconomy::IsActive(Read<std::uint32_t>(handler, NC2Layout::AISinglePlayerOffset::PlayerId)) ? 6u : NC2Runtime::AISinglePlayerLimits::EnergyRepairAndFamilyInterval;
		if (tick % familyInterval == 0) AISinglePlayerHandler_l_Women_WorkOn(handler);
		AISinglePlayerHandler_l_OpenCloseGates(handler);
	}
}
