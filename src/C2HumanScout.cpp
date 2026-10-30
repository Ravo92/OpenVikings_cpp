#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2GuideAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanEquipment.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanScout.h"
#include "C2HumanTaskAccess.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapPathfinderSearch.h"
#include "C2ObjectLayout.h"
#include "C2VehicleAccess.h"
#include "C2WallRoadConstruction.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		const void* RealTarget(const void* human)
		{

			return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition;
		}

		const void* WorkTarget(const void* human)
		{

			return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetWorkPosition;
		}

		const void* WorkCenter(const void* human)
		{

			return static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter;
		}

		void Queue(void* human, std::uint8_t opcode, std::uint32_t parameter, std::uint32_t revision)
		{
			Human_DoAddNewUserCommand(human, opcode, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), revision, parameter, 0);
		}

		struct SNotExploredPointSearch
		{
			std::uint32_t player;
			std::uint32_t count;
			std::uint32_t points[8];
		};

		static_assert(sizeof(SNotExploredPointSearch) == 40);

		// Original function: FUN_0045d5d1; semantic name inferred from Windows.
		bool __cdecl SearchNotExploredPoint(const void* point, const SMapPathfinderManagerSearchOverMapCallbackData* callback)
		{
			auto& data = *static_cast<SNotExploredPointSearch*>(callback->userData);
			const auto* micro = GameMap_GetConstMapMicroPointPtr(point);

			if ((Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || data.player >= 16 || (Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << data.player)) != 0)
				return false;

			data.points[data.count++] = Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates);

			return data.count == 8;
		}
	}

	// Original function: FUN_0043d3fe.
	std::uint32_t Human_DoExecuteUserCommand_BuildGuide(void* human, const void* point)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (!Human_IsJobScout(human))
			return 1;

		Write<std::uint8_t>(human, NC2Layout::HumanOffset::QueuedExploreAreaAndUpdateCenterCommand, 0);
		Human_Target_Reset(human);
		Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
		Human_WorkCenterMarker_Update(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 19);

		return 2;
	}

	// Original function: FUN_0043d45d.
	std::uint32_t Human_DoExecuteUserCommand_ExploreArea(void* human, const void* point)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return 0;

		if (!Human_IsJobScout(human))
			return 1;

		if (MapMigPoint_IsInsideMap(point))
		{
			Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(point, NC2Layout::MapCoordinatePairOffset::Coordinates));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
			Human_WorkCenterMarker_Update(human);
			Human_Target_SetTargetPositions(human, point, point);
		}
		else if (Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) != Read<std::uint32_t>(WorkCenter(human), NC2Layout::MapCoordinatePairOffset::Coordinates))
		{
			Human_Target_Reset(human);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 20);

		return 2;
	}

	// Original function: FUN_00445a90.
	void Human_l_FinishTask_Misc(void* human)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 0);
		Human_Target_Reset(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 8);

		if (InteractionTargetData_IsValid(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::SecondaryInteractionTarget))
			Human_Target_RestoreSecondaryTarget(human);
	}

	// Original function: FUN_0044621d; the queried command is opcode 18.
	void Human_l_FinishTask_BuildGuide(void* human)
	{
		if (!Human_UserControl_IsCommandTypeInQueue(human, 18))
		{
			Human_l_FinishTask_Misc(human);

			return;
		}

		Human_Target_Reset(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 8);
	}

	// Original function: FUN_0044624c.
	void Human_l_StartTask_BuildGuide(void* human)
	{
		Human_Target_SetTargetPositions(human, WorkCenter(human), WorkCenter(human));

		if (Human_IsAtTargetPoint(human))
		{
			Human_l_StartAtomicAnimation(human, 43, -1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
			return;
		}

		if (Human_l_StartAtomicWalk(human, RealTarget(human), WorkTarget(human)))
		{
			Queue(human, 19, MapMigPoint_GetLong(WorkCenter(human)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));

			return;
		}

		Human_Target_Reset(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 7);
	}

	// Original function: FUN_004462e1.
	void Human_l_StartTask_ExploreArea(void* human)
	{
		if (Human_Target_IsValid(human))
		{
			if (Human_IsAtTargetPoint(human))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
				return;
			}

			if (Human_l_StartAtomicWalk(human, RealTarget(human), WorkTarget(human)))
			{
				Queue(human, 20, MapMigPoint_GetLong(WorkTarget(human)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));
				return;
			}
		}
		else
		{
			std::uint32_t point = 0;

			if (!Human_FindNotExploredPointNearWorkCenter(human, &point))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 2);
				return;
			}

			if (Human_l_StartAtomicWalk(human, &point, &point))
			{
				Queue(human, 20, NXBasics::IntegerRange::UInt32Maximum, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));

				return;
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 1);
	}

	// Original function: FUN_004463c7.
	void Human_l_FailTask_ExploreArea(void* human)
	{
		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep))
		{
			case SwitchIds::HumanTaskFailure::NoWay:
				Human_UserMessage_SendToUserMessage(human, 3);
				return;
			case SwitchIds::HumanTaskFailure::NoWorkPoint:
				Human_UserMessage_SendToUserMessage(human, 8);
				return;
			case SwitchIds::HumanTaskFailure::WorkHouseUnavailable:
			{
				const auto houseId = Read<std::int32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId);

				if (houseId == -1 || Read<std::uint32_t>(CHouseArray::mStaticVars + static_cast<std::intptr_t>(houseId) * CHouseArray::RuntimeElementSize, NC2Layout::HouseOffset::ConstructionState) != 2)
					Human_UserMessage_SendToUserMessage(human, 11);

				if (MapMigPoint_IsInsideMap(WorkCenter(human)) && Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) != Read<std::uint32_t>(WorkCenter(human), NC2Layout::MapCoordinatePairOffset::Coordinates))
					Queue(human, 4, MapMigPoint_GetLong(WorkCenter(human)), 0);

				return;
			}
			default:
				Human_UserMessage_SendToUserMessage(human, 2);

				return;
		}
	}

	// Original function: FUN_00446468; 00445a6e shares the existing idle finish body.
	void Human_l_FinishTask_ExploreArea(void* human)
	{
		if (Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) != Read<std::uint32_t>(WorkCenter(human), NC2Layout::MapCoordinatePairOffset::Coordinates))
			Queue(human, 20, NXBasics::IntegerRange::UInt32Maximum, Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId));

		Human_l_FinishTask_Idle(human);
	}

	// Original function: FUN_0045d56f; Windows constant 004F4308 is 40.
	// No separate Mac identity was established for this Windows search function.
	bool Human_FindNotExploredPointNearWorkCenter(const void* human, void* point)
	{
		SNotExploredPointSearch data{ };
		data.player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);
		CMapPathfinderElement::SearchOverMap(WorkCenter(human), 40, true, SearchNotExploredPoint, &data, true, static_cast<std::int32_t>(data.player));

		if (data.count == 0)
			return false;

		Write(point, NC2Layout::MapCoordinatePairOffset::Coordinates, data.points[Game_Random_GetNumber() % data.count]);

		return true;
	}

	// Original function: FUN_00468f5d; house construction, repair, roads, walls and scout guides.
	void Human_l_PerformTaskEvent_Build(void* human)
	{
		if (!Human_Target_IsValid(human))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 7);
			return;
		}

		if (Human_IsAttachedToWorkHouse(human) || Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId) != NXBasics::IntegerRange::UInt32Maximum)
		{
			void* house = CHouseArray::mStaticVars + (Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId) != NXBasics::IntegerRange::UInt32Maximum ? Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId) : Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)) * CHouseArray::RuntimeElementSize;

			if (Read<std::uint32_t>(RealTarget(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition))
			{
				auto efficiency = static_cast<std::uint32_t>(Human_JobEfficiency_CalculateExperienceFactor(human)) + 150u;

				if (Human_HasEquippedTools(human))
				{
					efficiency = Human_l_JobEfficiency_CalculateToolFactor(human) * efficiency / 100u;
					// Original outlined wear 0046A3B3, with PE constants -100 and 10000.
					const auto condition = NXBasics::Runtime_TimeSigned32(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ToolCondition) - 100u);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::ToolCondition, condition < 0 ? 0u : static_cast<std::uint32_t>(condition));

					if (condition >= 0 && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedTools) == 31 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedTools) == 32) && condition > 10000)
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::ToolCondition, 10000);
				}

				for (auto steps = efficiency / 100u; steps != 0; --steps)
				{
					if (House_Construction_DoOneStep(house))
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
				}

				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == 2)
				{
					Human_l_JobExperience_Gain(human);

					if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3)
					{
						Human_Production_DidProduceGoods(human, 1);
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId, NXBasics::IntegerRange::UInt32Maximum);
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionWorkshopRevision, 0);
					}

					return;
				}
			}
		}

		if (Human_IsAttachedToRoadPoint(human))
		{
			Human_ClearCarriedGood(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);

			if (!RoadBuildManager_FinishRoad(gRoadBuildManager, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), RealTarget(human)))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
				return;
			}

			auto point = Read<std::uint32_t>(RealTarget(human), NC2Layout::MapCoordinatePairOffset::Coordinates);
			MapMigPoint_MoveCursorUnwrapped(point, 4);

			for (std::uint32_t direction = 0; direction < 6; ++direction)
			{
				MapMigPoint_MoveCursorUnwrapped(point, direction);
				RoadBuildManager_FinishRoad(gRoadBuildManager, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), &point);
			}

			Human_l_JobExperience_Gain(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
			return;
		}

		if (Human_IsAttachedToWallPoint(human))
		{
			bool done;

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
			{
				Human_ClearCarriedGood(human);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 0);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
				done = WallBuildManager_FinishWall(gWallBuildManager, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), RealTarget(human));
			}
			else
			{
				done = GameMap_MapMicroPoint_Landscape_ApplyTransition(RealTarget(human), 9) != -1;
			}

			if (done)
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
				Human_l_JobExperience_Gain(human);
			}
			else
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			}

			return;
		}

		if (Human_IsJobScout(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 43 && GuideManager_InsertGuideAtPosition(gGuideManager, nullptr, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), RealTarget(human), 1))
		{
			Human_l_JobExperience_Gain(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		}
	}
}