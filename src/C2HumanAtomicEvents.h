#pragma once

#include <cstdint>

namespace NC2Logic
{
	struct SStaticAtomicAnimationData;
	struct SStaticAtomicAnimationEventData;
	void Human_l_PerformTaskEvent(void* human, const SStaticAtomicAnimationEventData& event);
	void Human_l_PerformTaskEvent_SendSocial(void* human, std::int32_t amount);
	void Human_l_PerformTaskEvent_SendReligion(void* human, std::int32_t amount);
	void Human_l_PerformTaskEvent_SendHitpoints(void* human, std::int32_t amount);
	void Human_l_PerformTaskEvent_PlaySoundFX(void* human, std::uint32_t sound);
	void Human_l_PerformTaskEvent_ChangeGold(void* human, std::int32_t amount);
	void Human_l_PerformTaskEvent_ChangeWealth(void* human, std::int32_t amount);
	void Human_l_PerformTaskEvent_DidProduceOneGood(void* human);
	bool Human_IsInHomeHouse(const void* human);
	bool Human_IsInWorkHouse(const void* human);
	bool Human_IsJobIgnoringHomeHouse(const void* human);
	void Human_l_ResetAtomic(void* human);
}