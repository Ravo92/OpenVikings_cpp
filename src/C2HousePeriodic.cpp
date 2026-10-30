#include "C2AnimalAccess.h"
#include "C2DelayedActionManager.h"
#include "C2EngineDisplayAccess.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2GuideRouting.h"
#include "C2HappinessEvents.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanInitialization.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanTaskAccess.h"
#include "C2HumanWalking.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2MilitaryTargetSearch.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2StaticHouseTypeData.h"
#include "C2VehicleAccess.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	void __fastcall House_Misc_Destroy_House(void* house, void* unusedEdx);

	namespace
	{
		// Remaster: use the normal human walking radius (Windows 004F4300).
		constexpr std::uint32_t kBuilderWalkingRadius = 50;

		bool Remaster_BuilderCanWalkBetween(const void* source, const void* destination)
		{
			if (!MapMigPoint_IsInsideMap(source) || !MapMigPoint_IsInsideMap(destination))
				return false;

			if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(destination), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0)
				return false;

			if (Read<std::uint32_t>(source, NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(destination, NC2Layout::MapCoordinatePairOffset::Coordinates))
				return true;

			SMapPathfinderElementStorage probe{ };
			// This bounded search uses scratch state, not the human's active path or start quota.

			return MapPathfinderElement_l_FindWayDirectedWithResitance(&probe, source, destination, kBuilderWalkingRadius);
		}

		bool Remaster_BuilderCanReachHouse(const void* human, const void* house, std::uint32_t continent)
		{
			const void* source = Human_GetMapPositionRef(human);
			std::uint32_t work = 0;

			if (!House_Construction_GetNextWorkPoint(house, source, &work, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), continent) && !House_Construction_GetNextWorkPoint(house, source, &work, false, NXBasics::IntegerRange::UInt32Maximum, continent))
				return false;

			if (Remaster_BuilderCanWalkBetween(source, &work))
				return true;

			if (gGuideManager == nullptr || CGuideArray::sTheObjectPtr == nullptr)
				return false;

			std::int32_t sourceGuide = -1, targetGuide = -1;

			if (GuideManager_GetNextGuideId(gGuideManager, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), kBuilderWalkingRadius, source, &work, targetGuide, sourceGuide) == -1)
				return false;
			// The native same-guide case only writes the destination-side output.

			if (sourceGuide == -1)
				sourceGuide = targetGuide;

			const auto guidePoint = [&](std::int32_t id, std::uint32_t& point)
			{
				if (id < 0 || id >= CGuideArray::sTheObjectPtr->mCapacity)
					return false;

				const auto* guide = CGuideArray::mStaticVars + static_cast<std::uint32_t>(id) * CGuideArray::RuntimeElementSize;

				if (Read<std::uint8_t>(guide, NC2Layout::GuideOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(guide, NC2Layout::GuideOffset::PlayerId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId))
					return false;

				Guide_GetMovePoint(guide, &point);

				return MapMigPoint_IsInsideMap(&point);
			};
			std::uint32_t sourcePoint = 0, currentPoint = 0;

			if (!guidePoint(sourceGuide, sourcePoint) || !guidePoint(targetGuide, currentPoint))
				return false;

			if (!Remaster_BuilderCanWalkBetween(source, &sourcePoint) || !Remaster_BuilderCanWalkBetween(&currentPoint, &work))
				return false;
			// The native graph search returns the predecessor of its first argument.
			// Validate every leg toward the work point, including links blocked since their last update.

			for (std::int32_t checked = 0; checked < CGuideArray::sTheObjectPtr->mCapacity; ++checked)
			{
				if (targetGuide == sourceGuide)
					return true;

				const auto previous = GuideManager_GetNextGuideId(gGuideManager, targetGuide, sourceGuide);
				std::uint32_t previousPoint = 0;

				if (previous == targetGuide || !guidePoint(previous, previousPoint) || !Remaster_BuilderCanWalkBetween(&previousPoint, &currentPoint))
					return false;

				targetGuide = previous;
				currentPoint = previousPoint;
			}

			return false;
		}
	}

	// Original function: FUN_0040ace5.
	void __fastcall HouseArray_DoOneGameTick(void* array, void*)
	{
		auto* handler = static_cast<CArrayHandler*>(array);

		for (auto id = handler->mFirstScanIndex; id <= handler->mLastScanIndex; ++id)
		{
			void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0)
				House_DoOneGameTick(house);
		}
	}

	// Original function: FUN_00433694; target searches also run when the current tick has no arrows available.
	void House_DoOneGameTick(void* house)
	{
		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0 && Read<std::int32_t>(house, NC2Layout::HouseOffset::Hitpoints) <= 0)
		{
			HappinessManager_Messages_HouseDestroyedTroughAttack(gHappinessManager, house);
			House_Misc_Destroy_House(house, nullptr);
		}

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
		{
			const auto tick = static_cast<std::uint16_t>(Read<std::uint16_t>(house, NC2Layout::HouseOffset::DemolitionStepsDone) + 1u);
			Write(house, NC2Layout::HouseOffset::DemolitionStepsDone, tick);

			if (static_cast<std::int16_t>(tick) == Read<std::int16_t>(house, NC2Layout::HouseOffset::DemolitionStepsRequired) - 12)
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x1D, true, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId), 0, 0);

			if (static_cast<std::int16_t>(tick) >= Read<std::int16_t>(house, NC2Layout::HouseOffset::DemolitionStepsRequired))
			{
				CHouseArray::sTheObjectPtr->FreeHouse(Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId));
				return;
			}
		}

		if (House_GetHouseMainType(house) == 6)
		{
			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3)
			{
				// Windows frees the site first and reads its retained fields for vehicle allocation.
				CHouseArray::sTheObjectPtr->FreeHouse(Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId));
				Logic_EntryPoint_Add_Vehicle(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId), Read<std::uint32_t>(house, NC2Layout::HouseOffset::TribeType), gStaticHouseTypeStorage.records[Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType)].vehicleType, static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition, 0);
				return;
			}
		}
		else if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3)
		{
			Tool_Map_CheckUserInformationAboutEnemies(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId), static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition);
		}

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DefenceModeFlag) == 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
		{

			return;
		}

		CHumanIterator iterator;
		std::uint32_t defenders = 0;
		const auto* door = static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint;
		const auto player = Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId);

		for (auto id = iterator.GetFirstSectorHumanId(player, Read<std::uint32_t>(door, NC2Layout::MapCoordinatePairOffset::Coordinates), 0, false); id != -1; id = iterator.GetNextSectorHumanId())
		{
			++defenders;
		}

		auto remaining = defenders / 24u;

		if (defenders % 24u > gGameTime.tick % 24u)
			++remaining;

		const auto& weapon = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[Read<std::uint32_t>(house, NC2Layout::HouseOffset::TribeType)])[20];
		bool fired;
		do
		{
			fired = false;
			const auto humanFirst = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, door, weapon.minimumRange, weapon.maximumRange, true, 5, 10, true, true);

			if (humanFirst != -1 && remaining != 0)
			{
				House_l_Military_StartArrow(house, Read<std::uint32_t>(Human_GetMapPositionRef(CHumanArray::mStaticVars + static_cast<std::uint32_t>(humanFirst) * CHumanArray::RuntimeElementSize), NC2Layout::MapCoordinatePairOffset::Coordinates));
				--remaining;
				fired = true;
			}

			const auto vehicleFirst = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, door, weapon.minimumRange, weapon.maximumRange, true, 5, 10, false, false);

			if (vehicleFirst != -1 && remaining != 0)
			{
				House_l_Military_StartArrow(house, Read<std::uint32_t>(Vehicle_GetMapPositionRef(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(vehicleFirst) * CVehicleArray::RuntimeElementSize), NC2Layout::MapCoordinatePairOffset::Coordinates));
				--remaining;
				fired = true;
			}

			const auto animal = Tool_Animals_GetOneOfNextEnemyInRange(player, door, weapon.minimumRange, weapon.maximumRange, true, false, 5, 10);

			if (animal != -1 && remaining != 0)
			{
				House_l_Military_StartArrow(house, Read<std::uint32_t>(Animal_GetMapPositionRef(CAnimalArray::mStaticVars + static_cast<std::uint32_t>(animal) * CAnimalArray::RuntimeElementSize), NC2Layout::MapCoordinatePairOffset::Coordinates));
				--remaining;
				fired = true;
			}

			const auto houseFirst = Tool_GetOneOfNextEnemyHousesInRange(player, door, weapon.minimumRange, weapon.maximumRange, true, 5, 10);

			if (houseFirst != -1 && remaining != 0)
			{
				House_l_Military_StartArrow(house, Read<std::uint32_t>(CHouseArray::mStaticVars + static_cast<std::uint32_t>(houseFirst) * CHouseArray::RuntimeElementSize, NC2Layout::HouseOffset::MapPosition));
				--remaining;
				fired = true;
			}

			const auto humanSecond = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, door, weapon.minimumRange, weapon.maximumRange, false, 5, 10, true, true);

			if (humanSecond != -1 && remaining != 0)
			{
				House_l_Military_StartArrow(house, Read<std::uint32_t>(Human_GetMapPositionRef(CHumanArray::mStaticVars + static_cast<std::uint32_t>(humanSecond) * CHumanArray::RuntimeElementSize), NC2Layout::MapCoordinatePairOffset::Coordinates));
				--remaining;
				fired = true;
			}

			const auto vehicleSecond = Tool_ObjectId_GetOneOfNextEnemyMapMoveableInRange(player, door, weapon.minimumRange, weapon.maximumRange, false, 5, 10, false, false);

			if (vehicleSecond != -1 && remaining != 0)
			{
				House_l_Military_StartArrow(house, Read<std::uint32_t>(Vehicle_GetMapPositionRef(CVehicleArray::mStaticVars + static_cast<std::uint32_t>(vehicleSecond) * CVehicleArray::RuntimeElementSize), NC2Layout::MapCoordinatePairOffset::Coordinates));
				--remaining;
				fired = true;
			}

			const auto houseSecond = Tool_GetOneOfNextEnemyHousesInRange(player, door, weapon.minimumRange, weapon.maximumRange, false, 5, 10);

			if (houseSecond != -1 && remaining != 0)
			{
				House_l_Military_StartArrow(house, Read<std::uint32_t>(CHouseArray::mStaticVars + static_cast<std::uint32_t>(houseSecond) * CHouseArray::RuntimeElementSize, NC2Layout::HouseOffset::MapPosition));
				--remaining;
				fired = true;
			}
		} while (fired && remaining != 0);
	}

	// Original function: FUN_00433a9d; the target is passed by value and scatter consumes y before x.
	void House_l_Military_StartArrow(const void* house, std::uint32_t point)
	{
		const auto& weapon = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[Read<std::uint32_t>(house, NC2Layout::HouseOffset::TribeType)])[20];
		const auto source = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition);
		const auto* graphics = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
		std::uint32_t height = 0, offsetX = 20, offsetY = 20;
		const auto launchPoints = Read<std::uint32_t>(graphics, NC2Layout::EngineHouseLogicViewOffset::ArrowSlotCount);

		if (launchPoints != 0)
		{
			const auto selected = Game_Random_GetNumber() % launchPoints;
			height = Read<std::uint32_t>(graphics, NC2Layout::EngineHouseLogicViewOffset::ArrowSlotParameterUnsigned + selected * NC2Layout::RecordSize::HouseArrowSlotUnsigned);
			offsetX = Read<std::uint32_t>(graphics, NC2Layout::EngineHouseLogicViewOffset::ArrowSlotXUnsigned + selected * NC2Layout::RecordSize::HouseArrowSlotUnsigned);
			offsetY = Read<std::uint32_t>(graphics, NC2Layout::EngineHouseLogicViewOffset::ArrowSlotYUnsigned + selected * NC2Layout::RecordSize::HouseArrowSlotUnsigned);
		}

		const auto divisor = Game_Random_GetNumber() % 30u + 1u;
		const auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(&source, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&source, NC2Layout::MapCoordinatePairOffset::Y)));
		const auto spread = (distance >> 2) / divisor;
		const auto scatterDivisor = NXBasics::Runtime_TimeSigned32(spread + 1u), half = NXBasics::Runtime_TimeSigned32(spread) / 2;
		const auto y = static_cast<std::int16_t>(Game_Random_GetNumber()) % scatterDivisor - half;
		const auto x = static_cast<std::int16_t>(Game_Random_GetNumber()) % scatterDivisor - half;
		MapMigPoint_AddOffset(&point, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
		const auto flightTicks = (static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(&source, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&source, NC2Layout::MapCoordinatePairOffset::Y))) << 3) / static_cast<std::uint32_t>(weapon.speed);
		// Complete native 0x24-byte stack callback record; its original type name remains unresolved.
		const std::uint32_t arrow[9] = { source, point, height, offsetX, offsetY, 20, 20, static_cast<std::uint32_t>(weapon.munitionType), flightTicks };
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x23, true, reinterpret_cast<std::uintptr_t>(arrow), 0, 0);
		DelayedActionManager_DelayedAction_AddWeaponHit(gDelayedActionManager, flightTicks, 4, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId), &point, 20);
	}

	// Original function: FUN_0040ad1d; the upper occupied-range bound is read each iteration.
	void __fastcall HouseArray_DoOneGameSecond(void* array, void*)
	{
		auto* handler = static_cast<CArrayHandler*>(array);

		for (auto id = handler->mFirstScanIndex; id <= handler->mLastScanIndex; ++id)
		{
			void* house = CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::RuntimeElementSize;

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0)
				House_DoOneGameSecond(house);
		}
	}

	// Original function: FUN_00433337; the native radius query includes both religion and hitpoint effects.
	void House_Religion_Send(const void* house)
	{
		CHumanIterator iterator;

		for (auto id = iterator.GetFirstHumanId(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId), static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition, 50); id != -1; id = iterator.GetNextHumanId())
		{
			void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Religion) <= 8000)
				Write(human, NC2Layout::HumanOffset::Religion, Read<std::uint32_t>(human, NC2Layout::HumanOffset::Religion) + 250u);

			Write(human, NC2Layout::HumanOffset::Hitpoints, Read<std::uint32_t>(human, NC2Layout::HumanOffset::Hitpoints) + 50u);
		}
	}

	// Original function: FUN_00433bf9; builder recruitment, stock upkeep and civilian defence stay in native order.
	void House_DoOneGameSecond(void* house)
	{
		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
			return;

		if (House_GetHouseMainType(house) != 6 && (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || !House_Repair_IsFinished(house)))
		{
			const auto builders = House_Inhabitants_GetNumberOfAttachedHumans(house, 4);
			const auto maximum = House_Construction_GetTotalNumberOfBuildersAllowed(house);

			if (NXBasics::Runtime_TimeSigned32(builders) >= maximum)
			{
				Write<std::uint32_t>(house, NC2Layout::HouseOffset::BuilderRecruitmentCounter, 0);
			}
			else
			{
				const auto wait = Read<std::uint32_t>(house, NC2Layout::HouseOffset::BuilderRecruitmentCounter) + 1u;
				Write(house, NC2Layout::HouseOffset::BuilderRecruitmentCounter, wait);

				if (wait >= 5)
				{
					Write<std::uint32_t>(house, NC2Layout::HouseOffset::BuilderRecruitmentCounter, 5);
					CHumanIterator iterator;
					auto* base = static_cast<std::byte*>(house) + NC2Layout::HouseOffset::MapPosition;
					const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(base), NC2Layout::MapMicroPointOffset::ContinentId);
					void* selected = nullptr;
					std::uint32_t bestDistance = 20000;

					for (auto id = iterator.GetFirstSectorHumanId(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId), Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint), 10000, false); id != -1; id = iterator.GetNextSectorHumanId())
					{
						void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

						if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != 7 || Human_UserControl_GetNextJobType(human) != 7 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0 || Human_IsAttachedToBuildSite(human) || Human_IsAttachedToVehicle(human))
							continue;

						const void* position = Human_GetMapPositionRef(human);

						if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(position), NC2Layout::MapMicroPointOffset::ContinentId) != continent)
							continue;

						const auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(base, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(base, NC2Layout::MapCoordinatePairOffset::Y)));

						if (selected != nullptr && distance >= bestDistance)
							continue;

						if (!Remaster_BuilderCanReachHouse(human, house, continent))
							continue;

						selected = human;
						bestDistance = distance;

						if (distance < 15)
							break;
					}

					if (selected != nullptr && House_Inhabitants_AttachHuman(house, Read<std::uint32_t>(selected, NC2Layout::HumanOffset::HumanId), 4))
					{
						Human_l_AssignWorkHouse(selected, Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId));
						Write<std::uint32_t>(selected, NC2Layout::HumanOffset::WorkRetryCount, 0);
						Write(selected, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint));
						Human_WorkCenterMarker_Update(selected);
					}
				}
			}
		}
		else
		{
			const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);

			if (type == 37)
			{
				House_Religion_Send(house);
			}
			else if (type == 11)
			{
				if (!House_Stock_IsFull(house, 12))
					House_Stock_ModifyAmount(house, 12, 1, 0, false);
			}
			else if (type == 10)
			{
				if (!House_Stock_IsFull(house, 1))
					House_Stock_ModifyAmount(house, 1, 1, 0, false);
			}

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) != 1 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) != 37)
				House_HomeQualityEnhancer_ModifyValue(house, 2, -1);
		}

		for (std::uint32_t index = 0; index < 66; ++index)
		{
			const auto stock = Read<std::uint32_t>(house, NC2Layout::HouseOffset::StockAmounts + index * NC2Layout::RecordSize::HouseCounterUnsigned);
			const auto recent = Read<std::uint32_t>(house, NC2Layout::HouseOffset::FutureStockAmounts + index * NC2Layout::RecordSize::HouseCounterUnsigned);

			if (stock > recent)
			{
				Write(house, NC2Layout::HouseOffset::FutureStockAmounts + index * NC2Layout::RecordSize::HouseCounterUnsigned, recent + 1u);
			}
			else if (stock < recent)
			{
				Write(house, NC2Layout::HouseOffset::FutureStockAmounts + index * NC2Layout::RecordSize::HouseCounterUnsigned, recent - 1u);
			}
		}

		for (std::uint32_t index = 0; index < 66; ++index)
		{
			const auto offset = NC2Layout::HouseOffset::StockRequestCounters + index * 4u;

			if (Read<std::int32_t>(house, offset) > 0)
				Write(house, offset, Read<std::uint32_t>(house, offset) - 1u);
		}

		House_l_ActionOverlay_Update(house);

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DefenceModeFlag) == 0)
		{

			return;
		}

		CHumanIterator iterator;
		const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);
		const std::uint32_t maximum = type == 1 ? 30u : type == 41 ? 20u
		                                                           : 10u;
		std::uint32_t count = 0;
		const auto* door = static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint;
		auto outside = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint);
		Tool_Map_GetNotBlockedPosition(&outside, true);

		for (auto id = iterator.GetFirstSectorHumanId(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId), Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint), 0, false); id != -1; id = iterator.GetNextSectorHumanId())
		{
			if (count != maximum)
			{
				++count;
				continue;
			}

			void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

			if (Human_IsDefendingHouse(human, Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId)))
				Human_DoAddNewUserCommand(human, 4, static_cast<std::uint8_t>(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), MapMigPoint_GetLong(&outside), 0);
		}

		for (auto id = iterator.GetFirstSectorHumanId(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId), Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint), 40, false); id != -1 && count < maximum; id = iterator.GetNextSectorHumanId())
		{
			void* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;
			const void* position = Human_GetMapPositionRef(human);

			if (Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(door, NC2Layout::MapCoordinatePairOffset::Coordinates) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
				continue;

			if (Human_IsDefendingHouse(human, Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId)))
			{
				++count;
				continue;
			}

			if (Human_IsDefendingHouse(human, -1))
			{
				const auto oldId = Human_GetDefenseHouseId(human);
				const void* oldHouse = oldId == -1 ? nullptr : CHouseArray::mStaticVars + static_cast<std::uint32_t>(oldId) * CHouseArray::RuntimeElementSize;
				const auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(door, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(door, NC2Layout::MapCoordinatePairOffset::Y)));
				const auto previousDistance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(oldHouse, NC2Layout::HouseOffset::DoorEntryPointX), Read<std::int16_t>(oldHouse, NC2Layout::HouseOffset::DoorEntryPointY)));

				if (distance >= previousDistance)
					continue;
			}
			else if (Human_UserControl_IsExecutingUserCommand(human))
			{
				continue;
			}

			Human_DoAddNewUserCommand(human, 4, static_cast<std::uint8_t>(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 0, MapMigPoint_GetLong(door), 1);
			++count;
		}
	}

	// Original function: FUN_0043420f; only nonzero counters decrement, then notify on zero.
	void House_l_ActionOverlay_Update(void* house)
	{
		for (std::int32_t overlay = 0; overlay < 5; ++overlay)
		{
			const auto offset = NC2Layout::HouseOffset::ActionOverlayCounters + static_cast<std::uint32_t>(overlay) * 4u;
			const auto value = Read<std::uint32_t>(house, offset);

			if (value == 0)
				continue;

			Write(house, offset, value - 1u);

			if (value == 1)
				House_ActionOverlay_Stop(house, overlay);
		}
	}
}