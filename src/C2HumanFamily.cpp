#include "C2Debug.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanAtomicEvents.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanFamily.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2HumanNeeds.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "XNumericConstants.h"
#include "RemasterAIEconomy.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;
	namespace
	{
		void* Human(std::uint32_t id)
		{
			return CHumanArray::mStaticVars + id * CHumanArray::ElementSize;
		}
		void* House(std::uint32_t id)
		{
			return CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
		}
		void* Spouse(const void* human)
		{
			return Human(Read<std::uint32_t>(human, NC2Layout::HumanOffset::SpouseId));
		}
		void QueueChildCommand(void* human, std::uint8_t command)
		{
			Human_DoAddNewUserCommand(human, command, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
		}
		void AssignHome(void* human, std::uint32_t home)
		{
			Write(human, NC2Layout::HumanOffset::HomeHouseId, home);
			Write(human, NC2Layout::HumanOffset::HomeHouseRevision, Read<std::uint32_t>(House(home), NC2Layout::HouseOffset::UniqueId));
			House_Inhabitants_AttachHuman(House(home), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1);
		}
		std::int32_t HomeDistance(std::uint32_t first, std::uint32_t second)
		{
			return VE_HexagonDirection_GetDistance(Read<std::int16_t>(House(first), NC2Layout::HouseOffset::MapPositionX), Read<std::int16_t>(House(first), NC2Layout::HouseOffset::MapPositionY), Read<std::int16_t>(House(second), NC2Layout::HouseOffset::MapPositionX), Read<std::int16_t>(House(second), NC2Layout::HouseOffset::MapPositionY));
		}
		bool Remaster_AllowsWorkingMarriage(const void* human, const void* partner)
		{
			if (!RemasterAIEconomy::IsActive(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId))) return true;
			return RemasterAIEconomy::IsFamilyWorker(Human_IsSexFemale(human) ? partner : human);
		}
	}

	// Original function: FUN_0042ca45.
	bool Human_IsAbleToMarryNow(const void* human)
	{
		return Human_IsAbleToMarry(human) && Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) == 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 7;
	}

	// Original function: FUN_0042ca6b.
	bool Human_IsAbleToGetMarried(const void* human)
	{
		return Human_IsAbleToMarryNow(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) == 0;
	}

	// Original function: FUN_0042caaf; the sex predicate deliberately includes child jobs.
	bool Human_IsAbleToGetPregnant(const void* human)
	{
		const auto home = Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId);
		return Human_IsSexFemale(human) && Human_IsMarried(human) && home != NXBasics::IntegerRange::UInt32Maximum && !Human_IsHavingChild(human) && Read<std::uint32_t>(House(home), NC2Layout::HouseOffset::ConstructionState) == 3;
	}

	// Original function: FUN_0043c83b.
	std::uint32_t Human_DoExecuteUserCommand_Marry(void* human)
	{
		if (!Human_IsAbleToMarry(human)) return 1;
		if (!Human_IsAbleToMarryNow(human) || !Human_l_IsAtomicInterruptable(human)) return 0;
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 5)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0); Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 5);
		}
		return 2;
	}

	// Original function: FUN_0043c880.
	std::uint32_t Human_DoExecuteUserCommand_GetBoy(void* human)
	{
		if (!Human_IsAbleToGetPregnant(human)) return 1;
		if (!Human_l_IsAtomicInterruptable(human)) return 0;
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 11);
		return 2;
	}

	// Original function: FUN_0043c8b5.
	std::uint32_t Human_DoExecuteUserCommand_GetGirl(void* human)
	{
		if (!Human_IsAbleToGetPregnant(human)) return 1;
		if (!Human_l_IsAtomicInterruptable(human)) return 0;
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 12);
		return 2;
	}

	// Original function: FUN_0043c8ea; attach relatives first and roll them back on failure.
	std::uint32_t Human_DoExecuteUserCommand_AttachHomeHouse(void* human, std::int32_t houseId, std::uint32_t revision)
	{
		if (!Human_l_IsAtomicInterruptable(human)) return 0;
		if (Human_IsAttachedToHomeHouse(human) && Human_DoExecuteUserCommand_DetachHomeHouse(human) == 0) return 1;
		auto* house = House(static_cast<std::uint32_t>(houseId));
		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) != revision) return 1;
		bool attached = true;
		if (Human_IsMarried(human))
		{
			auto* partner = Spouse(human);
			attached = House_Inhabitants_AttachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::SpouseId), 1);
			if (attached)
			{
				Write(partner, NC2Layout::HumanOffset::HomeHouseId, static_cast<std::uint32_t>(houseId));
				Write(partner, NC2Layout::HumanOffset::HomeHouseRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
				if (Human_IsSexMale(human))
				{
					Write(partner, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint)); Write<std::uint32_t>(partner, NC2Layout::HumanOffset::WorkRetryCount, 0); Human_WorkCenterMarker_Update(partner);
				}
			}
		}
		if (attached && Human_IsHavingChild(human))
		{
			auto* child = Human(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId));
			attached = House_Inhabitants_AttachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId), 1);
			if (attached)
			{
				Write(child, NC2Layout::HumanOffset::HomeHouseId, static_cast<std::uint32_t>(houseId)); Write(child, NC2Layout::HumanOffset::HomeHouseRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
			}
		}
		if (attached) attached = House_Inhabitants_AttachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1);
		if (!attached)
		{
			Human_UserMessage_SendToUserMessage_CannotAttachHouse(human, 1);
			if (Human_IsMarried(human)) House_Inhabitants_DetachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::SpouseId), 1);
			if (Human_IsHavingChild(human)) House_Inhabitants_DetachHuman(house, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId), 1);
			return 1;
		}
		Write(human, NC2Layout::HumanOffset::HomeHouseId, static_cast<std::uint32_t>(houseId));
		Write(human, NC2Layout::HumanOffset::HomeHouseRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
		if (Human_IsSexFemale(human))
		{
			Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint)); Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0); Human_WorkCenterMarker_Update(human);
		}
		return 2;
	}

	// Original function: FUN_0043ca9f; a failed relative detachment keeps the caller attached.
	std::uint32_t Human_DoExecuteUserCommand_DetachHomeHouse(void* human)
	{
		if (!Human_l_IsAtomicInterruptable(human)) return 0;
		if (!Human_IsAttachedToHomeHouse(human)) return 1;
		bool failed = false;
		if (Human_IsMarried(human) && !House_Inhabitants_DetachHuman(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::SpouseId), 1)) failed = true;
		if (Human_IsHavingChild(human) && !House_Inhabitants_DetachHuman(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId), 1)) failed = true;
		if (!failed && House_Inhabitants_DetachHuman(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1)) return 2;
		Human_UserMessage_SendToUserMessage_CannotDetachHouse(human, 1);
		return failed ? 0u : 1u;
	}

	// Original function: FUN_00443deb; love uses task 13, independently of ordinary listening.
	bool Human_StartGettingLoved(void* human, void* partner)
	{
		const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);
		if ((task != 9 && (task != 5 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanId) != Read<std::uint32_t>(partner, NC2Layout::HumanOffset::HumanId))) || !Human_l_IsAtomicInterruptable(human)) return false;
		Human_Target_SetTargetPositions(human, Human_GetMapPositionRef(partner), Human_GetMapPositionRef(human));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 7);
		Write(human, NC2Layout::HumanOffset::TargetObjectId, Read<std::uint32_t>(partner, NC2Layout::HumanOffset::HumanId));
		Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(partner, NC2Layout::HumanOffset::UniqueId));
		Human_l_StartAtomicAnimation(human, 21, static_cast<std::int32_t>(Human_Target_GetDirectionTo(human)));
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 13);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		return true;
	}

	// Original function: FUN_0045e1a6; stop at the first eligible sector-list candidate.
	void* Human_FindHumanToMarry(const void* human)
	{
		CHumanIterator iterator;
		for (auto id = iterator.GetFirstSectorHumanId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates), 40, false); id != -1; id = iterator.GetNextSectorHumanId())
		{
			auto* other = Human(static_cast<std::uint32_t>(id));
			if (Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId) || Read<std::uint32_t>(other, NC2Layout::HumanOffset::TribeType) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) || Human_IsSexFemale(other) == Human_IsSexFemale(human) || !Human_IsAbleToGetMarried(other)) continue;
			if (!Remaster_AllowsWorkingMarriage(human, other)) continue;
			Game_Random_GetNumber(); // Windows samples its one-element candidate array as well.
			return other;
		}
		return nullptr;
	}

	// Original function: FUN_00444ba8; waiting reservations survive the failed walking branch.
	void Human_l_StartTask_Marry(void* human)
	{
		if (Human_IsDefendingHouse(human, -1))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3); return;
		}
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
			{
				Human_StopCurrentGoodCirculationMode(human); Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
			}
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Remember(human);
			return;
		}
		auto* partner = Human_FindHumanToMarry(human);
		if (partner != nullptr && Human_StartWaitingForOtherHuman(partner) && Human_l_StartAtomicWalk(human, Read<std::int32_t>(partner, NC2Layout::HumanOffset::MapMoveableId), 1))
		{
			Write(human, NC2Layout::HumanOffset::WaitingHumanId, Read<std::uint32_t>(partner, NC2Layout::HumanOffset::HumanId));
			Write(partner, NC2Layout::HumanOffset::WaitingHumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 5);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		}
		else
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3); Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 6);
		}
	}

	// Original function: FUN_00444c64; the bread pair is checked but good 16 alone is consumed.
	void Human_l_StartTask_GetChild(void* human, std::uint32_t task, std::uint8_t command)
	{
		auto* home = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId));
		const auto firstBread = House_Stock_RequestAmountOfGoods(home, 16, false);
		const auto secondBread = House_Stock_RequestAmountOfGoods(home, 17, false);
		if (firstBread + secondBread < 2u || Human_IsJobIgnoringHomeHouse(Spouse(human)) || Human_IsDefendingHouse(human, -1))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3); return;
		}
		const auto real = Read<std::uint32_t>(home, NC2Layout::HouseOffset::MapPosition), work = Read<std::uint32_t>(home, NC2Layout::HouseOffset::DoorEntryPoint);
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetPosition) != real) Human_Target_SetTargetPositions(human, &real, &work);
		if (Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetWorkPosition))
		{
			if (Human_l_StartAtomicWalk(human, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition, static_cast<std::byte*>(human) + NC2Layout::HumanOffset::TargetWorkPosition)) QueueChildCommand(human, command);
			else Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			return;
		}
		if (Read<std::uint32_t>(Human_GetMapPositionRef(human), NC2Layout::MapCoordinatePairOffset::Coordinates) != Read<std::uint32_t>(Human_GetMapPositionRef(Spouse(human)), NC2Layout::MapCoordinatePairOffset::Coordinates))
		{
			QueueChildCommand(human, command);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 9);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			Human_DoAddNewUserCommand(Spouse(human), 6, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), 0, 0, 0, 0);
			return;
		}
		House_Stock_ModifyAmount(home, 16, -2, 0, false);
		Human_l_StartAtomicAnimation(human, 80, -1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
		Write(human, NC2Layout::HumanOffset::CurrentTask, task);
	}

	// Original function: FUN_00445469; completion calls love before changing the spouse links.
	void Human_l_FinishTask_Marry(void* human)
	{
		if (!Human_IsMarried(human))
		{
			const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanId);
			if (id != NXBasics::IntegerRange::UInt32Maximum && Read<std::uint8_t>(Human(id), NC2Layout::HumanOffset::AllocatedFlag) != 0)
			{
				auto* partner = Human(id);
				Human_Target_SetTargetPositions(human, Human_GetMapPositionRef(partner), Human_GetMapPositionRef(human));
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 7);
				Write(human, NC2Layout::HumanOffset::TargetObjectId, Read<std::uint32_t>(partner, NC2Layout::HumanOffset::HumanId));
				Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(partner, NC2Layout::HumanOffset::UniqueId));
				if (Remaster_AllowsWorkingMarriage(human, partner) && Human_StartGettingLoved(partner, human))
				{
					Write(human, NC2Layout::HumanOffset::SpouseId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanId));
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanId, NXBasics::IntegerRange::UInt32Maximum);
					Write(partner, NC2Layout::HumanOffset::SpouseId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
					Write<std::uint32_t>(partner, NC2Layout::HumanOffset::WaitingHumanId, NXBasics::IntegerRange::UInt32Maximum);
					Human_StopWaitingForOtherHuman(partner);
					if (Human_IsHavingChild(human))
					{
						Write(partner, NC2Layout::HumanOffset::ChildId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId)); Write(Human(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId)), NC2Layout::HumanOffset::FatherId, Read<std::uint32_t>(partner, NC2Layout::HumanOffset::HumanId));
					}
					Human_l_FinishTask_Marry_AttachToBestHome(human);
					Human_l_StartAtomicAnimation(human, 20, static_cast<std::int32_t>(Human_Target_GetDirectionTo(human)));
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
					return;
				}
			}
			Human_StopWaitingForOtherHuman_Detach(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
			return;
		}
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 1, 0);
		CHumanIterator iterator;
		for (auto id = iterator.GetFirstHumanId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Human_GetMapPositionRef(human), 10); id != -1; id = iterator.GetNextHumanId())
		{
			auto* other = Human(static_cast<std::uint32_t>(id));
			if (!Human_IsAbleToTalk(other) || !Human_IsAbleToListen(other) || Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId) || Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId) == Read<std::uint32_t>(human, NC2Layout::HumanOffset::SpouseId)) continue;
			Human_DoAddNewUserCommand(other, 12, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId), 0, 0);
			if (Read<std::uint32_t>(other, NC2Layout::HumanOffset::CurrentTask) == 1 && Human_l_IsAtomicInterruptable(other)) Human_CurrentTask_DoInterrupt(other);
		}
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
	}

	// Original function: FUN_0044563a; equal work distances select the spouse's home.
	void Human_l_FinishTask_Marry_AttachToBestHome(void* human)
	{
		auto* partner = Spouse(human);
		if (Human_IsAttachedToHomeHouse(human))
		{
			if (Human_IsHavingChild(human))
			{
				auto* child = Human(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId)); House_Inhabitants_DetachHuman(House(Read<std::uint32_t>(child, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(child, NC2Layout::HumanOffset::HumanId), 1);
			}
			const auto otherHome = Read<std::uint32_t>(partner, NC2Layout::HumanOffset::HomeHouseId);
			const auto home = Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId);
			bool moveSelf = false;
			if (otherHome != NXBasics::IntegerRange::UInt32Maximum)
			{
				auto* man = Human_IsSexFemale(human) ? partner : human;
				const auto work = Read<std::uint32_t>(man, NC2Layout::HumanOffset::WorkAttachmentHouseId);
				moveSelf = work == NXBasics::IntegerRange::UInt32Maximum ? Human_IsSexFemale(human) : HomeDistance(work, home) >= HomeDistance(work, otherHome);
			}
			if (moveSelf)
			{
				House_Inhabitants_DetachHuman(House(home), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1);
				AssignHome(human, otherHome);
				Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(House(otherHome), NC2Layout::HouseOffset::DoorEntryPoint));
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
				Human_WorkCenterMarker_Update(human);
			}
			else
			{
				if (otherHome != NXBasics::IntegerRange::UInt32Maximum) House_Inhabitants_DetachHuman(House(otherHome), Read<std::uint32_t>(partner, NC2Layout::HumanOffset::HumanId), 1);
				AssignHome(partner, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId));
			}
		}
		else if (Read<std::uint32_t>(partner, NC2Layout::HumanOffset::HomeHouseId) != NXBasics::IntegerRange::UInt32Maximum)
		{
			const auto home = Read<std::uint32_t>(partner, NC2Layout::HumanOffset::HomeHouseId);
			AssignHome(human, home);
			Write(human, NC2Layout::HumanOffset::WorkCenter, Read<std::uint32_t>(House(home), NC2Layout::HouseOffset::DoorEntryPoint));
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WorkRetryCount, 0);
			Human_WorkCenterMarker_Update(human);
		}
		if (Human_IsAttachedToHomeHouse(human) && Human_IsHavingChild(human)) AssignHome(Human(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId));
	}

	// Original function: FUN_00445d23.
	void Human_l_FailTask_Marry(void* human)
	{
		Human_StopWaitingForOtherHuman_Detach(human);
		const auto failure = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount, 0);
		Human_UserMessage_SendToUserMessage(human, failure == 1 || failure == 7 ? 40u : failure == 6 ? 39u : 38u);
	}

	// Original function: FUN_00445d7b.
	void Human_l_FailTask_GetBoy(void* human)
	{
		QueueChildCommand(human, 15);
	}
	// Original function: FUN_00445d90.
	void Human_l_FailTask_GetGirl(void* human)
	{
		QueueChildCommand(human, 16);
	}

	// Original function: FUN_00469a16; allocation precedes family links and callback 0x18.
	void Human_l_PerformTaskEvent_GiveBirth(void* human)
	{
		const auto task = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask);
		if (task == 11 || task == 12) Write(human, NC2Layout::HumanOffset::ChildId, static_cast<std::uint32_t>(Logic_EntryPoint_Add_Human(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType), task == 11 ? 2u : 1u, Human_GetMapPositionRef(human), 0, 0)));
		else C2Debug_WarningBox("invalid task type for GiveBirth event! (must be getBoy or getGirl)");
		if (!Human_IsHavingChild(human)) return;
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 2, 0);
		// Windows 0045BFEC (UserMessage_SendToUserMessage_GaveBirth) is a no-op.
		auto* child = Human(Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId));
		Write(child, NC2Layout::HumanOffset::MotherId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		if (Human_IsMarried(human))
		{
			Write(Spouse(human), NC2Layout::HumanOffset::ChildId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId)); Write(child, NC2Layout::HumanOffset::FatherId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::SpouseId));
		}
		if (!Human_IsAttachedToHomeHouse(human)) return;
		House_Inhabitants_AttachHuman(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::ChildId), 1);
		Write(child, NC2Layout::HumanOffset::HomeHouseId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId));
		Write(child, NC2Layout::HumanOffset::HomeHouseRevision, Read<std::uint32_t>(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), NC2Layout::HouseOffset::UniqueId));
	}
}
