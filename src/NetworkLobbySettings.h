#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace NC2MainMenuGuiManager
{
	class CGuiManager;
}

namespace NNetwork::Lobby
{
	// Online lobby adapter; offline setup and native GUI rendering have separate owners.
	inline constexpr std::size_t MessageSize = 524;
	using Message = std::array<std::uint32_t, MessageSize / 4>;
	static_assert(sizeof(Message) == MessageSize);
	void SetControl(NC2MainMenuGuiManager::CGuiManager& gui, std::uint32_t slot, std::uint32_t kind);
	void AutoAssignPlayers(NC2MainMenuGuiManager::CGuiManager& gui);
	Message MakeMessage(const NC2MainMenuGuiManager::CGuiManager& gui, bool start);
	bool ReadMessage(NC2MainMenuGuiManager::CGuiManager& gui, const void* data, std::uint32_t bytes, bool& start);
}