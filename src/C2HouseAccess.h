#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	std::uint32_t House_GetPlayerId(const void* house);
	const void* House_GetMapPositionRef(const void* house);
	const void* House_Door_GetEntryPointRef(const void* house);
	bool House_Inhabitants_IsHumanAttachedAsWorker(const void* house, std::uint32_t human);
	bool House_Inhabitants_IsFreeWorkSlotAvailable(const void* house, std::uint32_t job);
	void* House_Inhabitants_GetAttachedHumanPtr(void* house, std::uint32_t slot);
	bool House_Religion_IsHolyFireActive(const void* house);
	void House_LandscapeOverlay_SetActive(void* house, std::int32_t overlay, bool enabled);
	bool House_IsMilitarySectorType(std::uint32_t type);
	void House_SetBehaviourFlag(void* house, std::uint32_t bit, bool enabled);
	void House_Door_GetEntryPoint(const void* house, void* point);
	bool House_Init(void* house, std::int32_t id, std::uint32_t player, std::uint32_t engineType, std::uint32_t stage, std::uint32_t point, bool finished, std::uint32_t missionCheckId);
	void House_Exit(void* house);
	void __fastcall House_Misc_Destroy_House(void* house, void* unusedEdx);
	void House_l_Inhabitants_Reset(void* house);
	void House_Inhabitants_DetachAllHumans(void* house);
	void House_Inhabitants_DetachAllHumans(void* house, std::uint32_t mask);
	void House_ActionOverlay_Stop(void* house, std::int32_t overlay);
	void House_l_ActionOverlay_StopAll(void* house);
	void House_Misc_Blocks_ClearAndRebuild(const void* house);
	void House_Misc_MapHeights_Modify(const void* house);
	void House_l_AttachToSector(void* house);
	void House_l_DetachFromSector(void* house);
	void House_ChangePlayerId(void* house, std::uint32_t player);
	void House_DoOneGameSecond(void* house);
	void House_DoOneGameTick(void* house);
	void House_Religion_Send(const void* house);
	void House_l_ActionOverlay_Update(void* house);

	struct SHouseConstructionGoodsEntry
	{
		std::uint32_t good;
		std::uint32_t requiredAmount;
		std::uint32_t deliveredAmount;
		std::uint32_t incomingAmount;
		std::uint32_t consumedAmount;
	};

	struct SHouseConstructionGoods
	{
		std::int32_t count;
		SHouseConstructionGoodsEntry entries[10];
		std::uint32_t totalRequired;
		std::uint32_t totalDelivered;
		std::uint32_t totalConsumed;
	};

	static_assert(sizeof(SHouseConstructionGoodsEntry) == 20);
	static_assert(offsetof(SHouseConstructionGoodsEntry, requiredAmount) == 4);
	static_assert(offsetof(SHouseConstructionGoodsEntry, deliveredAmount) == 8);
	static_assert(offsetof(SHouseConstructionGoodsEntry, incomingAmount) == 12);
	static_assert(offsetof(SHouseConstructionGoodsEntry, consumedAmount) == 16);
	static_assert(sizeof(SHouseConstructionGoods) == 216);
	static_assert(offsetof(SHouseConstructionGoods, totalRequired) == 204);
	static_assert(offsetof(SHouseConstructionGoods, totalDelivered) == 208);
	static_assert(offsetof(SHouseConstructionGoods, totalConsumed) == 212);
	void House_Construction_CalculateGoodsNeeded(std::uint32_t engineType, std::uint32_t developmentStage, bool onlyCurrentStage, SHouseConstructionGoods* goods);
	void House_Inhabitants_UpdateAllWorkers(void* house);
	bool __fastcall House_Construction_UpgradeStart(void* house, void* unusedEdx);
	bool __fastcall House_Construction_UpgradeStop(void* house, void* unusedEdx);
	void House_l_Construction_InitConstruction(void* house, std::uint32_t developmentStage, bool upgrade);
	void House_l_Construction_TransferFutureStock(void* house);
	void House_l_Construction_UpdateGoodsBeingBrought(void* house);
	bool House_Construction_IsNeedingGoods(const void* house);
	bool House_Construction_CanUseOneGoodNow(const void* house);
	bool House_Construction_DoUseOneGood(void* house);
	bool House_Construction_IsUseGoodRunning(const void* house);
	bool House_Construction_DoOneStep(void* house);
	void House_HitPoints_Reset(void* house);
	void House_l_DoorPoint_Calculate(void* house);
	void House_Inhabitants_UpdateAllHumans(void* house);
	void House_l_Exploration_Do(const void* house);
	void House_UserMessage_SendToUserMessage(const void* house, std::uint32_t type);
	void House_Misc_ClearLandscapesAndGuides(const void* house);
	void House_HitPoints_ModifyValue(void* house, std::int32_t delta);
	std::uint32_t House_Stock_GetMaximumAmountByStockId(const void* house, std::uint32_t stock);
	std::uint32_t House_Stock_GetMaximumAmount(const void* house, std::uint32_t good, bool futureStock);
	void House_Stock_SetMinimumValue(void* house, std::uint32_t good, std::uint32_t minimum);
	std::uint32_t House_Stock_GetGoodRequestCounter(const void* house, std::uint32_t good);
	std::uint32_t House_Stock_GetMinimumValue(const void* house, std::uint32_t good);
	bool House_HomeQualityEnhancer_SetAllowedFlag(void* house, std::uint32_t enhancer, bool allowed);
	bool House_HomeQualityEnhancer_ModifyValue(void* house, std::uint32_t enhancer, std::int32_t direction);
	bool House_Religion_GetNextPrayPoint(const void* house, const void* source, void* point, bool inTemple, std::uint32_t humanId, std::uint32_t continent);
	void House_Production_SetStoppedFlag(void* house, bool stopped);
	void House_l_Production_CheckProductionCounter(void* house);
	void House_Production_ChangeCounter(void* house, std::int32_t delta);
	void House_Production_DidProduceOneGood(void* house);
	bool House_HitPoints_IsDamaged(const void* house);
	bool House_ActionOverlay_IsActive(const void* house, std::uint32_t overlay);
	bool House_LandscapeOverlay_IsActive(const void* house, std::int32_t overlay);
	void House_ActionOverlay_Start(void* house, std::int32_t overlay, std::uint32_t duration);
	void* House_Inhabitants_GetAttachedHumanSlotPtrByHumanId(void* house, std::uint32_t human);
	void* House_Inhabitants_GetAttachedHumanSlotPtrBySlotId(void* house, std::uint32_t slot);
	std::int32_t House_Inhabitants_GetAttachedHumanSlotIdByHumanId(const void* house, std::uint32_t human);
	bool House_Inhabitants_IsHumanAttachedAsStudent(const void* house, std::uint32_t human);
	void House_Inhabitants_SetStudentJob(void* house, std::uint32_t human, std::uint32_t job);
	void House_Inhabitants_SetStudentGood(void* house, std::uint32_t human, std::uint32_t good);
	bool House_Inhabitants_CanHumanBeAttached(void* house, std::uint32_t human, std::uint32_t mask);
	bool House_Inhabitants_CanHumanBeAttachedAsHomie(void* house, std::uint32_t human);
	bool House_Inhabitants_CanHumanBeAttachedAsWorker(void* house, std::uint32_t human);
	bool House_Inhabitants_CanHumanBeAttachedAsBuilder(void* house, std::uint32_t human);
	bool House_Inhabitants_CanHumanBeAttachedAsStudent(void* house, std::uint32_t human);
	bool House_l_Inhabitants_CanHumanBeAttached_BaseCheck(const void* house, std::uint32_t human);
	std::uint32_t House_Inhabitants_GetNumberOfFamiliesLivingInHome(const void* house, std::uint32_t* families);
	std::int32_t House_Inhabitants_GetNumberOfAttachedWorkersOfPassedJob(const void* house, std::uint32_t job);
	std::uint32_t House_Inhabitants_GetNumberOfAttachedHumans(const void* house, std::uint32_t mask);
	std::uint32_t House_Inhabitants_GetNumberOfAttachedHumans(const void* house);
	std::uint32_t House_Inhabitants_GetNumberOfAttachedHumans_IgnoreBuilders(const void* house);
	std::int32_t House_Construction_GetTotalNumberOfBuildersAllowed(const void* house);
	bool House_Repair_IsFinished(const void* house);
	bool House_Construction_GetNextWorkPoint(const void* house, const void* source, void* point, bool avoidOtherBuilders, std::uint32_t humanId, std::uint32_t continent);
	void House_SetDefenceModeFlag(void* house, bool enabled);
	void House_DoNewUserCommand(void* house, const void* command);
	struct SStaticHouseTypeData;
	const SStaticHouseTypeData* House_GetHouseTypeDataPtr(const void* house);
	std::uint32_t House_GetHouseMainType(const void* house);
	void House_l_Military_StartArrow(const void* house, std::uint32_t point);
	std::int32_t House_Construction_GetPercentage(const void* house);
	std::uint32_t House_Stock_GetAmountOfGoods(const void* house, std::uint32_t goodId, bool stockOnly);
	std::uint32_t Tool_House_GetAmountOfGoods(const void* house, std::uint32_t* good, bool stockOnly);
	std::uint32_t House_Stock_RequestAmountOfGoods(void* house, std::uint32_t goodId, bool stockOnly);
	bool __fastcall House_Stock_ModifyAmount(void* house, void* unusedEdx, std::uint32_t good, std::uint32_t amount, bool construction);
	bool House_Stock_ModifyAmount(void* house, std::uint32_t good, std::int32_t amount, std::int32_t fraction, bool construction);
	bool House_l_Construction_PutGoodIntoStock(void* house, std::uint32_t good);
	bool House_Construction_IsNeedingGood(const void* house, std::uint32_t good);
	bool House_Stock_IsFull(const void* house, std::uint32_t good);
	bool House_Stock_IsFullSoon(const void* house, std::uint32_t good);
	bool House_Construction_IsNeedingFutureGood(const void* house, std::uint32_t good);
	std::uint32_t House_Stock_GetDemandForGood(const void* house, std::uint32_t good);
	bool House_Stock_HasDemandForGood(const void* house, std::uint32_t good);
	float House_Stock_GetAmountOfGoodX(const void* house, std::uint32_t goodId, bool includeFutureStock);
	bool House_Stock_CanStoreGoodType(const void* house, std::uint32_t goodId, bool futureStock);
	bool House_Stock_ModifyFutureAmount(void* house, std::uint32_t goodId, std::int32_t delta, bool modifyConstruction);
	std::int32_t House_l_Stock_GetStockSlotIdForGoodType(const void* house, std::uint32_t goodId);
	void* House_Misc_WalkBlockArea_GetPtr(const void* house);
	void House_Misc_Blocks_SetToMap(const void* house);
}