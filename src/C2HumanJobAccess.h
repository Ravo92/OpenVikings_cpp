#pragma once

#include <cstdint>

namespace NC2Logic
{
	bool Human_Misc_IsHidden(const void* human);
	bool Human_UserControl_DoesWantToMarry(const void* human);
	bool Human_UserControl_DoesWantToGetChild(const void* human);
	std::uint32_t Human_Tool_Gui_GetExperienceForEquippedWeapon(const void* human);
	std::int32_t Human_JobEfficiency_CalculateExperienceFactor(const void* human);
	std::int32_t Human_l_JobEfficiency_GetNeededRetries(const void* human);
	void Human_l_JobEfficiency_SetupNeededRetries(void* human);
	std::int32_t Human_l_JobEfficiency_CalculateNeededRetries(const void* human);
	std::int32_t Human_JobEfficiency_GetBaseRetryCounter(const void* human);
	void Human_ChangeToolCondition(void* human, std::int32_t delta);
	std::uint32_t Human_l_JobEfficiency_CalculateToolFactor(const void* human);
	void Human_UpdateCurrentBuildHouse(void* human);
	void Human_UpdateCurrentWorkHouse(void* human);
	void Human_UpdateCurrentHomeHouse(void* human);
	std::int32_t Human_l_JobEfficiency_CalculateOutputAmountDecimal(const void* human);
	void Tool_Human_GetExperienceType_GainedViaWorking(std::int32_t job, std::uint32_t good, std::uint32_t& general, std::uint32_t& specific);
	std::uint32_t Human_GetPlayerType(const void* human);
	std::uint32_t Human_GetMilitaryMode(const void* human);
	std::uint32_t Human_GetExperienceNeededYetForGoodProduction_Type(const void* human, std::uint32_t good);
	std::int32_t Human_GetExperienceNeededYetForGoodProduction_Value(const void* human, std::uint32_t good);
	std::uint32_t Human_UserControl_GetNextJobType(const void* human);
	std::uint32_t Tool_Human_GetReallyProducedGoodType(std::uint32_t job, std::uint32_t good);
	bool Human_IsProducingVehicle(const void* human);
	bool Human_IsAgeBaby(const void* human);
	bool Human_IsJobBuilder(const void* human);
	bool Human_IsJobScout(const void* human);
	bool Human_IsJobHunter(const void* human);
	bool Human_IsJobBreeder(const void* human);
	bool Human_IsJobTrader(const void* human);
	void* HumanMerchantArray_GetMerchantDataPtr(const void* manager, std::int32_t id);
	void* Human_GetMerchantDataPtr(const void* human);
	void* Human_l_GetMerchantDataPtr(const void* human);
	void* MerchantData_GetHouseDataPtr(void* merchant, std::uint32_t index);
	bool MerchantHouseData_IsValid(const void* houseData);
	bool Human_IsJobCarrier(const void* human);
	bool Human_IsJobFisher(const void* human);
	bool Human_IsForcedToHaveFinishedWorkHouse(const void* human);
	bool Human_IsAttachedToVehicle(const void* human);
	bool Human_IsJobNeedingSocial(const void* human);
	bool Human_IsJobNeedingReligion(const void* human);
	bool Human_CanEquipShoes(const void* human);
	bool Human_CanEquipToolIron(const void* human);
	bool Human_DoesExperienceAllowJobChange(const void* human, std::uint32_t job);
	bool Human_DoesExperienceAllowGoodProduction(const void* human, std::uint32_t good);
	bool Human_DoesExperienceAllowHouseBuilding(const void* human, std::uint32_t house);
	bool Human_DoesExperienceAllowEquipment(const void* human, std::uint32_t good);
	bool Human_DoesEducationAllowJobChange(const void* human, std::uint32_t job);
	bool Human_DoesEducationAllowGoodProduction(const void* human, std::uint32_t good);
	bool Human_IsCapableToProduceGood(const void* human, std::uint32_t good);
	bool Human_IsAbleToProduceGoodNow(const void* human, std::uint32_t good);
	bool Human_IsAbleToDoJob(const void* human, std::uint32_t job);
	bool Human_IsAllowedToDoJob(const void* human, std::uint32_t job);
	void Human_GainExperienceForEquippedWeapon(void* human, std::uint32_t amount);
	std::uint32_t Human_GetHumanAttributeMaximum_Hitpoints(const void* human);
	std::int32_t Human_GetHumanAttributeMargin_Hitpoints(const void* human);
	void Human_Equipment_SetShoeType(void* human, std::uint32_t good);
	void Human_SetupCurrentJobType(void* human);
	void Human_UpdateCurrentJobType(void* human);
	void Human_SetJobType(void* human, std::uint32_t job);
	void Human_SetupPlayerSpecificStuff(void* human);
	void Human_UpdatePlayerSpecificStuffAndCaps(void* human);
	void Human_Production_Setup(void* human);
	void Human_Production_Update(void* human);
	void Human_l_JobExperience_Gain(void* human);
	void Human_GainExperienceForWeapon(void* human, std::uint32_t weapon, std::uint32_t amount);
	bool Human_Production_Rotate(void* human);
	void Human_Production_ChangeCounter(void* human, std::uint32_t good, std::int32_t amount);
	void Human_Production_DidProduceGoods(void* human, std::uint32_t amount);
	void Human_Production_DidProduceGoods(void* human, std::int32_t amount, std::int32_t fraction);
	void Human_Production_CouldNotProduceCurrentGood(void* human);
	std::uint32_t Human_Production_GetCounter(const void* human, std::uint32_t good);
	bool Human_Production_IsInfinite(const void* human, std::uint32_t good);
	bool Human_Production_IsStopped(const void* human, std::uint32_t good);
	bool Human_Production_IsForbidden(const void* human, std::uint32_t good);
	bool Human_Production_IsIncapable(const void* human, std::uint32_t good);
	bool Human_Production_IsImpossible(const void* human, std::uint32_t good);
	void Human_ChangeProducedGoodType(void* human, std::uint32_t good);
	bool House_Production_IsAbleToProduceGood(const void* house, std::uint32_t good, bool primary);

	// Operations shared with human initialization and command handling.
	std::uint32_t Human_DoExecuteUserCommand_ChangeProducedGood(void* human, std::uint32_t good);
	bool Human_IsAttachedToLearnHouse(const void* human);
	void Human_WorkCenterMarker_Update(void* human);
	void Human_l_AssignWorkHouse(void* human, std::uint32_t house);
	std::int32_t HumanMerchantArray_AllocateMerchant(void* manager, std::uint32_t human);
	void HumanMerchantArray_FreeMerchant(void* manager, std::int32_t id);

	bool Human_IsAgeChild(const void* human);
	bool Human_IsJobCivilist(const void* human);
	bool Human_IsJobWorker(const void* human);
	bool Human_IsAttachedToWorkHouse2(const void* human);
	bool Human_IsProducingNothing(const void* human);
	bool Human_IsProducingFood(const void* human);
	bool Human_IsProducingMultipleGoods(const void* human);
	bool Human_IsForcedToHaveWorkHouse(const void* human);
	bool Human_IsAbleToUseVehicle(const void* human);
	bool Human_IsDoingNothing(const void* human);
	const void* Human_GetWorkCenterPointRef(const void* human);
}