#pragma once
#include <cstdint>

namespace NC2Logic
{
	// Contiguous Windows family-link view at CHuman +0xB14; no live object replacement.
	struct SHumanFamilyLinksStorage
	{
		std::uint32_t fatherId, motherId, waitingHumanId, spouseId, childId, homeHouseId, homeHouseRevision;
	};

	bool Human_IsAbleToMarryNow(const void* human);
	bool Human_IsAbleToGetMarried(const void* human);
	bool Human_IsAbleToGetPregnant(const void* human);
	void* Human_FindHumanToMarry(const void* human);
	bool Human_StartGettingLoved(void* human, void* partner);
	std::uint32_t Human_DoExecuteUserCommand_DetachHomeHouse(void* human);
	std::uint32_t Human_DoExecuteUserCommand_Marry(void* human);
	std::uint32_t Human_DoExecuteUserCommand_GetBoy(void* human);
	std::uint32_t Human_DoExecuteUserCommand_GetGirl(void* human);
	void Human_l_StartTask_Marry(void* human);
	void Human_l_StartTask_GetChild(void* human, std::uint32_t task, std::uint8_t command);
	void Human_l_FinishTask_Marry(void* human);
	void Human_l_FinishTask_Marry_AttachToBestHome(void* human);
	void Human_l_FailTask_Marry(void* human);
	void Human_l_FailTask_GetBoy(void* human);
	void Human_l_FailTask_GetGirl(void* human);
	void Human_l_PerformTaskEvent_GiveBirth(void* human);
}