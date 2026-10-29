#pragma once
#include <cstdint>

namespace NC2Logic
{
	bool Human_IsAboutToMarry(const void* human);
	bool Human_UserMessage_IsInvalid(const void* human, const void* message);

	void Human_UserMessage_SendToUserMessage_MustWaitForGood(void* human);
	void Human_UserMessage_SendToUserMessage_DidProduceOneGood(void* human, std::uint32_t good);
	void Human_UserMessage_SendToUserMessage_DidProduceAllGoods(void* human);
	void Human_UserMessage_SendToUserMessage_CouldNotProduceOneGood(void* human, std::uint32_t good);
	void Human_UserMessage_SendToUserMessage_CouldNotProduceAnyGoods(void* human);
}