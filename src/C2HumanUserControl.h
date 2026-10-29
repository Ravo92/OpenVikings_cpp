#pragma once
#include <cstdint>

namespace NC2Logic
{
	struct SUserCommandsForHumanHandler
	{
		std::int32_t count;
		std::uint32_t commands[25];
		void Init();
		void AddCommand(std::uint32_t command);
		bool IsCommandAllowed(std::int32_t command) const;
		void SortCommandsByPriority();
	};

	static_assert(sizeof(SUserCommandsForHumanHandler) == 104);
	std::uint32_t Tool_UserHumanCommandTypes_GetPriority(std::uint32_t command);
	std::uint32_t Tool_UserHumanCommandTypes_GetGroupType(std::int32_t command);
	bool Logic_UserControl_GetHumanCommands(std::int32_t id, SUserCommandsForHumanHandler* commands, bool parameter2, bool parameter3);
	void Human_UserControl_GetCommands(const void* human, SUserCommandsForHumanHandler* commands, bool parameter2, bool parameter3);
	bool Human_IsAbleToHaveWorkHouse(const void* human);
	bool Human_UserControl_CanChangeJob(const void* human);
	bool Human_UserControl_CanChooseJob(const void* human);
}