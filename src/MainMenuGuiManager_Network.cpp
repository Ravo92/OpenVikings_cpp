#include "NetworkLobbySettings.h"
#include "GameSetupSettings.h"
#include "C2LifetimeAccess.h"
#include "C2MapGuid.h"
#include "C2ObjectLayout.h"
#include "GuiToolsBase.h"
#include "MainMenuGuiManager.h"
#include "NetworkShutdown.h"
#include "XTextEncoding.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"
#include "XResultIdentifiers.h"

#include <cstring>
#include <cstdio>

namespace NC2MainMenuGuiManager
{
	using namespace NC2Lifetime;
	static constexpr GUID cTcpIpProvider = { 0xEBFE7BA0u, 0x628Du, 0x11D2u,{ 0xAE, 0x0F, 0, 0x60, 0x97, 0xB0, 0x14, 0x11 } };

	// Original function: FUN_004d757e; 0x14-byte summary records have a count cookie.
	static void* DestroySessionSummaries(void* records, std::uint32_t flags)
	{
		auto* bytes = static_cast<std::byte*>(records);
		void* allocation = records;
		if ((flags & 2u) != 0)
		{
			allocation = bytes - NC2Layout::RecordSize::SessionSummaryCountCookie;
			const auto count = Read<std::uint32_t>(allocation, NC2Layout::SessionSummaryArrayCookieOffset::ElementCount);
			auto index = count;
			while (index != 0)
			{
				void* record = bytes + --index * NC2Layout::RecordSize::SessionSummaryUnsigned;
				if (auto* name = Read<char*>(record, NC2Layout::SessionSummaryOffset::Name))
				{
					NXBasics::Runtime_Free(name);
					Write<char*>(record, NC2Layout::SessionSummaryOffset::Name, nullptr);
				}
			}
		}
		else if (auto* name = Read<char*>(records, NC2Layout::SessionSummaryOffset::Name))
		{
			NXBasics::Runtime_Free(name);
			Write<char*>(records, NC2Layout::SessionSummaryOffset::Name, nullptr);
		}
		if ((flags & 1u) != 0) NXBasics::Runtime_Free(allocation);
		return allocation;
	}

