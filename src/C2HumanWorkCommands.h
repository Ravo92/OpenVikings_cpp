#pragma once
#include <cstdint>

namespace NC2Logic
{
	std::uint32_t Human_DoExecuteUserCommand_AttachWorkHouse(void* human, std::int32_t house, std::uint32_t revision);
	std::uint32_t Human_DoExecuteUserCommand_AttachLearnHouse(void* human, std::int32_t house, std::uint32_t revision);
	std::uint32_t Human_DoExecuteUserCommand_AttachBuildHouse(void* human, std::int32_t house, std::uint32_t revision);
	std::uint32_t Human_DoExecuteUserCommand_DetachBuildHouse(void* human);
	std::uint32_t Human_DoExecuteUserCommand_DetachVehicle(void* human);
	std::uint32_t Human_DoExecuteUserCommand_AttachVehicle(void* human, std::int32_t vehicle, std::uint32_t revision);
	std::uint32_t Human_DoExecuteUserCommand_SetWorkCenter(void* human, const void* point);
	std::uint32_t Human_DoExecuteUserCommand_ChangeJob(void* human, std::int32_t job);
	std::uint32_t Human_DoExecuteUserCommand_SetJob(void* human, std::uint32_t job);
	std::uint32_t Human_DoExecuteUserCommand_ChangeProduction(void* human, std::uint32_t good, std::int32_t amount);
	std::uint32_t Human_DoExecuteUserCommand_LearnJob(void* human, std::uint32_t job);
	std::uint32_t Human_DoExecuteUserCommand_LearnGood(void* human, std::uint32_t good);
	std::uint32_t Human_DoExecuteUserCommand_RestartJobExecution(void* human);
	void Human_UserMessage_SendToUserMessage_CannotDetachVehicle(void* human);
}