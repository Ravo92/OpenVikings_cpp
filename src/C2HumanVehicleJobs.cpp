#include "C2EngineDisplayAccess.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HouseBuildPosition.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanProduction.h"
#include "C2HumanVehicleJobs.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2StaticHouseTypeData.h"
#include "C2StaticVehicleTypeData.h"
#include "C2VehicleAccess.h"
#include "C2VehicleIterator.h"
#include "C2VehicleLifecycle.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* House(std::uint32_t id)
		{

			return CHouseArray::mStaticVars + id * CHouseArray::RuntimeElementSize;
		}

		const void* WorkCenter(const void* human)
		{

			return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
		}

		void Fail(void* human, std::uint32_t reason)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write(human, NC2Layout::HumanOffset::CurrentTaskStep, reason);
		}

		std::int32_t WorkRadius(const void* human)
		{
			const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

			return job == 18 || job == 29 ? 10 : 20;
		}

		void StartBuild(void* human, std::uint32_t action)
		{
			Human_l_StartAtomicAnimation(human, action, static_cast<std::int32_t>(Human_Target_GetDirectionTo(human)));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		}

		// The two invalid-site branches of Windows 00474205 queue the same command.
		void RestartUnfinishedShips(const void* human)
		{
			CVehicleIterator iterator;

			for (auto id = iterator.GetFirstVehicleId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), WorkCenter(human), WorkRadius(human)); id != -1; id = iterator.GetNextVehicleId())
			{
				void* vehicle = CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::RuntimeElementSize;
				const auto type = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

				if ((type != 3 && type != 4) || Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) != 0)
					continue;

				std::byte command[20]{ };
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 103);
				Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
				Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, static_cast<std::uint32_t>(id));
				Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, MapMigPoint_GetLong(WorkCenter(human)));
				Vehicle_DoAddNewUserCommand(vehicle, command);
			}
		}
	}

	// Original function: FUN_0045d2c4; entrance micro-points are excluded.
	bool Human_l_IsHouseTypeOnPoint(const void* human, const void* point, std::uint32_t type)
	{
		const void* micro = GameMap_GetConstMapMicroPointPtr(point);

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 2 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 2u) != 0)
			return false;

		const void* house = House(Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId));

		return Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) && Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == type && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3;
	}

	// Original function: FUN_0045da7c; the aligned center's success deliberately follows the native false-return path.
	bool Human_FindVehicleBuildPointNearWorkCenter(void* human, std::uint32_t type, void* realPosition, void* workPosition)
	{
		std::uint32_t stage = 0;
		const auto engineType = NC2E2::Engine2d_EntryPoint_GetHouseId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), type, &stage);
		std::uint32_t macro, candidate;
		MapMagPoint_ConstructFromMig(&macro, WorkCenter(human), false);
		MapMigPoint_ConstructFromMag(&candidate, &macro, false);
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&candidate), NC2Layout::MapMicroPointOffset::ContinentId);
		const auto& houseData = gStaticHouseTypeStorage.records[type];
		const auto size = gStaticVehicleTypeStorage.records[houseData.vehicleType].logicSize;
		MapMigPoint_ConstructFromMag(&candidate, &macro, false);
		auto result = Human_l_FindVehicleBuildPointNearWorkCenter(human, engineType, stage, houseData.ignoreContinents != 0, continent, realPosition, workPosition, &candidate, size);

		if (result != 2)
		{
			for (std::int32_t radius = 1; radius < WorkRadius(human) / 2; ++radius)
			{
				MapMagPoint_MoveCursorUnwrapped(&macro, 4);

				for (std::uint32_t direction = 0; direction < 6; ++direction)
				{
					for (std::int32_t step = 0; step < radius; ++step)
					{
						MapMagPoint_MoveCursorUnwrapped(&macro, direction);

						if (!MapMagPoint_IsInsideMap(&macro))
							continue;

						MapMigPoint_ConstructFromMag(&candidate, &macro, false);
						const auto next = Human_l_FindVehicleBuildPointNearWorkCenter(human, engineType, stage, houseData.ignoreContinents != 0, continent, realPosition, workPosition, &candidate, size);

						if (next == 2)
							return true;

						if (next == 1 || (next == 0 && result != 1))
							result = next;
					}
				}
			}
		}

		if (result == 0)
		{
			Fail(human, 8);
		}
		else if (result == 1)
		{
			Fail(human, 9);
		}

		return false;
	}

	// Original function: FUN_0045dc3f; 0 is unsuitable terrain, 1 an obstructing vehicle, 2 a usable site.
	std::uint32_t Human_l_FindVehicleBuildPointNearWorkCenter(const void* human, std::int32_t engineType, std::uint32_t stage, bool ignoreContinents, std::uint32_t continent, void* realPosition, void* workPosition, const void* candidate, std::uint32_t size)
	{
		const void* micro = GameMap_GetConstMapMicroPointPtr(candidate);

		if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & 7u) < size || !Tool_House_IsAllowedToBeBuild(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), engineType, stage, Read<std::uint32_t>(candidate, NC2Layout::TargetDataOffset::Type), ignoreContinents, false))
			return 0;

		Write(workPosition, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(candidate, NC2Layout::TargetDataOffset::Type));

		if (ignoreContinents && (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ContinentId) == continent || !Tool_House_IsWorkPointOnContinent(candidate, engineType, stage, continent, workPosition)))
			return 0;

		const void* definition = NC2E2::Engine2d_EntryPoint_GetHouseDataPtr(engineType);
		const auto* area = Read<const CMapArea*>(definition, NC2Layout::EngineHouseLogicViewOffset::WalkAreasUnsigned + stage * NC2Layout::RecordSize::EngineHouseAreaPointerUnsigned);

		for (std::uint32_t row = 0; row < area->rowCount; ++row)
		{
			std::uint32_t point;
			MapMigPoint_ConstructOffset(&point, candidate, area->rows[row].x, area->rows[row].y, false);

			for (std::uint32_t column = 0; column < area->rows[row].width; ++column)
			{
				if (MapMigPoint_IsInsideMap(&point) && Tool_IsAnyNotMovingVehicleInsidePoint(&point))
					return 1;

				Write<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X, static_cast<std::uint16_t>(Read<std::uint16_t>(&point, NC2Layout::MapCoordinatePairOffset::X) + 1u));
			}
		}

		Write(realPosition, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(candidate, NC2Layout::TargetDataOffset::Type));

		return 2;
	}

	// Original function: FUN_0045dd63; retain the micro-grid ring order and the special center check.
	void* Human_FindVehicleConstructionSiteNearWorkCenter(const void* human, std::uint32_t type)
	{
		std::uint32_t stage = 0;
		const auto engineType = NC2E2::Engine2d_EntryPoint_GetHouseId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), type, &stage);
		auto point = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter);
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId);

		if (Human_l_IsHouseTypeOnPoint(human, &point, type))
			return House(static_cast<std::uint32_t>(Logic_EntryPoint_House_GetOnPosition(&point)));

		for (std::int32_t radius = 1; radius < WorkRadius(human); ++radius)
		{
			MapMigPoint_MoveCursorUnwrapped(point, 4);

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				for (std::int32_t step = 0; step < radius; ++step)
				{
					MapMigPoint_MoveCursorUnwrapped(point, direction);

					if (!MapMigPoint_IsInsideMap(&point) || !Human_l_IsHouseTypeOnPoint(human, &point, type))
						continue;

					std::uint32_t work = 0;

					if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(&point), NC2Layout::MapMicroPointOffset::ContinentId) == continent || Tool_House_IsWorkPointOnContinent(&point, engineType, stage, continent, &work))
						return House(static_cast<std::uint32_t>(Logic_EntryPoint_House_GetOnPosition(&point)));
				}
			}
		}

		return nullptr;
	}

	// Original function: FUN_0046c8c2; Carpenter and Armorer share this Windows entry.
	void Human_l_StartTask_ExecuteJob_Carpenter(void* human)
	{
		if (Human_IsProducingVehicle(human))
		{
			Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder(human);
		}
		else
		{
			Human_l_StartTask_ExecuteJob_Tool_GoodProducer(human);
		}
	}

	// Original function: FUN_00474205.
	void Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder(void* human)
	{
		if (!Human_IsAttachedToWorkHouse(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) == 0)
		{
			Fail(human, 2);
			return;
		}

		if (!Human_IsAbleToProduceGoodNow(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType)))
		{
			Fail(human, 0);
			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
			{
				Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(human);
			}
			else
			{
				Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_Collect(human);
			}

			return;
		}

		if (Human_Production_GetCounter(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType)) == 0)
		{
			Human_Production_Rotate(human);
			return;
		}

		if (!Human_IsAttachedToBuildSite(human))
		{
			Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_TryToBuild(human);

			if (!Human_IsAttachedToBuildSite(human))
				return;
		}
		else
		{
			const void* site = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId));

			if (Read<std::uint8_t>(site, NC2Layout::HouseOffset::AllocatedFlag) == 0)
			{
				Write<std::int32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId, -1);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionWorkshopRevision, 0);
				Human_Production_Rotate(human);
				RestartUnfinishedShips(human);
				return;
			}

			const auto type = Read<std::uint32_t>(site, NC2Layout::HouseOffset::LogicType);
			const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

			if (type < 42 || type > 46 || good != type + 17u)
			{
				Write<std::int32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId, -1);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionWorkshopRevision, 0);
				RestartUnfinishedShips(human);
				return;
			}
		}

		void* site = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId));

		if (Read<std::uint32_t>(site, NC2Layout::HouseOffset::ConstructionState) == 3 && House_Repair_IsFinished(site))
		{
			// Remaster: another builder may have completed this site; end the startup task instead of retrying it forever.
			Fail(human, 2);

			return;
		}

		if (House_Construction_CanUseOneGoodNow(site))
			House_Construction_DoUseOneGood(site);

		if (!House_Construction_IsUseGoodRunning(site) && House_Repair_IsFinished(site))
		{
			if (House_Construction_IsNeedingGoods(site))
			{
				Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_TryToCollect(human);
			}
			else
			{
				Fail(human, 10);
			}
		}
		else
		{
			Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_BuildVehicle(human);
		}
	}

	// Original function: FUN_004748b4.
	void Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_TryToCollect(void* human)
	{
		Human_Target_Reset(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		std::uint32_t real = 0, work = 0, good;

		if (Human_FindGoodsDemandedForBuildVehicle(human, &real, &work, &good, WorkCenter(human)))
		{
			Write(human, NC2Layout::HumanOffset::RequestedGood, good);

			if (Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
			{

				return;
			}
		}

		Fail(human, 4);
	}

	// Original function: FUN_004749d8; preserve this distinct vehicle-builder entry.
	void Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_Collect(void* human)
	{
		if (Human_IsAtTargetPoint(human))
		{
			if (Human_l_StartTask_Tool_PickupWantedGood_FromTargetPosition(human))
				return;

			Human_Target_Reset(human);
			Human_StopCurrentGoodCirculationMode(human);

			return;
		}

		Human_StopCurrentGoodCirculationMode(human);
		Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_TryToCollect(human);
	}

	// Original function: FUN_00474e6f.
	void Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_TryToBuild(void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);
		const auto type = good >= 59 && good <= 63 ? good - 17u : 42u;

		if (void* site = Human_FindVehicleConstructionSiteNearWorkCenter(human, type))
		{
			Human_l_AttachToVehicleBuildHouse(human, Read<std::int32_t>(site, NC2Layout::HouseOffset::HouseId));
			return;
		}

		std::uint32_t real = 0, work = 0;

		if (!Human_FindVehicleBuildPointNearWorkCenter(human, type, &real, &work))
		{

			return;
		}

		Logic_EntryPoint_Edit_Start();
		std::uint32_t stage = 0;
		const auto engineType = NC2E2::Engine2d_EntryPoint_GetHouseId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), type, &stage);
		const auto id = Logic_EntryPoint_House_Set(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), engineType, stage, &real, false, 0);
		Logic_EntryPoint_Edit_Stop();
		Human_l_AttachToVehicleBuildHouse(human, id);
	}

	// Original function: FUN_00476b84; Windows stores the workshop revision, not the construction-site revision.
	// No separate Mac identity was established for this outlined entry.
	void Human_l_AttachToVehicleBuildHouse(void* human, std::int32_t house)
	{
		Write(human, NC2Layout::HumanOffset::VehicleConstructionHouseId, house);
		Write(human, NC2Layout::HumanOffset::VehicleConstructionWorkshopRevision, Read<std::uint32_t>(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)), NC2Layout::HouseOffset::UniqueId));
		Human_UpdateCurrentWorkHouse(human);
	}

	// Original function: FUN_00474c3e; occupied builder positions are retried before the unrestricted search.
	void Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_BuildVehicle(void* human)
	{
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human)), NC2Layout::MapMicroPointOffset::ContinentId);
		const bool atTarget = Human_IsAtTargetPoint(human);
		void* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId) * CHouseArray::RuntimeElementSize;
		std::uint32_t work = 0;

		if (atTarget)
		{
			const void* realTarget = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition;

			if (Read<std::uint32_t>(realTarget, NC2Layout::MapCoordinatePairOffset::Coordinates) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition))
			{
				Human_Target_Reset(human);
				Human_l_StartTask_ExecuteJob_Tool_VehicleBuilder_BuildVehicle(human);
				return;
			}

			const void* workTarget = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetWorkPosition;

			if (!House_Construction_GetNextWorkPoint(house, workTarget, &work, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), continent) && !House_Construction_GetNextWorkPoint(house, workTarget, &work, false, NXBasics::IntegerRange::UInt32Maximum, continent))
			{
				Fail(human, 7);
				Human_Target_Reset(human);
				return;
			}

			if (Read<std::uint32_t>(workTarget, NC2Layout::MapCoordinatePairOffset::Coordinates) != work)
			{
				if (!Human_l_StartAtomicWalk(human, realTarget, &work))
				{
					Fail(human, 0);
					Human_Target_Reset(human);
				}

				return;
			}

			if (!House_Construction_IsUseGoodRunning(house))
			{
				Fail(human, 10);
				Human_Target_Reset(human);
				return;
			}
		}
		else
		{
			const auto real = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition);

			if (!House_Construction_GetNextWorkPoint(house, Human_GetMapPositionRef(human), &work, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), continent) && !House_Construction_GetNextWorkPoint(house, Human_GetMapPositionRef(human), &work, false, NXBasics::IntegerRange::UInt32Maximum, continent))
			{
				Fail(human, 7);
				Human_Target_Reset(human);
				return;
			}

			if (Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) != work)
			{
				if (!Human_l_StartAtomicWalk(human, &real, &work))
					Fail(human, 0);

				return;
			}

			if (!House_Construction_IsUseGoodRunning(house) && House_Repair_IsFinished(house))
			{
				Fail(human, 10);
				Human_Target_Reset(human);

				return;
			}

			Human_Target_SetTargetPositions(human, &real, &work);
		}

		StartBuild(human, 39);
	}

}