#pragma once
#include <cstdint>

namespace NC2Logic
{
	struct SUserCommandsForHouseHandler
	{
		std::int32_t count;
		std::uint32_t commands[20];
		void Init();
		void AddCommand(std::uint32_t command);
	};

	static_assert(sizeof(SUserCommandsForHouseHandler) == 84);
	bool Logic_UserControl_GetHouseCommands(std::int32_t id, std::int32_t player, SUserCommandsForHouseHandler* commands);
	void House_UserControl_GetCommands(const void* house, std::int32_t player, SUserCommandsForHouseHandler* commands);
	bool House_Construction_CanBeUpgraded(const void* house);
	bool House_CanEnableDefenceMode(const void* house);
	void Tool_GlobalDefendMode_Start(std::uint32_t player);
	void Tool_GlobalDefendMode_Stop(std::uint32_t player);
}