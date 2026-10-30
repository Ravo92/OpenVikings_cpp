#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanConstruction.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanProduction.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2VehicleAccess.h"
#include "C2WallRoadConstruction.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* WorkCenter(void* human)
		{

			return static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
		}

		void Fail(void* human, std::uint32_t reason)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write(human, NC2Layout::HumanOffset::CurrentTaskStep, reason);
		}

		void StartBuild(void* human, std::uint32_t action)
		{
			Human_l_StartAtomicAnimation(human, action, static_cast<std::int32_t>(Human_Target_GetDirectionTo(human)));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		}
	}

	// Original function: FUN_0046c822.
	void Human_l_StartTask_ExecuteJob_Builder(void* human)
	{
		if (Human_IsAttachedToWorkHouse(human))
		{
			Human_l_StartTask_ExecuteJob_Tool_HouseBuilder(human);
			return;
		}

		if (Human_IsAttachedToRoadPoint(human))
		{
			Human_l_StartTask_ExecuteJob_Tool_RoadBuilder(human);
			return;
		}

		if (Human_IsAttachedToWallPoint(human))
		{
			Human_l_StartTask_ExecuteJob_Tool_WallBuilder(human);

			return;
		}

		Human_WorkCenterMarker_Remove(human);
		Fail(human, 2);
	}

	// Original function: FUN_00473fff; the native cargo-return branch shares 00474F17 with FillStock.
	void Human_l_StartTask_ExecuteJob_Tool_HouseBuilder(void* human)
	{
		if (!Human_IsAttachedToWorkHouse(human))
		{
			Fail(human, 2);
			return;
		}

		void* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;

		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 && House_Repair_IsFinished(house))
		{
			House_Inhabitants_DetachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 4);
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
				Human_l_StartTask_ExecuteJob_Tool_HouseBuilder_Collect(human);
			}

			return;
		}

		if (House_Construction_CanUseOneGoodNow(house))
			House_Construction_DoUseOneGood(house);

		if (!House_Construction_IsUseGoodRunning(house) && House_Repair_IsFinished(house))
		{
			if (House_Construction_IsNeedingGoods(house))
			{
				Human_l_StartTask_ExecuteJob_Tool_HouseBuilder_TryToCollect(human);
			}
			else
			{
				Fail(human, 10);
			}
		}
		else
		{
			Human_l_StartTask_ExecuteJob_Tool_HouseBuilder_BuildHouse(human);
		}
	}

	// Original function: FUN_0047472f.
	void Human_l_StartTask_ExecuteJob_Tool_HouseBuilder_TryToCollect(void* human)
	{
		Human_Target_Reset(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		std::uint32_t real = 0, work = 0, good;

		if (Human_FindGoodsDemandedForBuildHouse(human, &real, &work, &good, WorkCenter(human)))
		{
			Write(human, NC2Layout::HumanOffset::RequestedGood, good);

			if (Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
			{

				return;
			}
		}

		Fail(human, 4);
	}

	// Original function: FUN_00474939.
	void Human_l_StartTask_ExecuteJob_Tool_HouseBuilder_Collect(void* human)
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
		Human_l_StartTask_ExecuteJob_Tool_HouseBuilder_TryToCollect(human);
	}

	// Original function: FUN_00474a0d; occupied builder positions are retried before the unrestricted search.
	void Human_l_StartTask_ExecuteJob_Tool_HouseBuilder_BuildHouse(void* human)
	{
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human)), NC2Layout::MapMicroPointOffset::ContinentId);
		const bool atTarget = Human_IsAtTargetPoint(human);
		void* house = CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize;
		std::uint32_t work = 0;

		if (atTarget)
		{
			const void* realTarget = static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition;

			if (Read<std::uint32_t>(realTarget, NC2Layout::MapCoordinatePairOffset::Coordinates) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition))
			{
				Human_Target_Reset(human);
				Human_l_StartTask_ExecuteJob_Tool_HouseBuilder_BuildHouse(human);
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

	// Original function: FUN_004740d4.
	void Human_l_StartTask_ExecuteJob_Tool_RoadBuilder(void* human)
	{
		if (!Human_IsAttachedToRoadPoint(human))
		{
			Fail(human, 2);
			return;
		}

		if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(WorkCenter(human)), NC2Layout::MapMicroPointOffset::ObjectType) == 2)
		{
			RoadBuildManager_ClearRoadBuildFlagForAnyPlayer(gRoadBuildManager, WorkCenter(human));

			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == 0)
		{
			Human_l_StartTask_ExecuteJob_Tool_RoadBuilder_TryToCollect(human);
		}
		else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_ExecuteJob_Tool_RoadBuilder_BringAway(human);
		}
		else
		{
			Human_l_StartTask_ExecuteJob_Tool_RoadBuilder_Collect(human);
		}
	}

	// Original function: FUN_00474143.
	void Human_l_StartTask_ExecuteJob_Tool_WallBuilder(void* human)
	{
		if (!Human_IsAttachedToWallPoint(human))
		{
			Fail(human, 2);
			return;
		}

		const auto* micro = GameMap_GetConstMapMicroPointPtr(WorkCenter(human));

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2)
		{
			WallBuildManager_ClearWallBuildFlagForAnyPlayer(gWallBuildManager, WorkCenter(human));
			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
			{
				Human_l_StartTask_ExecuteJob_Tool_WallBuilder_BringAway(human);
			}
			else
			{
				Human_l_StartTask_ExecuteJob_Tool_WallBuilder_Collect(human);
			}

			return;
		}

		if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::TerrainFlags) & 8u) != 0)
		{
			Human_l_StartTask_ExecuteJob_Tool_WallBuilder_TryToCollect(human);
			return;
		}

		auto work = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter);

		if (!GameMap_Misc_GetLandscapeWorkPosition(WorkCenter(human), &work))
		{
			Fail(human, 7);

			return;
		}

		Human_l_StartTask_ExecuteJob_Tool_WallBuilder_Repair(human);
	}

	// Original function: FUN_00474516.
	void Human_l_StartTask_ExecuteJob_Tool_RoadBuilder_BringAway(void* human)
	{
		if (Human_Target_HasTarget(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetPosition) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter))
		{
			Human_Target_Reset(human);
			Human_l_StartTask_ExecuteJob_Tool_RoadBuilder_BringAway(human);
			return;
		}

		if (Human_IsAtTargetPoint(human))
		{
			StartBuild(human, 41);
			return;
		}

		if (Human_IsAtWorkCenter(human))
		{
			Human_Target_SetTargetPositions(human, WorkCenter(human), WorkCenter(human));
			StartBuild(human, 41);

			return;
		}

		if (!Human_l_StartAtomicWalk(human, WorkCenter(human), WorkCenter(human)))
			Fail(human, 1);
	}

	// Original function: FUN_004745ae.
	void Human_l_StartTask_ExecuteJob_Tool_WallBuilder_BringAway(void* human)
	{
		if (Human_Target_HasTarget(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetPosition) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter))
		{
			Human_Target_Reset(human);
			Human_l_StartTask_ExecuteJob_Tool_WallBuilder_BringAway(human);
			return;
		}

		if (Human_IsAtTargetPoint(human))
		{
			StartBuild(human, 42);
			return;
		}

		if (Human_IsAtWorkCenter(human))
		{
			Human_Target_SetTargetPositions(human, WorkCenter(human), WorkCenter(human));
			StartBuild(human, 42);

			return;
		}

		if (!Human_l_StartAtomicWalk(human, WorkCenter(human), WorkCenter(human)))
			Fail(human, 1);
	}

	// Original function: FUN_00474646.
	void Human_l_StartTask_ExecuteJob_Tool_WallBuilder_Repair(void* human)
	{
		if (Human_Target_HasTarget(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetPosition) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter))
		{
			Human_Target_Reset(human);
			Human_l_StartTask_ExecuteJob_Tool_WallBuilder_BringAway(human);
			return;
		}

		if (Human_IsAtTargetPoint(human))
		{
			StartBuild(human, 42);
			return;
		}

		auto work = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter);

		if (!GameMap_Misc_GetLandscapeWorkPosition(WorkCenter(human), &work))
		{
			Fail(human, 7);

			return;
		}

		Human_Target_SetTargetPositions(human, WorkCenter(human), &work);

		if (Human_IsAtTargetPoint(human))
		{
			StartBuild(human, 42);
		}
		else if (!Human_l_StartAtomicWalk(human, WorkCenter(human), &work))
		{
			Fail(human, 1);
		}
	}

	// Original function: FUN_004747b4.
	void Human_l_StartTask_ExecuteJob_Tool_RoadBuilder_TryToCollect(void* human)
	{
		Human_Target_Reset(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 3);
		std::uint32_t real = 0, work = 0;

		if (Human_FindGood_Complex(human, 3, &real, &work, WorkCenter(human)) && Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
		{

			return;
		}

		Fail(human, 4);
	}

	// Original function: FUN_00474833.
	void Human_l_StartTask_ExecuteJob_Tool_WallBuilder_TryToCollect(void* human)
	{
		Human_Target_Reset(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 5);
		std::uint32_t real = 0, work = 0;

		if (Human_FindGood_Complex(human, 5, &real, &work, WorkCenter(human)) && Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
		{

			return;
		}

		Fail(human, 4);
	}

	// Original function: FUN_0047496e.
	void Human_l_StartTask_ExecuteJob_Tool_RoadBuilder_Collect(void* human)
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
		Human_l_StartTask_ExecuteJob_Tool_RoadBuilder_TryToCollect(human);
	}

	// Original function: FUN_004749a3.
	void Human_l_StartTask_ExecuteJob_Tool_WallBuilder_Collect(void* human)
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
		Human_l_StartTask_ExecuteJob_Tool_WallBuilder_TryToCollect(human);
	}
}