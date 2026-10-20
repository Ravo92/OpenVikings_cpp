#include "NetworkLobbySettings.h"
#include "GameSetupSettings.h"
#include "GameSetupStartup.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "MainMenuGuiManager.h"
#include "NetworkShutdown.h"
#include <algorithm>
#include <cstring>

namespace NNetwork::Lobby
{
	using namespace NC2Lifetime;
	using NC2MainMenuGuiManager::CGuiManager;
	using namespace NRemaster::GameSetup;

	namespace { constexpr std::uint32_t ExtensionMagic = 0x59424F4C; }
	void SetControl(CGuiManager& gui, std::uint32_t slot, std::uint32_t kind)
	{
		if (slot >= 20 || kind > 2 || (Allowed(gui, slot) & (1u << kind)) == 0) return;
		if (kind == 1)
		{
			if (Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::SelectedPlayerId) == 0) Write(&gui, NC2Layout::MainMenuManagerOffset::SelectedPlayerId, Network_GetLocalPlayerId());
			SetSelectedTribe(slot, 0);
			gui.Network_AssignSelectedPlayer(slot);
			return;
		}
		if (kind == 2 && Read<std::uint32_t>(&gui, Row(slot) + 4) == 0 && Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::LobbySavegameId) == 0) Write(&gui, Row(slot) + 12, Read<std::uint8_t>(&gui, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? (std::min)(slot, 9u) : gui.Network_FindNextPlayerColor(UINT32_MAX));
		Write(&gui, Row(slot), std::uint32_t{0});
		Write(&gui, Row(slot) + 4, kind);
		if (kind == 0) SetSelectedTribe(slot, 0);
		gui.Screen_UpdateGui();
		gui.Network_SendLobbyData();
	}
	void AutoAssignPlayers(CGuiManager& gui)
	{
		if (!Network_IsHost() || Read<std::uint32_t>(gui.lobbyData, 0) == 0) return;
		const auto oldSelection = Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::SelectedPlayerId);
		const auto count = (std::min)(10u, Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::PlayerCount));
		for (std::uint32_t index = 0; index < count; ++index)
		{
			const auto id = Read<std::uint32_t>(&gui, NC2Layout::MainMenuManagerOffset::FirstPlayerId + index * NC2Layout::RecordSize::MenuNetworkPlayerUnsigned);
			if (id == 0) continue;
			bool assigned = false;
			for (std::uint32_t slot = 0; slot < 20; ++slot) if (Read<std::uint32_t>(&gui, Row(slot)) == id) assigned = true;
			if (assigned) continue;
			for (unsigned pass = 0; pass < 2 && !assigned; ++pass)
				for (std::uint32_t slot = 0; slot < 20; ++slot)
				{
					if ((Allowed(gui, slot) & 2) == 0 || Read<std::uint32_t>(&gui, Row(slot)) != 0 || (pass == 0 && Read<std::uint32_t>(&gui, Row(slot) + 4) == 2)) continue;
					Write(&gui, NC2Layout::MainMenuManagerOffset::SelectedPlayerId, id);
					SetSelectedTribe(slot, 0); gui.Network_AssignSelectedPlayer(slot); assigned = true; break;
				}
		}
		Write(&gui, NC2Layout::MainMenuManagerOffset::SelectedPlayerId, oldSelection != 0 && gui.Network_GetPlayerListIndex(oldSelection) != UINT32_MAX ? oldSelection : Network_GetLocalPlayerId());
		gui.Screen_UpdateGui();
	}
	Message MakeMessage(const CGuiManager& gui, bool start)
	{
		Message message{};
		message[0] = 1; message[1] = start ? 1u : 0u;
		std::memcpy(reinterpret_cast<std::byte*>(message.data()) + 8, gui.lobbyData, sizeof(gui.lobbyData));
		message[90] = ExtensionMagic;
		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			message[91 + slot] = GetChoices().mapTribes[slot];
			message[111 + slot] = Customizable(gui, slot) && Read<std::uint32_t>(&gui, Row(slot) + 4) == 2 ? GetChoices().tribes[slot] : 0;
		}
		return message;
	}
	bool ReadMessage(CGuiManager& gui, const void* data, std::uint32_t bytes, bool& start)
	{
		if (data == nullptr || bytes != MessageSize) return false;
		Message message; std::memcpy(message.data(), data, sizeof(message));
		if (message[0] != 1 || message[1] > 1 || message[2] > 2 || message[90] != ExtensionMagic) return false;
		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			const auto tribe = message[111 + slot], base = message[91 + slot];
			const auto flags = message[8 + slot * 4 + 2], kind = message[8 + slot * 4 + 1];
			if (base > 7 || kind > 2 || message[8 + slot * 4 + 3] >= 10 || (tribe != 0 && (!Playable(tribe) || !Playable(base) || message[2] != 1 || kind != 2 || (flags & 2) == 0 || (flags & 0x80000000u) != 0))) return false;
		}
		if (!Network_IsHost())
		{
			std::memcpy(gui.lobbyData, message.data() + 2, sizeof(gui.lobbyData));
			for (std::uint32_t slot = 0; slot < 20; ++slot) { SetMapTribe(slot, message[91 + slot]); SetSelectedTribe(slot, message[111 + slot]); }
		}
		start = message[1] != 0;
		if (start)
		{
			SPlayerChoices startupChoices;
			for (std::uint32_t slot = 0; slot < 20; ++slot) { startupChoices.mapTribes[slot] = message[91 + slot]; startupChoices.tribes[slot] = message[111 + slot]; }
			Startup::Prepare(message.data() + 3, startupChoices, message[2] == 1);
		}
		return true;
	}
}
