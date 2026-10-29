#pragma once

#include <cstdint>

namespace NC2Logic
{
	bool Human_IsWalking(const void* human);
	bool Human_l_Item_IsGoodTypeEquipped(const void* human, std::uint32_t good);
	bool Human_Item_IsAmuletEquipped_Speed(const void* human);
	std::int32_t Human_WalkSpeed_GetMoveSpeed(const void* human, std::uint32_t resistance);
	void Human_WalkSpeed_Update(void* human, std::uint32_t resistance);
	bool Human_AI_DoNewMapPositionReached(void* human, bool destinationReached);
	bool Human_AI_DoNewMapPositionReached_DestinationNotReached(void* human);
	void GameAssistantSingleHumanData_System_Init(void* data);
	void GameAssistantSingleHumanData_CommandHistory_Add(void* data, std::uint32_t command, std::uint32_t parameter1, std::uint32_t parameter2, std::uint32_t duration);
	bool GameAssistantSingleHumanData_CommandHistory_IsInArray(void* data, std::uint32_t command);
	bool GameAssistantSingleHumanData_CommandHistory_IsInArray(void* data, std::uint32_t command, std::uint32_t parameter1, std::uint32_t parameter2);
	void GameAssistantSingleHumanData_CommandHistory_Validate(void* data);
	bool GameAssistantSingleHumanData_CommandHistory_IsEquipCommandInArray(void* data);
	void Map_AnimalTool_ChangePlayerId(std::uint32_t player, const void* point);
	void Map_AnimalTool_ChangePlayerId(std::uint32_t player, std::uint32_t point, std::uint32_t radius);
	void Animal_l_DetachLeader(void* animal);
	void Animal_ChangePlayerId(void* animal, std::uint32_t player);
	void Tool_Map_CheckUserInformationAboutEnemies(std::uint32_t player, const void* point);
	bool Tool_Map_GetNotBlockedPosition(void* point, bool avoidHouses);
	bool Human_FindNextFreeMapPoint(const void* human, void* point);
	bool Human_FindNextVehicleFreeMapPoint(const void* human, void* point, std::uint32_t minimumDistance);
}