#include "C2Debug.h"
#include "C2EngineLandscapeAccess.h"
#include "C2GameControlStartup.h"
#include "C2GameEngineStartup.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanAtomicEvents.h"
#include "C2HumanCollectionEvents.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanLandscapeSearch.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapResourceManagers.h"
#include "C2ObjectLayout.h"
#include "C2StaticGoodTypeData.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		const void* Real(const void* human)
		{

			return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition;
		}

		bool ValidateEventTarget(void* human)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == 0)
				Human_Target_Reset(human);

			if (Human_Target_IsValid(human))
				return true;

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 0);

			return false;
		}

		void WearTools(void* human)
		{
			if (Human_HasEquippedTools(human))
				Human_ChangeToolCondition(human, -100);
		}

		bool RetryCompleted(void* human)
		{
			Human_l_JobEfficiency_SetupNeededRetries(human);
			Write(human, NC2Layout::HumanOffset::WorkRetryCount, Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount) + 1u);

			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkRetryCount) >= Human_l_JobEfficiency_GetNeededRetries(human))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
				return true;
			}

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
			WearTools(human);
			Human_StopCurrentGoodCirculationMode(human);

			return false;
		}
	}

	// Original function: FUN_0046949e.
	void Human_l_PerformTaskEvent_Transform(void* human)
	{
		if (!ValidateEventTarget(human))
			return;

		if (Human_IsAttachedToWorkHouse(human))
		{
			const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);
			std::int32_t landscape = 0;

			switch (good)
			{
				case SwitchIds::GoodType::Wheat:
					landscape = 27;
					break;
				case SwitchIds::GoodType::Herb:
					landscape = 33;
					break;
				case SwitchIds::GoodType::Mushroom:
					landscape = 36;
					break;
				default:
					C2Debug_WarningBoxV("Production of this good is not possible at all: %s", gStaticGoodTypeStorage.records[good].name);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
					Human_StopCurrentGoodCirculationMode(human);

					return;
			}

			Logic_EntryPoint_Edit_Start();
			const auto engineId = NC2LogicExternals::External_Landscape_GetEngineIdOfMapLandscape(landscape, false);
			const auto success = NC2LogicExternals::External_Landscape_SetMapMicroPointLandscape(Real(human), engineId, 1, true, -1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, success ? 2u : 3u);
			Logic_EntryPoint_Edit_Stop();
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		}

		Human_StopCurrentGoodCirculationMode(human);
	}

	// Original function: FUN_00469565; native retries release future-stock reservations.
	void Human_l_PerformTaskEvent_Grow(void* human)
	{
		if (!ValidateEventTarget(human) || !RetryCompleted(human))
		{

			return;
		}

		if (GameMap_MapMicroPoint_Landscape_ApplyTransition(Real(human), 11) != -1)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) == 5)
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x17, true, 10, MapMigPoint_GetLong(Real(human)), 0);

			Human_l_JobExperience_Gain(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
			WearTools(human);

			if (Human_IsAttachedToWorkHouse(human))
				House_Production_DidProduceOneGood(CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::RuntimeElementSize);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		}

		Human_StopCurrentGoodCirculationMode(human);
		Human_Target_Reset(human);
	}

	// Original function: FUN_0046968b; both landscape setter calls are present in Windows.
	void Human_l_PerformTaskEvent_SplitUp(void* human)
	{
		if (!ValidateEventTarget(human) || !RetryCompleted(human))
			return;

		const auto engineId = GameMap_MapMicroPoint_Landscape_ApplyTransition(Real(human), 5);
		Human_StopCurrentGoodCirculationMode(human);
		std::uint32_t pile = 0;

		if (engineId == -1 || !Human_FindPilePointNearPosition(human, engineId, Human_GetMapPositionRef(human), &pile))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);

			return;
		}

		GameMapModifyManager_StartGameMapModify();
		NC2LogicExternals::External_Landscape_SetMapMicroPointLandscape(&pile, engineId, 1, true, -1);
		GameMap_GetConstMapMicroPointPtr(&pile);
		NC2LogicExternals::External_Landscape_SetMapMicroPointLandscape(&pile, engineId, 1, true, -1);
		GameMapModifyManager_GameMapModified(10, &pile, 0);
		GameMapModifyManager_StopGameMapModify();
		WearTools(human);
		Human_l_JobExperience_Gain(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
	}

	// Original function: FUN_004697d2; cursor movement precedes each transition.
	void Human_l_PerformTaskEvent_Plant(void* human)
	{
		if (!ValidateEventTarget(human))
		{

			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		auto cursor = Read<std::uint32_t>(Real(human), NC2Layout::MapCoordinatePairOffset::Coordinates);
		MapMigPoint_MoveCursorUnwrapped(cursor, 4);
		bool changed = false;

		for (std::uint32_t direction = 0; direction < 6; ++direction)
		{
			MapMigPoint_MoveCursorUnwrapped(cursor, direction);

			if (MapMigPoint_IsInsideMap(&cursor) && GameMap_MapMicroPoint_Landscape_ApplyTransition(&cursor, 7) != -1)
				changed = true;
		}

		if (GameMap_MapMicroPoint_Landscape_ApplyTransition(Real(human), 7) != -1)
			changed = true;

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, changed ? 2u : 3u);
		WearTools(human);
		Human_StopCurrentGoodCirculationMode(human);
	}

	// Original function: FUN_004698c4.
	void Human_l_PerformTaskEvent_CatchFish(void* human)
	{
		Human_l_JobEfficiency_SetupNeededRetries(human);
		Write(human, NC2Layout::HumanOffset::WorkRetryCount, Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount) + 1u);

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkRetryCount) < Human_l_JobEfficiency_GetNeededRetries(human))
		{
			Human_l_ResetAtomic(human);
			Human_l_StartAtomicAnimation(human, 38, Human_Target_GetDirectionTo(human));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
			WearTools(human);
			Human_StopCurrentGoodCirculationMode(human);

			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		std::uint32_t swarmPosition = 0;
		const auto caught = static_cast<CMapFishManager*>(gMapFishManager)->GetOneFishFromNearestSwarm(Real(human), 20, &swarmPosition);
		Human_l_ResetAtomic(human);
		Human_l_StartAtomicAnimation(human, caught ? 37u : 38u, Human_Target_GetDirectionTo(human));

		if (caught)
		{
			Human_l_JobExperience_Gain(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::FoodProductionNeedCheckDeferrals, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
			// Native outlined carried-good setter 0042DB6C invokes callback 0x21.
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood, 16);
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, 16);
			Human_l_PerformTaskEvent_DidProduceOneGood(human);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 5)
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 4);

			GameMap_Exploration_DoExplore(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), &swarmPosition, (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ExplorationRadius) >> 1) + 1u);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Human_StopCurrentGoodCirculationMode(human);
		}

		WearTools(human);
		Human_Target_Reset(human);
	}
}