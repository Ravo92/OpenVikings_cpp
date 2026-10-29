#pragma once

#include <cstdint>

namespace NC2Logic
{
	void __fastcall GameControl_Player_LeavesGame(void* gameControl, void* unusedEdx, unsigned int player, bool originalFlag, bool removeObjects);
	void __fastcall GameControl_Player_LeftGameModifyObjects(void* gameControl, void* unusedEdx, unsigned int player, bool originalFlag, bool removeObjects);
	std::int32_t Logic_EntryPoint_GetDiplomacyState(std::int32_t player, std::int32_t otherPlayer);
	bool GameControl_Player_IsNetworkPlayer(const void* gameControl, std::int32_t player);
	bool Remaster_GameControl_Player_HasDifficultyBenefits(const void* gameControl, std::uint32_t player);
	std::int32_t GameControl_GetPlayerByNetworkId(const void* gameControl, std::uint32_t networkId);
	std::int32_t GameTime_Network_GetPlayerByNetworkId(std::uint32_t networkId);
	bool GameControl_SpecialItem_Use(void* gameControl, std::int32_t player, std::uint32_t type, std::uint32_t item);
	bool GameControl_SpecialItem_Add(void* gameControl, std::int32_t player, std::uint32_t type, std::uint32_t item, const void* position);
	std::int32_t GameControl_Diplomacy_GetState(const void* gameControl, std::int32_t player, std::int32_t otherPlayer);
	const char* GameControl_Names_GetPlayerTribeNamePtr(const void* gameControl, std::int32_t player);
	const char* GameControl_Names_GetPlayerTribeShortNamePtr(const void* gameControl, std::int32_t player);
	void GameControl_Misc_GetPlayerName(const void* gameControl, std::int32_t player, bool shortName, char* output);
	bool __fastcall GameControl_Diplomacy_SetState(void* gameControl, void* unusedEdx, std::uint32_t player, std::uint32_t otherPlayer, std::uint32_t state);
	void GameControl_QueueNetworkPlayerDeparture(void* gameControl, std::uint32_t networkPlayerId, bool originalFlag);
}