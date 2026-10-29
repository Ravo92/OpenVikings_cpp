#include "C2AIMovementMessages.h"
#include "C2Debug.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HouseBuildPosition.h"
#include "C2HumanAnimalJobs.h"
#include "C2HumanAtomicEvents.h"
#include "C2HumanAttackTasks.h"
#include "C2HumanChest.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanConstruction.h"
#include "C2HumanEquipment.h"
#include "C2HumanFamily.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanHuntingJobs.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanMapCollection.h"
#include "C2HumanMessageAccess.h"
#include "C2HumanMilitaryJobs.h"
#include "C2HumanNeeds.h"
#include "C2HumanProduction.h"
#include "C2HumanScout.h"
#include "C2HumanSocialJobs.h"
#include "C2HumanTaskAccess.h"
#include "C2HumanTraderJobs.h"
#include "C2HumanTransportJobs.h"
#include "C2HumanVehicleJobs.h"
#include "C2HumanWalking.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MilitaryTargetSearch.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2StaticActorTypeDataAccess.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* Moveable(const void* human)
		{

			return gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;
		}

		const void* Position(const void* human)
		{

			return Human_GetMapPositionRef(human);
		}

		const void* JobData(const void* human)
		{

			return static_cast<const std::byte*>(gStaticJobTypeData) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType;
		}

	}

	// Original function: FUN_0042cb92; Mac IsAbleToDoGoodieNow.
	bool Human_IsAbleToDoGoodieNow(const void* human)
	{
		if (Human_IsJobSoldier(human) || Human_IsJobHero(human))
			return false;

		const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);

		if (task != 1 && (task < 24 || task > 29))
			return false;

		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) != 23 && !Human_Misc_IsHidden(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) == 0 && Human_l_IsAtomicInterruptable(human);
	}

	// Original function: FUN_0042ceaf.
	void Human_Target_ResetSecondaryTarget(void* human)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryInteractionTarget, 0);
	}

	// Original function: FUN_004369c8.
	bool Human_UserControl_IsExecutingUserCommand(const void* human)
	{

		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 4 || Human_UserControl_IsPriorityCommandInQueue(human);
	}

	// Original function: FUN_004433d1; all native task startup branches are connected.
	void Human_CurrentTask_DoStartUp(void* human)
	{
		if (!Human_l_IsAtomicInterruptable(human))
			return;

		const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);

		if (task >= 24 && task <= 29)
		{
			Human_l_StartTask_Goodie(human);
			return;
		}

		if (task == 2)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 2);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
			return;
		}

		if (task == 7)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanCount, 0);

			switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter))
			{
				case SwitchIds::HumanNeed::Food:
					Human_l_StartTask_FulfillNeeds_Food(human);
					break;
				case SwitchIds::HumanNeed::Stamina:
					Human_l_StartTask_FulfillNeeds_Stamina(human);
					break;
				case SwitchIds::HumanNeed::Social:
					Human_l_StartTask_FulfillNeeds_Social(human);
					break;
				case SwitchIds::HumanNeed::Religion:
					Human_l_StartTask_FulfillNeeds_Religion(human);
					break;
				default:
					break;
			}

			return;
		}

		if (task == 16)
		{
			Human_l_StartTask_Equip(human);
			return;
		}

		if (task == 14)
		{
			Human_l_StartTask_Attack(human);
			return;
		}

		if (task == 18)
		{
			Human_l_StartTask_OpenChest(human);
			return;
		}

		if (task == 19)
		{
			Human_l_StartTask_BuildGuide(human);
			return;
		}

		if (task == 20)
		{
			Human_l_StartTask_ExploreArea(human);
			return;
		}

		if (task == 21)
		{
			Human_l_StartTask_MoveInsideVehicle(human);
			return;
		}

		if (task == 5)
		{
			Human_l_StartTask_Marry(human);
			return;
		}

		if (task == 11)
		{
			Human_l_StartTask_GetChild(human, 11, 15);
			return;
		}

		if (task == 12)
		{
			Human_l_StartTask_GetChild(human, 12, 16);
			return;
		}

		if (task == 3)
		{
			Human_l_StartTask_GoHome(human);
			return;
		}

		if (task == 17)
		{
			Human_l_StartTask_Equip_DropItem(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood));
			return;
		}

		if (task != 1 && task != 8 && task != 9)
			return;

		if (Human_DoExecuteNextUserCommand(human))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::IdleStartCount, 0);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == (task == 1 ? 0u : 1u) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 1)
			{
				Human_Target_Reset(human);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, task == 9 ? 9 : 8);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			}

			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != task)
			return;

		if (task == 8 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::FoodProductionNeedCheckDeferrals) != 0 && TGoodTypes_IsFood(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType)))
		{
			Write(human, NC2Layout::HumanOffset::FoodProductionNeedCheckDeferrals, Read<std::uint32_t>(human, NC2Layout::HumanOffset::FoodProductionNeedCheckDeferrals) - 1u);
		}
		else
		{
			Human_AI_DoCheckForUrgentNeeds(human);
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 7)
			return;

		if (task == 9)
		{
			Human_l_StartTask_WaitForOtherHuman(human);
			return;
		}

		if (task == 8)
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 0);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanCount) != 0)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 9);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);

			return;
		}

		if (task == 1)
		{
			Human_l_StartTask_Idle(human);
		}
		else
		{
			Human_l_StartTask_ExecuteJob(human);
		}
	}

	// Original function: FUN_004436c5; all 29 entries retain the native preamble.
	void Human_CurrentTask_DoFinish(void* human)
	{
		const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);

		if (task != 1)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount, 0);

			if (!Human_IsDoingGoodie(human))
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::IdleStartCount, 0);
		}

		if (task < 1 || task > 29)
			return;

		if (task == 10)
		{

			return; // Original table entry 00443790 is a no-op after the preamble.
		}

		if (task == 3)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		}
		else if (task == 8)
		{
			Human_l_FinishTask_ExecuteJob(human);
		}
		else if (task == 14 || task == 15)
		{
			Human_l_FinishTask_Attack(human);
		}
		else if (task == 5)
		{
			Human_l_FinishTask_Marry(human);
		}
		// Only task 18 uses 0044621D; guide task 19 finishes through the ordinary idle entry.
		else if (task == 18)
		{
			Human_l_FinishTask_BuildGuide(human);
		}
		else if (task == 20)
		{
			Human_l_FinishTask_ExploreArea(human);
		}
		else if (task == 21)
		{
			Human_l_FinishTask_MoveInsideVehicle(human);
		}
		else if (task == 16 || task == 17)
		{
			Human_l_FinishTask_Equip(human);
		}
		else if (task == 2 || task == 23)
		{
			if (task == 23)
				Human_Target_Reset(human);

			Human_l_FinishTask_Goto(human);
		}
		else if (task == 4)
		{
			Human_l_FinishTask_GotoAndWait(human);
		}
		else if (task == 24)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		}
		else if (task == 7)
		{
			Human_l_FinishTask_FulfillNeeds(human);
		}
		else if (task >= 25 && task <= 29)
		{
			Human_l_FinishTask_Goodie(human);
		}
		else if (task == 9 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanCount) != 0)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		}
		else
		{
			Human_l_FinishTask_Idle(human);
		}
	}

	// Original function: FUN_00445cb2; clear cargo only for the native failure cases.
	void Human_l_FailTask_Idle(void* human)
	{
		const auto failure = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep);

		if (failure != 7 && (failure != 11 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) == 0))
		{

			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
			Human_StopCurrentGoodCirculationMode(human);

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood, 0);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, 0);
	}

	// Original function: FUN_00444a36.
	void Human_l_StartTask_GoAway(void* human, std::uint32_t point, std::uint32_t radius)
	{
		Human_StopCurrentGoodCirculationMode(human);
		const auto found = radius != 0 ? Human_FindNextVehicleFreeMapPoint(human, &point, radius) : Human_FindNextFreeMapPoint(human, &point);

		if (found && Human_l_StartAtomicWalkAndWait(human, &point))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 23);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
			Human_l_StartTask_Idle(human);
		}
	}

	// Original function: FUN_00445DA5; the user message precedes the AI notification.
	void Human_l_FailTask_Goto(void* human)
	{
		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep))
		{
			case SwitchIds::HumanTaskFailure::NoWay:
				Human_UserMessage_SendToUserMessage(human, 3);

				if (Human_GetPlayerType(human) == 2)
					AIManager_Message_NoWayFound(gAIManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId), static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition);

				return;
			case SwitchIds::HumanTaskFailure::CannotFindGood:
				Human_UserMessage_SendToUserMessage_CannotFindGood_Generic(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood));
				return;
			case SwitchIds::HumanTaskFailure::CannotFindBuildGood:
				Human_UserMessage_SendToUserMessage_CannotFindGood_ForBuild(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood));
				return;
			case SwitchIds::HumanTaskFailure::CannotFindHomeGood:
				Human_UserMessage_SendToUserMessage_CannotFindGood_ForHome(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood));
				return;
			case SwitchIds::HumanTaskFailure::NoTargetHuman:
				Human_UserMessage_SendToUserMessage(human, 7);
				return;
			case SwitchIds::HumanTaskFailure::NoWorkPoint:
				Human_UserMessage_SendToUserMessage(human, 8);
				return;
			default:
				Human_UserMessage_SendToUserMessage(human, 2);

				return;
		}
	}

	// Original function: FUN_00445acf; finite-production rotation precedes threshold messages.
	void Human_l_FailTask_ExecuteJob(void* human)
	{
		const auto job = Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount) >= 2 && job >= 8 && job <= 30 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) != 0)
		{
			std::uint32_t productionCount = 0;

			for (std::uint32_t good = 0; good < 66; ++good)
			{
				const auto count = Read<std::int32_t>(human, NC2Layout::HumanOffset::ProductionCounters + good * NC2Layout::RecordSize::HumanJobValueUnsigned);

				if (count > 0 && count <= 11)
					++productionCount;
			}

			if (productionCount >= 2)
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount, 0);
				Human_Production_CouldNotProduceCurrentGood(human);
			}
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount) >= 5)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount, 0);

			switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep))
			{
				case SwitchIds::HumanTaskFailure::NoWork:
					if (!Human_IsAttachedToLearnHouse(human))
						Human_UserMessage_SendToUserMessage(human, 2);

					break;
				case SwitchIds::HumanTaskFailure::NoWay:
					Human_UserMessage_SendToUserMessage(human, 3);
					break;
				case SwitchIds::HumanTaskFailure::WorkHouseUnavailable:
					if (Human_IsJobBuilder(human))
					{
						std::uint32_t real = 0, work = 0;

						if (Human_FindDamagedWallNearWorkCenter(human, &real, &work))
						{
							Write(human, NC2Layout::HumanOffset::WorkCenter, real);
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
							Human_WorkCenterMarker_Update(human);
							Human_Target_SetTargetPositions(human, &real, &work);

							return;
						}
					}

					if (job >= 8 && job <= 30)
					{
						const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId);

						if (id == NXBasics::IntegerRange::UInt32Maximum || Read<std::uint32_t>(CHouseArray::mStaticVars + id * CHouseArray::ElementSize, NC2Layout::HouseOffset::ConstructionState) != 2)
							Human_UserMessage_SendToUserMessage(human, 11);
					}

					break;
				case SwitchIds::HumanTaskFailure::CannotFindGood:
					Human_UserMessage_SendToUserMessage_CannotFindGood_Generic(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood));
					break;
				case SwitchIds::HumanTaskFailure::CannotFindBuildGood:
					Human_UserMessage_SendToUserMessage_CannotFindGood_ForBuild(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood));
					break;
				case SwitchIds::HumanTaskFailure::CannotFindHomeGood:
					Human_UserMessage_SendToUserMessage_CannotFindGood_ForHome(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood));
					break;
				case SwitchIds::HumanTaskFailure::NoTargetHuman:
					Human_UserMessage_SendToUserMessage(human, 7);
					break;
				case SwitchIds::HumanTaskFailure::NoWorkPoint:
					Human_UserMessage_SendToUserMessage(human, 8);
					break;
				case SwitchIds::HumanTaskFailure::NoVehicleBuildPlace:
					Human_UserMessage_SendToUserMessage(human, 9);
					break;
				case SwitchIds::HumanTaskFailure::VehicleBuildPlaceOccupied:
					Human_UserMessage_SendToUserMessage(human, 10);
					break;
				case SwitchIds::HumanTaskFailure::Silent:
					break; // Windows 0045BFEC returns without emitting a message.
				case SwitchIds::HumanTaskFailure::MustWaitForGood:
					Human_UserMessage_SendToUserMessage_MustWaitForGood(human);
					break;
				case SwitchIds::HumanTaskFailure::CannotAffordTraining:
					Human_UserMessage_SendToUserMessage(human, 14);

					if (Human_IsAttachedToLearnHouse(human))
						House_Inhabitants_DetachHuman(CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::ElementSize, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 8);

					break;
				case SwitchIds::HumanTaskFailure::VehicleUnavailable:
					Human_UserMessage_SendToUserMessage(human, 15);
					break;
				case SwitchIds::HumanTaskFailure::TraderHouseUnavailable:
					Human_UserMessage_SendToUserMessage(human, 16);
					break;
				default:
					Human_UserMessage_SendToUserMessage(human, 2);
					break;
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
	}

	// Original function: FUN_00443807; AI pursuit retries and failure notifications retain native order.
	void Human_CurrentTask_DoFailure(void* human)
	{
		const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
			Human_StopCurrentGoodCirculationMode(human);

		Write(human, NC2Layout::HumanOffset::TaskFailureCount, Read<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount) + 1u);
		bool doJob = true, checkUrgent = true;

		if (task == 8)
		{
			doJob = false;
			Human_l_FailTask_ExecuteJob(human);
		}
		else if (task == 14)
		{
			Human_l_FailTask_Attack(human);
		}
		else if (task == 15)
		{
			auto* target = static_cast<std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTarget;

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter) != 0 || (!TargetData_IsValid(target) && !TargetData_IsValid(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::SecondaryMilitaryTarget)))
			{
				Human_l_FailTask_Attack(human);
			}
			else
			{
				const auto type = TargetData_GetTargetType(target);

				if (type == 6)
				{
					const auto& weapon = static_cast<const SStaticWeaponTypeData*>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)])[Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon)];

					if (weapon.maximumRange == 1 && TargetData_MapPos_NextAttackPoint(target))
					{
						std::uint32_t point = 0;
						TargetData_MapPos_GetAttackPosition(target, &point);

						if (Human_l_StartAtomicWalk(human, &point, &point))
						{
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 15);
							Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
							return;
						}
					}
				}
				else if (Human_GetPlayerType(human) == 2)
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);
					std::uint32_t point = 0;

					if (Tool_GetOneOfNearestEnemyLandscapes(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Human_GetMapPositionRef(human), 0, 18, true, 1, 0, &point))
					{
						Write(target, NC2Layout::TargetDataOffset::ObjectIdOrMapPosition, point);
						Write<std::uint32_t>(target, NC2Layout::TargetDataOffset::Type, 6);
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
						return;
					}

					if (TargetData_IsValid(target))
					{
						TargetData_GetMapPosition(target, &point);
						AIManager_Message_NoWayFoundToEnemy(gAIManager, Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId), &point);
					}
				}

				Human_UserMessage_SendToUserMessage(human, 3);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 0);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryMilitaryTarget, 0);
				doJob = false;
			}
		}
		else if (task == 7)
		{
			const auto need = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter);

			if (need >= 2 && need <= 4)
			{
				doJob = false;
			}
			else if (need == 1)
			{
				doJob = TGoodTypes_IsFood(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType));
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::FoodProductionNeedCheckDeferrals, 10);
			}

			checkUrgent = false;
			Human_l_FailTask_FulfillNeeds(human);
		}
		else if (task == 2 || task == 4)
		{
			doJob = false;

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter) != 0)
			{
				Human_l_FailTask_FulfillNeeds(human);
				const auto need = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter);

				if (need != 0)
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 7);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);

					if (need == 1)
					{
						Human_l_StartTask_FulfillNeeds_Food(human);
					}
					else if (need == 2)
					{
						Human_l_StartTask_FulfillNeeds_Stamina(human);
					}
					else if (need == 3)
					{
						Human_l_StartTask_FulfillNeeds_Social_Monologuize(human);
					}
					else if (need == 4)
					{
						Human_l_StartTask_FulfillNeeds_Religion(human);
					}

					return;
				}
			}
			else
			{
				Human_l_FailTask_Goto(human);
			}
		}
		else if (task == 1)
		{
			Human_l_FailTask_Idle(human);
		}
		else if (task == 3)
		{
			Human_l_FailTask_GoHome(human);
		}
		else if (task == 5)
		{
			Human_l_FailTask_Marry(human);
		}
		else if (task == 11)
		{
			Human_l_FailTask_GetBoy(human);
		}
		else if (task == 12)
		{
			Human_l_FailTask_GetGirl(human);
		}
		else if (task == 22)
		{
			Human_l_FailTask_Misc(human);
		}
		else if (task == 16)
		{
			Human_l_FailTask_Equip(human);
		}
		else if (task == 17)
		{
			Human_l_FailTask_Drop(human);
		}
		else if (task == 18)
		{
			Human_Target_Reset(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		}
		else if (task == 19)
		{
			Human_UserMessage_SendToUserMessage(human, 2);
		}
		else if (task == 20)
		{
			Human_l_FailTask_ExploreArea(human);
		}
		else if (task == 21)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		}
		else if (task == 23)
		{
			Human_l_FailTask_Goto(human);
			doJob = false;

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter) != 0)
				Human_l_FailTask_FulfillNeeds(human);
		}
		else if (task >= 24 && task <= 29)
		{
			Human_l_FailTask_Goodie(human);
		}

		if (Human_IsJobSoldier(human))
		{
			if ((task == 2 || task == 4) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep) == 1)
			{
				if (!Human_IsAttachedToWorkHouse(human))
				{
					Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Coordinates));
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
					Human_WorkCenterMarker_Update(human);
				}
				else if (Human_GetPlayerType(human) == 2)
				{
					Human_DoAddNewUserCommand(human, 29, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseRevision));
				}
			}

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter) != 0 && Human_IsRefreshingInWorld(human))
				doJob = false;
		}

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::TaskFailureCount) > 10)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount, 0);
			doJob = false;
		}

		Human_Target_Reset(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 2);

		if (doJob)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 8);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);

			if (checkUrgent)
				Human_AI_DoCheckForUrgentNeeds(human);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 7)
				return;

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 0);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanCount) == 0)
			{
				Human_l_StartTask_ExecuteJob(human);

				return;
			}
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanCount) != 0)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 9);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			Human_l_StartTask_WaitForOtherHuman(human);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			Human_l_StartTask_Idle(human);
		}
	}

	// Original function: FUN_004445c9; nearby valid opponents transfer to task 14.
	void Human_l_StartTask_Idle(void* human)
	{
		if (!Human_IsJobSoldier(human) && !Human_IsJobHero(human) && TargetData_IsValid(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTarget))
		{
			std::uint32_t point = 0;
			TargetData_GetMapPosition(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::PrimaryMilitaryTarget, &point);
			const auto distance = VE_HexagonDirection_GetDistance(Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(&point, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(Position(human), NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Y));

			if (distance < 50)
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 14);
				return;
			} // Game.exe 004F4300.

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget, 0);
		}

		void* moveable = Moveable(human);
		const auto idleCount = Read<std::uint32_t>(human, NC2Layout::HumanOffset::IdleStartCount) + 1u;
		Write(human, NC2Layout::HumanOffset::IdleStartCount, idleCount);

		if ((idleCount == 1u || idleCount % 4u == 0) && Remaster_Human_TryEquipAvailableWeapon(human))
			return;

		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
				Human_StopCurrentGoodCirculationMode(human);

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
			const void* micro = GameMap_GetConstMapMicroPointPtr(Position(human));
			const auto soldierAtWork = Human_IsJobSoldier(human) && Human_IsAttachedToWorkHouse(human) && Human_IsInWorkHouse(human);

			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandActiveFlag) == 0 && !soldierAtWork)
			{
				if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && Read<std::uint8_t>(CHouseArray::mStaticVars + Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId) * CHouseArray::ElementSize, NC2Layout::HouseOffset::DefenceModeFlag) != 0)
				{
					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
					{
						Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
						return;
					}

					Human_l_StartAtomicAnimation(human, 5, -1);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
					return;
				}

				const auto occupant = Read<std::int16_t>(micro, NC2Layout::MapMicroPointOffset::FirstMapMoveableId);

				if (occupant != Read<std::int32_t>(human, NC2Layout::HumanOffset::MapMoveableId) || Read<std::int32_t>(moveable, NC2Layout::MapMoveableOffset::NextCellMoveableId) != -1)
				{
					CMapMoveableIterator iterator;

					for (auto id = iterator.GetFirstMapMoveableId(Position(human)); id != -1; id = iterator.GetNextMapMoveableId())
					{
						if (id == Read<std::int32_t>(human, NC2Layout::HumanOffset::MapMoveableId))
							continue;

						const auto* other = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

						if (Read<std::uint32_t>(other, NC2Layout::MapMoveableOffset::ObjectType) == 1 && Read<std::uint32_t>(CHumanArray::mStaticVars + Read<std::uint32_t>(other, NC2Layout::MapMoveableOffset::ObjectId) * CHumanArray::ElementSize, NC2Layout::HumanOffset::CurrentTask) == 23)
							continue;

						Human_DoAddNewUserCommand(human, 5, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), 0, 0, 0, 0);
						Human_DoExecuteNextUserCommand(human);
						Write<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandActiveFlag, 1);

						if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 1)
							return;
					}
				}
			}

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
			{
				Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
				return;
			}

			if (idleCount > 2u && Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2 && !soldierAtWork)
			{
				Human_l_StartTask_GoAway(human, 0, 0);
				return;
			}
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0 && Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
		{
			auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
			const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

			if (Vehicle_Stock_CanStoreGoodType(vehicle, good) && !Vehicle_Stock_IsFullSoon(vehicle))
			{
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
					Human_StopCurrentGoodCirculationMode(human);

				const auto revision = Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId);

				if (!Vehicle_Stock_ModifyFutureAmount(vehicle, good, 1, revision))
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
					return;
				}

				if (!Vehicle_Stock_ModifyAmount(vehicle, good, 1, revision))
				{
					Vehicle_Stock_ModifyFutureAmount(vehicle, good, -1, revision);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
					return;
				}

				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood, 0);
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, 0);
			}
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood, 0);
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, 0);
		}

		const auto atomic = Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction);

		if ((atomic == 0 || atomic == 1) && idleCount >= 10u && (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 2u) == 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::PrimaryMilitaryTarget) == 0 && !Human_IsAttachedToVehicle(human))
		{
			// Empty eligible queues were checked by the startup dispatcher.
			const auto distance = VE_HexagonDirection_GetDistance(Read<std::int16_t>(Position(human), NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(Position(human), NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(human, NC2Layout::HumanOffset::WorkCenterX), Read<std::int16_t>(human, NC2Layout::HumanOffset::WorkCenterY));

			if (distance >= 5)
			{
				auto point = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter);

				if (Tool_Map_GetNotBlockedPosition(&point, true) && Human_l_StartAtomicWalk(human, &point, &point))
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::IdleStartCount, 0);
					return;
				}
			}
			else if (Human_IsAbleToDoGoodieNow(human))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 24);

				return;
			}
		}

		if (atomic == 0 || atomic == 1)
		{
			std::uint32_t action = 5;

			if (idleCount < 5u)
			{
				action = Game_Random_GetNumber() % 3u + 2u;
			}
			else if (idleCount >= 10u && !Human_IsJobSoldier(human) && !Human_IsJobHero(human))
			{
				action = idleCount < 15u ? 6u : 7u;
			}

			Human_l_StartAtomicAnimation(human, action, -1);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
	}

	// Original function: FUN_00445a6e. The idle counter is deliberately retained.
	void Human_l_FinishTask_Idle(void* human)
	{
		Human_Target_Reset(human);
		Human_Target_ResetSecondaryTarget(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 8);
	}

	// Original function: FUN_0046f76b.
	void Human_l_FinishTask_ExecuteJob(void* human)
	{
		Human_UpdateCurrentJobType(human);
		Human_WorkCenterMarker_Update(human);
		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

		if (job != 7 && job != 8)
			Human_Target_Reset(human);

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
	}

	// Original function: FUN_0046c39e; complete native job selector and common vehicle continuation.
	void Human_l_StartTask_ExecuteJob(void* human)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 0);
		const auto circulation = Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode);

		if (circulation == 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 2u) != 0)
		{
			if (circulation != 0)
				Human_StopCurrentGoodCirculationMode(human);

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
			Human_l_StartTask_Idle(human);
			return;
		}

		if (Human_IsDefendingHouse(human, -1))
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
				Human_StopCurrentGoodCirculationMode(human);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
			{
				Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			}
			else
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
				Human_l_StartTask_Idle(human);
			}

			return;
		}

		if (Human_IsAttachedToLearnHouse(human))
		{
			Human_l_StartTask_ExerciseJob(human);
			return;
		}

		const auto attached = Human_IsAttachedToWorkHouse(human);

		if ((Human_IsForcedToHaveWorkHouse(human) && !attached) || (attached && Read<std::uint8_t>(JobData(human), NC2Layout::StaticJobTypeOffset::MustHaveFinishedWorkHouse) != 0 && Read<std::uint32_t>(CHouseArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) * CHouseArray::ElementSize, NC2Layout::HouseOffset::ConstructionState) != 3))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 2);
			return;
		}

		if (!Human_l_IsAtomicInterruptable(human))
			return;

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == 0 && Read<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandActiveFlag) == 0)
		{
			const auto tool = Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedTools), shoes = Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedShoes);

			if (tool != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::ToolCondition) == 0)
			{
				Human_DoAddNewUserCommand(human, 80, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), 0, 0, tool, NXBasics::IntegerRange::UInt32Maximum);
			}
			else if (shoes != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::ShoeCondition) == 0)
			{
				Human_DoAddNewUserCommand(human, 79, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), 0, 0, shoes, NXBasics::IntegerRange::UInt32Maximum);
			}

			if ((tool != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::ToolCondition) == 0) || (shoes != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::ShoeCondition) == 0))
			{
				Human_DoExecuteNextUserCommand(human);
				Write<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandActiveFlag, 1);
			}

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 8)
				return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) != 0)
		{
			if (!Human_Target_IsValid(human))
			{
				Human_StopCurrentGoodCirculationMode(human);
				Human_Target_Reset(human);
			}
			else if (!Human_IsAtTargetPoint(human))
			{
				const auto target = Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget);

				if (target == 2)
				{
					Write(human, NC2Layout::HumanOffset::RequestedGood, Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetGoodType));

					if (Human_l_StartAtomicWalkAndFetch(human, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetWorkPosition, false))
					{
						Human_IsJobTrader(human);
						return;
					}
				}
				else if (Human_l_StartAtomicWalk(human, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetWorkPosition))
				{
					Write(human, NC2Layout::HumanOffset::InteractionTarget, target);
					return;
				}

				Human_Target_Reset(human);
			}
		}

		if (Human_IsAttachedToVehicle(human))
		{
			const auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
			const auto vehicleJob = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::JobType);

			if (!(Human_IsJobTrader(human) && (vehicleJob == 50 || vehicleJob == 51)) && !Human_IsJobCarrier(human))
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 ? 1u : 21u);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);

				return;
			}
		}

		const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

		switch (job)
		{
			case SwitchIds::HumanJob::Woman:
				Human_l_StartTask_ExecuteJob_Woman(human);
				break;
			case SwitchIds::HumanJob::Builder:
				Human_l_StartTask_ExecuteJob_Builder(human);
				break;
			case SwitchIds::HumanJob::Collector:
				Human_l_StartTask_ExecuteJob_Collector(human);
				break;
			case SwitchIds::HumanJob::Joiner:
			case SwitchIds::HumanJob::Armorer:
				Human_l_StartTask_ExecuteJob_Carpenter(human);
				break;
			case SwitchIds::HumanJob::Hunter:
				Human_l_StartTask_ExecuteJob_Hunter(human);
				break;
			case SwitchIds::HumanJob::Breeder:
				Human_l_StartTask_ExecuteJob_Breeder(human);
				break;
			case SwitchIds::HumanJob::Farmer:
				Human_l_StartTask_ExecuteJob_Farmer(human);
				break;
			case SwitchIds::HumanJob::Fisher:
				Human_l_StartTask_ExecuteJob_Fisher(human);
				break;
			case SwitchIds::HumanJob::Carrier:
				Human_l_StartTask_ExecuteJob_Carrier(human);
				break;
			case SwitchIds::HumanJob::Trader:
				Human_l_StartTask_ExecuteJob_Trader(human);
				break;
			case SwitchIds::HumanJob::Jester:
				Human_l_StartTask_ExecuteJob_Jester(human);
				break;
			case SwitchIds::HumanJob::HerbMushGuy:
				Human_l_StartTask_ExecuteJob_HerbGuy(human);
				break;
			case SwitchIds::HumanJob::Druid:
				Human_l_StartTask_ExecuteJob_Druid(human);
				break;
			case SwitchIds::HumanJob::SoldierUnarmed:
			case SwitchIds::HumanJob::SoldierSpearWooden:
			case SwitchIds::HumanJob::SoldierSpearIron:
			case SwitchIds::HumanJob::SoldierSwordShort:
			case SwitchIds::HumanJob::SoldierSwordLong:
			case SwitchIds::HumanJob::SoldierSaberShort:
			case SwitchIds::HumanJob::SoldierSaberLong:
			case SwitchIds::HumanJob::SoldierAxeSmall:
			case SwitchIds::HumanJob::SoldierAxeBig:
			case SwitchIds::HumanJob::SoldierBowShort:
			case SwitchIds::HumanJob::SoldierBowLong:
			case SwitchIds::HumanJob::HeroUnarmed:
			case SwitchIds::HumanJob::HeroSpearSIEGFRIED:
			case SwitchIds::HumanJob::HeroSwordBJARNI:
			case SwitchIds::HumanJob::HeroSaberHATSCHI:
			case SwitchIds::HumanJob::HeroAxe:
			case SwitchIds::HumanJob::HeroineBowXENA:
				Human_l_StartTask_ExecuteJob_Soldier(human);
				break;
			case SwitchIds::HumanJob::Potter:
			case SwitchIds::HumanJob::Mason:
			case SwitchIds::HumanJob::Smith:
			case SwitchIds::HumanJob::CoinMaker:
			case SwitchIds::HumanJob::Sewer:
			case SwitchIds::HumanJob::Miller:
			case SwitchIds::HumanJob::Baker:
			case SwitchIds::HumanJob::Brewer:
				Human_l_StartTask_ExecuteJob_Tool_GoodProducer(human);
				break;
			case SwitchIds::HumanJob::BabyFemale:
			case SwitchIds::HumanJob::BabyMale:
			case SwitchIds::HumanJob::ChildFemale:
			case SwitchIds::HumanJob::ChildMale:
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::IdleStartCount, 10); // Windows constant 004F3C34.
				break;
			case SwitchIds::HumanJob::Civilist:
			case SwitchIds::HumanJob::FisherSea:
			case SwitchIds::HumanJob::TraderSea:
			case SwitchIds::HumanJob::Scout:
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
				break;
			default:
				C2Debug_WarningBox("human creature is trying to start execution of invalid job type!!!");
				break;
		}

		if (Human_IsAttachedToVehicle(human) && Human_IsJobCarrier(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == 3 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep) == 2)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0 ? 1u : 21u);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		}
	}

	// Original function: FUN_00463346. Performing an atomic precedes checking
	// interruptibility and entering the task machine in the same game tick.
	void Human_AI_DoUpdateAI(void* human)
	{
		Human_CurrentTask_DoPerform(human);

		if (Human_l_IsAtomicInterruptable(human))
			Human_AI_DoUpdateTaskMachine(human);
	}

	// Original function: FUN_00463362; native transitions continue until an active task is reached.
	void Human_AI_DoUpdateTaskMachine(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) != 0 && !Human_Target_IsValid(human))
		{
			const auto target = Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget);

			if (target == 2 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood) == 0)
				Write(human, NC2Layout::HumanOffset::RequestedGood, gStaticDependencyGoodByLandscape[Human_GetLandscapeTypeAtTargetPosition(human)]);

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, target == 1 ? 1 : target == 2              ? 3 : target == 3 || target == 4 ? 7 : target == 7                ? 6 : 0);
		}

		void* moveable = Moveable(human);

		if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0 && ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Position(human)), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 || (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) == 0 && Remaster_House_IsConstructionTrap(Position(human)))))
		{
			Human_CurrentTask_DoInterrupt(human);
			Human_l_StartTask_GoAway(human, 0, 0);
		}

		if ((Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter), NC2Layout::MapMicroPointOffset::WalkingFlags) & NXBasics::BitMask::Bit7) != 0 && !Human_IsAttachedToWallPoint(human))
		{
			auto workCenter = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter);

			if (Tool_Map_GetNotBlockedPosition(&workCenter, false))
			{
				Write(human, NC2Layout::HumanOffset::WorkCenter, workCenter);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
				Human_WorkCenterMarker_Update(human);
			}
		}

		for (;;)
		{
			const auto status = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus);

			if (status == 1)
			{
				Write<std::uint8_t>(human, NC2Layout::HumanOffset::UserCommandActiveFlag, 0);
				Write<std::uint8_t>(human, NC2Layout::HumanOffset::UrgentNeedsCheckedFlag, 0);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 2);

				return;
			}

			if (status == 0)
			{
				Human_CurrentTask_DoStartUp(human);
			}
			else if (status == 2)
			{
				Human_CurrentTask_DoFinish(human);
			}
			else if (status == 3)
			{
				Human_CurrentTask_DoFailure(human);
			}
		}
	}
}