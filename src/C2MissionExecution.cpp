#include "C2AISavegameAccess.h"
#include "C2AnimalAccess.h"
#include "C2CampaignStaticDataManager.h"
#include "C2Debug.h"
#include "C2EngineLandscapeAccess.h"
#include "C2EngineLifetimeAccess.h"
#include "C2EngineMapTerrainAccess.h"
#include "C2GameControlNetwork.h"
#include "C2GameControlStartup.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2GuideAccess.h"
#include "C2HouseBuildPosition.h"
#include "C2HumanChest.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanInitialization.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanUserControl.h"
#include "C2HumanWalking.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapPathfinderSearch.h"
#include "C2MapResourceTools.h"
#include "C2MissionExecution.h"
#include "C2MissionOnscreenInfoAccess.h"
#include "C2MissionStartObjects.h"
#include "C2MultiPlayerGoalAccess.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2RuntimeConstants.h"
#include "C2SavegameIO.h"
#include "C2ScriptObjectAccess.h"
#include "C2StaticVehicleTypeData.h"
#include "C2StringAccess.h"
#include "C2TributeAccess.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "C2VehicleIterator.h"
#include "C2WallRoadConstruction.h"
#include "DSManager.h"
#include "InGameGuiManager.h"
#include "MasterControlProgram.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_00422fdb; the original index is unchecked.
	std::uint32_t SResult::GetParameter(std::uint32_t index) const
	{

		return Read<std::uint32_t>(this, NC2Layout::MissionResultOffset::ParametersUnsigned + index * NC2Layout::RecordSize::MissionResultParameterUnsigned);
	}

	namespace
	{
		void* GetMission(void* manager, std::uint32_t index)
		{

			return static_cast<std::byte*>(manager) + NC2Layout::MissionManagerOffset::Missions + index * NC2Layout::RecordSize::MissionDataUnsigned;
		}

		bool GoalsMatch(std::int32_t mode, std::int32_t count, std::int32_t fulfilled)
		{
			switch (mode)
			{
				case SwitchIds::MissionGoalMatchMode::All:
					return fulfilled == count;
				case SwitchIds::MissionGoalMatchMode::Any:
					return fulfilled >= 1;
				case SwitchIds::MissionGoalMatchMode::MajorityAtLeastHalf:
					return fulfilled >= (count >> 1) + count % 2;
				case SwitchIds::MissionGoalMatchMode::None:
					return fulfilled == 0;
				default:

					return false;
			}
		}
	}

	// Original function: FUN_004230ee; only tick callbacks at multiples of 36 evaluate goals.
	bool __cdecl MissionManager_l_Mission_Callback(TCallbackTypes type, std::uintptr_t /*userData*/, std::uintptr_t callbackValueTick, std::uintptr_t /*param2*/, std::uintptr_t /*param3*/)
	{
		std::uint32_t tick = static_cast<std::uint32_t>(callbackValueTick);

		if (type == SwitchIds::CallbackType::GameTick && tick % NC2Runtime::MissionTiming::EvaluationIntervalTicksUnsigned == 0)
			return MissionManager_CheckAllMissionGoals(gMissionManager);

		return true;
	}

	// Original function: FUN_0041e2ae; parent restoration takes precedence over submap start.
	bool MissionManager_CheckAllMissionGoals(void* manager)
	{
		Write<std::uint8_t>(manager, NC2Layout::MissionManagerOffset::StartSubMapFlag, 0);
		Write<std::uint8_t>(manager, NC2Layout::MissionManagerOffset::EvaluationInterruptedFlag, 0);

		for (std::int32_t index = 0; index < Read<std::int32_t>(manager, NC2Layout::MissionManagerOffset::MissionCount); ++index)
		{
			void* mission = GetMission(manager, static_cast<std::uint32_t>(index));

			if (Read<std::uint8_t>(mission, NC2Layout::MissionDataOffset::ActiveFlag) == 0)
				continue;

			MissionManager_l_CheckMission(manager, mission, true);

			if (Read<std::uint8_t>(manager, NC2Layout::MissionManagerOffset::EvaluationInterruptedFlag) != 0)
				break;
		}

		if (Read<std::uint8_t>(manager, NC2Layout::MissionManagerOffset::ReturnToParentFlag) != 0)
		{
			SubMap_BackToParent();
			return false;
		}

		if (Read<std::uint8_t>(manager, NC2Layout::MissionManagerOffset::StartSubMapFlag) != 0)
		{
			const auto campaignId = Read<std::uint32_t>(manager, NC2Layout::MissionManagerOffset::SubMapCampaignId), mapId = Read<std::uint32_t>(manager, NC2Layout::MissionManagerOffset::SubMapId), pathfinderLimitOverride = Read<std::uint32_t>(gGameControl, NC2Layout::GameControlOffset::DifficultyLevel);
			SubMap_StartSubMap(campaignId, mapId, pathfinderLimitOverride);
			// The old mission manager can have been freed by either transition.
			return false;
		}

		return true;
	}

	// Original function: FUN_0041e31b; timed goals are handled before the general predicate.
	bool MissionManager_l_CheckMission(void* manager, void* mission, bool executeResults)
	{
		std::int32_t fulfilled = 0;

		for (std::int32_t index = 0; index < Read<std::int32_t>(mission, NC2Layout::MissionDataOffset::GoalCount); ++index)
		{
			auto* goal = reinterpret_cast<SMissionGoal*>(static_cast<std::byte*>(mission) + NC2Layout::MissionDataOffset::Goals + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::MissionGoalUnsigned);
			bool success;

			if (goal->type == SwitchIds::MissionGoal::TimeGone || goal->type == SwitchIds::MissionGoal::RandomTimeGone)
			{
				auto seconds = goal->parameters[0];

				if (goal->type == SwitchIds::MissionGoal::RandomTimeGone)
				{
					if (goal->parameters[1] == 0)
					{
						const auto half = static_cast<std::uint32_t>(NXBasics::Runtime_TimeSigned32(seconds) / 2);
						goal->parameters[1] = Game_Random_GetNumber() % half + half;
					}

					seconds = goal->parameters[1];
				}

				success = Read<std::uint32_t>(mission, NC2Layout::MissionDataOffset::ActivationTick) + seconds * NC2Runtime::MissionTiming::TicksPerSecondUnsigned <= gGameTime.tick;

				if (success && goal->type == SwitchIds::MissionGoal::RandomTimeGone)
					goal->parameters[1] = 0;
			}
			else
			{
				success = MissionManager_l_CheckGoal(manager, *goal);
			}

			Write<std::uint8_t>(goal, NC2Layout::MissionGoalOffset::FulfilledFlag, success);

			if (success)
				++fulfilled;
		}

		const bool success = GoalsMatch(Read<std::int32_t>(mission, NC2Layout::MissionDataOffset::GoalMatchMode), Read<std::int32_t>(mission, NC2Layout::MissionDataOffset::GoalCount), fulfilled);

		if (success && executeResults)
		{
			MissionData_SetMissionActiveFlag(mission, false);

			for (std::int32_t index = 0; index < Read<std::int32_t>(mission, NC2Layout::MissionDataOffset::ResultCount); ++index)
			{
				const auto* result = reinterpret_cast<const SResult*>(static_cast<const std::byte*>(mission) + NC2Layout::MissionDataOffset::Results + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::MissionResultUnsigned);
				MissionManager_l_ExecuteResult(manager, *result);
			}
		}

		Write<std::uint8_t>(mission, NC2Layout::MissionDataOffset::FulfilledFlag, success);

		return success;
	}

	// Original function: FUN_0041e45e; all 63 native goal IDs retain their distinct branches.
	bool MissionManager_l_CheckGoal(void* manager, const SMissionGoal& goal)
	{
		const auto* p = goal.parameters;

		switch (goal.type)
		{
			case SwitchIds::MissionGoal::True:
				return true;
			case SwitchIds::MissionGoal::BuildHouses:
			{
				CHouseIterator iterator;
				std::uint32_t count = 0;
				bool fulfilled = false;

				for (auto id = iterator.GetFirstHouseId(p[0]); id != -1; id = iterator.GetNextHouseId())
				{
					void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

					if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) != p[1] || Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3)
						continue;

					++count;
					Write(house, NC2Layout::HouseOffset::MissionCheckId, p[3]);

					if (NXBasics::Runtime_TimeSigned32(count) >= NXBasics::Runtime_TimeSigned32(p[2]))
						fulfilled = true;
				}

				return fulfilled;
			}
			case SwitchIds::MissionGoal::GoodsInHouses:
			{
				std::uint32_t count = 0;

				for (auto id = MissionCheck_GetFirstHouseIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHouseIdWithMissionCheckId(p[0], id))
				{
					const void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;
					count += House_Stock_GetAmountOfGoods(house, p[1], false);

					if (NXBasics::Runtime_TimeSigned32(count) >= NXBasics::Runtime_TimeSigned32(p[2]))
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::FindHouses:
				for (auto id = MissionCheck_GetFirstHouseIdWithMissionCheckId(p[1]); id != -1; id = MissionCheck_GetNextHouseIdWithMissionCheckId(p[1], id))
				{
					const void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;
					const void* micro = GameMap_GetConstMapMicroPointPtr(House_GetMapPositionRef(house));

					if (p[0] >= 16 || (Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << p[0])) != 0)
						return true;
				}

				return false;
			case SwitchIds::MissionGoal::FindPosByHumans:
				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(p[0], id))
				{
					const void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
					const void* point = Human_GetMapPositionRef(human);

					if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapY), static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]))) <= p[3])
						return true;
				}

				return false;
			case SwitchIds::MissionGoal::FindPosByVehicles:
				for (auto id = MissionCheck_GetFirstVehicleIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextVehicleIdWithMissionCheckId(p[0], id))
				{
					const void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize;
					const void* point = Vehicle_GetMapPositionRef(vehicle);

					if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapY), static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]))) <= p[3])
						return true;
				}

				return false;
			case SwitchIds::MissionGoal::FindHousesByHumans:
				for (auto humanId = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]); humanId != -1; humanId = MissionCheck_GetNextHumanIdWithMissionCheckId(p[0], humanId))
				{
					const void* point = Human_GetMapPositionRef(CHumanArray::mStaticVars + static_cast<std::uint32_t>(humanId) * CHumanArray::RuntimeElementSize);

					for (auto houseId = MissionCheck_GetFirstHouseIdWithMissionCheckId(p[1]); houseId != -1; houseId = MissionCheck_GetNextHouseIdWithMissionCheckId(p[1], houseId))
					{
						const void* target = House_GetMapPositionRef(CHouseArray::mStaticVars + static_cast<std::uint32_t>(houseId) * CHouseArray::RuntimeElementSize);

						if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapY), Read<std::int16_t>(target, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(target, NC2Layout::MapMigPointOffset::MapY))) <= p[2])
							return true;
					}
				}

				return false;
			case SwitchIds::MissionGoal::FindHousesByVehicles:
				for (auto vehicleId = MissionCheck_GetFirstVehicleIdWithMissionCheckId(p[0]); vehicleId != -1; vehicleId = MissionCheck_GetNextVehicleIdWithMissionCheckId(p[0], vehicleId))
				{
					const void* point = Vehicle_GetMapPositionRef(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(vehicleId) * CVehicleArray::RuntimeElementSize);

					for (auto houseId = MissionCheck_GetFirstHouseIdWithMissionCheckId(p[1]); houseId != -1; houseId = MissionCheck_GetNextHouseIdWithMissionCheckId(p[1], houseId))
					{
						const void* target = House_GetMapPositionRef(CHouseArray::mStaticVars + static_cast<std::uint32_t>(houseId) * CHouseArray::RuntimeElementSize);

						if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(point, NC2Layout::MapMigPointOffset::MapY), Read<std::int16_t>(target, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(target, NC2Layout::MapMigPointOffset::MapY))) <= p[2])
							return true;
					}
				}

				return false;
			case SwitchIds::MissionGoal::HumansDied:
				return MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]) == -1;
			case SwitchIds::MissionGoal::VehiclesDied:
				return MissionCheck_GetFirstVehicleIdWithMissionCheckId(p[0]) == -1;
			case SwitchIds::MissionGoal::HousesDied:
				return MissionCheck_GetFirstHouseIdWithMissionCheckId(p[0]) == -1;
			case SwitchIds::MissionGoal::PlayerDied:
				return NXBasics::Runtime_TimeSigned32(p[0]) < 20 && Read<std::uint8_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::DeadFlagUnsigned) != 0;
			case SwitchIds::MissionGoal::FindPosByPlayersMapMoveable:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				CHumanIterator humans;

				if (humans.GetFirstHumanId(p[0], &point, NXBasics::Runtime_TimeSigned32(p[3])) != -1)
					return true;

				CVehicleIterator vehicles;
				return vehicles.GetFirstVehicleId(p[0], &point, NXBasics::Runtime_TimeSigned32(p[3])) != -1;
			}
			case SwitchIds::MissionGoal::BuildHouseOnContinent:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId);
				CHouseIterator iterator;

				for (auto id = iterator.GetFirstHouseId(p[0]); id != -1; id = iterator.GetNextHouseId())
				{
					const void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

					if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == p[3] && Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(House_GetMapPositionRef(house)), NC2Layout::MapMicroPointOffset::ContinentId) == continent)
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::DiplomacyState:
				return GameControl_Diplomacy_GetState(gGameControl, NXBasics::Runtime_TimeSigned32(goal.parameters[0]), NXBasics::Runtime_TimeSigned32(goal.parameters[1])) == NXBasics::Runtime_TimeSigned32(goal.parameters[2]);
			case SwitchIds::MissionGoal::HumansWithHome:
			{
				CHumanIterator iterator;
				std::uint32_t count = 0;

				for (auto id = iterator.GetFirstHumanId(p[0]); id != -1; id = iterator.GetNextHumanId())
				{
					const void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

					if (!Human_IsAttachedToHomeHouse(human) || !Human_IsAgeAdult(human))
						continue;

					const void* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId) * CHouseArray::RuntimeElementSize;

					if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 && NXBasics::Runtime_TimeSigned32(++count) >= NXBasics::Runtime_TimeSigned32(p[1]))
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::CheckMission:
				return MissionManager_l_CheckMission(manager, GetMission(manager, goal.parameters[0]), false);
			case SwitchIds::MissionGoal::PayTribute:
			{
				const auto* tribute = static_cast<const STributeData*>(gTributeManager) + p[0];
				return tribute->allocated != 0 && tribute->paid != 0;
			}
			case SwitchIds::MissionGoal::Population:
			{
				CHumanIterator iterator;
				std::int32_t count = 0;

				for (auto id = iterator.GetFirstHumanId(goal.parameters[0]); id != -1; id = iterator.GetNextHumanId())
				{
					++count;

					if (NXBasics::Runtime_TimeSigned32(goal.parameters[1]) <= count)
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::JobEnabled:
				return Read<std::uint8_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + p[1] * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + p[2] + NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned) != 0;
			case SwitchIds::MissionGoal::GoodProduceable:
				return Read<std::uint8_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + p[1] * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + p[2] + NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned) != 0;
			case SwitchIds::MissionGoal::HumanAttachedToWorkHouse:
			{
				CHumanIterator iterator;
				std::uint32_t count = 0;

				for (auto id = iterator.GetFirstHumanId(p[0]); id != -1; id = iterator.GetNextHumanId())
				{
					const void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == p[1] && Human_IsAttachedToWorkHouse(human) && NXBasics::Runtime_TimeSigned32(++count) >= NXBasics::Runtime_TimeSigned32(p[2]))
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::IfMissionIsActive:
				return Read<std::uint8_t>(GetMission(manager, goal.parameters[0]), NC2Layout::MissionDataOffset::ActiveFlag) != 0;
			case SwitchIds::MissionGoal::IsMissionDone:
			{
				const void* mission = GetMission(manager, goal.parameters[0]);
				const auto count = Read<std::int32_t>(mission, NC2Layout::MissionDataOffset::GoalCount);
				std::int32_t fulfilled = 0;

				for (std::int32_t index = 0; index < count; ++index)
				{
					if (Read<std::uint8_t>(mission, NC2Layout::MissionDataOffset::FirstGoalFulfilledFlagUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::MissionGoalUnsigned) != 0)
						++fulfilled;
				}

				return GoalsMatch(Read<std::int32_t>(mission, NC2Layout::MissionDataOffset::GoalMatchMode), count, fulfilled);
			}
			case SwitchIds::MissionGoal::NumberOfHousesInArea:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[3]), static_cast<std::int16_t>(p[4]), false);
				return Tool_CountHousesOnMap(p[0], p[1], point, p[5]) >= p[2];
			}
			case SwitchIds::MissionGoal::NumberOfGoodsInHousesInArea:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[3]), static_cast<std::int16_t>(p[4]), false);
				return Tool_CountGoodsInHouses(p[0], p[1], point, p[5]) >= p[2];
			}
			case SwitchIds::MissionGoal::NumberOfGoodsInVehiclesInArea:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[4]), static_cast<std::int16_t>(p[5]), false);
				return Tool_CountGoodsInVehicles(p[0], p[1], p[2], point, p[6]) >= p[3];
			}

			case SwitchIds::MissionGoal::BuildVehicles:
			{
				CVehicleIterator iterator;
				std::uint32_t count = 0;
				bool fulfilled = false;

				for (auto id = iterator.GetFirstVehicleId(p[0]); id != -1; id = iterator.GetNextVehicleId())
				{
					void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize;

					if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType) != p[1])
						continue;

					Write(vehicle, NC2Layout::VehicleOffset::MissionCheckId, p[3]);

					if (NXBasics::Runtime_TimeSigned32(++count) >= NXBasics::Runtime_TimeSigned32(p[2]))
						fulfilled = true;
				}

				return fulfilled;
			}
			case SwitchIds::MissionGoal::BuildHumans:
			{
				CHumanIterator iterator;
				std::uint32_t count = 0;
				bool fulfilled = false;

				for (auto id = iterator.GetFirstHumanId(p[0]); id != -1; id = iterator.GetNextHumanId())
				{
					void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != p[1])
						continue;

					if (p[3] != 12345)
						Write(human, NC2Layout::HumanOffset::MissionCheckId, p[3]);

					if (NXBasics::Runtime_TimeSigned32(++count) >= NXBasics::Runtime_TimeSigned32(p[2]))
						fulfilled = true;
				}

				return fulfilled;
			}
			case SwitchIds::MissionGoal::GoodsInVehicles:
			{
				std::uint32_t count = 0;

				for (auto id = MissionCheck_GetFirstVehicleIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextVehicleIdWithMissionCheckId(p[0], id))
				{
					count += Vehicle_Stock_GetAmountOfGoods(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize, p[1]);

					if (NXBasics::Runtime_TimeSigned32(count) >= NXBasics::Runtime_TimeSigned32(p[2]))
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::GoodsGlobal:
			{
				CHouseIterator iterator;
				std::uint32_t count = 0;

				for (auto id = iterator.GetFirstHouseId(p[0]); id != -1; id = iterator.GetNextHouseId())
				{
					const void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

					if (!House_Stock_CanStoreGoodType(house, p[1], true))
						continue;

					count += House_Stock_GetAmountOfGoods(house, p[1], true);

					if (NXBasics::Runtime_TimeSigned32(count) >= NXBasics::Runtime_TimeSigned32(p[2]))
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::FindPos:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				return p[0] >= 16 || (Read<std::uint16_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << p[0])) != 0;
			}
			case SwitchIds::MissionGoal::FindHumans:
			case SwitchIds::MissionGoal::FindVehicles:
			case SwitchIds::MissionGoal::FindAnimals:
			{
				const auto first = goal.type == SwitchIds::MissionGoal::FindHumans ? MissionCheck_GetFirstHumanIdWithMissionCheckId : goal.type == SwitchIds::MissionGoal::FindVehicles ? MissionCheck_GetFirstVehicleIdWithMissionCheckId
				                                                                                                                                                                        : MissionCheck_GetFirstAnimalIdWithMissionCheckId;
				const auto next = goal.type == SwitchIds::MissionGoal::FindHumans ? MissionCheck_GetNextHumanIdWithMissionCheckId : goal.type == SwitchIds::MissionGoal::FindVehicles ? MissionCheck_GetNextVehicleIdWithMissionCheckId
				                                                                                                                                                                      : MissionCheck_GetNextAnimalIdWithMissionCheckId;

				for (auto id = first(p[1]); id != -1; id = next(p[1], id))
				{
					const void* point = goal.type == SwitchIds::MissionGoal::FindHumans ? Human_GetMapPositionRef(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize) : goal.type == SwitchIds::MissionGoal::FindVehicles ? Vehicle_GetMapPositionRef(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize)
					                                                                                                                                                                                                                                               : Animal_GetMapPositionRef(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::RuntimeElementSize);
					const auto mask = Read<std::uint16_t>(GameMap_GetConstMapMicroPointPtr(point), NC2Layout::MapMicroPointOffset::ExplorationMask);

					if (p[0] >= 16 || (mask & (1u << p[0])) != 0)
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::FindHumansByHumans:
			case SwitchIds::MissionGoal::FindHumansByVehicles:
			case SwitchIds::MissionGoal::FindVehiclesByVehicles:
			{
				const auto firstOuter = goal.type == SwitchIds::MissionGoal::FindHumansByHumans ? MissionCheck_GetFirstHumanIdWithMissionCheckId : MissionCheck_GetFirstVehicleIdWithMissionCheckId;
				const auto nextOuter = goal.type == SwitchIds::MissionGoal::FindHumansByHumans ? MissionCheck_GetNextHumanIdWithMissionCheckId : MissionCheck_GetNextVehicleIdWithMissionCheckId;
				const auto firstInner = goal.type == SwitchIds::MissionGoal::FindVehiclesByVehicles ? MissionCheck_GetFirstVehicleIdWithMissionCheckId : MissionCheck_GetFirstHumanIdWithMissionCheckId;
				const auto nextInner = goal.type == SwitchIds::MissionGoal::FindVehiclesByVehicles ? MissionCheck_GetNextVehicleIdWithMissionCheckId : MissionCheck_GetNextHumanIdWithMissionCheckId;

				for (auto outer = firstOuter(p[0]); outer != -1; outer = nextOuter(p[0], outer))
				{
					const void* a = goal.type == SwitchIds::MissionGoal::FindHumansByHumans ? Human_GetMapPositionRef(CHumanArray::mStaticVars + static_cast<std::uint32_t>(outer) * CHumanArray::RuntimeElementSize) : Vehicle_GetMapPositionRef(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(outer) * CVehicleArray::RuntimeElementSize);

					for (auto inner = firstInner(p[1]); inner != -1; inner = nextInner(p[1], inner))
					{
						const void* b = goal.type == SwitchIds::MissionGoal::FindVehiclesByVehicles ? Vehicle_GetMapPositionRef(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(inner) * CVehicleArray::RuntimeElementSize) : Human_GetMapPositionRef(CHumanArray::mStaticVars + static_cast<std::uint32_t>(inner) * CHumanArray::RuntimeElementSize);

						if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(a, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(a, NC2Layout::MapMigPointOffset::MapY), Read<std::int16_t>(b, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(b, NC2Layout::MapMigPointOffset::MapY))) <= p[2])
							return true;
					}
				}

				return false;
			}
			case SwitchIds::MissionGoal::NumberOfSoldiers:
			{
				CHumanIterator iterator;
				std::uint32_t count = 0;

				for (auto id = iterator.GetFirstHumanId(p[0]); id != -1; id = iterator.GetNextHumanId())
				{
					if (Human_IsJobSoldier(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize) && NXBasics::Runtime_TimeSigned32(++count) >= NXBasics::Runtime_TimeSigned32(p[1]))
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::DetectGuide:
			{
				CGuideIterator iterator;
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				return iterator.GetFirstGuideId(p[0], &point, p[3]) != -1;
			}
				// Timed goals are evaluated by l_CheckMission, not by this native dispatcher.
			case SwitchIds::MissionGoal::TimeGone:
			case SwitchIds::MissionGoal::RandomTimeGone:
				return false;
			case SwitchIds::MissionGoal::PlayerSeen:
				return Read<std::uint8_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + p[1] + NC2Layout::GameControlPlayerOffset::PlayerSeenFlagsUnsigned) != 0;
			case SwitchIds::MissionGoal::IsHumanInVehicle:
				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(p[0], id))
				{
					const void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

					if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) == 0 || Read<std::uint32_t>(CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::RuntimeElementSize, NC2Layout::VehicleOffset::MissionCheckId) != p[1])
						return false;
				}

				return true;
			case SwitchIds::MissionGoal::FindHumansByPlayersMM:
				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(p[0], id))
				{
					const void* point = Human_GetMapPositionRef(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize);
					CHumanIterator humans;

					if (humans.GetFirstHumanId(p[1], point, NXBasics::Runtime_TimeSigned32(p[2])) != -1)
						return true;

					CVehicleIterator vehicles;

					if (vehicles.GetFirstVehicleId(p[1], point, NXBasics::Runtime_TimeSigned32(p[2])) != -1)
						return true;
				}

				return false;
			case SwitchIds::MissionGoal::SoldiersDied:
				return Read<std::uint32_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::SoldiersDiedUnsigned) >= p[1];
			case SwitchIds::MissionGoal::AnimalsDied:
				return MissionCheck_GetNumberOfAnimalsWithMissionCheckId(p[0]) == 0;
			case SwitchIds::MissionGoal::PlayerAttackedByPlayer:
				return Read<std::uint8_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + p[1] + NC2Layout::GameControlPlayerOffset::AttackedByPlayersUnsigned) != 0;
			case SwitchIds::MissionGoal::NumberOfHumansDied:
				return Read<std::uint32_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::HumansDiedUnsigned) >= p[1];
			case SwitchIds::MissionGoal::NumberOfGoodsTraded:
				return NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + p[1] * NC2Layout::RecordSize::PlayerTradeCounterUnsigned + NC2Layout::GameControlPlayerOffset::GoodsReceivedFromPlayersUnsigned)) >= NXBasics::Runtime_TimeSigned32(p[2]);
			case SwitchIds::MissionGoal::CheckNumberOfWildAnimals:
			case SwitchIds::MissionGoal::NumberOfAnimals:
			{
				CAnimalIterator iterator;
				std::uint32_t count = 0;
				const auto type = goal.type == SwitchIds::MissionGoal::CheckNumberOfWildAnimals ? p[0] : p[1], threshold = goal.type == SwitchIds::MissionGoal::CheckNumberOfWildAnimals ? p[1] : p[2];

				for (auto id = iterator.GetFirstAnimalId(goal.type == SwitchIds::MissionGoal::CheckNumberOfWildAnimals ? 20 : p[0]); id != -1; id = iterator.GetNextAnimalId())
				{
					if (Read<std::uint32_t>(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::RuntimeElementSize, NC2Layout::AnimalOffset::TribeType) == type && NXBasics::Runtime_TimeSigned32(++count) >= NXBasics::Runtime_TimeSigned32(threshold))
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::NumberOfHumansKilled:
				return Read<std::uint32_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::CiviliansKilledUnsigned) + Read<std::uint32_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::SoldiersKilledUnsigned) >= p[1];
			case SwitchIds::MissionGoal::HumanIsOnContinent:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);

				if (!MapMigPoint_IsInsideMap(&point))
					return false;

				const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId);

				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(p[0], id))
				{
					if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize)), NC2Layout::MapMicroPointOffset::ContinentId) == continent)
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::NumberOfSoldiersNearPos:
			case SwitchIds::MissionGoal::NumberOfCivilainsNearPos:
			{
				std::uint32_t point, count = 0;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				CHumanIterator humans;

				for (auto id = humans.GetFirstSectorHumanId(p[0], point, NXBasics::Runtime_TimeSigned32(p[3]), goal.type == SwitchIds::MissionGoal::NumberOfSoldiersNearPos); id != -1; id = humans.GetNextSectorHumanId())
				{
					const void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

					if (goal.type == SwitchIds::MissionGoal::NumberOfCivilainsNearPos || (Human_IsJobSoldier(human) && !Human_IsJobHero(human)))
						++count;
				}

				CVehicleIterator vehicles;

				for (auto id = vehicles.GetFirstVehicleId(p[0], &point, NXBasics::Runtime_TimeSigned32(p[3])); id != -1; id = vehicles.GetNextVehicleId())
				{
					const void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize;
					auto qualifies = [&](std::uint32_t humanId)
					{
						const void* human = CHumanArray::mStaticVars + humanId * CHumanArray::RuntimeElementSize;

						return Human_IsJobSoldier(human) == (goal.type == SwitchIds::MissionGoal::NumberOfSoldiersNearPos) && !Human_IsJobHero(human);
					};

					if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::CommanderAttachedFlag) != 0 && qualifies(static_cast<std::uint32_t>(Vehicle_Passengers_GetAttachedCommanderId(vehicle))))
						++count;

					if (Vehicle_Passengers_GetNumberOfAttachedHumans(vehicle, true) != 0)
						for (std::uint32_t slot = 0; slot < static_cast<std::uint32_t>(Vehicle_GetVehicleTypeDataPtr(vehicle)->passengerSlots); ++slot)
						{
							const void* reference = static_cast<const std::byte*>(vehicle) + NC2Layout::VehicleOffset::HumanPassengers + slot * NC2Layout::RecordSize::VehiclePassenger;

							if (HumanReference_IsAllocated(reference) && qualifies(Read<std::uint32_t>(reference, NC2Layout::VehiclePassengerOffset::ObjectId)))
								++count;
						}
				}

				return NXBasics::Runtime_TimeSigned32(count) >= NXBasics::Runtime_TimeSigned32(p[4]);
			}
			case SwitchIds::MissionGoal::ChestNearPos:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[0]), static_cast<std::int16_t>(p[1]), false);
				auto isChest = [&]()
				{
					if (!MapMigPoint_IsInsideMap(&point))
						return false;

					const void* micro = GameMap_GetConstMapMicroPointPtr(&point);

					return Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 1 && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0x55 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0x56);
				};

				if (isChest())
					return true;

				for (std::int32_t radius = 1; radius <= NXBasics::Runtime_TimeSigned32(p[2]); ++radius)
				{
					MapMigPoint_MoveCursorUnwrapped(point, 4);

					for (std::uint32_t direction = 0; direction < 6; ++direction)
					{
						for (std::int32_t step = 0; step < radius; ++step)
						{
							if (isChest())
								return true;

							MapMigPoint_MoveCursorUnwrapped(point, direction);
						}
					}
				}

				return false;
			}
			case SwitchIds::MissionGoal::NumberOfGoodsInArea:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[3]), static_cast<std::int16_t>(p[4]), false);
				const auto houseCount = Tool_CountGoodsInHouses(p[0], p[1], point, p[5]);
				const auto count = houseCount + Tool_CountGoodsOnMap(p[1], point, p[5]);
				return count >= p[2];
			}
			case SwitchIds::MissionGoal::NumberOfVehiclesInArea:
			case SwitchIds::MissionGoal::NumberOfAnimalsInArea:
			{
				std::uint32_t point, count = 0;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[3]), static_cast<std::int16_t>(p[4]), false);

				if (goal.type == SwitchIds::MissionGoal::NumberOfVehiclesInArea)
				{
					CVehicleIterator iterator;

					for (auto id = iterator.GetFirstVehicleId(p[0], &point, NXBasics::Runtime_TimeSigned32(p[5])); id != -1; id = iterator.GetNextVehicleId())
					{
						if (Read<std::uint32_t>(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize, NC2Layout::VehicleOffset::LogicType) == p[1])
							++count;
					}

					return count >= p[2];
				}

				CAnimalIterator iterator;

				for (auto id = iterator.GetFirstAnimalId(p[0]); id != -1; id = iterator.GetNextAnimalId())
				{
					const void* animal = CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::RuntimeElementSize;

					if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType) != p[1])
						continue;

					const void* a = Animal_GetMapPositionRef(animal);

					if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(a, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(a, NC2Layout::MapMigPointOffset::MapY), Read<std::int16_t>(&point, NC2Layout::MapMigPointOffset::MapX), Read<std::int16_t>(&point, NC2Layout::MapMigPointOffset::MapY))) <= p[5] && NXBasics::Runtime_TimeSigned32(++count) >= NXBasics::Runtime_TimeSigned32(p[2]))
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::CheckHumanJob:
			{
				CHumanIterator iterator;

				for (auto id = iterator.GetFirstHumanId(); id != -1; id = iterator.GetNextHumanId())
				{
					const void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MissionCheckId) == p[0] && Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == p[1])
						return true;
				}

				return false;
			}
			case SwitchIds::MissionGoal::IsAnyLandscapeOnPoint:
			case SwitchIds::MissionGoal::IsLandscapePlayer10ConstructionSignOnPoint:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[0]), static_cast<std::int16_t>(p[1]), false);

				if (!MapMigPoint_IsInsideMap(&point))
					return false;

				const void* micro = GameMap_GetConstMapMicroPointPtr(&point);

				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 1 || Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) == 0)
					return false;

				if (goal.type == SwitchIds::MissionGoal::IsAnyLandscapeOnPoint)
					return true;

				const auto engine = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(&point);

				if (engine == -1)
					return false;

				const auto target = NC2LogicExternals::External_Landscape_GetLandscapeIdByNamePtr("player10 construction sign");
				return target != -1 && engine == target;
			}

			default:

				return false;
		}
	}

	// Original function: FUN_0041f9f7; all 103 native result IDs, including the two no-op entries.
	void MissionManager_l_ExecuteResult(void* manager, const SResult& result)
	{
		const auto* p = result.parameters;

		auto redraw = [](std::uint32_t point, std::int32_t radius)
		{
			CallbackManager_GetExistingObjectPtr()->ExecuteCallback(0x25, true, 4, point, 0);

			for (std::int32_t ring = 1; ring <= radius; ++ring)
			{
				MapMigPoint_MoveCursorUnwrapped(point, 4);

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					for (std::int32_t step = 0; step < ring; ++step)
					{
						CallbackManager_GetExistingObjectPtr()->ExecuteCallback(0x25, true, 4, point, 0);
						MapMigPoint_MoveCursorUnwrapped(point, direction);
					}
				}
			}
		};
		// Windows 004230EA/00471F57 share getter bodies; these reads retain the human receiver.
		auto moveHuman = [&](std::int32_t id, std::uint32_t target)
		{
			void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
			redraw(Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapMigPointOffset::Coordinates), 1);
			MapMoveable_Attachment_AttachToMap(gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable, &target);
			Human_DoAddNewUserCommand(human, 4, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), ArrayObject_GetId(human, nullptr), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), target, 0);
			GameMap_Exploration_DoExplore(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), &target, Human_GetExplorationRange(human, nullptr));
			redraw(target, 1);
		};
		// The same Windows getters read moveable object type and marker type in this loop.
		auto removeMarkers = [](std::uint32_t point, std::uint32_t type)
		{
			for (;;)
			{
				CMapMoveableIterator iterator;
				auto id = iterator.GetFirstMapMoveableId(&point);

				for (; id != -1; id = iterator.GetNextMapMoveableId())
				{
					const void* moveable = GetMapMoveablePtr(id);

					if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) == 4 && Read<std::uint32_t>(MapMoveable_GetMarkerPtr(moveable, nullptr), NC2Layout::MarkerOffset::Type) == type)
						break;
				}

				if (id == -1)
					break;

				Logic_EntryPoint_FreeMapMoveable(id);
				// Deletion invalidates the cursor; Windows restarts at the first object on this point.
			}
		};

		switch (result.type)
		{
			case SwitchIds::MissionResult::SetHuman:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[3]), static_cast<std::int16_t>(p[4]), false);
				Logic_EntryPoint_Add_Human(p[0], p[1], p[2], &point, p[5], p[6]);
				break;
			}
			case SwitchIds::MissionResult::SetVehicle:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[3]), static_cast<std::int16_t>(p[4]), false);
				const auto vehicleId = Logic_EntryPoint_Add_Vehicle(p[0], p[1], p[2], &point, p[5]);

				if (vehicleId == -1 || p[6] == 0)
					break;

				void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(vehicleId) * CVehicleArray::RuntimeElementSize;
				const auto* type = Vehicle_GetVehicleTypeDataPtr(vehicle);
				std::uint32_t entryPoint = 0;
				Vehicle_Door_GetEntryPoint(vehicle, &entryPoint, 0);

				if (!MapMigPoint_IsInsideMap(&entryPoint))
					break;

				const auto humanId = Logic_EntryPoint_Add_Human(p[0], p[1], static_cast<std::uint32_t>(type->commanderType), &entryPoint, p[5], 0);

				if (humanId == -1)
					break;

				void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(humanId) * CHumanArray::RuntimeElementSize;
				// Windows tests nonzero, including the command's unchanged/failed result 1.

				if (Human_DoExecuteUserCommand_AttachVehicle(human, vehicleId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId)) != 0 && Human_VehicleMisc_Enter(human) != 0)
					break;

				CHumanArray::sTheObjectPtr->FreeHuman(humanId);
				break;
			}
			case SwitchIds::MissionResult::SetHouse:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[4]), static_cast<std::int16_t>(p[5]), false);
				const auto engineType = NC2LogicExternals::External_HouseType_GetEngineId(result.text);
				std::uint32_t search[4] = { p[0], static_cast<std::uint32_t>(engineType), p[2], 0 };

				if (CMapPathfinderElement::SearchOverMap(&point, 12, false, Tool_HouseBuildPointSearchEvaluationFunction, search, false, -1))
				{
					Logic_EntryPoint_House_Set(p[0], static_cast<std::uint32_t>(engineType), p[2], search + 3, p[3] != 0, p[6]);
				}
				else
				{
					C2Debug_WarningBoxV("Can't set House near Position %d %d!!!", p[4], p[5]);
				}

				break;
			}
			case SwitchIds::MissionResult::SetLandscape:
			{
				Logic_EntryPoint_Edit_Start();
				const auto engineId = NC2LogicExternals::External_Landscape_GetLandscapeIdByNamePtr(result.text);
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[0]), static_cast<std::int16_t>(p[1]), false);
				NC2LogicExternals::External_Landscape_SetMapMicroPointLandscape(&point, engineId, p[3], p[4] != 0, -1);
				Logic_EntryPoint_Edit_Stop();
				break;
			}
			case SwitchIds::MissionResult::RemoveHumans:
				Write<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDeathStatisticsFlag, 1);

				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(p[0], id))
				{
					Logic_EntryPoint_FreeMapMoveable(Read<std::uint32_t>(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize, NC2Layout::HumanOffset::MapMoveableId));
				}

				Write<std::uint8_t>(&gGameStaticVars, NC2Layout::GameStaticVarsOffset::SuppressDeathStatisticsFlag, 0);
				break;
			case SwitchIds::MissionResult::RemoveVehicles:
				for (auto id = MissionCheck_GetFirstVehicleIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextVehicleIdWithMissionCheckId(p[0], id))
				{
					Logic_EntryPoint_FreeMapMoveable(Read<std::uint32_t>(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize, NC2Layout::VehicleOffset::MapMoveableId));
				}

				break;
			case SwitchIds::MissionResult::RemoveHouses:
				for (auto id = MissionCheck_GetFirstHouseIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHouseIdWithMissionCheckId(p[0], id))
				{
					Logic_EntryPoint_House_Remove(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize + 0x1C);
				}

				break;
			case SwitchIds::MissionResult::RemoveLandscape:
			{
				Logic_EntryPoint_Edit_Start();
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[0]), static_cast<std::int16_t>(p[1]), false);
				Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(&point, 0, 0, -1, 0, -1);
				Logic_EntryPoint_Edit_Stop();
				break;
			}
			case SwitchIds::MissionResult::PlayCutscene:
				if (p[1] != 0)
					Write(manager, NC2Layout::MissionManagerOffset::CurrentBriefingId, p[0]);

				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x1B, 1, p[0], 0, 0);
				Write<std::uint8_t>(manager, NC2Layout::MissionManagerOffset::EvaluationInterruptedFlag, 1);

				if (NDSManager::CManager::sTheObjectPtr != nullptr)
					NDSManager::CManager::sTheObjectPtr->FireAndForget_LoadAndPlay("data\\engine2d\\bin\\sounds\\gui\\Briefing_Popup.wav", 100, NXBasics::IntegerRange::UInt32Maximum, 1, false, false, 100);

				break;
			case SwitchIds::MissionResult::ActivateMission:
				MissionData_SetMissionActiveFlag(GetMission(manager, p[0]), true);
				break;
			case SwitchIds::MissionResult::DeactivateMission:
				MissionData_SetMissionActiveFlag(GetMission(manager, p[0]), false);
				break;
			case SwitchIds::MissionResult::MissionWon:
				NC2LogicExternals::External_Misc_SendToUserStringMessage(StringAccess_GetMiscStringPtr(10), false);
				Write<std::uint8_t>(manager, NC2Layout::MissionManagerOffset::WonFlag, 1);
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, p[0], 6, 0);

				if (gGameStaticVars.mapType == SwitchIds::GameMapType::Multiplayer)
					MultiPlayerGoalManager_ExternalTrigger_WonByMissionManager(gMultiPlayerGoalManager, p[0]);

				break;
			case SwitchIds::MissionResult::MissionFailed:
				NC2LogicExternals::External_Misc_SendToUserStringMessage(StringAccess_GetMiscStringPtr(11), false);
				Write<std::uint8_t>(manager, NC2Layout::MissionManagerOffset::FailedFlag, 1);
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, p[0], 7, 0);

				if (gGameStaticVars.mapType == SwitchIds::GameMapType::Multiplayer)
					MultiPlayerGoalManager_ExternalTrigger_LostByMissionManager(gMultiPlayerGoalManager, p[0]);

				break;
			case SwitchIds::MissionResult::AllowMap:
				gCampaignStaticDataManager->CampaignSubMission_DoAllow(p[0], p[1], true);
				break;
			case SwitchIds::MissionResult::ExploreArea:
			{
				// SMapMigPoint::SetPosition(..., false) truncates both coordinates to WORDs.
				const std::uint32_t point = static_cast<std::uint16_t>(p[1]) | (static_cast<std::uint32_t>(static_cast<std::uint16_t>(p[2])) << 16);

				if (Read<std::uint16_t>(&point, NC2Layout::MapMigPointOffset::MapX) != 0 && Read<std::uint16_t>(&point, NC2Layout::MapMigPointOffset::MapY) != 0 && p[3] != 0)
				{
					GameMap_Exploration_DoExplore(p[0], &point, p[3]);
				}
				else
				{
					GameMap_Exploration_DoExploreAll(p[0]);
				}

				break;
			}
			case SwitchIds::MissionResult::Exit:
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x2F, 1, 0, 0, 0);
				break;
			case SwitchIds::MissionResult::SetExternalFlag:
				AIManager_EntryPoint_Condition_ExternalActivate_SetFlag(gAIManager, p[0], p[1], p[2] != 0);
				break;
			case SwitchIds::MissionResult::SetDiplomacy:
				GameControl_Diplomacy_SetState(gGameControl, nullptr, p[0], p[1], p[2]);
				break;
			case SwitchIds::MissionResult::SetRandomChestOnRandomPos:
				Tool_MapChest_SetRandomChestOnRandomPosition(p[0]);
				break;
			case SwitchIds::MissionResult::SetRandomChestOnPosition:
			{
				const auto mask = p[0];
				const auto y = p[2];
				const auto x = p[1];
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), false);
				Tool_MapChest_SetRandomChestOnPosition(mask, &point);
				break;
			}
			case SwitchIds::MissionResult::SetVisible:
				Write<std::uint8_t>(GetMission(manager, p[0]), NC2Layout::MissionDataOffset::VisibleFlag, p[1] != 0);
				break;
			case SwitchIds::MissionResult::ChangeHumanPlayerId:
				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(p[0], id))
				{
					Human_ChangePlayerId(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize, NXBasics::Runtime_TimeSigned32(p[1]), true);
				}

				break;
			case SwitchIds::MissionResult::ChangePlayerPlayerId:
			{
				CHumanIterator humans;
				CAnimalIterator animals;
				CVehicleIterator vehicles;
				CHouseIterator houses;
				CGuideIterator guides;

				for (auto id = vehicles.GetFirstVehicleId(p[0]); id != -1; id = vehicles.GetNextVehicleId())
				{
					Vehicle_ChangePlayerId(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize, p[1]);
				}

				for (auto id = houses.GetFirstHouseId(p[0]); id != -1; id = houses.GetNextHouseId())
				{
					House_ChangePlayerId(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize, p[1]);
				}

				for (auto id = humans.GetFirstHumanId(p[0]); id != -1; id = humans.GetNextHumanId())
				{
					Human_ChangePlayerId(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize, NXBasics::Runtime_TimeSigned32(p[1]), false);
				}

				for (auto id = animals.GetFirstAnimalId(p[0]); id != -1; id = animals.GetNextAnimalId())
				{
					Animal_ChangePlayerId(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::RuntimeElementSize, p[1]);
				}

				for (auto id = guides.GetFirstGuideId(p[0]); id != -1; id = guides.GetNextGuideId())
				{
					Write(CGuideArray::mStaticVars + static_cast<std::uint32_t>(id) * CGuideArray::RuntimeElementSize, NC2Layout::GuideOffset::PlayerId, p[1]);
					GuideManager_RecalculateGuideConnections(gGuideManager, id);
				}

				if (NXBasics::Runtime_TimeSigned32(p[1]) < 20 && Read<std::uint32_t>(gGameControl, p[1] * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::Type) == 2)
					AIManager_EntryPoint_RebuildHouseArray(gAIManager, NXBasics::Runtime_TimeSigned32(p[1]));

				break;
			}
			case SwitchIds::MissionResult::SendHuman:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				Tool_Map_GetNotBlockedPosition(&point, false);

				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(p[0], id))
				{
					void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
					const auto objectId = Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId);
					const auto owner = Read<std::uint8_t>(human, NC2Layout::HumanOffset::PlayerId);
					Human_DoAddNewUserCommand(human, 4, owner, objectId, 0, MapMigPoint_GetLong(&point), 0);
				}

				break;
			}
			case SwitchIds::MissionResult::SendVehicle:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				Tool_Map_GetNotBlockedPosition(&point, false);
				alignas(4) std::byte command[NC2Layout::RecordSize::UserNetworkCommandSigned]{ };
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, SwitchIds::UserCommand::VehicleGotoAndWait);
				Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, MapMigPoint_GetLong(&point));

				for (auto id = MissionCheck_GetFirstVehicleIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextVehicleIdWithMissionCheckId(p[0], id))
				{
					void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize;
					Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::PlayerId));
					Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
					Vehicle_DoAddNewUserCommand(vehicle, command);
				}

				break;
			}
			case SwitchIds::MissionResult::DockVehicle:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				Tool_Map_GetNotBlockedPosition(&point, false);
				alignas(4) std::byte command[NC2Layout::RecordSize::UserNetworkCommandSigned]{ };
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, SwitchIds::UserCommand::VehicleDock);
				Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, MapMigPoint_GetLong(&point));

				for (auto id = MissionCheck_GetFirstVehicleIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextVehicleIdWithMissionCheckId(p[0], id))
				{
					void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize;
					Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::PlayerId));
					Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
					Vehicle_DoAddNewUserCommand(vehicle, command);
				}

				break;
			}
			case SwitchIds::MissionResult::PlaySoundEffect:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x17, true, p[0], MapMigPoint_GetLong(&point), 0);
				break;
			}
			case SwitchIds::MissionResult::CreateTribute:
				TributeManager_CreateTribute(gTributeManager, p[1], p[2], p[0]);
				(static_cast<STributeData*>(gTributeManager) + p[0])->descriptionStringId = p[3];
				break;
			case SwitchIds::MissionResult::AddTributeGoods:
				TributeManager_AddTribute(gTributeManager, p[0], p[1], p[2]);
				break;
			case SwitchIds::MissionResult::AllowJob:
			{
				const auto player = p[0], tribe = p[1], job = p[2];
				const auto offset = player * NC2Layout::RecordSize::GameControlPlayer + tribe * 56u + job + 0x18u;

				if (Read<std::uint8_t>(gGameControl, offset) != 0)
					break;

				Write<std::uint8_t>(gGameControl, offset, 1);
				CHumanIterator iterator;

				for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
				{
					void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == tribe)
						Human_UpdatePlayerSpecificStuffAndCaps(human);
				}

				break;
			}
			case SwitchIds::MissionResult::AllowHouse:
			{
				const auto player = p[0], tribe = p[1], house = p[2];
				const auto offset = player * NC2Layout::RecordSize::GameControlPlayer + tribe * 55u + house + 0x2820u;

				if (Read<std::uint8_t>(gGameControl, offset) != 0)
					break;

				Write<std::uint8_t>(gGameControl, offset, 1);
				CHumanIterator iterator;

				for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
				{
					void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == tribe)
						Human_UpdatePlayerSpecificStuffAndCaps(human);
				}

				break;
			}
			case SwitchIds::MissionResult::AddGoodsToHouses:
				for (auto id = MissionCheck_GetFirstHouseIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHouseIdWithMissionCheckId(p[0], id))
				{
					void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

					if (House_Stock_CanStoreGoodType(house, p[1], false) || House_Stock_CanStoreGoodType(house, p[1], true))
						House_Stock_ModifyAmount(house, nullptr, p[1], p[2], false);
				}

				break;
			case SwitchIds::MissionResult::StartSubMission:
				Write(manager, NC2Layout::MissionManagerOffset::SubMapCampaignId, p[0]);
				Write(manager, NC2Layout::MissionManagerOffset::SubMapId, p[1]);
				Write<std::uint8_t>(manager, NC2Layout::MissionManagerOffset::StartSubMapFlag, 1);
				break;
			case SwitchIds::MissionResult::EndSubMission:
				Write<std::uint8_t>(manager, NC2Layout::MissionManagerOffset::ReturnToParentFlag, 1);
				break;
			case SwitchIds::MissionResult::ChangeVehiclesPlayerId:
				for (auto id = MissionCheck_GetFirstVehicleIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextVehicleIdWithMissionCheckId(p[0], id))
				{
					Vehicle_ChangePlayerId(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize, p[1]);
				}

				break;
			case SwitchIds::MissionResult::ChangeHousesPlayerId:
				for (auto id = MissionCheck_GetFirstHouseIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHouseIdWithMissionCheckId(p[0], id))
				{
					House_ChangePlayerId(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize, p[1]);
				}

				break;
			case SwitchIds::MissionResult::SetImportHumanFlag:
				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(p[0], id))
				{
					void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
					const auto flags = Read<std::uint8_t>(human, NC2Layout::HumanOffset::MiscFlags);
					Write<std::uint8_t>(human, NC2Layout::HumanOffset::MiscFlags, p[1] != 0 ? flags | NXBasics::BitMask::Bit7 : flags & NXBasics::IntegerRange::Int8Maximum);
				}

				break;
			case SwitchIds::MissionResult::EnableJob:
			{
				const auto offset = p[0] * NC2Layout::RecordSize::GameControlPlayer + p[1] * NC2Runtime::PlayerAllowanceStride::JobsAndEquipmentUnsigned + p[2] + NC2Layout::GameControlPlayerOffset::AvailableJobsUnsigned;

				if (Read<std::uint8_t>(gGameControl, offset) == 0)
					Write<std::uint8_t>(gGameControl, offset, 1);

				break;
			}
			case SwitchIds::MissionResult::EnableHouse:
				if (Read<std::uint8_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + p[1] * NC2Runtime::PlayerAllowanceStride::HousesUnsigned + p[2] + NC2Layout::GameControlPlayerOffset::AvailableHousesUnsigned) == 0)
					PlayerAllowedThings_MarkHouseAvailable(static_cast<std::byte*>(gGameControl) + p[0] * NC2Layout::RecordSize::GameControlPlayer + NC2Layout::GameControlPlayerOffset::AllowedThings, p[1], p[2]);

				break;
			case SwitchIds::MissionResult::DisableAll:
				for (std::int32_t index = 0; index < Read<std::int32_t>(manager, NC2Layout::MissionManagerOffset::MissionCount); ++index)
				{
					MissionData_SetMissionActiveFlag(GetMission(manager, static_cast<std::uint32_t>(index)), false);
				}

				break;
			case SwitchIds::MissionResult::AllowGood:
			{
				const auto player = p[0], tribe = p[1], good = p[2];
				const auto offset = player * NC2Layout::RecordSize::GameControlPlayer + tribe * 66u + good + 0x1278u;

				if (Read<std::uint8_t>(gGameControl, offset) != 0)
					break;

				Write<std::uint8_t>(gGameControl, offset, 1);
				CHumanIterator iterator;

				for (auto id = iterator.GetFirstHumanId(player); id != -1; id = iterator.GetNextHumanId())
				{
					void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == tribe)
						Human_UpdatePlayerSpecificStuffAndCaps(human);
				}

				break;
			}
			case SwitchIds::MissionResult::EnableGood:
			{
				const auto offset = p[0] * NC2Layout::RecordSize::GameControlPlayer + p[1] * NC2Runtime::PlayerAllowanceStride::GoodsUnsigned + p[2] + NC2Layout::GameControlPlayerOffset::AvailableGoodsUnsigned;

				if (Read<std::uint8_t>(gGameControl, offset) == 0)
					Write<std::uint8_t>(gGameControl, offset, 1);

				break;
			}
			case SwitchIds::MissionResult::MissionQuitAndPlayVideo:
			{
				char fileName[100];
				NXBasics::Runtime_PrintString(fileName, "seq_%4.4d", NXBasics::Runtime_TimeSigned32(p[0]));
				NC2LogicExternals::External_Misc_PropertyString_AddString("start_fmv", fileName);
				break;
			}
			case SwitchIds::MissionResult::StartEarthquake:
				NC2LogicExternals::External_FX_Earthquake_Start(p[0] * 1000u);

				if (NDSManager::CManager::sTheObjectPtr != nullptr)
					NDSManager::CManager::sTheObjectPtr->FireAndForget_LoadAndPlay("data\\engine2d\\bin\\sounds\\misc\\earthquak.wav", 100, NXBasics::IntegerRange::UInt32Maximum, 1, false, false, 100);

				break;
			case SwitchIds::MissionResult::SelectHuman:
			{
				const auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]);
				// Windows 00420FA7 tests the mission-check ID, not the second script parameter.

				if (id != -1)
					NC2LogicExternals::External_Gui_SelectHuman(id, p[0] != 0);

				break;
			}
			case SwitchIds::MissionResult::AddGoodsToMapArea:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[2]), static_cast<std::int16_t>(p[3]), false);
				Tool_AddGoodsToMapArea(p[0], NXBasics::Runtime_TimeSigned32(p[1]), point, p[4], p[5] != 0, p[6]);
				break;
			}
			case SwitchIds::MissionResult::RemoveGoodsFromMapArea:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[2]), static_cast<std::int16_t>(p[3]), false);
				Tool_RemoveGoodsFromMapArea(p[0], NXBasics::Runtime_TimeSigned32(p[1]), point, p[4], p[5] != 0, p[6]);
				break;
			}
			case SwitchIds::MissionResult::SetHouseExtensionLevel:
			{
				std::int32_t houses[10], count = 0;
				CHouseIterator iterator;

				for (auto id = iterator.GetFirstHouseId(); id != -1; id = iterator.GetNextHouseId())
				{
					if (House_GetMissionCheckId(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize, nullptr) != p[0])
						continue;

					houses[count++] = id;

					if (count == 10)
						break;
				}

				for (std::int32_t index = 0; index < count; ++index)
				{
					const void* house = GetHousePtr(houses[index]);
					const auto player = House_GetPlayerId(house), engineType = Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType);
					const auto point = Read<std::uint32_t>(House_GetMapPositionRef(house), NC2Layout::MapMigPointOffset::Coordinates);
					Logic_EntryPoint_House_Remove(House_GetMapPositionRef(house));
					Logic_EntryPoint_House_Set(player, engineType, p[1], &point, true, p[0]);
				}

				break;
			}
			case SwitchIds::MissionResult::InfoCountHousesInArea:
				if (gMissionOnscreenInfoManager != nullptr)
					MissionOnscreenInfoManager_Info_Set(gMissionOnscreenInfoManager, p[0], p[1], 3, p[2], p[3], p[4], p[5], p[6], p[7], 0, 0, 0);

				break;
			case SwitchIds::MissionResult::RemoveHPsOfHousesInArea:
			case SwitchIds::MissionResult::RemoveHPsOfHousesInAreaX:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				std::int32_t houses[100], count = 0;
				CHouseIterator iterator;

				for (auto id = iterator.GetFirstHouseId(p[0], &point, p[3]); id != -1; id = iterator.GetNextHouseId())
				{
					if (result.type == 87 && House_GetMissionCheckId(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize, nullptr) == p[5])
						continue;

					houses[count++] = id;

					if (count == 100)
						break;
				}

				const auto delta = NXBasics::Runtime_TimeSigned32(0u - p[4]);

				for (std::int32_t index = 0; index < count; ++index)
				{
					House_HitPoints_ModifyValue(CHouseArray::mStaticVars + static_cast<std::uint32_t>(houses[index]) * CHouseArray::RuntimeElementSize, delta);
				}

				break;
			}
			case SwitchIds::MissionResult::SetHouseOverlayState:
				for (auto id = MissionCheck_GetFirstHouseIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHouseIdWithMissionCheckId(p[0], id))
				{
					House_LandscapeOverlay_SetActive(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize, NXBasics::Runtime_TimeSigned32(p[1]), p[2] != 0);
				}

				break;
			case SwitchIds::MissionResult::SetHouseBuildForbiddenArea:
			{
				std::uint32_t point;
				MapMigPoint_SetPosition(&point, static_cast<std::int16_t>(p[0]), static_cast<std::int16_t>(p[1]), false);
				Map_HouseBuildForbiddenFlagTool_SetArea(point, NXBasics::Runtime_TimeSigned32(p[2]), p[3] != 0);
				break;
			}
			case SwitchIds::MissionResult::SetHouseBehaviourFlag:
				for (auto id = MissionCheck_GetFirstHouseIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHouseIdWithMissionCheckId(p[0], id))
				{
					House_SetBehaviourFlag(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize, p[1], p[2] != 0);
				}

				break;

			case SwitchIds::MissionResult::None:
			case SwitchIds::MissionResult::CloseMap:
				break;
			case SwitchIds::MissionResult::AddGoodsToVehicle:
				for (auto id = MissionCheck_GetFirstVehicleIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextVehicleIdWithMissionCheckId(p[0], id))
				{
					void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize;

					if (!Vehicle_Stock_CanStoreGoodType(vehicle, p[1]))
						continue;

					Vehicle_Stock_ModifyFutureAmount(vehicle, p[1], NXBasics::Runtime_TimeSigned32(p[2]), NXBasics::IntegerRange::UInt32Maximum);
					Vehicle_Stock_ModifyAmount(vehicle, p[1], NXBasics::Runtime_TimeSigned32(p[2]), NXBasics::IntegerRange::UInt32Maximum);
					const auto wanted = Vehicle_Stock_GetAmountOfWantedGoods(vehicle, p[1]);
					Vehicle_Stock_SetWantedAmount(vehicle, p[1], wanted + p[2]);
				}

				break;
			case SwitchIds::MissionResult::AddGoodsToAnyStock:
			{
				std::uint32_t remaining = p[2];
				CHouseIterator houses;

				for (auto id = houses.GetFirstHouseId(p[0]); id != -1; id = houses.GetNextHouseId())
				{
					void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

					if (House_GetHouseMainType(house) != 1 || !House_Stock_CanStoreGoodType(house, p[1], false))
						continue;

					const auto maximum = House_Stock_GetMaximumAmount(house, p[1], false);
					const auto space = maximum - House_Stock_GetAmountOfGoods(house, p[1], false);

					if (NXBasics::Runtime_TimeSigned32(remaining) <= NXBasics::Runtime_TimeSigned32(space))
					{
						House_Stock_ModifyAmount(house, p[1], NXBasics::Runtime_TimeSigned32(remaining), 0, false);
						return;
					}

					House_Stock_ModifyAmount(house, p[1], NXBasics::Runtime_TimeSigned32(space), 0, false);
					remaining -= space;
				}

				break;
			}
			case SwitchIds::MissionResult::ChangePlayerIdInArea:
			{
				CHumanIterator humans;
				CAnimalIterator animals;
				CVehicleIterator vehicles;
				CHouseIterator houses;
				CGuideIterator guides;
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[2]), static_cast<std::int16_t>(p[3]), false);

				for (auto id = vehicles.GetFirstVehicleId(p[0], &point, p[4]); id != -1; id = vehicles.GetNextVehicleId())
				{
					Vehicle_ChangePlayerId(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize, p[1]);
				}

				for (auto id = houses.GetFirstHouseId(p[0], &point, p[4]); id != -1; id = houses.GetNextHouseId())
				{
					House_ChangePlayerId(CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize, p[1]);
				}

				for (auto id = humans.GetFirstHumanId(p[0], &point, NXBasics::Runtime_TimeSigned32(p[4])); id != -1; id = humans.GetNextHumanId())
				{
					Human_ChangePlayerId(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize, NXBasics::Runtime_TimeSigned32(p[1]), true);
				}

				for (auto id = animals.GetFirstAnimalId(p[0], &point, NXBasics::Runtime_TimeSigned32(p[4])); id != -1; id = animals.GetNextAnimalId())
				{
					Animal_ChangePlayerId(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::RuntimeElementSize, p[1]);
				}

				for (auto id = guides.GetFirstGuideId(p[0], &point, p[4]); id != -1; id = guides.GetNextGuideId())
				{
					Write(CGuideArray::mStaticVars + static_cast<std::uint32_t>(id) * CGuideArray::RuntimeElementSize, NC2Layout::GuideOffset::PlayerId, p[1]);
					GuideManager_RecalculateGuideConnections(gGuideManager, id);
				}

				break;
			}
			case SwitchIds::MissionResult::SetDiplomacyNotChangeableFlag:
				Write<std::uint8_t>(gGameControl, p[0] * NC2Layout::RecordSize::GameControlPlayer + p[1] + NC2Layout::GameControlPlayerOffset::DiplomacyLockedUnsigned, p[2] != 0);
				Write<std::uint8_t>(gGameControl, p[1] * NC2Layout::RecordSize::GameControlPlayer + p[0] + NC2Layout::GameControlPlayerOffset::DiplomacyLockedUnsigned, p[2] != 0);
				break;
			case SwitchIds::MissionResult::RemoveFXWaveLandscapeInArea:
			case SwitchIds::MissionResult::RemoveFXSmokeLandscapeInArea:
			case SwitchIds::MissionResult::RemoveBlockerLandscapeInArea:
			case SwitchIds::MissionResult::RemoveFX1LandscapeInArea:
			case SwitchIds::MissionResult::RemoveFX2LandscapeInArea:
			{
				std::uint32_t point;
				Logic_EntryPoint_Edit_Start();
				// Entire native string region 00500D70..00500E4B. Duplicate fire-2 lookup in result 84 is intentional.
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[0]), static_cast<std::int16_t>(p[1]), false);
				const char* waves[] = { "fx wave", "fx wave land" };
				const char* smoke[] = { "fx fire", "fx fire 2", "fx smoke", "fx fire small", "fx wave land", "fx fire incense", "fx fog", "fx fog waterfall", "fx fog waterfall00", "fx waterfall" };
				const char* blocker[] = { "block" };
				const char* fog[] = { "fx fog", "fx fog waterfall", "fx fog waterfall00", "fx waterfall" };
				const char* fire[] = { "fx fire", "fx fire 2", "fx fire house 0", "fx fire house 1", "fx fire 2", "fx fire small", "fx smoke", "fx fire house 2" };
				const char* const* names = waves;
				std::uint32_t count = 2;

				if (result.type == 79)
				{
					names = smoke;
					count = 10;
				}
				else if (result.type == 82)
				{
					names = blocker;
					count = 1;
				}
				else if (result.type == 83)
				{
					names = fog;
					count = 4;
				}
				else if (result.type == 84)
				{
					names = fire;
					count = 8;
				}

				std::int32_t engines[10];

				for (std::uint32_t index = 0; index < count; ++index)
				{
					engines[index] = NC2LogicExternals::External_Landscape_GetLandscapeIdByNamePtr(names[index]);
					// Windows exits without Edit_Stop when an engine definition is missing.

					if (engines[index] == -1)
					{

						return;
					}
				}

				auto clear = [&]()
				{
					for (std::uint32_t index = 0; index < count; ++index)
					{
						if (NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(&point) == engines[index])
						{
							Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(&point, 0, 0, -1, 0, -1);
							break;
						}
					}
				};
				clear();

				for (std::uint32_t ring = 1; ring <= p[2]; ++ring)
				{
					MapMigPoint_MoveCursorUnwrapped(point, 4);

					for (std::uint32_t direction = 0; direction < 6; ++direction)
					{
						for (std::uint32_t step = 0; step < ring; ++step)
						{
							MapMigPoint_MoveCursorUnwrapped(point, direction);

							if (MapMigPoint_IsInsideMap(&point))
								clear();
						}
					}
				}

				Logic_EntryPoint_Edit_Stop();
				break;
			}
			case SwitchIds::MissionResult::SetWallGateOpen:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				MapTool_WallGate_SetState(p[0], &point, p[3] != 0);
				break;
			}
			case SwitchIds::MissionResult::SetPlayerBehaviourFlag:
			case SwitchIds::MissionResult::SetHumanBehaviourFlag:
			{
				auto change = [&](std::int32_t id)
				{
					void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
					const auto flags = Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags);
					Write(human, NC2Layout::HumanOffset::MiscFlags, p[2] != 0 ? flags | p[1] : flags & ~p[1]);
				};

				if (result.type == 49)
				{
					CHumanIterator humans;

					for (auto id = humans.GetFirstHumanId(p[0]); id != -1; id = humans.GetNextHumanId())
					{
						change(id);
					}
				}
				else
				{
					for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(p[0], id))
					{
						change(id);
					}
				}

				break;
			}
			case SwitchIds::MissionResult::RemoveHumansNearPos:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[0]), static_cast<std::int16_t>(p[1]), false);
				auto mark = [&]()
				{
					CMapMoveableIterator moveables;

					for (auto id = moveables.GetFirstMapMoveableId(&point); id != -1; id = moveables.GetNextMapMoveableId())
					{
						void* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

						if (Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) == 1)
							Write<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::RemovalRequestedFlag, 1);
					}
				};
				mark();

				for (std::int32_t ring = 1; ring <= NXBasics::Runtime_TimeSigned32(p[2]); ++ring)
				{
					MapMigPoint_MoveCursorUnwrapped(point, 4);

					for (std::uint32_t direction = 0; direction < 6; ++direction)
					{
						for (std::int32_t step = 0; step < ring; ++step)
						{
							MapMigPoint_MoveCursorUnwrapped(point, direction);

							if (MapMigPoint_IsInsideMap(&point))
								mark();
						}
					}
				}

				break;
			}
			case SwitchIds::MissionResult::StopHumanByPlayerId:
			{
				alignas(4) std::byte walk[NC2Layout::RecordSize::UserNetworkCommandSigned]{ }, work[NC2Layout::RecordSize::UserNetworkCommandSigned]{ };
				Write<std::uint8_t>(walk, NC2Layout::UserNetworkCommandOffset::Opcode, SwitchIds::UserCommand::GotoAndWait);
				Write<std::uint8_t>(work, NC2Layout::UserNetworkCommandOffset::Opcode, SwitchIds::UserCommand::SetWorkCenter);
				Write<std::uint8_t>(walk, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(p[0]));
				Write<std::uint8_t>(work, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(p[0]));
				CHumanIterator humans;

				for (auto id = humans.GetFirstHumanId(p[0]); id != -1; id = humans.GetNextHumanId())
				{
					void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
					std::uint32_t point = 0;
					MapMoveable_Attachment_GetNextMapPosition(gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable, &point);

					if (Human_IsWalking(human))
					{
						Write(walk, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
						Write(walk, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
						Write(walk, NC2Layout::UserNetworkCommandOffset::Parameter1, point);
						Human_DoAddNewUserCommand(human, walk);
					}

					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter) != point)
					{
						Write(work, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
						Write(work, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
						Write(work, NC2Layout::UserNetworkCommandOffset::Parameter1, point);
						Human_DoAddNewUserCommand(human, work);
					}
				}

				break;
			}
			case SwitchIds::MissionResult::SetAnimal:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[3]), static_cast<std::int16_t>(p[4]), false);
				Logic_EntryPoint_Add_Animal(p[0], p[1], p[2], &point, p[5], p[6]);
				break;
			}
			case SwitchIds::MissionResult::RemoveAnimals:
				for (auto id = MissionCheck_GetFirstAnimalIdWithMissionCheckId(p[0]); id != -1; id = MissionCheck_GetNextAnimalIdWithMissionCheckId(p[0], id))
				{
					Logic_EntryPoint_FreeMapMoveable(Read<std::uint32_t>(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::RuntimeElementSize, NC2Layout::AnimalOffset::MapMoveableId));
				}

				break;
			case SwitchIds::MissionResult::ChangeHumanObjectIdInArea:
			case SwitchIds::MissionResult::ChangeMissionIdOfHumanInRange:
			{
				const auto first = result.type == 56 ? 1u : 2u;
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[first]), static_cast<std::int16_t>(p[first + 1]), false);
				CHumanIterator humans;

				for (auto id = humans.GetFirstHumanId(p[0], &point, NXBasics::Runtime_TimeSigned32(p[first + 2])); id != -1; id = humans.GetNextHumanId())
				{
					Write(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize, NC2Layout::HumanOffset::MissionCheckId, p[result.type == 56 ? 4 : 1]);
				}

				break;
			}
			case SwitchIds::MissionResult::HealHumansInArea:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[0]), static_cast<std::int16_t>(p[1]), false);
				CHumanIterator humans;

				for (auto id = humans.GetFirstHumanId(&point, NXBasics::Runtime_TimeSigned32(p[2])); id != -1; id = humans.GetNextHumanId())
				{
					void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
					Write(human, NC2Layout::HumanOffset::Hitpoints, Human_GetHumanAttributeMaximum_Hitpoints(human));
				}

				break;
			}
			case SwitchIds::MissionResult::ClearTribute:
				Write<std::uint8_t>(gTributeManager, p[0] * NC2Layout::RecordSize::TributeUnsigned, 0);
				break;
			case SwitchIds::MissionResult::SetGuiMarker:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				NC2LogicExternals::External_Gui_SetMarker(p[0], &point);
				break;
			}
			case SwitchIds::MissionResult::SetHumanName:
			{
				const auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(p[0]);

				if (id != -1)
					NC2LogicExternals::External_Human_SetName(id, p[1]);

				break;
			}
			case SwitchIds::MissionResult::SetWeather:
			{
				std::uint32_t first, second;
				MapMigPoint_Construct(&first, static_cast<std::int16_t>(p[0] - p[2]), static_cast<std::int16_t>(p[1] - p[2]), false);
				MapMigPoint_ValidatePosition(&first);
				MapMigPoint_Construct(&second, static_cast<std::int16_t>(p[0] + p[2]), static_cast<std::int16_t>(p[1] + p[2]), false);
				MapMigPoint_ValidatePosition(&second);
				NC2LogicExternals::External_FX_Weather_SetRectangle(first, second, p[3] != 0, NXBasics::Runtime_TimeSigned32(p[4]));
				break;
			}
			case SwitchIds::MissionResult::ChangeMissionIdOfPlayer:
			{
				CHumanIterator humans;

				for (auto id = humans.GetFirstHumanId(p[0]); id != -1; id = humans.GetNextHumanId())
				{
					Write(CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize, NC2Layout::HumanOffset::MissionCheckId, p[1]);
				}

				break;
			}
			case SwitchIds::MissionResult::SetVertexColor:
			case SwitchIds::MissionResult::SetVertexColorOnLand:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[0]), static_cast<std::int16_t>(p[1]), false);
				NC2LogicExternals::External_VertexColor_SetArea(&point, p[2], NXBasics::Runtime_TimeSigned32(p[3]), result.type == 97);
				break;
			}
			case SwitchIds::MissionResult::RemoveLandscapesInArea:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[0]), static_cast<std::int16_t>(p[1]), false);
				Logic_EntryPoint_Edit_Start();
				Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(&point, 0, 0, -1, 0, -1);

				for (std::int32_t ring = 1; ring < NXBasics::Runtime_TimeSigned32(p[2]); ++ring)
				{
					MapMigPoint_MoveCursorUnwrapped(point, 4);

					for (std::uint32_t direction = 0; direction < 6; ++direction)
					{
						for (std::int32_t step = 0; step < ring; ++step)
						{
							MapMigPoint_MoveCursorUnwrapped(point, direction);

							if (MapMigPoint_IsInsideMap(&point))
								Logic_EntryPoint_Edit_MapMicroPoint_Landscape_SetType(&point, 0, 0, -1, 0, -1);
						}
					}
				}

				Logic_EntryPoint_Edit_Stop();
				break;
			}
			case SwitchIds::MissionResult::MoveUnitsInArea:
			{
				std::uint32_t origin, target;
				MapMigPoint_Construct(&origin, static_cast<std::int16_t>(p[1]), static_cast<std::int16_t>(p[2]), false);
				MapMigPoint_Construct(&target, static_cast<std::int16_t>(p[4]), static_cast<std::int16_t>(p[5]), false);

				if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(static_cast<std::int16_t>(origin), static_cast<std::int16_t>(origin >> 16), static_cast<std::int16_t>(target), static_cast<std::int16_t>(target >> 16))) <= p[3])
					break;

				bool moved = false;
				std::int32_t ids[20], count = 0;
				CHumanIterator humans;

				for (auto id = humans.GetFirstHumanId(p[0], &origin, NXBasics::Runtime_TimeSigned32(p[3])); id != -1; id = humans.GetNextHumanId())
				{
					const void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
					const void* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;

					if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0 || Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
						continue;

					ids[count++] = id;
					moved = true;

					if (count == 20)
						break;
				}

				for (std::int32_t index = 0; index < count; ++index)
				{
					moveHuman(ids[index], target);
				}

				count = 0;
				CVehicleIterator vehicles;

				for (auto id = vehicles.GetFirstVehicleId(p[0], &origin, p[3]); id != -1; id = vehicles.GetNextVehicleId())
				{
					const void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize;

					if (Read<std::uint8_t>(gMapMoveableArrayStorage.entries + Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable, NC2Layout::MapMoveableOffset::WalkingFlag) != 0)
						continue;

					ids[count++] = id;
					moved = true;

					if (count == 20)
						break;
				}

				for (std::int32_t index = 0; index < count; ++index)
				{
					void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(ids[index]) * CVehicleArray::RuntimeElementSize;
					redraw(Read<std::uint32_t>(Vehicle_GetMapPositionRef(vehicle), NC2Layout::MapMigPointOffset::Coordinates), 4);
					MapMoveable_Attachment_AttachToMap(gMapMoveableArrayStorage.entries + Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable, &target);
					alignas(4) std::byte command[NC2Layout::RecordSize::UserNetworkCommandSigned]{ };
					Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, SwitchIds::UserCommand::VehicleGotoAndWait);
					Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::PlayerId));
					Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
					Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
					Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, target);
					Vehicle_DoAddNewUserCommand(vehicle, command);
					GameMap_Exploration_DoExplore(Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::PlayerId), &target, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::ExplorationRadius));
					redraw(target, 4);
				}

				if (moved && NDSManager::CManager::sTheObjectPtr != nullptr)
					NDSManager::CManager::sTheObjectPtr->FireAndForget_LoadAndPlay("data\\engine2d\\bin\\sounds\\misc\\teleporter.wav", 100, NXBasics::IntegerRange::UInt32Maximum, 1, false, false, 100);

				break;
			}
			case SwitchIds::MissionResult::InfoClear:
			case SwitchIds::MissionResult::InfoShowString:
			case SwitchIds::MissionResult::InfoCountGoodsInArea:
			case SwitchIds::MissionResult::InfoCountHumenInArea:
			case SwitchIds::MissionResult::InfoCountSoldiersInArea:
			case SwitchIds::MissionResult::InfoCountAnimalsInArea:
				if (gMissionOnscreenInfoManager != nullptr)
				{
					const auto type = result.type - 72;
					MissionOnscreenInfoManager_Info_Set(gMissionOnscreenInfoManager, p[0], p[1], type, type == 0 ? NXBasics::IntegerRange::UInt32Maximum : p[2], type >= 2 ? p[3] : 0, type >= 2 ? p[4] : 0, type >= 2 ? p[5] : 0, type >= 2 ? p[6] : 0, type == 2 || type == 6 ? p[7] : 0, 0, 0, 0);
				}

				break;
			case SwitchIds::MissionResult::ChangeAnimalPlayerIdInArea:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[2]), static_cast<std::int16_t>(p[3]), false);
				std::int32_t ids[50], count = 0;
				CAnimalIterator animals;

				for (auto id = animals.GetFirstAnimalId(p[0]); id != -1; id = animals.GetNextAnimalId())
				{
					const void* animal = CAnimalArray::mStaticVars + static_cast<std::uint32_t>(id) * CAnimalArray::RuntimeElementSize;

					if (Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType) != p[1])
						continue;

					const auto position = Read<std::uint32_t>(Animal_GetMapPositionRef(animal), NC2Layout::MapMigPointOffset::Coordinates);

					if (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(static_cast<std::int16_t>(point), static_cast<std::int16_t>(point >> 16), static_cast<std::int16_t>(position), static_cast<std::int16_t>(position >> 16))) > p[4])
						continue;

					ids[count++] = id;

					if (count == 50 || (p[5] != 0 && count >= NXBasics::Runtime_TimeSigned32(p[5])))
						break;
				}

				for (std::int32_t index = 0; index < count; ++index)
				{
					Animal_ChangePlayerId(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(ids[index]) * CAnimalArray::RuntimeElementSize, p[6]);
				}

				break;
			}
			case SwitchIds::MissionResult::SetCameraPosition:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(result.GetParameter(0)), static_cast<std::int16_t>(result.GetParameter(1)), false);
				NC2LogicExternals::External_Camera_SetToPosition(&point);
				break;
			}
			case SwitchIds::MissionResult::SetHumanX:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[3]), static_cast<std::int16_t>(p[4]), false);

				for (std::int32_t index = 0; index < NXBasics::Runtime_TimeSigned32(p[7]); ++index)
				{
					Logic_EntryPoint_Add_Human(p[0], p[1], p[2], &point, p[5], p[6]);
				}

				break;
			}
			case SwitchIds::MissionResult::AttachHumanToVehicle:
			{
				const auto mission = result.GetParameter(0);
				const auto vehicleId = MissionCheck_GetFirstVehicleIdWithMissionCheckId(result.GetParameter(1));

				if (vehicleId == -1)
					break;

				const void* vehicle = GetVehiclePtr(vehicleId);

				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(mission); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(mission, id))
				{
					void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
					SUserCommandsForHumanHandler commands;
					Human_UserControl_GetCommands(human, &commands, false, false);

					if (commands.IsCommandAllowed(0x0C) && Vehicle_Passengers_CanHumanBeAttached(vehicle, id))
						Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::AttachVehicle, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), ArrayObject_GetId(human, nullptr), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), ArrayObject_GetId(vehicle, nullptr), Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
				}

				break;
			}
			case SwitchIds::MissionResult::DetachHumanFromVehicle:
			{
				const auto mission = result.GetParameter(0);

				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(mission); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(mission, id))
				{
					void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
					SUserCommandsForHumanHandler commands;
					Human_UserControl_GetCommands(human, &commands, false, false);

					if (commands.IsCommandAllowed(0x0D))
						Human_DoAddNewUserCommand(human, SwitchIds::UserCommand::DetachVehicle, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), ArrayObject_GetId(human, nullptr), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
				}

				break;
			}
			case SwitchIds::MissionResult::MoveHuman:
			{
				const auto mission = result.GetParameter(0);
				std::uint32_t target;
				MapMigPoint_Construct(&target, static_cast<std::int16_t>(result.GetParameter(1)), static_cast<std::int16_t>(result.GetParameter(2)), false);
				std::int32_t ids[20], count = 0;

				for (auto id = MissionCheck_GetFirstHumanIdWithMissionCheckId(mission); id != -1; id = MissionCheck_GetNextHumanIdWithMissionCheckId(mission, id))
				{
					ids[count++] = id;

					if (count == 20)
						break;
				}

				for (std::int32_t index = 0; index < count; ++index)
				{
					moveHuman(ids[index], target);
				}

				break;
			}
			case SwitchIds::MissionResult::ChangeMissionIdOfVehiclesInRange:
			{
				std::uint32_t point;
				const auto player = result.GetParameter(0), mission = result.GetParameter(1);
				const auto y = result.GetParameter(3), x = result.GetParameter(2);
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), false);
				const auto radius = result.GetParameter(4);
				CVehicleIterator vehicles;

				for (auto id = vehicles.GetFirstVehicleId(player, &point, NXBasics::Runtime_TimeSigned32(radius)); id != -1; id = vehicles.GetNextVehicleId())
				{
					Vehicle_SetMissionCheckId(GetVehiclePtr(id), nullptr, mission);
				}

				break;
			}
			case SwitchIds::MissionResult::RemoveVehiclesWithMissionId:
			{
				const auto mission = result.GetParameter(0);
				const bool passengers = result.GetParameter(1) != 0;
				std::int32_t ids[50], count = 0;

				for (auto id = MissionCheck_GetFirstVehicleIdWithMissionCheckId(mission); id != -1; id = MissionCheck_GetNextVehicleIdWithMissionCheckId(mission, id))
				{
					ids[count++] = id;

					if (count == 50)
						break;
				}

				const bool deathStatistics = Game_GetSuppressDeathStatisticsFlag(), wreck = Game_GetSuppressVehicleWreckFlag();
				Game_SetSuppressDeathStatisticsFlag(true);
				Game_SetSuppressVehicleWreckFlag(true);

				for (std::int32_t index = 0; index < count; ++index)
				{
					void* vehicle = GetVehiclePtr(ids[index]);

					if (passengers)
					{
						for (std::uint32_t slot = 0; slot < Vehicle_Passengers_GetNumberOfSlots(vehicle, nullptr); ++slot)
						{
							const void* reference = Vehicle_Passengers_GetAttachedHumanSlotPtr(vehicle, nullptr, slot);

							if (!HumanReference_IsAllocated(reference))
								continue;

							const void* human = GetHumanPtr(HumanReference_GetHumanId(reference, nullptr));
							Logic_EntryPoint_FreeMapMoveable(Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId));
						}

						if (Vehicle_Passengers_IsCommanderAttached(vehicle, nullptr))
							Logic_EntryPoint_FreeMapMoveable(Read<std::uint32_t>(GetHumanPtr(Vehicle_Passengers_GetAttachedCommanderId(vehicle)), NC2Layout::HumanOffset::MapMoveableId));
					}

					redraw(Read<std::uint32_t>(Vehicle_GetMapPositionRef(vehicle), NC2Layout::MapMigPointOffset::Coordinates), 2);
					Logic_EntryPoint_FreeMapMoveable(Vehicle_GetMapMoveableId(vehicle, nullptr));
				}

				Game_SetSuppressDeathStatisticsFlag(deathStatistics);
				Game_SetSuppressVehicleWreckFlag(wreck);
				break;
			}
			case SwitchIds::MissionResult::SetImportLandscapeMarker:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(p[0]), static_cast<std::int16_t>(p[1]), false);

				if (!MapMigPoint_IsInsideMap(&point))
					break;

				if (p[2] != 0)
				{
					MarkerArray_AllocateMarker(MarkerArray_GetExistingObjectPtr(), nullptr, 2, &point, NXBasics::IntegerRange::UInt32Maximum);
				}
				else
				{
					removeMarkers(point, 2);
				}

				break;
			}
			case SwitchIds::MissionResult::ChangeMissionIdOfPlayersVehiclesOnContinent:
			{
				std::uint32_t point;
				const auto player = result.GetParameter(0), mission = result.GetParameter(3);
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(result.GetParameter(1)), static_cast<std::int16_t>(result.GetParameter(2)), false);

				if (!MapMigPoint_IsInsideMap(&point))
					break;

				const auto continent = MapMicroPoint_GetContinent(GameMap_GetConstMapMicroPointPtr(&point), nullptr);

				if (continent == NXBasics::IntegerRange::UInt8MaximumSigned)
					break;

				CVehicleIterator vehicles;

				for (auto id = vehicles.GetFirstVehicleId(player); id != -1; id = vehicles.GetNextVehicleId())
				{
					void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize;

					if (Vehicle_Attachment_IsInsideVehicle(vehicle))
						continue;

					auto position = Read<std::uint32_t>(Vehicle_GetMapPositionRef(vehicle), NC2Layout::MapMigPointOffset::Coordinates);

					if (Vehicle_IsDocked(vehicle, nullptr))
						position = Read<std::uint32_t>(Vehicle_GetConstMapMicroPointPtr(vehicle), NC2Layout::MapMigPointOffset::Coordinates);

					if (MapMigPoint_IsInsideMap(&position) && MapMicroPoint_GetContinent(GameMap_GetConstMapMicroPointPtr(&position), nullptr) == continent)
						Vehicle_SetMissionCheckId(vehicle, nullptr, mission);
				}

				break;
			}
			case SwitchIds::MissionResult::ChangeMissionIdOfVehicles:
			{
				const auto oldMission = result.GetParameter(0), newMission = result.GetParameter(1);
				CVehicleIterator vehicles;

				for (auto id = vehicles.GetFirstVehicleId(); id != -1; id = vehicles.GetNextVehicleId())
				{
					void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize;

					if (Vehicle_GetMissionCheckId(vehicle, nullptr) == oldMission)
						Vehicle_SetMissionCheckId(vehicle, nullptr, newMission);
				}

				break;
			}
			case SwitchIds::MissionResult::SetMapAreaMarker:
			case SwitchIds::MissionResult::SetMapAreaMarkerMagic:
			{
				std::uint32_t point;
				MapMigPoint_Construct(&point, static_cast<std::int16_t>(result.GetParameter(0)), static_cast<std::int16_t>(result.GetParameter(1)), false);
				auto radius = result.GetParameter(2), spacing = result.GetParameter(4);
				const bool enabled = result.GetParameter(3) != 0;

				if (radius == 0)
					radius = 1;

				if (spacing == 0)
					spacing = 1;

				if (!MapMigPoint_IsInsideMap(&point))
					break;

				MapMigPoint_MoveCursorUnwrapped(&point, 4, radius);
				const auto type = result.type == 101 ? 3u : 4u;

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					for (std::uint32_t step = 0; step < radius; ++step)
					{
						if (step % spacing == 0)
						{
							if (enabled)
							{
								MarkerArray_AllocateMarker(MarkerArray_GetExistingObjectPtr(), nullptr, type, &point, NXBasics::IntegerRange::UInt32Maximum);
							}
							else
							{
								removeMarkers(point, type);
							}
						}

						MapMigPoint_MoveCursorUnwrapped(point, direction);
					}
				}

				break;
			}
			default:
				break;
		}
	}

}