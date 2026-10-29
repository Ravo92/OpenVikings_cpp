#include "C2GameMapAccess.h"
#include "C2HouseAccess.h"
#include "C2HumanAtomicEvents.h"
#include "C2HumanCommandAccess.h"
#include "C2HumanGoodieTasks.h"
#include "C2HumanGoods.h"
#include "C2HumanInitialization.h"
#include "C2HumanJobAccess.h"
#include "C2HumanNeeds.h"
#include "C2HumanTaskAccess.h"
#include "C2HumanWalking.h"
#include "C2InteractionTargetAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	namespace
	{
		void* House(std::uint32_t id)
		{

			return CHouseArray::mStaticVars + id * CHouseArray::ElementSize;
		}

		void AddAttribute(void* human, std::uint32_t offset, std::uint32_t amount)
		{
			Write(human, offset, Read<std::uint32_t>(human, offset) + amount);
		}
	}

	// Original function: FUN_0042cb08.
	bool Human_IsAbleToTalk(const void* human)
	{

		return Read<std::uint8_t>(gStaticJobTypeData, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::TalkAllowedUnsigned) != 0;
	}

	// Original function: FUN_0042cb4c.
	bool Human_IsAbleToPray(const void* human)
	{

		return Read<std::uint8_t>(gStaticJobTypeData, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::PrayAllowedUnsigned) != 0;
	}

	// Original function: FUN_0042cf0f.
	bool TGoodTypes_IsFood(std::uint32_t good)
	{

		return (good >= 16 && good <= 23) || good == 56;
	}

	// Original function: FUN_0042cbf7.
	bool Human_IsAbleToEatCandy(const void* human)
	{

		return Read<std::uint8_t>(gStaticJobTypeData, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::EatCandyAllowedUnsigned) != 0;
	}

	// Original function: FUN_0042d24e; retain the lookup even though Windows
	// unconditionally permits sleeping after it.
	bool Human_CanSleepAtTargetMapPosition(const void* human)
	{
		GameMap_GetMapMicroPointPtr(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition);

		return true;
	}

	// Original functions: FUN_0042d25e and FUN_0042d27e.
	bool Human_CanPrayAtTargetMapPosition(const void* human)
	{

		return Human_CanPrayAtMapRealPosition(human, static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::TargetPosition);
	}

	bool Human_CanPrayAtMapRealPosition(const void* human, const void* point)
	{
		const void* micro = GameMap_GetMapMicroPointPtr(point);

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) != 2 || (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectData) & 1u) == 0)
			return false;

		const auto id = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId);
		const void* house = House(id);

		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3 || Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
			return false;

		if (id != Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId) && House_GetHouseMainType(house) == 2)
			return false;

		return Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) == 1 || House_Religion_IsHolyFireActive(house);
	}

	// Original function: FUN_0042cc12.
	bool Human_IsRefreshingInWorld(const void* human)
	{

		return Read<std::uint8_t>(human, NC2Layout::HumanOffset::MilitaryWorldRefreshmentFlag) != 0;
	}

	// Original function: FUN_0042cdd3; soldier/hero jobs do not enter the lookup.
	bool Human_IsDefendingHouse(const void* human, std::int32_t house)
	{
		if (Human_IsJobSoldier(human) || Human_IsJobHero(human))
			return false;

		const auto defending = Human_GetDefenseHouseId(human);

		return house == -1 ? defending != -1 : defending == house;
	}

	// Original function: FUN_0044187e. The occupied micro-point house precedes
	// the valid house target; allocation is not independently checked here.
	std::int32_t Human_GetDefenseHouseId(const void* human)
	{
		const void* micro = GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human));

		if (Read<std::uint8_t>(micro, NC2Layout::MapMicroPointOffset::ObjectType) == 2)
		{
			const auto id = Read<std::uint16_t>(micro, NC2Layout::MapMicroPointOffset::HouseId);

			if (Read<std::uint8_t>(House(id), NC2Layout::HouseOffset::DefenceModeFlag) != 0)
				return id;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget) == 5 && Human_Target_IsValid(human))
		{
			const void* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetObjectId));

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DefenceModeFlag) != 0)
				return Read<std::int32_t>(house, NC2Layout::HouseOffset::HouseId);
		}

		return -1;
	}

	// Original function: FUN_0045bde4; retain the two original message filters.
	void Human_UserMessage_SendToUserMessage(void* human, std::uint32_t type)
	{
		if (type == 11 && (Human_GetPlayerType(human) == 2 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 5 || !Human_IsAgeAdult(human) || Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0))
			return;

		if (type == 2 && Human_IsAttachedToWorkHouse(human) && Read<std::uint8_t>(gStaticJobTypeData, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::MustHaveFinishedWorkHouseUnsigned) != 0 && Read<std::uint32_t>(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId)), NC2Layout::HouseOffset::ConstructionState) != 3)
		{

			return;
		}

		std::byte message[772];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::Type, type);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0045be9c.
	void Human_UserMessage_SendToUserMessage_CannotFindGood_Generic(void* human, std::uint32_t good)
	{
		if (good == 64 || GameAssistantSingleHumanData_CommandHistory_IsEquipCommandInArray(static_cast<std::byte*>(human) + NC2Layout::HumanOffset::AssistantData))
		{

			return;
		}

		std::byte message[772];
		ToUserMessage_Construct(message);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 4);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem, good);
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0045bf0b.
	void Human_UserMessage_SendToUserMessage_CannotFindGood_ForBuild(void* human, std::uint32_t good)
	{
		std::byte message[772];
		ToUserMessage_Construct(message);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 5);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem, good);
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0045bf64; retain both unlocked/allowed-good checks.
	void Human_UserMessage_SendToUserMessage_CannotFindGood_ForHome(void* human, std::uint32_t good)
	{
		const void* player = static_cast<const std::byte*>(gGameControl) + Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId) * NC2Layout::RecordSize::GameControlPlayer;
		const auto offset = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) * 66u + good;

		if (Read<std::uint8_t>(player, offset + NC2Layout::GameControlPlayerOffset::AvailableGoods) == 0 || Read<std::uint8_t>(player, offset + NC2Layout::GameControlPlayerOffset::AllowedGoods) == 0)
		{

			return;
		}

		std::byte message[772];
		ToUserMessage_Construct(message);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 6);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem, good);
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0047024a; social failure deliberately retains its need.
	void Human_l_FailTask_FulfillNeeds(void* human)
	{
		const auto need = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter);

		if (Human_GetPlayerType(human) == 2)
		{
			if (need >= 1 && need <= 4)
				Write<std::uint32_t>(human, need == 1 ? NC2Layout::HumanOffset::Food : need == 2 ? NC2Layout::HumanOffset::Stamina : need == 3   ? NC2Layout::HumanOffset::Social : NC2Layout::HumanOffset::Religion, 8000);
		}
		else
		{
			if (need == 3)
				return;

			if (need == 1 || need == 2)
			{
				if (Read<std::int32_t>(human, NC2Layout::HumanOffset::TaskFailureCount) < 2)
				{
					Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);

					return;
				}

				if (!Human_UserControl_IsCommandTypeInQueue(human, need == 1 ? 7 : 8))
				{
					if (Read<std::int32_t>(human, need == 1 ? NC2Layout::HumanOffset::Food : NC2Layout::HumanOffset::Stamina) <= 2000)
					{
						Human_UserMessage_SendToUserMessage(human, need == 1 ? 0x1C : 0x1D);
					}
					else if (need == 1)
					{
						Human_UserMessage_SendToUserMessage_CannotFindGood_Generic(human, Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood));
					}
					else
					{
						Human_UserMessage_SendToUserMessage(human, 2);
					}
				}
			}
			else if (need == 4)
			{
				Human_UserMessage_SendToUserMessage(human, 0x1F);
			}
		}

		Write(human, NC2Layout::HumanOffset::TaskFailureCount, Read<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount) - 1u);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
	}

	// Original function: FUN_0045cf66. Higher unsigned useability wins; a tie
	// retains the earlier slot, and a partially used item receives the original +3.
	std::int32_t Human_l_Item_GetItemSlotIndexForUrgentNeed(const void* human, std::uint32_t need)
	{
		std::int32_t selected = -1;
		std::uint32_t best = 0;

		for (std::uint32_t slot = 0; slot < 4; ++slot)
		{
			const auto offset = NC2Layout::HumanOffset::InventorySlots + slot * NC2Layout::RecordSize::HumanInventorySlot;

			if (Read<std::uint32_t>(human, offset) == 0 || Read<std::int32_t>(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition) <= 0)
				continue;

			auto rank = Human_l_Item_GetUseabilityForUrgentNeed(human, need, slot);

			if (rank == 0)
				continue;

			if (!Human_l_Item_IsItemUnused(human, static_cast<std::int32_t>(slot)))
				rank += 3u;

			if (rank > best)
			{
				best = rank;
				selected = static_cast<std::int32_t>(slot);
			}
		}

		return selected;
	}

	// Original function: FUN_0045cfd1; complete nine-entry selector 0045D084..0045D0A7.
	void Human_l_Item_UseItemInSlot(void* human, std::int32_t slot)
	{
		if (slot == -1)
		{

			return;
		}

		const auto offset = NC2Layout::HumanOffset::InventorySlots + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HumanInventorySlot;

		switch (Read<std::uint32_t>(human, offset))
		{
			case SwitchIds::GoodType::Mead:
				AddAttribute(human, NC2Layout::HumanOffset::Food, 5000);
				AddAttribute(human, NC2Layout::HumanOffset::Stamina, 5000);
				AddAttribute(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition, 0u - 50u);
				break;
			case SwitchIds::GoodType::PotionFoodSmall:
				AddAttribute(human, NC2Layout::HumanOffset::Food, 10000);
				AddAttribute(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition, 0u - 50u);
				break;
			case SwitchIds::GoodType::PotionFoodBig:
				AddAttribute(human, NC2Layout::HumanOffset::Food, 10000);
				AddAttribute(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition, 0u - 20u);
				break;
			case SwitchIds::GoodType::PotionStaminaSmall:
				AddAttribute(human, NC2Layout::HumanOffset::Stamina, 10000);
				AddAttribute(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition, 0u - 50u);
				break;
			case SwitchIds::GoodType::PotionStaminaBig:
				AddAttribute(human, NC2Layout::HumanOffset::Stamina, 10000);
				AddAttribute(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition, 0u - 20u);
				break;
			case SwitchIds::GoodType::PotionHealSmall:
				AddAttribute(human, NC2Layout::HumanOffset::Hitpoints, 2000);
				AddAttribute(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition, 0u - 50u);
				break;
			case SwitchIds::GoodType::PotionHealBig:
				AddAttribute(human, NC2Layout::HumanOffset::Hitpoints, 2000);
				AddAttribute(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition, 0u - 20u);
				break;
			case SwitchIds::GoodType::AmuletFood:
				AddAttribute(human, NC2Layout::HumanOffset::Food, 4000);
				break;
			case SwitchIds::GoodType::AmuletStamina:
				AddAttribute(human, NC2Layout::HumanOffset::Stamina, 4000);
				break;
			default:
				break;
		}

		if (Read<std::int32_t>(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition) <= 0)
			Write<std::uint32_t>(human, offset, 0);
	}

	// Original function: FUN_0045d0a8.
	bool Human_l_Item_IsItemUnused(const void* human, std::int32_t slot)
	{

		return slot != -1 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::InventorySlots + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HumanInventorySlot) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::InventoryConditions + static_cast<std::uint32_t>(slot) * NC2Layout::RecordSize::HumanInventorySlot) == 100u;
	}

	// Original function: FUN_0045d0f4; slot indices are not range-checked.
	std::uint32_t Human_l_Item_GetUseabilityForUrgentNeed(const void* human, std::uint32_t need, std::uint32_t slot)
	{
		const auto offset = NC2Layout::HumanOffset::InventorySlots + slot * NC2Layout::RecordSize::HumanInventorySlot;
		const auto good = Read<std::uint32_t>(human, offset);

		if (good == 0 || Read<std::int32_t>(human, offset + NC2Layout::HumanInventorySlotOffset::PercentageCondition) <= 0 || !Tool_Item_CanBeUsedForNeed(need, good))
			return 0;

		if (Tool_Item_IsAmulett(good))
			return 3;

		if (Tool_Item_IsBigPotion(good))
			return 2;

		return Tool_Item_IsSmallPotion(good) ? 1u : 0u;
	}

	// Original function: FUN_0045d20a.
	bool Tool_Item_CanBeUsedForNeed(std::uint32_t need, std::uint32_t good)
	{
		if (need == 1)
			return good == 43 || good == 44 || good == 45 || good == 50;

		if (need == 2)
			return good == 43 || good == 46 || good == 47 || good == 51;

		if (need == 5)
			return good == 48 || good == 49;

		return false;
	}

	// Original function: FUN_0045d262.
	bool Tool_Item_IsAmulett(std::uint32_t good)
	{

		return good >= 50 && good <= 55;
	}

	// Original function: FUN_0045d28b.
	bool Tool_Item_IsSmallPotion(std::uint32_t good)
	{

		return good == 45 || good == 47 || good == 49;
	}

	// Original function: FUN_0045d2a5.
	bool Tool_Item_IsBigPotion(std::uint32_t good)
	{

		return good == 43 || good == 44 || good == 46 || good == 48;
	}

	// Original function: FUN_00463509; priority is food, stamina, social, religion.
	void Human_AI_DoCheckForUrgentNeeds(void* human)
	{
		if (!Human_l_IsAtomicInterruptable(human) || Human_UserControl_IsExecutingUserCommand(human) || Read<std::uint8_t>(human, NC2Layout::HumanOffset::UrgentNeedsCheckedFlag) != 0)
			return;

		Write<std::uint8_t>(human, NC2Layout::HumanOffset::UrgentNeedsCheckedFlag, 1);

		if (Human_AI_IsIgnoringNeeds(human))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 0);
			return;
		}

		std::uint32_t need = 0;
		const auto alreadyFulfilling = [human](std::uint32_t type)
		{

			return Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter) == type && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 7 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == 1;
		};

		if (!Human_IsDefendingHouse(human, -1))
		{
			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Religion) <= 2000 && Human_IsJobNeedingReligion(human) && !alreadyFulfilling(4))
				need = 4;

			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Social) <= 2000 && Human_IsJobNeedingSocial(human) && !alreadyFulfilling(3))
				need = 3;

			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Stamina) <= 2000 && !alreadyFulfilling(2))
				need = 2;
		}

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) <= 2000 && !alreadyFulfilling(1))
			need = 1;

		if (need == 0)
			return;

		const auto item = Human_l_Item_GetItemSlotIndexForUrgentNeed(human, need);

		if (item != -1)
		{
			Human_l_Item_UseItemInSlot(human, item);
			return;
		}

		if (!Human_IsRefreshingInWorld(human))
		{

			return;
		}

		Human_CurrentTask_DoInterrupt(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write(human, NC2Layout::HumanOffset::CurrentTaskParameter, need);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 7);
	}

	// Original function: FUN_0046f797; retain the Windows home refill priority.
	void Human_l_StartTask_FulfillNeeds_AtHome(void* human)
	{
		if (Human_IsSexMale(human) && Human_IsMarried(human))
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::Social, 10000);

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Stamina) <= 8000)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 2);
		}
		else if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) <= 8000 && House_Stock_GetAmountOfGoods(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), 16, false) > 0)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 1);
		}
		else if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Religion) <= 8000 && Human_IsJobNeedingReligion(human))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 4);
		}
		else if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Social) <= 8000 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::SpouseId) != NXBasics::IntegerRange::UInt32Maximum && Human_IsInHomeHouse(CHumanArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::SpouseId) * CHumanArray::ElementSize))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 3);
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter) != 0)
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 7);
	}

	// Original function: FUN_0047014c.
	void Human_l_FinishTask_FulfillNeeds(void* human)
	{
		const auto need = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter);
		bool repeat = false;
		std::uint32_t message = 0;

		switch (need)
		{
			case SwitchIds::HumanNeed::Food:
				repeat = Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) <= 2000 || Human_IsCarryingFood(human);
				message = 0x1C;
				break;
			case SwitchIds::HumanNeed::Stamina:
				repeat = Read<std::int32_t>(human, NC2Layout::HumanOffset::Stamina) <= 2000;
				message = 0x1D;
				break;
			case SwitchIds::HumanNeed::Social:
				repeat = Read<std::int32_t>(human, NC2Layout::HumanOffset::Social) <= 2000 && Human_IsJobNeedingSocial(human);
				message = 0x1E;
				break;
			case SwitchIds::HumanNeed::Religion:
				repeat = Read<std::int32_t>(human, NC2Layout::HumanOffset::Religion) <= 2000 && Human_IsJobNeedingReligion(human);
				message = 0x1F;
				break;
			default:
				break;
		}

		if (need == 1)
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::FoodProductionNeedCheckDeferrals, 0);

		if (repeat)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		}
		else if (message != 0)
		{
			NC2LogicExternals::External_Misc_RemoveUserMessage(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), message);
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) != 2)
		{

			return;
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 8);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 0);

		if (Human_IsInHomeHouse(human))
			Human_l_StartTask_FulfillNeeds_AtHome(human);

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 7)
		{
			Human_Target_Reset(human);

			if (InteractionTargetData_IsValid(static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::SecondaryInteractionTarget))
				Human_Target_RestoreSecondaryTarget(human);
		}
	}

	// Original function: FUN_0046f881.
	void Human_l_StartTask_FulfillNeeds_Food(void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

		if (Human_IsCarryingGoodExceptFood(human))
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Remember(human);
			return;
		}

		if (good != 0)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
			{
				Human_StopCurrentGoodCirculationMode(human);
			}
			else
			{
				Human_Target_ResetSecondaryTarget(human);
			}

			const auto cargo = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);
			const bool candy = Human_IsAbleToEatCandy(human) && (cargo == 20 || cargo == 17);
			// Original outlined operation 00438CEC; no separate Mac name recovered.
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood, 0);
			CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, 0);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
			Human_l_StartAtomicAnimation(human, candy ? 11u : 10u, -1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
			return;
		}

		if (!Human_IsRefreshingInWorld(human) && Human_UserControl_GetLastCommandType(human) != 7)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode) != 0)
		{
			if (!TGoodTypes_IsFood(Read<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood)))
			{
				Human_CurrentTask_DoInterrupt(human);

				return;
			}

			Human_l_StartTask_FulfillNeeds_Food_TryToCollect(human);
		}
		else
		{
			Human_l_StartTask_FulfillNeeds_Food_Collect(human);
		}
	}

	// Original function: FUN_0046f95c.
	void Human_l_StartTask_FulfillNeeds_Food_TryToCollect(void* human)
	{
		if (Human_IsAtTargetPoint(human))
		{
			if (Human_l_StartTask_Tool_PickupWantedGood_FromTargetPosition(human))
			{

				return;
			}

			Human_Target_Reset(human);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 0);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::GoodCirculationMode, 0);
	}

	// Original function: FUN_0046f98a; collect from a vehicle, defence house, home or map search.
	void Human_l_StartTask_FulfillNeeds_Food_Collect(void* human)
	{
		Human_Target_Reset(human);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 16);
		std::uint32_t real = 0, work = 0;

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
		{
			auto* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;

			for (std::uint32_t good = 16; good < 24; ++good)
			{
				if (Vehicle_Stock_GetAmountOfGoods(vehicle, good) == 0)
					continue;

				const auto revision = Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId);

				if (!Vehicle_Stock_ModifyFutureAmount(vehicle, good, -1, revision))
					continue;

				if (!Vehicle_Stock_ModifyAmount(vehicle, good, -1, revision))
				{
					Vehicle_Stock_ModifyFutureAmount(vehicle, good, 1, revision);
					continue;
				}

				Write(human, NC2Layout::HumanOffset::CarriedGood, good);
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, good);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
				return;
			}

			const auto kind = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

			if ((kind == 3 || kind == 4) && Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::DockedFlag) == 0)
				goto failed;

			std::uint32_t entry = 0;

			if (!Vehicle_Door_GetEntryPoint(vehicle, &entry, 0))
				goto failed;

			const auto good = Human_FindFood_Complex(human, &real, &work, &entry);

			if (good == 0)
				goto failed;

			if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAction) == 1)
			{
				alignas(4) std::byte command[20]{ };
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 0x70);
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, static_cast<std::uint8_t>(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId)));
				Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
				Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
				Vehicle_DoAddNewUserCommand(vehicle, command);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskParameter, 0);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
				return;
			}

			Write(human, NC2Layout::HumanOffset::RequestedGood, good);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryInteractionTarget, 6);
			Write(human, NC2Layout::HumanOffset::SecondaryTargetObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId));
			Write(human, NC2Layout::HumanOffset::SecondaryTargetObjectRevision, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision));

			if (Human_l_StartAtomicWalkAndFetch(human, &real, &work, true))
				return;

			goto failed;
		}

		if (Human_Misc_IsHidden(human) && Human_IsDefendingHouse(human, -1))
		{
			const auto id = Human_GetDefenseHouseId(human);
			auto* house = id == -1 ? nullptr : House(static_cast<std::uint32_t>(id));

			for (std::uint32_t good = 16; good < 24; ++good)
			{
				if (!House_Stock_CanStoreGoodType(house, good, true) || House_Stock_GetAmountOfGoods(house, good, true) == 0)
					continue;

				House_Stock_ModifyAmount(house, good, -1, 0, false);
				House_Stock_ModifyFutureAmount(house, good, -1, false);
				Write(human, NC2Layout::HumanOffset::CarriedGood, good);
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x21, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, good);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 2);
				return;
			}

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 0);
			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount) == 0 && !Human_IsJobIgnoringHomeHouse(human) && Human_FindFoodInHomeHouse(human, &real, &work))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::RequestedGood, 16);

			if (Human_l_StartAtomicWalkAndFetch(human, &real, &work, true))
				return;
		}

		{
			const auto good = Human_FindFood_Complex(human, &real, &work, Human_GetMapPositionRef(human));

			if (good != 0)
			{
				Write(human, NC2Layout::HumanOffset::RequestedGood, good);

				if (Human_l_StartAtomicWalkAndFetch(human, &real, &work, true))
				{

					return;
				}
			}
		}
	failed:
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 3);
	}

	// Original function: FUN_0046fc36.
	void Human_l_StartTask_FulfillNeeds_Stamina(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Remember(human);
			return;
		}

		if (!Human_IsRefreshingInWorld(human) && Human_UserControl_GetLastCommandType(human) != 8)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			return;
		}

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
		{
			void* vehicle = CVehicleArray::mStaticVars + Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId) * CVehicleArray::ElementSize;
			const auto kind = Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::LogicType);

			if (kind == 3 || kind == 4)
			{
				Human_l_StartAtomicAnimation(human, 8, -1);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
				return;
			}

			if (Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::AtomicAction) == 1)
			{
				std::byte command[20]{ };
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::Opcode, 0x70);
				Write<std::uint8_t>(command, NC2Layout::UserNetworkCommandOffset::PlayerId, Read<std::uint8_t>(human, NC2Layout::HumanOffset::PlayerId));
				Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::VehicleId));
				Write(command, NC2Layout::UserNetworkCommandOffset::ObjectRevision, Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId));
				Vehicle_DoAddNewUserCommand(vehicle, command);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 0);
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask, 1);
				return;
			}

			Write<std::uint32_t>(human, NC2Layout::HumanOffset::SecondaryInteractionTarget, 6);
			Write(human, NC2Layout::HumanOffset::SecondaryTargetObjectId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId));
			Write(human, NC2Layout::HumanOffset::SecondaryTargetObjectRevision, Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision));
			Vehicle_Passengers_MoveOut_h(vehicle, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		}

		if (Human_IsAtTargetPoint(human) && Human_CanSleepAtTargetMapPosition(human))
		{
			Human_l_StartAtomicAnimation(human, 8, -1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);

			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::TaskFailureCount) == 0 && !Human_IsJobIgnoringHomeHouse(human) && Human_IsAttachedToHomeHouse(human) && Read<std::uint32_t>(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), NC2Layout::HouseOffset::ConstructionState) == 3)
		{
			Human_l_StartTask_FulfillNeeds_Stamina_AtHome(human);
		}
		else
		{
			Human_l_StartTask_FulfillNeeds_Stamina_OnMap(human);
		}
	}

	// Original function: FUN_0046fdad.
	void Human_l_StartTask_FulfillNeeds_Stamina_AtHome(void* human)
	{
		if (Human_IsInHomeHouse(human))
		{
			Human_l_StartAtomicAnimation(human, 8, -1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);

			return;
		}

		const void* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId));
		const auto real = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition);
		const auto work = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DoorEntryPoint);

		if (!Human_l_StartAtomicWalk(human, &real, &work))
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_0046fe29.
	void Human_l_StartTask_FulfillNeeds_Stamina_OnMap(void* human)
	{
		std::uint32_t real = 0, work = 0;

		if (Human_FindPlaceToSleep(human, &real, &work))
		{
			if (Human_l_StartAtomicWalk(human, &real, &work))
			{

				return;
			}
		}
		else
		{
			Human_UserMessage_SendToUserMessage(human, 0x1D);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_0046fe7f.
	void Human_l_StartTask_FulfillNeeds_Social(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Remember(human);
			return;
		}

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) == 0)
		{
			void* other = Human_FindHumanToTalkInTalkRange_Adult(human);

			if (other != nullptr && Human_StartWaitingForOtherHuman(other))
			{
				Human_Target_SetTargetPositions(human, Human_GetMapPositionRef(other), Human_GetMapPositionRef(human));
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::InteractionTarget, 7);
				Write(human, NC2Layout::HumanOffset::TargetObjectId, Read<std::uint32_t>(other, NC2Layout::HumanOffset::HumanId));
				Write(human, NC2Layout::HumanOffset::TargetObjectRevision, Read<std::uint32_t>(other, NC2Layout::HumanOffset::UniqueId));
				Human_StartListening(other, human);
				Human_StopWaitingForOtherHuman(other);
				Human_l_StartAtomicAnimation(human, 14, static_cast<std::int32_t>(Human_Target_GetDirectionTo(human)));
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
				return;
			}

			other = Human_FindHumanToTalkInSearchRange_Adult(human);

			if (other != nullptr && Human_StartWaitingForOtherHuman(other))
			{
				Human_StopWaitingForOtherHuman(other);

				if (Human_l_StartAtomicWalk(human, Read<std::int32_t>(other, NC2Layout::HumanOffset::MapMoveableId), 1))
				{

					return;
				}
			}
		}

		Human_l_StartTask_FulfillNeeds_Social_Monologuize(human);
	}

	// Original function: FUN_0046ff51; separate original function from the inline
	// monologue branch in Social, even though the effects are the same.
	void Human_l_StartTask_FulfillNeeds_Social_Monologuize(void* human)
	{
		Human_l_StartAtomicAnimation(human, 13, -1);
		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
	}

	// Original function: FUN_0046ff69.
	void Human_l_StartTask_FulfillNeeds_Religion(void* human)
	{
		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood) != 0)
		{
			Human_l_StartTask_Tool_DropCarriedGoodNearby_Remember(human);
			return;
		}

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 0);
			return;
		}

		if (!Human_IsJobIgnoringHomeHouse(human) && Human_IsAttachedToHomeHouse(human))
		{
			const void* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId));

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0 && Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3 && House_Stock_GetAmountOfGoods(house, 15, false) > 0)
			{
				Human_l_StartTask_FulfillNeeds_Religion_AtHome(human);

				if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) != 3)
				{

					return;
				}
			}
		}

		Human_l_StartTask_FulfillNeeds_Religion_AtTemple(human);
	}

	// Original function: FUN_0046fff6. Windows ignores the prayer-point result here.
	void Human_l_StartTask_FulfillNeeds_Religion_AtHome(void* human)
	{
		const void* house = House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId));

		if (Human_IsAtTargetPoint(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::TargetPosition) == Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition))
		{
			Human_l_StartAtomicAnimation(human, 12, -1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);

			return;
		}

		const auto real = Read<std::uint32_t>(house, NC2Layout::HouseOffset::MapPosition);
		std::uint32_t work = 0;
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(Human_GetMapPositionRef(human)), NC2Layout::MapMicroPointOffset::ContinentId);
		House_Religion_GetNextPrayPoint(house, Human_GetMapPositionRef(human), &work, false, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), continent);

		if (!Human_l_StartAtomicWalk(human, &real, &work))
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_004700c9.
	void Human_l_StartTask_FulfillNeeds_Religion_AtTemple(void* human)
	{
		if (Human_IsAtTargetPoint(human) && Human_CanPrayAtTargetMapPosition(human))
		{
			Human_l_StartAtomicAnimation(human, 12, -1);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 1);
			return;
		}

		std::uint32_t real = 0, work = 0;

		if (Human_FindTempleForPraying(human, &real, &work))
		{
			if (Human_l_StartAtomicWalk(human, &real, &work))
			{

				return;
			}
		}
		else
		{
			Human_UserMessage_SendToUserMessage(human, 0x1F);
		}

		Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
	}

	// Original function: FUN_00460c1b. Equal distances replace the preceding candidate.
	bool Human_FindTempleForPraying(void* human, void* real, void* work)
	{
		const void* position = Human_GetMapPositionRef(human);
		const auto continent = Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(position), NC2Layout::MapMicroPointOffset::ContinentId);
		std::uint32_t nearest = 50000;
		const void* selected = nullptr;
		CHouseIterator iterator;

		for (std::uint32_t pass = 0; pass < 2; ++pass)
		{
			if (pass != 0 && nearest <= 40)
				break;

			for (auto id = iterator.GetFirstSectorHouseId(Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), position, 10000, pass != 0); id != -1; id = iterator.GetNextSectorHouseId())
			{
				const void* house = House(static_cast<std::uint32_t>(id));

				if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType) != (pass == 0 ? 37u : 1u) || Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) != 3)
					continue;

				if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0)
					return false;

				if (Read<std::uint8_t>(GameMap_GetConstMapMicroPointPtr(static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::MapPosition), NC2Layout::MapMicroPointOffset::ContinentId) != continent)
					continue;

				const auto distance = static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::X), Read<std::int16_t>(position, NC2Layout::MapCoordinatePairOffset::Y), Read<std::int16_t>(house, NC2Layout::HouseOffset::MapPositionX), Read<std::int16_t>(house, NC2Layout::HouseOffset::MapPositionY)));

				if (distance > nearest)
					continue;

				nearest = distance;
				selected = house;

				if (distance <= 40)
					break;
			}
		}

		if (selected == nullptr)
			return false;

		if (!House_Religion_GetNextPrayPoint(selected, position, work, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), continent))
		{
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus, 3);
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStep, 7);
			return false;
		}

		Write(real, NC2Layout::MapCoordinatePairOffset::Coordinates, Read<std::uint32_t>(selected, NC2Layout::HouseOffset::MapPosition));

		return true;
	}

	// Original function: FUN_00463678; native conditions, aging and death notification order.
	void Human_AI_DoUpdateConditions(void* human)
	{
		if (Human_AI_IsIgnoringNeeds(human))
		{
			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints) < static_cast<std::int32_t>(Human_GetHumanAttributeMaximum_Hitpoints(human)))
				AddAttribute(human, NC2Layout::HumanOffset::Hitpoints, 1);
		}
		else
		{
			AddAttribute(human, NC2Layout::HumanOffset::Food, 0u - 1u);

			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) <= 0)
			{
				if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 8u) == 0)
					AddAttribute(human, NC2Layout::HumanOffset::Hitpoints, 0u - 2u);

				Write<std::uint32_t>(human, NC2Layout::HumanOffset::Food, 0);
			}
			else if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints) < static_cast<std::int32_t>(Human_GetHumanAttributeMaximum_Hitpoints(human)))
			{
				AddAttribute(human, NC2Layout::HumanOffset::Hitpoints, 1);
			}

			AddAttribute(human, NC2Layout::HumanOffset::Stamina, 0u - 1u);

			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Stamina) <= 0)
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::Stamina, 0);

			if (Human_IsJobNeedingSocial(human))
				AddAttribute(human, NC2Layout::HumanOffset::Social, 0u - 1u);

			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Social) <= 0)
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::Social, 0);

			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Religion) <= 0)
				Write<std::uint32_t>(human, NC2Layout::HumanOffset::Religion, 0);
		}
		// Complete Windows constant region 004F3B38..004F3B77: maxima 10000,
		// refill values 8000, urgent thresholds 2000, warning thresholds 1000.

		for (const auto offset : { 0xAA8u, 0xAACu, 0xAB0u, 0xAB4u })
		{
			if (Read<std::int32_t>(human, offset) > 15000)
				Write<std::uint32_t>(human, offset, 15000);
		}

		const auto hitpointMaximum = Human_GetHumanAttributeMaximum_Hitpoints(human);
		const auto hitpointLimit = hitpointMaximum + (hitpointMaximum >> 1);

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints) > static_cast<std::int32_t>(hitpointLimit))
			Write(human, NC2Layout::HumanOffset::Hitpoints, hitpointLimit);

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::WaitingHumanCount) <= 0)
			Write<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanCount, 0);

		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit18) == 0)
			AddAttribute(human, 0xAA4, 1);

		if (!Human_IsAgeAdult(human))
		{
			const auto job = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
			const auto age = Read<std::int32_t>(human, NC2Layout::HumanOffset::AgeCounter);

			if (((job == 1 || job == 2) && age > 1440) || ((job == 3 || job == 4) && age > 2880))
			{
				const bool becameAdult = job >= 3;
				Write<std::uint8_t>(human, job == 1 || job == 3 ? NC2Layout::HumanOffset::AllowedJobFlagsSigned + SwitchIds::HumanJob::BabyFemale : NC2Layout::HumanOffset::AllowedJobFlagsSigned + SwitchIds::HumanJob::BabyMale, 0);
				Write<std::uint8_t>(human, job == 1 || job == 3 ? NC2Layout::HumanOffset::AllowedJobFlagsSigned + SwitchIds::HumanJob::ChildFemale : NC2Layout::HumanOffset::AllowedJobFlagsSigned + SwitchIds::HumanJob::ChildMale, becameAdult ? 0 : 1);
				Human_SetJobType(human, job + 2u);
				CCallbackManager::sTheObjectPtr->ExecuteCallback(4, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1, 1);

				if (becameAdult)
				{
					for (const auto offset : { 0xB14u, 0xB18u })
					{
						const auto parent = Read<std::uint32_t>(human, offset);

						if (parent != NXBasics::IntegerRange::UInt32Maximum)
						{
							Write<std::uint32_t>(CHumanArray::mStaticVars + parent * CHumanArray::ElementSize, NC2Layout::HumanOffset::ChildId, NXBasics::IntegerRange::UInt32Maximum);
							Write<std::uint32_t>(human, offset, NXBasics::IntegerRange::UInt32Maximum);
						}
					}

					if (Human_IsAttachedToHomeHouse(human))
					{
						House_Inhabitants_DetachHuman(House(Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId)), Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId), 1);
					}

					Human_UserMessage_SendToUserMessage(human, 0x25);
				}
			}
		}

		if ((Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 8u) == 0 && Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints) <= Human_GetHumanAttributeMargin_Hitpoints(human))
		{
			auto item = Human_l_Item_GetItemSlotIndexForUrgentNeed(human, 5);

			if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints) <= 0)
			{
				while (item != -1 && Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints) <= 0)
				{
					Human_l_Item_UseItemInSlot(human, item);
					item = Human_l_Item_GetItemSlotIndexForUrgentNeed(human, 5);
				}

				if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints) <= 0)
				{
					std::byte message[772];
					ToUserMessage_Construct(message);
					Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 0x3B);
					Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
					Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
					ToUserMessage_FinishMessage(message);
					Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HumanId, NXBasics::IntegerRange::UInt32Maximum);
					NC2LogicExternals::External_Misc_SendToUserMessage(message);
					Write<std::uint8_t>(gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable, NC2Layout::MapMoveableOffset::RemovalRequestedFlag, 1);
					CCallbackManager::sTheObjectPtr->ExecuteCallback(0x18, true, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId), 4, 0);
					return;
				}
			}
			else if (item == -1)
			{
				Human_UserMessage_SendToUserMessage(human, 0x21);
			}
		}

		if (gGameTime.tick % 12u != 0)
		{

			return;
		}

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 1 && Human_l_IsAtomicInterruptable(human))
			Human_AI_DoCheckForUrgentNeeds(human);

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) <= 1000)
			Human_UserMessage_SendToUserMessage(human, 0x1C);

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) <= 0)
			Human_UserMessage_SendToUserMessage(human, 0x20);

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Stamina) <= 1000)
			Human_UserMessage_SendToUserMessage(human, 0x1D);

		if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Religion) <= 1000 && Human_IsJobNeedingReligion(human))
			Human_UserMessage_SendToUserMessage(human, 0x1F);
	}

	// Original function: FUN_0042cb74.
	bool Human_IsAbleToDoGoodie(const void* human)
	{
		const auto job = Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType);

		return !((job >= 31 && job <= 41) || (job >= 42 && job <= 47));
	}

	// Original function: FUN_0042d271; Windows retains the lookup and permits sleep unconditionally.
	bool Human_CanSleepAtMapRealPosition(const void* point)
	{
		GameMap_GetMapMicroPointPtr(point);

		return true;
	}

	// Original function: FUN_00463ad2; Windows outlined predicate, semantic name inferred from its callers.
	bool Human_AI_IsIgnoringNeeds(const void* human)
	{

		return !Human_IsAgeAdult(human) || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 1u) != 0;
	}

	// Original function: FUN_0047033c; Windows outlined cargo predicate.
	bool Human_IsCarryingFood(const void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

		return good != 0 && TGoodTypes_IsFood(good);
	}

	// Original function: FUN_00470358; Windows outlined cargo predicate.
	bool Human_IsCarryingGoodExceptFood(const void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::CarriedGood);

		return good != 0 && !TGoodTypes_IsFood(good);
	}

}