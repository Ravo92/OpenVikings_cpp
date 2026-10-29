#pragma once
#include <cstdint>

namespace NC2Logic
{
	void ToUserMessage_Construct(void* message);
	void ToUserMessage_FinishMessage(void* message);
	void ToUserMessage_CopyConstruct(void* message, const void* source);
	void Human_UserMessage_SendToUserMessage_CanProduceNewGood(void* human, std::uint32_t good);
	void Human_UserMessage_SendToUserMessage_CanDoNewJob(void* human, std::uint32_t job);
	void Human_UserMessage_SendToUserMessage_CanEquipNewItem(void* human, std::uint32_t item);
	void Human_UserMessage_SendToUserMessage_CannotAttachHouse(void* human, std::uint32_t mask);
	void Human_UserMessage_SendToUserMessage_CannotAttachVehicle(void* human);
	void Human_UserMessage_SendToUserMessage_CannotDetachHouse(void* human, std::uint32_t mask);
	bool ToUserMessage_IsValid(const void* message, bool checkHuman);
	bool ToUserMessage_IsEqualTo(const void* message, const void* other);
	bool ToUserMessage_IsSimmilarTo(const void* message, const void* other);
}

namespace NC2LogicExternals
{
	void External_Gui_SetMarker(std::uint32_t index, const std::uint32_t* point);
	void External_Camera_SetToPosition(const std::uint32_t* point);
	void External_Gui_SelectHuman(std::int32_t humanId, bool followOnly);
	void External_Misc_SendToUserStringMessage(const char* text, bool unused);
	void External_Misc_SendToUserMessage(const void* message);
	void External_Misc_RemoveUserMessage(std::uint32_t humanId, std::uint32_t type);
}