#pragma once

#include <cstdint>

namespace NC2Logic
{
	bool Human_UsesExtendedPathfinder(const void* human);
	bool Human_IsAbleToListenNow(const void* human);
	bool Human_IsAbleToListen(const void* human);
	bool Human_IsDoingGoodie(const void* human);
	bool Human_StartWaitingForOtherHuman(void* human);
	bool Human_StartListening(void* human, void* speaker);
	void Human_Target_SetTargetPositions(void* human, const void* realPosition, const void* workPosition);
	void Human_Target_SetWorkPosition(void* human, const void* position);
	std::uint32_t Human_Target_GetDirectionTo(const void* human);
	void* Human_FindHumanToTalkInTalkRange_Any(const void* human);
	void* Human_FindHumanToTalkInSearchRange_Any(const void* human);
	bool Human_FindMapPointNearby(const void* human, void* point);
	bool Human_FindPlaceToSleep(const void* human, void* realPosition, void* workPosition);
	bool Human_FindDamagedWallNearWorkCenter(const void* human, void* realPosition, void* workPosition);
	void* Human_FindAnimalForPlaying(const void* human);
	bool Human_l_StartAtomicWalk(void* human, const void* realPosition, const void* workPosition);
	bool Human_l_StartAtomicWalk(void* human, std::int32_t moveableId, std::uint32_t distance);
	bool Human_l_StartAtomicWalkAndWait(void* human, const void* point);
	bool Human_HouseMisc_Leave(void* human);
	void Human_HouseMisc_Exit(void* human);
	void Human_l_StartTask_WaitForOtherHuman(void* human);
	void Human_l_StartTask_Listen(void* human, std::uint32_t direction);
	void Human_l_StartTask_GoHome(void* human);
	void Human_l_FailTask_GoHome(void* human);
	void Human_l_FailTask_Misc(void* human);
	void Human_l_StartTask_Goodie(void* human);
	void Human_l_StartTask_Goodie_Helper_Baby(void* human);
	void Human_l_StartTask_Goodie_Helper_Child(void* human);
	void Human_l_StartTask_Goodie_Helper_WalkAround(void* human);
	void Human_l_StartTask_Goodie_Helper_Sleep(void* human);
	void Human_l_StartTask_Goodie_Helper_TalkToAny(void* human);
	void Human_l_StartTask_Goodie_Helper_PlayWithAnimal(void* human);
	void Human_l_FinishTask_Goodie(void* human);
	void Human_l_FailTask_Goodie(void* human);

	void* Human_FindHumanToTalkInTalkRange_Adult(const void* human);
	void* Human_FindHumanToTalkInSearchRange_Adult(const void* human);
}