	// Original function: FUN_004d70b6; Windows-only provider list, ten 0x78-byte rows.
	bool CGuiManager::Network_EnumerateServiceProviders()
	{
		auto* network = GetNetworkInstance();
		NetworkServiceProviderInfo* providers = nullptr;
		DWORD count = 0;
		if (!network->GetServiceProviders(&providers, &count, false))
		{
			Debug_ErrorString_Add("Cannot get service providers");
			if (providers != nullptr) NXBasics::Runtime_Free(providers);
			return false;
		}
		Write<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::ProviderCount, 0);
		for (std::uint32_t index = 0; index < count; ++index)
		{
			const auto& provider = providers[index];
			if (std::memcmp(&provider.guid, &cTcpIpProvider, sizeof(GUID)) != 0) continue;
			char name[200];
			NNetwork::ConvertNetworkWideToUtf8(name, provider.pwszName, static_cast<std::int32_t>(std::size(name)));
			const auto row = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::ProviderCount);
			auto* destination = reinterpret_cast<std::byte*>(this) + NC2Layout::MainMenuManagerOffset::ProviderRows + row * NC2Layout::RecordSize::MenuNamedGuidRowUnsigned;
			NXBasics::Text_Copy(reinterpret_cast<char*>(destination), 100, name);
			std::memcpy(destination + NC2Layout::MainMenuManagerOffset::ProviderCount, &provider.guid, sizeof(GUID));
			Write(this, NC2Layout::MainMenuManagerOffset::ProviderCount, row + 1u);
			if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::ProviderCount) == 10) break;
		}
		if (providers != nullptr) NXBasics::Runtime_Free(providers);
		return true;
	}

	// Original function: FUN_004d71f5.
	bool CGuiManager::Network_HostDefaultSession()
	{
		char name[100]{};
		std::snprintf(name, sizeof(name), "%.45s - %.39s - V1.00", NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xB4), NNetwork::Network_GetCallsign());
		return Network_HostSession(name, 10);
	}

	// Original function: FUN_004d7286; Windows CDPNet host data has a 0x20-byte layout.
	bool CGuiManager::Network_HostSession(const char* sessionName, std::uint32_t maximumPlayers)
	{
		const char* error;
		if (!Network_SelectedProviderIsTcpIp()) error = "Host start session - no service provider selected";
		else
		{
			auto* network = GetNetworkInstance();
			wchar_t callsign[200]; NNetwork::ConvertNetworkUtf8ToWide(callsign, NNetwork::Network_GetCallsign(), static_cast<std::int32_t>(std::size(callsign)));
			wchar_t name[200]; NNetwork::ConvertNetworkUtf8ToWide(name, sessionName, static_cast<std::int32_t>(std::size(name)));
			NNetwork::SHostSessionData hostData; std::memset(&hostData, 0, sizeof(hostData)); std::memset(&hostData, 0, sizeof(hostData));
			hostData.maximumPlayers = maximumPlayers; hostData.sessionName = name; hostData.password = nullptr;
			hostData.clientServer = 1; hostData.application = NNetwork::gNetworkApplicationGuid;
			const auto* provider = reinterpret_cast<const GUID*>(reinterpret_cast<const std::byte*>(this) + NC2Layout::MainMenuManagerOffset::FirstProviderGuid + Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedProviderIndex) * NC2Layout::RecordSize::MenuNamedGuidRowUnsigned);
			if (!network->HostSession(provider, callsign, &hostData, false)) error = "Host start session - could not host session";
			else if (!NNetwork::AttachDynamicNetwork(&gDynamicNetworkData, network)) error = "Host start session - could not attach to network";
			else
			{
				auto* destination = reinterpret_cast<char*>(this) + NC2Layout::MainMenuManagerOffset::SessionName;
				NXBasics::Text_Copy(destination, 100, sessionName);
				return true;
			}
		}
		Debug_ErrorString_Add(error);
		return false;
	}

	// Original function: FUN_004d73e5; the empty-address branch tests != 0, not SUCCEEDED.
	bool CGuiManager::Network_StartSessionSearch(const char* address)
	{
		const char* error;
		if (!Network_SelectedProviderIsTcpIp()) error = "Join session - no service provider selected";
		else
		{
			auto* network = GetNetworkInstance();
			if (address != nullptr && std::strlen(address) != 0)
			{
				wchar_t wideAddress[102]; NNetwork::ConvertNetworkUtf8ToWide(wideAddress, address, static_cast<std::int32_t>(std::size(wideAddress)));
				const auto* provider = reinterpret_cast<const GUID*>(reinterpret_cast<const std::byte*>(this) + NC2Layout::MainMenuManagerOffset::FirstProviderGuid + Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedProviderIndex) * NC2Layout::RecordSize::MenuNamedGuidRowUnsigned);
				const auto result = network->StartSearchingSessions(provider, &NNetwork::gNetworkApplicationGuid, wideAddress);
				if (result >= 0) return true;
				error = result == static_cast<HRESULT>(SwitchIds::DirectXResult::Code_DPNERR_ADDRESSING) ? "Join session - passed invalid host name" : "Join session - could not start searching for sessions";
			}
			else
			{
				const auto* provider = reinterpret_cast<const GUID*>(reinterpret_cast<const std::byte*>(this) + NC2Layout::MainMenuManagerOffset::FirstProviderGuid + Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedProviderIndex) * NC2Layout::RecordSize::MenuNamedGuidRowUnsigned);
				if (SUCCEEDED(network->StartSearchingSessions(provider, &NNetwork::gNetworkApplicationGuid, nullptr))) return true;
				error = "Join session - could not start searching for sessions";
			}
		}
		Debug_ErrorString_Add(error);
		return false;
	}

	// Original function: FUN_004d75eb; the copied session is released on both join paths.
	bool CGuiManager::Network_JoinSession(std::uint32_t index)
	{
		if (index >= Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SessionCount) || index >= 10) return false;
		auto* network = GetNetworkInstance();
		wchar_t callsign[200]; NNetwork::ConvertNetworkUtf8ToWide(callsign, NNetwork::Network_GetCallsign(), static_cast<std::int32_t>(std::size(callsign)));
		const auto offset = index * NC2Layout::RecordSize::MenuNamedGuidRowUnsigned;
		const auto* guid = reinterpret_cast<const GUID*>(reinterpret_cast<const std::byte*>(this) + offset + NC2Layout::MainMenuManagerOffset::FirstSessionGuid);
		auto* session = NNetwork::CopySessionNode(network->sessions, guid);
		const bool joined = network->JoinSession(callsign, session);
		if (!joined) Debug_ErrorString_Add("Join select session - could not join");
		if (session != nullptr)
		{
			ReleaseSessionNodeResources(session); NXBasics::Runtime_Free(session);
		}
		if (!joined) return false;
		if (!NNetwork::AttachDynamicNetwork(&gDynamicNetworkData, network))
		{
			Debug_ErrorString_Add("Join select session - could not attach to network");
			return false;
		}
		auto* destination = reinterpret_cast<char*>(sTheObjectPtr) + NC2Layout::MainMenuManagerOffset::SessionName;
		const auto* name = reinterpret_cast<const char*>(this) + offset + NC2Layout::MainMenuManagerOffset::SessionRows;
		NXBasics::Text_Copy(destination, 100, name);
		return true;
	}

	// Original function: FUN_004d74a4; false means no changed session list.
	bool CGuiManager::Network_UpdateSessionList()
	{
		auto* network = GetNetworkInstance();
		network->Pump();
		auto* sessions = network->sessions;
		if (sessions == nullptr) return false;
		const auto changed = sessions->flags;
		sessions->flags = 0;
		if (changed == 0) return false;
		auto* storage = reinterpret_cast<std::byte*>(this) + NC2Layout::MainMenuManagerOffset::SessionCount;
		std::memset(storage, 0, NC2Layout::RecordSize::MenuSessionList);
		NNetwork::SSessionSummary* records = nullptr;
		DWORD count = 0;
		NNetwork::GetSessionSummaries(sessions, &records, &count);
		const auto shown = (count < 10u ? count : 10u);
		Write(this, NC2Layout::MainMenuManagerOffset::SessionCount, shown);
		for (DWORD index = 0; index < shown; ++index)
		{
			auto* record = reinterpret_cast<std::byte*>(records) + index * NC2Layout::RecordSize::SessionSummaryUnsigned;
			char name[200];
			NNetwork::ConvertNetworkWideToUtf8(name, Read<wchar_t*>(record, NC2Layout::SessionSummaryOffset::Name), static_cast<std::int32_t>(std::size(name)));
			auto* destination = storage + NC2Layout::MenuNetworkListOffset::Rows + index * NC2Layout::RecordSize::MenuNamedGuidRowUnsigned;
			NXBasics::Text_Copy(reinterpret_cast<char*>(destination), 100, name);
			std::memcpy(destination + NC2Layout::MainMenuManagerOffset::ProviderCount, record + NC2Layout::SessionSummaryOffset::InstanceGuid, sizeof(GUID));
		}
		if (records != nullptr) DestroySessionSummaries(records, 3);
		return true;
	}

	// Original function: FUN_004d76cc.
	bool CGuiManager::Network_UpdatePlayerList()
	{
		auto* network = GetNetworkInstance();
		std::memset(reinterpret_cast<std::byte*>(this) + NC2Layout::MainMenuManagerOffset::PlayerCount, 0, NC2Layout::RecordSize::MenuPlayerListWithSelection);
		NNetwork::SPlayerInfo* players = nullptr;
		DWORD count = 0;
		if (!network->GetPlayerInfo(&players, &count))
		{
			Debug_ErrorString_Add("Update player list - could not get player info");
			return false;
		}
		const auto shown = (count < 10u ? count : 10u);
		Write(this, NC2Layout::MainMenuManagerOffset::PlayerCount, shown);
		for (DWORD index = 0; index < shown; ++index)
		{
			char name[200];
			NNetwork::ConvertNetworkWideToUtf8(name, players[index].playerName, static_cast<std::int32_t>(std::size(name)));
			auto* destination = reinterpret_cast<std::byte*>(this) + NC2Layout::MainMenuManagerOffset::PlayerRows + index * NC2Layout::MainMenuManagerOffset::ProviderRowsUnsigned;
			NXBasics::Text_Copy(reinterpret_cast<char*>(destination), 100, name);
			Write(destination, NC2Layout::MenuNetworkPlayerOffset::HostFlag, players[index].isHost);
			Write(destination, NC2Layout::MainMenuManagerOffset::ProviderCount, players[index].playerId);
		}
		NXBasics::Runtime_Free(players);
		Write(this, NC2Layout::MainMenuManagerOffset::PlayerChangeCount, network->playerChangeCount);
		const auto selected = Read<NetworkPlayerId>(this, NC2Layout::MainMenuManagerOffset::SelectedPlayerId);
		if (selected != 0)
		{
			DWORD index = 0;
			for (; index < Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::PlayerCount); ++index)
				if (Read<NetworkPlayerId>(this, NC2Layout::MainMenuManagerOffset::FirstPlayerId + index * NC2Layout::MainMenuManagerOffset::ProviderRowsUnsigned) == selected) break;
			if (index == Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::PlayerCount)) Write<NetworkPlayerId>(this, NC2Layout::MainMenuManagerOffset::SelectedPlayerId, 0);
		}
		return true;
	}

	// Original function: FUN_004d782f.
	bool CGuiManager::Network_PlayerListHasChanged() const
	{
		auto* network = GetNetworkInstance();
		network->Pump();
		return Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::PlayerChangeCount) != network->playerChangeCount;
	}

	// Original function: FUN_004d7849; no bounds check on the selected index.
	bool CGuiManager::Network_SelectedProviderIsTcpIp() const
	{
		const auto selected = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SelectedProviderIndex);
		if (selected >= 10 || selected >= Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::ProviderCount)) return false;
		const auto address = reinterpret_cast<std::uintptr_t>(this) + selected * NC2Layout::RecordSize::MenuNamedGuidRowUnsigned + NC2Layout::MainMenuManagerOffset::FirstProviderGuidUnsigned;
		if (address == 0) return false;
		return std::memcmp(reinterpret_cast<const void*>(address), &cTcpIpProvider, sizeof(GUID)) == 0;
	}

	// Original function: FUN_004d7aa7.
	void CGuiManager::Network_SendLobbyData()
	{
		if (NRemaster::GameSetup::IsLocal(*this)) return;
		if (!NNetwork::Network_IsHost()) return;
		const auto message = NNetwork::Lobby::MakeMessage(*this, false);
		NNetwork::SendDynamicNetworkMessage(&gDynamicNetworkData, 0, message.data(), sizeof(message));
	}

	// Original function: FUN_004d7b06; distinct tail-forwarding entry.
	void CGuiManager::Network_InitializeCallsign()
	{
		NNetwork::Network_InitializeCallsign();
	}

	// Original function: FUN_004d7b0b.
	bool CGuiManager::Network_AutoHost()
	{
		const char* error = nullptr;
		if (!StartNetworkForMenu(this)) error = "Could not init multi player module";
		else
		{
			NNetwork::Network_InitializeCallsign();
			Chat_ClearAll();
			if (!Network_EnumerateServiceProviders()) error = "Could not enumerate service provider";
			else
			{
				std::uint32_t index = 0;
				for (; index < Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::ProviderCount); ++index)
					if (std::memcmp(reinterpret_cast<std::byte*>(this) + NC2Layout::MainMenuManagerOffset::FirstProviderGuid + index * NC2Layout::RecordSize::MenuNamedGuidRowUnsigned, &cTcpIpProvider, sizeof(GUID)) == 0) break;
				if (index == Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::ProviderCount)) error = "No TCPIP service provider found";
				else
				{
					Write(this, NC2Layout::MainMenuManagerOffset::SelectedProviderIndex, index);
					if (!Network_HostDefaultSession()) error = "Could not host a session";
				}
			}
		}
		if (error != nullptr)
		{
			Debug_ErrorString_Add(error); return false;
		}
		return true;
	}

	// Original function: FUN_004d7ba7; yield while the worker discovers sessions.
	bool CGuiManager::Network_AutoJoin(const char* address)
	{
		const char* error = nullptr;
		if (!StartNetworkForMenu(this)) error = "Could not init multi player module";
		else
		{
			NNetwork::Network_InitializeCallsign();
			Chat_ClearAll();
			if (!Network_EnumerateServiceProviders()) error = "Could not enumerate service providers";
			else
			{
				std::uint32_t index = 0;
				for (; index < Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::ProviderCount); ++index)
					if (std::memcmp(reinterpret_cast<std::byte*>(this) + NC2Layout::MainMenuManagerOffset::FirstProviderGuid + index * NC2Layout::RecordSize::MenuNamedGuidRowUnsigned, &cTcpIpProvider, sizeof(GUID)) == 0) break;
				if (index == Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::ProviderCount)) error = "No TCPIP service provider found";
				else
				{
					Write(this, NC2Layout::MainMenuManagerOffset::SelectedProviderIndex, index);
					if (!Network_StartSessionSearch(address)) error = "Could not join host";
					else
					{
						const DWORD start = timeGetTime();
						for (;;)
						{
							if (Network_UpdateSessionList() && Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::SessionCount) > 0)
							{
								if (!Network_JoinSession(0)) error = "Could not join session on host - failed";
								break;
							}
							if (timeGetTime() - start > 10000u)
							{
								error = "Could not join session on host - timeout"; break;
							}
							Sleep(10);
						}
					}
				}
			}
		}
		if (error != nullptr)
		{
			Debug_ErrorString_Add(error); return false;
		}
		return true;
	}

	// Original function: FUN_004d7c95.
	void CGuiManager::Chat_ClearAll()
	{
		std::memset(chatHistoryStorage, 0, NC2Layout::RecordSize::MenuChatHistory);
	}

	// Original function: FUN_004d7d56.
	const char* CGuiManager::Network_GetPlayerName(std::uint32_t playerId) const
	{
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::PlayerCount); ++index)
			if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::FirstPlayerId + index * NC2Layout::MainMenuManagerOffset::ProviderRowsUnsigned) == playerId) return reinterpret_cast<const char*>(this) + NC2Layout::MainMenuManagerOffset::PlayerRows + index * NC2Layout::MainMenuManagerOffset::ProviderRowsUnsigned;
		return nullptr;
	}

	// Original function: FUN_004d7db7.
	void CGuiManager::Network_FormatPlayerDescription(std::uint32_t index, char* destination) const
	{
		auto* network = GetNetworkInstance();
		const auto offset = index * NC2Layout::MainMenuManagerOffset::ProviderRowsUnsigned;
		const auto id = Read<NetworkPlayerId>(this, NC2Layout::MainMenuManagerOffset::FirstPlayerId + offset);
		std::strcpy(destination, reinterpret_cast<const char*>(this) + NC2Layout::MainMenuManagerOffset::PlayerRows + offset);
		if (Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FirstPlayerHostFlag + offset) != 0)
		{
			std::strcat(destination, " (");
			std::strcat(destination, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xBA));
			std::strcat(destination, ")");
		}
		if (id != NNetwork::Network_GetLocalPlayerId())
		{
			NetworkConnectionInfo connection;
			if (network->GetConnectionData(id, &connection))
			{
				char latency[20];
				NXBasics::Runtime_PrintString(latency, " (%dms)", connection.dwRoundTripLatencyMS);
				std::strcat(destination, latency);
			}
		}
	}

	// Original function: FUN_004d8169; mode is read after the initial clear.
	void CGuiManager::Network_ClearLobbyData()
	{
		NRemaster::GameSetup::Reset();
		std::memset(lobbyData, 0, NC2Layout::RecordSize::MenuLobby);
		for (std::uint32_t index = 0; index < 20; ++index)
		{
			auto* row = reinterpret_cast<std::byte*>(this) + NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + index * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned;
			Write<std::uint32_t>(row, NC2Layout::MenuLobbyPlayerOffset::NetworkPlayerId, 0);
			Write<std::uint32_t>(row, NC2Layout::MenuLobbyPlayerOffset::Type, 0);
			Write<std::uint32_t>(row, NC2Layout::MenuLobbyPlayerOffset::Options, 0);
			Write(row, NC2Layout::MenuLobbyPlayerOffset::Color, Read<std::uint8_t>(this, NC2Layout::MainMenuManagerOffset::FixedPlayerColors) != 0 ? (index < 9u ? index : 9u) : 0u);
		}
		Screen_UpdateGui();
		Network_SendLobbyData();
	}

	// Original function: FUN_004d7899.
	bool CGuiManager::Network_AllowsConnections() const
	{
		return GetNetworkInstance()->allowConnections != 0;
	}

	// Original function: FUN_004d79a7; validates the twenty lobby player rows.
	bool CGuiManager::Network_CanStartGame(char* errorText) const
	{
		if (errorText != nullptr) errorText[0] = 0;
		auto reject = [&] (std::uint32_t message)
		{
			if (errorText != nullptr) std::strcpy(errorText, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(message));
			return false;
		};
		const auto mode = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbyMode);
		if (mode == 0) return reject(0xB5);
		if (mode == 1 && !reinterpret_cast<const NC2Logic::SCleanMapGuid*>(lobbyData + NC2Layout::MenuLobbyOffset::MapGuid)->IsValid()) return reject(0xB6);
		std::uint32_t index = 0;
		for (; index < 20; ++index) if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + index * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned) != 0) break;
		if (index == 20) return reject(0xB7);
		for (index = 0; index < 20; ++index)
		{
			const auto flags = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::FirstLobbyPlayerOptions + index * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned);
			if (flags != 0 && (flags & 1) == 0 && Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::FirstLobbyPlayerType + index * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned) == 0)
			{
				if (errorText != nullptr) NXBasics::Runtime_PrintString(errorText, NC2GuiToolsBase::StringTool_GetMainMenuStringPtr(0xB8), index);
				return false;
			}
		}
		for (index = 0; index < 20; ++index) if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::LobbyPlayerRows + index * NC2Layout::RecordSize::MenuLobbyPlayerRowUnsigned) == NNetwork::Network_GetLocalPlayerId()) return true;
		return reject(0xB9);
	}

	// Original function: FUN_004d7d8d.
	std::uint32_t CGuiManager::Network_GetPlayerListIndex(std::uint32_t playerId) const
	{
		const auto count = Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::PlayerCount);
		for (std::uint32_t index = 0; index < count; ++index) if (Read<std::uint32_t>(this, NC2Layout::MainMenuManagerOffset::FirstPlayerId + index * NC2Layout::MainMenuManagerOffset::ProviderRowsUnsigned) == playerId) return index;
		return NXBasics::IntegerRange::UInt32Maximum;
	}
}
