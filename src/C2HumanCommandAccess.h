#pragma once
#include <cstdint>

namespace NC2Logic
{
	bool Human_IsJobWoman(const void* human);
	bool Human_IsAtTargetPoint(const void* human);
	bool Human_IsAtWorkCenter(const void* human);
	bool Human_Target_HasTarget(const void* human);
	void Human_CurrentTask_DoPerform(void* human);
	void Human_DoCheckAllUserCommands(void* human);
	bool Human_DoCheckUserCommand(const void* human, const void* command);
	void Human_ResetUserCommandArray(void* human);
	bool Human_l_IsAtomicInterruptable(const void* human);
	bool Human_StopWaitingForOtherHuman(void* human);
	void Human_StopWaitingForOtherHuman_Detach(void* human);
	bool Human_UserControl_IsCommandTypeInQueue(const void* human, std::uint8_t opcode);
	std::uint32_t Human_UserControl_GetLastCommandType(const void* human);
	void Human_CurrentTask_DoInterrupt(void* human);
	bool Human_UserControl_IsAPriorityCommand(std::uint32_t opcode);
	bool Human_UserControl_IsAInterruptingCommand(std::uint32_t opcode);
	bool Human_UserControl_IsPriorityCommandInQueue(const void* human);
	std::int32_t Human_UserControl_GetPriorityCommandIndexInQueue(const void* human);
	void Human_AI_DoPathfinderFailed(void* human);
	void Human_AI_DoPathfinderStopped(void* human);
	void Human_AI_DoNewMapPositionReached_DestinationReached(void* human);
	std::uint32_t Human_GetLandscapeTypeAtTargetPosition(const void* human);
	void Human_Target_Reset(void* human);
	void Human_Target_SetType(void* human, std::uint32_t type);
	bool Human_Target_IsValid(const void* human);
	std::uint32_t Human_Equipment_GetWeaponType(const void* human);
	void Human_StopCurrentGoodCirculationMode(void* human);
	bool Human_DoAddNewUserCommand(void* human, const void* command);
	bool Human_DoAddNewUserCommand(void* human, std::uint8_t opcode, std::uint8_t player, std::uint32_t id, std::uint32_t revision, std::uint32_t parameter1, std::uint32_t parameter2);
	std::uint32_t Human_UserControl_GetDefaultInteractionCommand_TrySendTo(const void* human, void* command, const void* point, bool parameter3, bool parameter4);
	bool Human_DoExecuteNextUserCommand(void* human);
	std::uint32_t Human_DoExecuteUserCommand(void* human, const void* command);
	std::uint32_t Human_DoExecuteUserCommand_GotoAndWait(void* human, const void* point);
	std::uint32_t Human_DoExecuteUserCommand_Cheer(void* human, std::uint32_t speakerId, std::uint32_t revision);
	std::uint32_t Human_DoExecuteUserCommand_DetachLearnHouse(void* human);
	void Human_DoExecuteAllPriorityUserCommandsWhichDoNotInterrupt(void* human);
	bool Human_HasEquippedItemType(const void* human, std::uint32_t good, std::int32_t slot);
	void Human_l_FinishTask_Goto(void* human);
	bool Human_l_UserCommand_DetachFromVehicle(const void* human, const void* command);
	std::uint32_t Human_DoExecuteUserCommand_GoAway(void* human, const void* point, std::uint32_t radius);
	std::uint32_t Human_DoExecuteUserCommand_Eat(void* human);
	std::uint32_t Human_DoExecuteUserCommand_Sleep(void* human);
	std::uint32_t Human_DoExecuteUserCommand_Talk(void* human);
	std::uint32_t Human_DoExecuteUserCommand_Pray(void* human);
	std::uint32_t Human_DoExecuteUserCommand_Goodie(void* human, std::int32_t task);
	std::uint32_t Human_DoExecuteUserCommand_AttackHuman(void* human, std::int32_t id, std::uint32_t revision);
	std::uint32_t Human_DoExecuteUserCommand_AttackAnimal(void* human, std::int32_t id, std::uint32_t revision);
	std::uint32_t Human_DoExecuteUserCommand_AttackHouse(void* human, std::int32_t id, std::uint32_t revision);
	std::uint32_t Human_DoExecuteUserCommand_AttackVehicle(void* human, std::int32_t id, std::uint32_t revision);
	std::uint32_t Human_DoExecuteUserCommand_AttackMapPosition(void* human, std::uint32_t packedPoint);
	std::uint32_t Human_DoExecuteUserCommand_MoveInsideVehicle(void* human, std::int32_t id, std::uint32_t revision);

	void Human_l_FinishTask_GotoAndWait(void* human);
}