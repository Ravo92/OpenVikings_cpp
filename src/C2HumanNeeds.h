#pragma once

#include <cstdint>

namespace NC2Logic
{
	bool Human_IsAbleToTalk(const void* human);
	bool Human_IsAbleToPray(const void* human);
	bool TGoodTypes_IsFood(std::uint32_t good);
	bool Human_IsAbleToEatCandy(const void* human);
	bool Human_CanSleepAtTargetMapPosition(const void* human);
	bool Human_CanPrayAtMapRealPosition(const void* human, const void* point);
	bool Human_CanPrayAtTargetMapPosition(const void* human);
	bool Human_FindTempleForPraying(void* human, void* real, void* work);
	void Human_l_StartTask_FulfillNeeds_Food(void* human);
	void Human_l_StartTask_FulfillNeeds_Food_TryToCollect(void* human);
	void Human_l_StartTask_FulfillNeeds_Food_Collect(void* human);
	void Human_l_StartTask_FulfillNeeds_AtHome(void* human);
	void Human_l_FinishTask_FulfillNeeds(void* human);
	void Human_l_FailTask_FulfillNeeds(void* human);
	void Human_l_StartTask_FulfillNeeds_Stamina(void* human);
	void Human_l_StartTask_FulfillNeeds_Stamina_AtHome(void* human);
	void Human_l_StartTask_FulfillNeeds_Stamina_OnMap(void* human);
	void Human_l_StartTask_FulfillNeeds_Social(void* human);
	void Human_l_StartTask_FulfillNeeds_Social_Monologuize(void* human);
	void Human_l_StartTask_FulfillNeeds_Religion(void* human);
	void Human_l_StartTask_FulfillNeeds_Religion_AtHome(void* human);
	void Human_l_StartTask_FulfillNeeds_Religion_AtTemple(void* human);
	void Human_AI_DoUpdateConditions(void* human);
	void Human_AI_DoCheckForUrgentNeeds(void* human);
	std::int32_t Human_GetDefenseHouseId(const void* human);
	bool Human_IsDefendingHouse(const void* human, std::int32_t house);
	bool Human_IsRefreshingInWorld(const void* human);
	bool Tool_Item_CanBeUsedForNeed(std::uint32_t need, std::uint32_t good);
	bool Tool_Item_IsAmulett(std::uint32_t good);
	bool Tool_Item_IsBigPotion(std::uint32_t good);
	bool Tool_Item_IsSmallPotion(std::uint32_t good);
	bool Human_l_Item_IsItemUnused(const void* human, std::int32_t slot);
	std::uint32_t Human_l_Item_GetUseabilityForUrgentNeed(const void* human, std::uint32_t need, std::uint32_t slot);
	std::int32_t Human_l_Item_GetItemSlotIndexForUrgentNeed(const void* human, std::uint32_t need);
	void Human_l_Item_UseItemInSlot(void* human, std::int32_t slot);
	void Human_UserMessage_SendToUserMessage(void* human, std::uint32_t type);
	void Human_UserMessage_SendToUserMessage_CannotFindGood_Generic(void* human, std::uint32_t good);
	void Human_UserMessage_SendToUserMessage_CannotFindGood_ForBuild(void* human, std::uint32_t good);
	void Human_UserMessage_SendToUserMessage_CannotFindGood_ForHome(void* human, std::uint32_t good);

	bool Human_IsAbleToDoGoodie(const void* human);
	bool Human_CanSleepAtMapRealPosition(const void* point);
	bool Human_AI_IsIgnoringNeeds(const void* human);
	bool Human_IsCarryingFood(const void* human);
	bool Human_IsCarryingGoodExceptFood(const void* human);
}