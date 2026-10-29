#include "C2Debug.h"
#include "C2GameControlStartup.h"
#include "C2HouseAccess.h"
#include "C2HumanAtomicEvents.h"
#include "C2HumanChest.h"
#include "C2HumanCollectionEvents.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanFamily.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanProduction.h"
#include "C2HumanScout.h"
#include "C2HumanWeaponEvents.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2StaticAtomicAnimationData.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XDiagnostics.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		// Windows 00468961..004689D1: event selectors, CHuman fields and accepted task states.
		constexpr std::int32_t WorkingExperienceEvent = 28;
		constexpr std::int32_t JobTrainingEvent = 29;
		constexpr std::uint32_t HumanTaskStatusOffset = 0x36C;
		constexpr std::uint32_t HumanAtomicActionOffset = 0xAB8;
		constexpr std::uint32_t HumanFlagsOffset = 0xB84;
		constexpr std::uint32_t ExperienceGainBlockedFlag = 0x800;
		constexpr std::uint32_t TaskStatusInProgress = 1;
		constexpr std::uint32_t TaskStatusCompleted = 2;
		constexpr std::uint32_t JobTrainingAction = 89;
		constexpr std::uint32_t WeaponExerciseAction = 90;

		void* House(std::uint32_t id)
		{

			return CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
		}

		void* TargetHuman(const void* human)
		{

			return CHumanArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectId) * CHumanArray::ElementSize;
		}

		void AddAttribute(void* human, std::uint32_t offset, std::uint32_t amount)
		{
			Write(human, offset, Read<std::uint32_t>(human, offset) + amount);
		}

	}

	// Original function: FUN_0042c93f.
	bool Human_IsJobIgnoringHomeHouse(const void* human)
	{

		return Read<std::uint8_t>(gStaticJobTypeData, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::IgnoresHomeHouseUnsigned) != 0;
	}

	// Original function: FUN_0042cd5d; PE bytes recover the misaligned export.
	bool Human_IsInHomeHouse(const void* human)
	{
		const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId);

		return id != NXBasics::IntegerRange::UInt32Maximum && Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(House(id), NC2Layout::HouseOffset::DoorEntryPoint);
	}

	// Original function: FUN_0042cd98.
	bool Human_IsInWorkHouse(const void* human)
	{
		const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId);

		return id != NXBasics::IntegerRange::UInt32Maximum && Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(House(id), NC2Layout::HouseOffset::DoorEntryPoint);
	}

	// Original function: FUN_0045cca7.
	void Human_l_ResetAtomic(void* human)
	{
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction, 0);
		Write<std::uint8_t>(human, NC2Layout::HumanOffset::AtomicAnimationRunningFlag, 0);
	}

	// Original function: FUN_00468b68; Windows invokes callback 0x17.
	void Human_l_PerformTaskEvent_PlaySoundFX(void* human, std::uint32_t sound)
	{
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x17, true, sound, MapMigPoint_GetLong(Human_GetMapPositionRef(human)), 0);
	}

	// Original function: FUN_00468678; complete selector table 00468AD8..
	// 00468B67. Building events use the complete native context dispatcher.
	void Human_l_PerformTaskEvent(void* human, const SStaticAtomicAnimationEventData& event)
	{
		const auto amount = static_cast<std::uint32_t>(event.parameter);

		switch (event.type)
		{
			case SwitchIds::AtomicAnimationEvent::ModifyStamina:
				if (!Human_AI_IsIgnoringNeeds(human))
				{
					auto delta = amount;

					if (Human_IsInHomeHouse(human))
					{
						if (House_HomeQualityEnhancer_ModifyValue(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), 0, -1))
							delta *= 2u;
					}
					else if (!Human_IsJobIgnoringHomeHouse(human))
					{
						delta = static_cast<std::uint32_t>(event.parameter / 2);
					}

					AddAttribute(human, NC2Layout::HumanOffset::Stamina, delta);
				}

				return;
			case SwitchIds::AtomicAnimationEvent::ModifyFood:
				if (!Human_AI_IsIgnoringNeeds(human))
					AddAttribute(human, NC2Layout::HumanOffset::Food, Human_IsInHomeHouse(human) ? amount * 2u : amount);

				return;
			case SwitchIds::AtomicAnimationEvent::ModifySocial:
			{
				const auto action = Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction);

				if ((action == 14 || action == 15) && !Human_Target_IsValid(human) && Human_l_IsAtomicInterruptable(human))
				{
					if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 7)
					{
						void* other = TargetHuman(human);

						if (Read<std::uint8_t>(other, NC2Layout::HumanOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(other, NC2Layout::HumanOffset::UniqueId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectRevision))
							Human_StopWaitingForOtherHuman(other);
					}

					Human_CurrentTask_DoInterrupt(human);
					Human_l_ResetAtomic(human);
					return;
				}

				if (!Human_AI_IsIgnoringNeeds(human))
					AddAttribute(human, NC2Layout::HumanOffset::Social, amount);

				return;
			}
			case SwitchIds::AtomicAnimationEvent::ModifyReligion:
				if (!Human_AI_IsIgnoringNeeds(human))
					AddAttribute(human, NC2Layout::HumanOffset::Religion, amount);

				return;
			case SwitchIds::AtomicAnimationEvent::ModifyHitpoints:
				if (!Human_AI_IsIgnoringNeeds(human) && (event.parameter > 0 || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 8u) == 0))
					AddAttribute(human, NC2Layout::HumanOffset::Hitpoints, amount);

				return;
			case SwitchIds::AtomicAnimationEvent::StopGoodCirculationUnsupported:
			case SwitchIds::AtomicAnimationEvent::StopGoodCirculationUnsupportedAlias:
				NXBasics::ReportWarning("l_PerformTaskEvent_SendHitpoints: not implemented yet!");
				Human_StopCurrentGoodCirculationMode(human);
				return;
			case SwitchIds::AtomicAnimationEvent::SendSocial:
				Human_l_PerformTaskEvent_SendSocial(human, event.parameter);
				return;
			case SwitchIds::AtomicAnimationEvent::SendReligion:
				Human_l_PerformTaskEvent_SendReligion(human, event.parameter);
				return;
			case SwitchIds::AtomicAnimationEvent::SendHitpoints:
				Human_l_PerformTaskEvent_SendHitpoints(human, event.parameter);
				return;
			case SwitchIds::AtomicAnimationEvent::Pickup:
				Human_l_PerformTaskEvent_Pickup(human);
				return;
			case SwitchIds::AtomicAnimationEvent::Pileup:
				Human_l_PerformTaskEvent_Pileup(human);
				return;
			case SwitchIds::AtomicAnimationEvent::SplitUp:
				Human_l_PerformTaskEvent_SplitUp(human);
				return;
			case SwitchIds::AtomicAnimationEvent::Transform:
				Human_l_PerformTaskEvent_Transform(human);
				return;
			case SwitchIds::AtomicAnimationEvent::Plant:
				Human_l_PerformTaskEvent_Plant(human);
				return;
			case SwitchIds::AtomicAnimationEvent::Grow:
				Human_l_PerformTaskEvent_Grow(human);
				return;
			case SwitchIds::AtomicAnimationEvent::CatchFish:
				Human_l_PerformTaskEvent_CatchFish(human);
				return;
			case SwitchIds::AtomicAnimationEvent::Produce:
				Human_l_PerformTaskEvent_Produce(human);
				return;
			case SwitchIds::AtomicAnimationEvent::Build:
				Human_l_PerformTaskEvent_Build(human);
				return;
			case SwitchIds::AtomicAnimationEvent::GiveBirth:
				Human_l_PerformTaskEvent_GiveBirth(human);
				return;
			case SwitchIds::AtomicAnimationEvent::Attack:
				Human_l_PerformTaskEvent_Attack(human);
				return;
			case SwitchIds::AtomicAnimationEvent::ModifyWorkHouseStock:
			{
				if (Human_IsAttachedToWorkHouse(human))
				{
					void* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId));
					const auto output = Human_l_JobEfficiency_CalculateOutputAmountDecimal(human);

					if (House_Stock_ModifyAmount(house, amount, output / 10, output % 10, false))
						Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
				}
				else
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 2);
				}

				return;
			}
			case WorkingExperienceEvent:
				// Remaster: school training grants its selected skill, not unrelated current-job experience.

				if (Read<std::uint32_t>(human, HumanAtomicActionOffset) == JobTrainingAction)
					return;

				if ((Read<std::uint32_t>(human, HumanFlagsOffset) & ExperienceGainBlockedFlag) == 0 && (Read<std::uint32_t>(human, HumanTaskStatusOffset) == TaskStatusInProgress || Read<std::uint32_t>(human, HumanTaskStatusOffset) == TaskStatusCompleted))
					Human_l_JobExperience_Gain(human);

				return;
			case JobTrainingEvent:
				if (Read<std::uint32_t>(human, HumanTaskStatusOffset) == TaskStatusInProgress || Read<std::uint32_t>(human, HumanTaskStatusOffset) == TaskStatusCompleted)
				{
					if (Read<std::uint32_t>(human, HumanAtomicActionOffset) == JobTrainingAction)
					{
						Human_l_JobTraining_Train(human, amount);
					}
					else if (Read<std::uint32_t>(human, HumanAtomicActionOffset) == WeaponExerciseAction)
					{
						Human_l_JobTraining_Exercise(human, amount);
					}
				}

				return;
			case SwitchIds::AtomicAnimationEvent::ChangeGold:
				Human_l_PerformTaskEvent_ChangeGold(human, event.parameter);
				return;
			case SwitchIds::AtomicAnimationEvent::ChangeWealth:
				Human_l_PerformTaskEvent_ChangeWealth(human, event.parameter);
				return;
			case SwitchIds::AtomicAnimationEvent::DidProduceOneGood:
				Human_l_PerformTaskEvent_DidProduceOneGood(human);
				return;
			case SwitchIds::AtomicAnimationEvent::OpenChest:
				Human_l_PerformTaskEvent_OpenChest(human);
				return;
			case SwitchIds::AtomicAnimationEvent::Merge:
				C2Debug_WarningBox("cAtomicAnimationEventTypeMerge not handled yet!");
				return;
			case SwitchIds::AtomicAnimationEvent::Shrink:
				C2Debug_WarningBox("cAtomicAnimationEventTypeShrink not handled yet!");
				return;
			case SwitchIds::AtomicAnimationEvent::LeaveHouse:
				Human_HouseMisc_Leave(human);
				return;
			case SwitchIds::AtomicAnimationEvent::EnterHouse:
				Human_HouseMisc_Enter(human);
				return;
			case SwitchIds::AtomicAnimationEvent::PlaySoundFX:
				Human_l_PerformTaskEvent_PlaySoundFX(human, amount);
				return;
			case SwitchIds::AtomicAnimationEvent::ChangeMiscPalette:
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x22, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), amount, 0);
				return;
			case SwitchIds::AtomicAnimationEvent::HouseParticleEffect:
			{
				std::uint32_t position;

				if (Human_IsInHomeHouse(human))
				{
					position = Read<std::uint32_t>(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), NC2Layout::HouseOffset::MapPosition);
				}
				else if (Human_IsInWorkHouse(human))
				{
					position = Read<std::uint32_t>(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)), NC2Layout::HouseOffset::MapPosition);
				}
				else
				{
					return;
				}

				MapMigPoint_MoveCursorUnwrapped(position, 1);
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x25, true, amount, MapMigPoint_GetLong(&position), 0);
				return;
			}
			default:

				return;
		}
	}

	// Original function: FUN_0046a1bf; positive values deliberately do nothing.
	void Human_l_PerformTaskEvent_ChangeGold(void* human, std::int32_t amount)
	{
		if (amount >= 0)
			return;

		if (Human_IsAttachedToLearnHouse(human) && Human_Misc_IsHidden(human))
		{
			auto* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId));

			if (Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) == Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint) && House_Production_IsAbleToProduceGood(house, 8, false))
			{
				if (!House_Stock_ModifyAmount(house, 8, amount, 0, false))
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 13);
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 7);
				}

				return;
			}
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_0046a291.
	void Human_l_PerformTaskEvent_ChangeWealth(void* human, std::int32_t amount)
	{
		if (GameControl_Wealth_ChangeAmount(gGameControl, Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), amount))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 13);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 7);
		}
	}

	// Original function: FUN_0046a2d9; update counters only after successful completion.
	void Human_l_PerformTaskEvent_DidProduceOneGood(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) != 2)
		{

			return;
		}

		if (Human_IsAttachedToWorkHouse(human))
			House_Production_DidProduceOneGood(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)));

		Human_Production_DidProduceGoods(human, 1u);
	}

	// Original function: FUN_0046a0be. The unsafe pointer/revision check does not
	// inspect the allocation byte and is intentionally different from Target_IsValid.
	void Human_l_PerformTaskEvent_SendSocial(void* human, std::int32_t amount)
	{
		void* other = TargetHuman(human);

		if (other == nullptr || Read<std::uint32_t>(other, NC2Layout::HumanOffset::UniqueId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectRevision))
			return;

		AddAttribute(other, 0xAB0, static_cast<std::uint32_t>(amount));

		if (amount < 0)
		{

			return;
		}

		Human_DoAddNewUserCommand(other, 12, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);

		if (Read<std::uint32_t>(other, NC2Layout::HumanOffset::CurrentTask) == 1 && Human_l_IsAtomicInterruptable(other))
			Human_CurrentTask_DoInterrupt(other);
	}

	// Original function: FUN_0046a121; Windows PE constant 004F3B50 is 8000.
	void Human_l_PerformTaskEvent_SendReligion(void* human, std::int32_t amount)
	{
		void* other = TargetHuman(human);

		if (other == nullptr || Read<std::uint32_t>(other, NC2Layout::HumanOffset::UniqueId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectRevision))
		{
			Human_Target_Reset(human);

			return;
		}

		if (Read<std::int32_t>(other, NC2Layout::HumanOffset::Religion) <= 8000)
			AddAttribute(other, 0xAB4, static_cast<std::uint32_t>(amount));
	}

	// Original function: FUN_0046a165; the receiver maximum is checked before adding.
	void Human_l_PerformTaskEvent_SendHitpoints(void* human, std::int32_t amount)
	{
		void* other = TargetHuman(human);

		if (other == nullptr || Read<std::uint32_t>(other, NC2Layout::HumanOffset::UniqueId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectRevision))
		{
			Human_Target_Reset(human);

			return;
		}

		const auto hitpoints = Read<std::int32_t>(other, NC2Layout::HumanOffset::Hitpoints);

		if (hitpoints < static_cast<std::int32_t>(Human_GetHumanAttributeMaximum_Hitpoints(other)) && (amount > 0 || (Read<std::uint32_t>(other, NC2Layout::HumanOffset::MiscFlags) & 8u) == 0))
			AddAttribute(other, 0xAA0, static_cast<std::uint32_t>(amount));
	}
}