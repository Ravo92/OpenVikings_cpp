#include "C2Debug.h"
#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanAnimalJobs.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanProduction.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2StaticAtomicAnimationData.h"
#include "C2StaticGoodTypeData.h"
#include "C2StaticTribeTypeData.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* House(std::uint32_t id)
		{

			return CHouseArray::mStaticVars + id * CHouseArray::RuntimeElementSize;
		}

		void* WorkHouse(const void* human)
		{

			return House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId));
		}

		void* Real(void* human)
		{

			return static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition;
		}

		void* Work(void* human)
		{

			return static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetWorkPosition;
		}

		void Fail(void* human, std::uint32_t code)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write(human, NC2Layout::HumanOffset::CurrentTaskStep, code);
		}

		void Start(void* human, std::uint32_t animation)
		{
			Human_l_StartAtomicAnimation(human, animation, 6);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		}

		bool InputsAvailable(void* house, const SStaticGoodTypeData& good)
		{
			for (std::uint32_t index = 0; index < good.distinctProductionInputCount; ++index)
			{
				if (House_Stock_RequestAmountOfGoods(house, static_cast<std::uint32_t>(good.distinctProductionInputs[index]), false) < good.productionInputAmounts[index])
					return false;
			}

			return true;
		}

		bool WalkToConsumer(void* human)
		{
			std::uint32_t real = 0, work = 0;

			return Human_FindConsumerNearWorkCenter(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood), &real, &work) && Human_l_StartAtomicWalkAndBring(human, &real, &work);
		}

		void FinishTraining(void* human)
		{
			if (Human_IsAttachedToLearnHouse(human) && !House_Inhabitants_DetachHuman(WorkHouse(human), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 8))
			{
				std::byte command[20]{ };
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 33);
				Human_DoAddNewUserCommand(human, command);
			}

			Human_DoAddNewUserCommand(human, 5, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), 0, 0, 0, 0);
		}
	}

	// Original function: FUN_00473b06; learning-house startup precedes ordinary job checks.
	void Human_l_StartTask_ExerciseJob(void* human)
	{
		auto* house = WorkHouse(human);

		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3)
		{
			Fail(human, 0);
			return;
		}

		if (Human_IsAtTargetPoint(human))
		{
			if (Read<std::uint32_t>(Real(human), NC2Layout::MapCoordinatePairOffset::Coordinates) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition))
			{
				Human_Target_Reset(human);
				return;
			}

			if (Read<std::uint32_t>(Work(human), NC2Layout::MapCoordinatePairOffset::Coordinates) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint))
				C2Debug_WarningBox("standing at invalid position for interaction with house stock!");

			const auto animation = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == 39 && Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlagsSigned + SwitchIds::HumanJob::SoldierUnarmed) != 0 ? 90u : 89u;
			Human_l_StartAtomicAnimation(human, animation, -1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
			House_ActionOverlay_Start(house, 3, static_cast<std::uint32_t>(gStaticAtomicAnimationStorage.records[Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationId)].length));
			House_ActionOverlay_Start(house, 4, static_cast<std::uint32_t>(gStaticAtomicAnimationStorage.records[Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationId)].length));
			return;
		}

		const auto real = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition), work = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint);

		if (Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) != work)
		{
			if (!Human_l_StartAtomicWalk(human, &real, &work))
			{
				Fail(human, 1);

				return;
			}
		}
		else if (!Human_Misc_IsHidden(human))
		{
			Human_HouseMisc_Enter(human);
		}

		Human_Target_SetTargetPositions(human, &real, &work);
	}

	// Original function: FUN_00473c9b; test the old training amount before adding this event.
	void Human_l_JobTraining_Train(void* human, std::uint32_t amount)
	{
		auto* house = WorkHouse(human);
		std::uint32_t job = 0, good = 0;

		switch (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType))
		{
			case SwitchIds::TrainingHouseType::School:
			{
				const auto* slot = House_Inhabitants_GetAttachedHumanSlotPtrByHumanId(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
				job = Read<std::uint32_t>(slot, NC2Layout::HouseAttachmentOffset::TrainingJobType);
				good = Read<std::uint32_t>(slot, NC2Layout::HouseAttachmentOffset::TrainingGoodType);
				break;
			}
			case SwitchIds::TrainingHouseType::Barracks:
			{
				auto weapon = Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedWeapon);

				if (weapon == 19)
					weapon = 1;

				job = Read<std::uint32_t>(gStaticWeaponTypeArrays[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)], weapon * NC2Layout::RecordSize::StaticWeaponTypeUnsigned + NC2Layout::StaticWeaponTypeOffset::JobTypeUnsigned);
				break;
			}
			default:
				return;
		}

		if (job == 0)
			return;

		const auto& tribe = gStaticTribeTypeData[Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType)];
		const auto required = tribe.jobTraining[job] + (good == 0 ? 0u : tribe.goodTraining[good]);
		const auto previous = Read<std::uint32_t>(human, NC2Layout::HumanOffset::EducationLevel);

		if (required > previous)
		{
			Write(human, NC2Layout::HumanOffset::EducationLevel, previous > 10000u - amount ? 10000u : previous + amount);

			return;
		}

		if (!Human_IsAbleToDoJob(human, job))
		{
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedJobFlags + job, 1);
			Human_UserMessage_SendToUserMessage_CanDoNewJob(human, job);
		}

		if (good != 0 && Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good) == 0)
		{
			Write<std::uint8_t>(human, NC2Layout::HumanOffset::AllowedProductionGoodFlags + good, 1);
			Human_UserMessage_SendToUserMessage_CanProduceNewGood(human, good);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::EducationLevel, 0);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) != job)
		{
			Human_SetJobType(human, job);
		}
		else
		{
			Human_SetupCurrentJobType(human);
			Human_Production_Setup(human);
		}

		Human_SetupPlayerSpecificStuff(human);

		if (good != 0 && Human_IsAbleToProduceGoodNow(human, good))
			Human_ChangeProducedGoodType(human, good);

		FinishTraining(human);
	}

	// Original function: FUN_00473e4c; exercise detaches after every event, independently of unlocks.
	void Human_l_JobTraining_Exercise(void* human, std::uint32_t amount)
	{
		Human_GainExperienceForEquippedWeapon(human, amount);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::EducationLevel, 0);
		FinishTraining(human);
	}

	// Original function: FUN_0046c7be; a woman's household work requires a completed home.
	void Human_l_StartTask_ExecuteJob_Woman(void* human)
	{
		if (!Human_IsAttachedToHomeHouse(human) || Read<std::uint32_t>(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), NC2Layout::HouseOffset::ConstructionState) != 3)
		{
			Fail(human, 2);

			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) == 0)
		{
			Human_l_StartTask_ExecuteJob_Tool_FillHome_TryToFetch(human);
		}
		else if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_ExecuteJob_Tool_FillHome_BringAway(human);
		}
		else
		{
			Human_l_StartTask_ExecuteJob_Tool_FillHome_Fetch(human);
		}
	}

	// Original function: FUN_0047553c; household deliveries have their own stock semantics.
	void Human_l_StartTask_ExecuteJob_Tool_FillHome_BringAway(void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

		if (Human_IsAtTargetPoint(human))
		{
			auto* house = Human_GetHousePtrAtTargetPosition(human);

			if (house != nullptr)
			{
				if (House_Stock_CanStoreGoodType(house, good, false) && !House_Stock_IsFull(house, good))
				{
					Start(human, 23);
				}
				else
				{
					Human_Target_Reset(human);
				}

				return;
			}

			if (WalkToConsumer(human))
				return;

			if (Map_GoodTool_CanBePutOnMap(good, Real(human)))
			{
				Start(human, 23);
				return;
			}
		}
		else
		{
			auto* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId));

			if (House_Stock_CanStoreGoodType(house, good, false) && House_Stock_HasDemandForGood(house, good) && Human_l_StartAtomicWalk(human, static_cast<std::byte*>(house) + NC2Layout::HouseOffset::MapPosition, static_cast<std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint))
				return;

			if (WalkToConsumer(human))
				return;
		}

		std::uint32_t real = 0, work = 0;

		if (Human_FindPilePointNearby(human, &real, &work) && Human_l_StartAtomicWalk(human, &real, &work))
		{

			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_004756eb; invalid collection destinations release reservations.
	void Human_l_StartTask_ExecuteJob_Tool_FillHome_Fetch(void* human)
	{
		if (Human_IsAtTargetPoint(human))
		{
			if (Human_l_StartTask_Tool_PickupWantedGood_FromTargetPosition(human))
			{

				return;
			}

			Human_Target_Reset(human);
			Human_StopCurrentGoodCirculationMode(human);
		}
		else
		{
			Human_StopCurrentGoodCirculationMode(human);
			Human_l_StartTask_ExecuteJob_Tool_FillHome_TryToFetch(human);
		}
	}

	// Original function: FUN_00475720; keep the search's failure message code when walking fails.
	void Human_l_StartTask_ExecuteJob_Tool_FillHome_TryToFetch(void* human)
	{
		Human_Target_Reset(human);
		Fail(human, 2);

		if (!Human_IsAttachedToHomeHouse(human))
			return;

		std::uint32_t real = 0, work = 0, good = 0;

		if (Human_FindGoodsDemandedForHomeHouse(human, &real, &work, &good, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter))
		{
			Write(human, NC2Layout::HumanOffset::RequestedGood, good);

			if (Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
			{

				return;
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_00473ec6; shared indoor producer entry point.
	void Human_l_StartTask_ExecuteJob_Tool_GoodProducer(void* human)
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

		switch (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode))
		{
			case SwitchIds::GoodCirculationMode::FillStockFetch:
				Human_l_StartTask_ExecuteJob_Tool_FillStock_Fetch(human);
				return;
			case SwitchIds::GoodCirculationMode::FillStockBringAway:
				Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(human);
				return;
			case SwitchIds::GoodCirculationMode::FlushStock:
				Human_l_StartTask_ExecuteJob_Tool_FlushStock_Fetch(human);
				return;
			case SwitchIds::GoodCirculationMode::None:
				break;
			default:
				return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Forget(human);
			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProductionCounters + Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) * NC2Layout::RecordSize::HumanJobValueUnsigned) == 0)
		{
			Human_Production_Rotate(human);
			return;
		}

		auto* house = WorkHouse(human);
		const auto* job = static_cast<const std::byte*>(gStaticJobTypeData) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType;

		if ((Read<std::uint8_t>(job, NC2Layout::StaticJobTypeOffset::MustHaveFinishedWorkHouse) != 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3) || Read<std::uint8_t>(house, NC2Layout::HouseOffset::ProductionStoppedFlag) != 0 || !House_Stock_CanStoreGoodType(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType), true))
		{
			Fail(human, 2);

			return;
		}

		if (House_Stock_IsFull(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType)))
		{
			Write(human, NC2Layout::HumanOffset::RequestedGood, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 5);
			Human_l_StartTask_ExecuteJob_Tool_FlushStock_Fetch(human);
		}
		else
		{
			Human_l_StartTask_ExecuteJob_Tool_TryToProduce(human);
		}
	}

	// Original function: FUN_00474f17; returns cargo to construction, stock or map piles.
	void Human_l_StartTask_ExecuteJob_Tool_FillStock_BringAway(void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

		if (Human_IsAtTargetPoint(human))
		{
			const auto id = Logic_EntryPoint_House_GetOnPosition(Real(human));

			if (id != -1)
			{
				auto* house = House(static_cast<std::uint32_t>(id));

				if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3)
				{
					if ((House_Stock_CanStoreGoodType(house, good, false) && House_Stock_HasDemandForGood(house, good)) || (static_cast<std::uint32_t>(id) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) && House_Stock_CanStoreGoodType(house, good, true) && House_Stock_HasDemandForGood(house, good)))
					{
						Start(human, 23);
						return;
					}
				}

				if (House_Construction_IsNeedingGood(house, good))
				{
					Start(human, 23);
				}
				else
				{
					Human_StopCurrentGoodCirculationMode(human);
				}

				return;
			}

			if (good == 3 && Human_IsAttachedToRoadPoint(human))
			{
				Start(human, 41);
				return;
			}

			if (good == 5 && Human_IsAttachedToWallPoint(human))
			{
				Start(human, 42);
				return;
			}

			if (Human_IsAttachedToWorkHouse(human) && House_GetHouseMainType(WorkHouse(human)) != 1 && WalkToConsumer(human))
				return;

			if (Map_GoodTool_CanBePutOnMap(good, Real(human)))
			{
				Start(human, 23);
				return;
			}

			std::uint32_t real = 0, work = 0;

			if (Human_FindPilePointNearby(human, &real, &work) && Human_l_StartAtomicWalk(human, &real, &work))
				return;

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			return;
		}

		if (Human_IsAttachedToWorkHouse(human))
		{
			auto* house = Human_IsProducingVehicle(human) ? House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::VehicleConstructionHouseId)) : WorkHouse(human);

			if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3)
			{
				if ((House_Stock_CanStoreGoodType(house, good, false) || House_Stock_CanStoreGoodType(house, good, true)) && House_Stock_HasDemandForGood(house, good) && !House_Stock_IsFullSoon(house, good) && Human_l_StartAtomicWalk(human, static_cast<std::byte*>(house) + 0x1C, static_cast<std::byte*>(house) + 0x24))
					return;
			}
			else if (House_Construction_IsNeedingGood(house, good))
			{
				const auto* position = Human_GetMapPositionRef(human);
				const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(position), NC2Layout::MapMicroPointOffset::ContinentId);
				std::uint32_t work = 0;

				if (!(House_Construction_GetNextWorkPoint(house, position, &work, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), continent) || House_Construction_GetNextWorkPoint(house, position, &work, false, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), continent)) || !Human_l_StartAtomicWalk(human, static_cast<std::byte*>(house) + 0x1C, &work))
					Fail(human, 7);

				return;
			}

			Human_StopCurrentGoodCirculationMode(human);

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 5)
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 4);

			if (House_GetHouseMainType(house) != 1 && WalkToConsumer(human))
				return;
		}
		else if (good == 3 || good == 5)
		{
			const auto action = Human_IsAttachedToRoadPoint(human) ? 41u : Human_IsAttachedToWallPoint(human) ? 42u
			                                                                                                  : 0u;

			if (action != 0)
			{
				auto real = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkCenter), work = real;

				if (action == 42 && !GameMap_Misc_GetLandscapeWorkPosition(&real, &work))
				{
					Fail(human, 7);
					return;
				}

				Human_Target_SetTargetPositions(human, &real, &work);

				if (Human_IsAtTargetPoint(human))
				{
					Start(human, action);
				}
				else if (!Human_l_StartAtomicWalk(human, &real, &work))
				{
					Fail(human, 1);
				}

				return;
			}
		}

		std::uint32_t real = 0, work = 0;

		if (Human_FindPilePointNearWorkCenter(human, &real, &work) && Human_l_StartAtomicWalk(human, &real, &work))
			return;

		real = work = 0;

		if (Human_FindPilePointNearby(human, &real, &work) && Human_l_StartAtomicWalk(human, &real, &work))
		{

			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_004753cb; preserve Windows' >= test in the unreached branch.
	void Human_l_StartTask_ExecuteJob_Tool_FillStock_Fetch(void* human)
	{
		if (Human_IsAtTargetPoint(human))
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood) == 0)
			{
				Human_l_StartTask_ExecuteJob_Tool_CollectForProduction(human);
				return;
			}

			auto* house = Human_GetHousePtrAtTargetPosition(human);

			if (house != nullptr)
			{
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetWorkPosition) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint))
					C2Debug_WarningBox("standing at invalid position for interaction with house stock!");

				if (House_Stock_CanStoreGoodType(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), true) && House_Stock_RequestAmountOfGoods(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), true) != 0)
				{
					const auto type = Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType);
					Start(human, type == 10 ? 44 : type == 11 ? 45 : 22);
					return;
				}
			}
			else if (Human_GetLandscapeTypeAtTargetPosition(human) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetLandscapeType))
			{
				Start(human, 22);
				return;
			}

			Human_Target_Reset(human);
			Human_StopCurrentGoodCirculationMode(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
			return;
		}

		const auto& good = gStaticGoodTypeStorage.records[Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType)];
		auto* house = WorkHouse(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);

		for (std::uint32_t index = 0; index < good.distinctProductionInputCount; ++index)
		{
			Write(human, NC2Layout::HumanOffset::RequestedGood, static_cast<std::uint32_t>(good.distinctProductionInputs[index]));

			if (House_Stock_RequestAmountOfGoods(house, static_cast<std::uint32_t>(good.distinctProductionInputs[index]), false) >= good.productionInputAmounts[index] && Human_l_StartTask_ExecuteJob_Tool_CollectWantedGood(human))
			{

				return;
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_00475a3a; cargo-bearing flushing delegates to the bring-away leaf.
	void Human_l_StartTask_ExecuteJob_Tool_FlushStock_Fetch(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_ExecuteJob_Tool_FlushStock_BringAway(human);
			return;
		}

		auto* house = WorkHouse(human);

		if (Human_IsAtTargetPoint(human))
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetPosition) == Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition) && House_Stock_IsFull(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood)))
			{
				Start(human, 22);
				return;
			}

			Human_Target_Reset(human);
			Human_StopCurrentGoodCirculationMode(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
			return;
		}

		if (!Human_FindConsumerNearWorkCenter(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood)))
		{
			Fail(human, 11);
			return;
		}

		Human_Target_SetTargetPositions(human, static_cast<std::byte*>(house) + 0x1C, static_cast<std::byte*>(house) + 0x24);

		if (Human_IsAtTargetPoint(human))
		{
			if (House_Stock_IsFull(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood)))
			{
				Start(human, 22);
				return;
			}

			Human_StopCurrentGoodCirculationMode(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		}
		else if (Human_l_StartAtomicWalk(human, Real(human), Work(human)))
		{

			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_00475a4d; Windows uses ordinary walking for map-pile fallback.
	void Human_l_StartTask_ExecuteJob_Tool_FlushStock_BringAway(void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

		if (Human_IsAtTargetPoint(human))
		{
			const auto id = Logic_EntryPoint_House_GetOnPosition(Real(human));

			if (id != -1)
			{
				auto* house = House(static_cast<std::uint32_t>(id));

				if (House_Stock_CanStoreGoodType(house, good, false) && House_Stock_HasDemandForGood(house, good))
				{
					Start(human, 23);
				}
				else
				{
					Human_Target_Reset(human);
				}

				return;
			}

			if (WalkToConsumer(human))
				return;

			if (Map_GoodTool_CanBePutOnMap(good, Real(human)))
			{
				Start(human, 23);
				return;
			}
		}
		else if (Human_IsAttachedToWorkHouse(human) && House_GetHouseMainType(WorkHouse(human)) != 1 && WalkToConsumer(human))
		{
			return;
		}

		std::uint32_t real = 0, work = 0;

		if (Human_FindPilePointNearby(human, &real, &work) && Human_l_StartAtomicWalk(human, &real, &work))
		{

			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_00475d34.
	void Human_l_StartTask_ExecuteJob_Tool_TryToProduce(void* human)
	{
		if (InputsAvailable(WorkHouse(human), gStaticGoodTypeStorage.records[Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType)]))
		{
			Human_l_StartTask_ExecuteJob_Tool_Produce(human);
		}
		else
		{
			Human_l_StartTask_ExecuteJob_Tool_CollectForProduction(human);
		}
	}

	// Original function: FUN_00475da4; try deficient input types in their original order.
	void Human_l_StartTask_ExecuteJob_Tool_CollectForProduction(void* human)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		Human_Target_Reset(human);
		const auto& good = gStaticGoodTypeStorage.records[Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType)];
		auto* house = WorkHouse(human);

		for (std::uint32_t index = 0; index < good.distinctProductionInputCount; ++index)
		{
			if (House_Stock_RequestAmountOfGoods(house, static_cast<std::uint32_t>(good.distinctProductionInputs[index]), false) >= good.productionInputAmounts[index])
				continue;

			Write(human, NC2Layout::HumanOffset::RequestedGood, static_cast<std::uint32_t>(good.distinctProductionInputs[index]));

			if (Human_l_StartTask_ExecuteJob_Tool_CollectWantedGood(human))
			{

				return;
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_00475e31.
	bool Human_l_StartTask_ExecuteJob_Tool_CollectWantedGood(void* human)
	{
		std::uint32_t real = 0, work = 0;

		if (Human_FindGood_Complex(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood), &real, &work, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::WorkCenter) && Human_l_StartAtomicWalkAndFetch(human, &real, &work, false))
			return true;

		Fail(human, 3);

		return false;
	}

	// Original function: FUN_00475e9a; verify interaction position and inputs again at arrival.
	void Human_l_StartTask_ExecuteJob_Tool_Produce(void* human)
	{
		for (;;)
		{
			const auto& good = gStaticGoodTypeStorage.records[Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType)];
			auto* house = WorkHouse(human);

			if (Human_IsAtTargetPoint(human))
			{
				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetPosition) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition))
				{
					Human_Target_Reset(human);
					continue;
				}

				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetWorkPosition) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint))
					C2Debug_WarningBox("standing at invalid position for interaction with house stock!");

				if (!InputsAvailable(house, good))
				{
					Fail(human, 10);
					Human_Target_Reset(human);
					return;
				}

				Human_l_StartAtomicAnimation(human, static_cast<std::uint32_t>(good.atomicForProduction), -1);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
				House_ActionOverlay_Start(house, 3, static_cast<std::uint32_t>(gStaticAtomicAnimationStorage.records[Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationId)].length));
				House_ActionOverlay_Start(house, 4, static_cast<std::uint32_t>(gStaticAtomicAnimationStorage.records[Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAnimationId)].length));
				return;
			}

			const auto real = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition), work = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint);

			if (Human_l_StartAtomicWalk(human, &real, &work))
			{
				Human_Target_SetTargetPositions(human, &real, &work);
			}
			else
			{
				Fail(human, 0);
			}

			return;
		}
	}

	// Original function: FUN_00469401; decimal amounts, tool wear and callbacks retain native order.
	void Human_l_PerformTaskEvent_Produce_Good(void* human)
	{
		auto* house = WorkHouse(human);
		const auto decimal = Human_l_JobEfficiency_CalculateOutputAmountDecimal(human);
		const auto amount = decimal / 10, fraction = decimal % 10;
		House_Stock_ModifyAmount(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType), amount, fraction, false);

		if (Human_HasEquippedTools(human))
			Human_ChangeToolCondition(human, -100);

		if (TGoodTypes_IsFood(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType)))
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::FoodProductionNeedCheckDeferrals, 0);

		Human_l_JobExperience_Gain(human);
		House_Production_DidProduceOneGood(house);
		Human_Production_DidProduceGoods(human, amount, fraction);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
	}

	// Original function: FUN_004691ce; retain input consumption before animal allocation.
	void Human_l_PerformTaskEvent_Produce(void* human)
	{
		const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (id == 0)
		{
			Fail(human, 2);

			return;
		}

		if (Human_IsAttachedToWorkHouse(human))
		{
			auto* house = WorkHouse(human);
			const auto& good = gStaticGoodTypeStorage.records[id];
			const bool inputsAvailable = InputsAvailable(house, good);
			const bool stockFullSoon = House_Stock_IsFullSoon(house, id);

			if (inputsAvailable && !stockFullSoon)
			{
				for (std::uint32_t index = 0; index < good.productionInputCount; ++index)
				{
					House_Stock_ModifyAmount(house, static_cast<std::uint32_t>(good.productionInputs[index]), -1, 0, false);
				}

				if (id == 57 || id == 58)
				{
					Human_l_PerformTaskEvent_Produce_Animal(human);
				}
				else
				{
					Human_l_PerformTaskEvent_Produce_Good(human);
				}
			}
			else
			{
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			}
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		}

		Human_Target_Reset(human);
	}
}