#include <winsock2.h>
#include <ws2tcpip.h>
#include "NetworkShutdown.h"
#include "FileTransferManager.h"
#include "XCrtFileAccess.h"

#include <algorithm>
#include <cstring>
#include <new>

namespace NNetwork
{
	namespace
	{
		std::uint32_t PacketWord(const std::uint8_t* bytes) { std::uint32_t result; std::memcpy(&result, bytes, sizeof(result)); return result; }

		bool SameSessions(SSessionList* list, const std::vector<ModernSession>& sessions)
		{
			if (list->count != sessions.size()) return false;
			auto* node = list->first;
			for (const auto& session : sessions)
			{
				if (node == nullptr || std::memcmp(&node->description->guidInstance, &session.instance, sizeof(GUID)) != 0 || std::wcscmp(node->description->pwszSessionName, session.name.c_str()) != 0 || node->description->dwCurrentPlayers != session.currentPlayers || node->address0C->host != session.address.host || node->address0C->port != session.address.port) return false;
				node = node->next;
			}
			return true;
		}
	}

	// Modern replacements of the former 004DC45F..004DDA6F transport boundary.
	bool CDPNet::Initialize(void* context, bool option)
	{
		(void)option;
		if (peer != nullptr) return true;
		callbackContext = context;
		try { peer = new CModernNetwork; } catch (...) { return false; }
		if (!peer->Ready()) { delete peer; peer = nullptr; return false; }
		flags = 1; initializationTick = GetTickCount(); return true;
	}
	bool CDPNet::InitializePeer(void* context, bool option) { return Initialize(context, option); }

	CDPNet::~CDPNet()
	{
		Uninitialize();
		DeleteCriticalSection(&receivedMessagesCriticalSection);
		DeleteCriticalSection(&controlPacketCriticalSection);
		DeleteCriticalSection(&sendStatisticsCriticalSection);
		DeleteCriticalSection(&playerEventsCriticalSection);
		DeleteCriticalSection(&criticalBC);
		DeleteCriticalSection(&sessionSearchCriticalSection);
	}

	bool CDPNet::Uninitialize()
	{
		if (peer != nullptr) { peer->Leave(); delete peer; peer = nullptr; }
		StopSearchingSessions();
		ReleaseReceivedMessages(receivedMessages, receivedMessageCount); receivedMessages = nullptr; receivedMessageCount = 0;
		ClearEventQueue(&playerEvents); ClearEventQueue(&events28);
		std::memset(&controlPacketReceiveState, 0, sizeof(controlPacketReceiveState));
		std::memset(sendStatisticsStorage, 0, sizeof(sendStatisticsStorage));
		flags = 0; return true;
	}

	bool CDPNet::LeaveSession()
	{
		if (peer == nullptr) return false;
		peer->Leave();
		ReleaseReceivedMessages(receivedMessages, receivedMessageCount); receivedMessages = nullptr; receivedMessageCount = 0;
		ClearEventQueue(&playerEvents); ClearEventQueue(&events28);
		flags = 1; playerInfoChanged = 1; ++playerChangeCount; return true;
	}

	bool CDPNet::StopSearchingSessions()
	{
		if (peer != nullptr) peer->StopSearch();
		if (sessions != nullptr) { DestroySessionListContents(sessions); NXBasics::Runtime_Free(sessions); sessions = nullptr; }
		searchOperation = 0; flags &= ~4u; return true;
	}

	NetworkPlayerId CDPNet::GetLocalPlayerId() { return peer != nullptr ? peer->LocalPlayer() : 0; }
	NetworkPlayerId CDPNet::GetHostPlayerId() { return peer != nullptr ? peer->HostPlayer() : 0; }
	bool CDPNet::GetConnectionData(NetworkPlayerId player, NetworkConnectionInfo* output) { return peer != nullptr && output != nullptr && peer->ConnectionInfo(player, *output); }

	bool CDPNet::GetPlayerIds(NetworkPlayerId** output, DWORD* count)
	{
		if (output == nullptr || count == nullptr) return false;
		*output = nullptr; *count = 0; if (peer == nullptr) return false;
		const auto players = peer->Players();
		if (players.empty()) return true;
		auto* records = static_cast<NetworkPlayerId*>(NXBasics::Runtime_AllocateWithNewHandler(players.size() * sizeof(NetworkPlayerId), 1));
		if (records == nullptr) return false;
		for (std::size_t i = 0; i < players.size(); ++i) records[i] = players[i].playerId;
		*output = records; *count = static_cast<DWORD>(players.size()); return true;
	}

	bool CDPNet::GetPlayerInfo(SPlayerInfo** output, DWORD* count)
	{
		if (output == nullptr || count == nullptr) return false;
		*output = nullptr; *count = 0; if (peer == nullptr) return false;
		auto players = peer->Players();
		if (players.empty()) return true;
		for (auto& player : players)
		{
			NetworkConnectionInfo info{};
			if (peer->ConnectionInfo(player.playerId, info)) { player.bytesSent = info.dwBytesSentGuaranteed; player.bytesReceived = info.dwBytesReceivedGuaranteed; player.roundTripLatencyMS = info.dwRoundTripLatencyMS; player.throughputBPS = info.dwThroughputBPS; }
		}
		auto* records = static_cast<SPlayerInfo*>(NXBasics::Runtime_AllocateWithNewHandler(players.size() * sizeof(SPlayerInfo), 1));
		if (records == nullptr) return false;
		std::memcpy(records, players.data(), players.size() * sizeof(SPlayerInfo));
		*output = records; *count = static_cast<DWORD>(players.size()); playerInfoChanged = 0; return true;
	}

	bool CDPNet::GetPlayerName(NetworkPlayerId player, wchar_t** output)
	{
		if (output == nullptr) return false;
		*output = nullptr; if (peer == nullptr) return false;
		for (const auto& entry : peer->Players()) if (entry.playerId == player) { *output = NXBasics::Runtime_DuplicateWideString(entry.playerName); return *output != nullptr; }
		return false;
	}

	bool CDPNet::GetServiceProviders(NetworkServiceProviderInfo** output, DWORD* count, bool allProviders)
	{
		(void)allProviders;
		if (output == nullptr || count == nullptr) return false;
		*output = nullptr; *count = 0;
		constexpr wchar_t name[] = L"OpenVikings TCP/IP";
		auto* memory = static_cast<std::uint8_t*>(NXBasics::Runtime_AllocateWithNewHandler(sizeof(NetworkServiceProviderInfo) + sizeof(name), 1));
		if (memory == nullptr) return false;
		std::memset(memory, 0, sizeof(NetworkServiceProviderInfo) + sizeof(name));
		auto* provider = reinterpret_cast<NetworkServiceProviderInfo*>(memory);
		provider->guid = TcpIpProvider; provider->pwszName = reinterpret_cast<wchar_t*>(memory + sizeof(*provider)); std::memcpy(provider->pwszName, name, sizeof(name));
		*output = provider; *count = 1; return true;
	}

	bool CDPNet::HostSession(const GUID* provider, const wchar_t* callsign, const SHostSessionData* description, bool queryAddressing)
	{
		(void)queryAddressing;
		if (peer == nullptr || flags != 1 || provider == nullptr || std::memcmp(provider, &TcpIpProvider, sizeof(GUID)) != 0 || description == nullptr || description->password != nullptr) return false;
		if (!peer->Host(description->application, description->sessionName, callsign, description->maximumPlayers)) return false;
		allowConnections = 1; flags = 3; Pump(); return true;
	}

	HRESULT CDPNet::StartSearchingSessions(const GUID* provider, const GUID* application, const wchar_t* hostName)
	{
		if (peer == nullptr || flags != 1 || provider == nullptr || application == nullptr || std::memcmp(provider, &TcpIpProvider, sizeof(GUID)) != 0) return E_INVALIDARG;
		if (!peer->Search(*application, hostName)) return HRESULT_FROM_WIN32(peer->LastError() != 0 ? peer->LastError() : WSAEINVAL);
		sessions = static_cast<SSessionList*>(NXBasics::Runtime_AllocateWithNewHandler(sizeof(SSessionList), 1));
		if (sessions == nullptr) { peer->Leave(); return E_OUTOFMEMORY; }
		*sessions = {}; sessions->flags = 1; searchOperation = 1; flags |= 4; return S_OK;
	}

	bool CDPNet::JoinSession(const wchar_t* callsign, const SSessionNode* session)
	{
		if (peer == nullptr || (flags & 4) == 0 || session == nullptr || session->description == nullptr || session->address0C == nullptr) return false;
		ModernSession description; description.instance = session->description->guidInstance; description.application = session->description->guidApplication; description.address = *session->address0C;
		const bool joined = peer->Join(description, callsign);
		StopSearchingSessions(); flags = joined ? 9 : 1; allowConnections = joined ? 1 : 0; if (joined) Pump(); return joined;
	}

	bool CDPNet::SetAllowConnections(bool allow)
	{
		if (peer == nullptr || (flags & 2) == 0) return false;
		allowConnections = allow; peer->AllowConnections(allow);
		const std::uint8_t packet[2] = { 2, static_cast<std::uint8_t>(allow) }; return SendPacket(0, packet);
	}
	bool CDPNet::KickPlayer(NetworkPlayerId player) { return peer != nullptr && peer->Kick(player); }
	bool CDPNet::HasPlayerConnectionFailed(NetworkPlayerId player) { NetworkConnectionInfo info{}; return peer == nullptr || !peer->ConnectionInfo(player, info); }

	bool CDPNet::SendPacket(NetworkPlayerId recipient, const void* source)
	{
		if (peer == nullptr || source == nullptr) return false;
		const auto* packet = static_cast<const std::uint8_t*>(source);
		std::uint32_t size = 0; bool noLoopback = false;
		std::uint8_t converted[212]{};
		switch (packet[0])
		{
			case 2: size = 2; noLoopback = true; break;
			case 3:
				for (unsigned i = 0; i < 128; ++i) if (packet[2 + i * 2] == 0 && packet[3 + i * 2] == 0) { size = i * 2 + 4; break; }
				break;
			case 4: if (PacketWord(packet + 4) > 1024) return false; size = PacketWord(packet + 4) + 12; break;
			case 5: if (packet[8] > 200) return false; size = packet[8] + 12; noLoopback = true; break;
			case 6: size = 12; noLoopback = true; break;
			case 7: size = 2; noLoopback = true; break;
			case 8: size = 2; break;
			case 9: size = 16; break;
			case 10:
				if (packet[8] > 200) return false;
				converted[0] = 5; std::memcpy(converted + 4, packet + 4, 4); converted[8] = packet[8]; std::memcpy(converted + 9, packet + 9, packet[8]); packet = converted; size = sizeof(converted); break;
			case 11: size = 280; noLoopback = true; break;
			case 12: size = 24; break;
			case 13: size = 20; break;
			case 14: size = 28; break;
			case 15: size = 2072; break;
			default: return false;
		}
		if (size == 0) return false;
		std::vector<std::uint8_t> canonical(packet, packet + size);
		if (canonical[0] != 3 && size >= 4) std::fill(canonical.begin() + 1, canonical.begin() + 4, std::uint8_t{});
		if (canonical[0] == 5) std::fill(canonical.begin() + 9 + canonical[8], canonical.end(), std::uint8_t{});
		return peer->Send(recipient, canonical.data(), size, noLoopback);
	}

	bool CDPNet::SaveReceivedMessage(NetworkPlayerId sender, const void* packet, std::uint32_t size)
	{
		EnterCriticalSection(&receivedMessagesCriticalSection);
		if (receivedMessageCount >= 4096) { LeaveCriticalSection(&receivedMessagesCriticalSection); return false; }
		auto* payload = NXBasics::Runtime_AllocateWithNewHandler(size, 1);
		auto* records = static_cast<SReceivedMessage*>(NXBasics::Runtime_AllocateWithNewHandler((receivedMessageCount + 1) * sizeof(SReceivedMessage), 1));
		if (payload == nullptr || records == nullptr) { NXBasics::Runtime_Free(payload); NXBasics::Runtime_Free(records); LeaveCriticalSection(&receivedMessagesCriticalSection); return false; }
		if (receivedMessageCount != 0) std::memcpy(records, receivedMessages, receivedMessageCount * sizeof(SReceivedMessage));
		std::memcpy(payload, packet, size); records[receivedMessageCount] = { sender, size, payload, 0 };
		NXBasics::Runtime_Free(receivedMessages); receivedMessages = records; ++receivedMessageCount;
		LeaveCriticalSection(&receivedMessagesCriticalSection); return true;
	}
	void CDPNet::GetReceivedMessages(SReceivedMessage** output, std::uint32_t* count)
	{
		EnterCriticalSection(&receivedMessagesCriticalSection);
		*output = receivedMessages; *count = receivedMessageCount; receivedMessages = nullptr; receivedMessageCount = 0;
		LeaveCriticalSection(&receivedMessagesCriticalSection);
	}
	void CDPNet::ReleaseReceivedMessages(SReceivedMessage* messages, std::uint32_t count)
	{
		for (std::uint32_t i = 0; i < count; ++i) NXBasics::Runtime_Free(messages[i].data);
		NXBasics::Runtime_Free(messages);
	}

	void CDPNet::Pump()
	{
		if (peer == nullptr) return;
		for (auto& event : peer->Poll())
		{
			if (event.kind == ModernEvent::Kind::Joined || event.kind == ModernEvent::Kind::Left)
			{
				playerInfoChanged = 1; ++playerChangeCount; AddPlayerEvent(event.player, event.reason, event.playerName.c_str());
			}
			else if (event.kind == ModernEvent::Kind::Terminated) flags &= ~10u;
			else if (event.kind == ModernEvent::Kind::Packet)
			{
				const auto* packet = event.packet.data(); const auto size = static_cast<std::uint32_t>(event.packet.size());
				if (!ValidateNetworkPacket(packet, size)) continue;
				if (packet[0] == 2) { if (event.player == GetHostPlayerId()) allowConnections = packet[1]; }
				else if (packet[0] == 6)
				{
					if (controlPacketReceiveState.awaitingPacket != 0) { controlPacketReceiveState.receivedValue = PacketWord(packet + 4); controlPacketReceiveState.awaitingPacket = 0; }
				}
				else if (packet[0] == 7) continue;
				else if (packet[0] >= 11)
				{
					if (auto* manager = CFileTransferManager::GetInstance()) manager->DispatchMessage(event.player, packet, size);
				}
				else if (!SaveReceivedMessage(event.player, packet, size))
				{
					OutputDebugStringA("OpenVikings: receive queue exhausted; closing session.\n");
					peer->Leave(); flags &= ~10u; ++playerChangeCount; AddPlayerEvent(GetLocalPlayerId(), 6); break;
				}
			}
		}
		if ((flags & 10u) != 0 && peer->LocalPlayer() == 0) { flags &= ~10u; ++playerChangeCount; AddPlayerEvent(0, 6, L"Host"); }
		if ((flags & 4) == 0 || sessions == nullptr) return;
		const auto found = peer->Sessions(); if (SameSessions(sessions, found)) return;
		SSessionList replacement{}; replacement.flags = 1;
		for (const auto& session : found)
		{
			auto* node = static_cast<SSessionNode*>(NXBasics::Runtime_AllocateWithNewHandler(sizeof(SSessionNode), 1));
			if (node == nullptr) break;
			*node = {};
			node->description = static_cast<NetworkSessionDescription*>(NXBasics::Runtime_AllocateWithNewHandler(sizeof(NetworkSessionDescription), 1));
			if (node->description == nullptr) { NXBasics::Runtime_Free(node); break; }
			*node->description = {}; node->description->dwSize = sizeof(NetworkSessionDescription); node->description->guidInstance = session.instance; node->description->guidApplication = session.application;
			node->description->dwMaxPlayers = session.maximumPlayers; node->description->dwCurrentPlayers = session.currentPlayers;
			node->description->pwszSessionName = NXBasics::Runtime_DuplicateWideString(session.name.c_str());
			if (node->description->pwszSessionName == nullptr || FAILED(session.address.Duplicate(&node->address0C)) || FAILED(session.address.Duplicate(&node->address10))) { ReleaseSessionNodeResources(node); NXBasics::Runtime_Free(node); break; }
			node->lastResponseTime = GetTickCount();
			if (replacement.first == nullptr) { replacement.first = node; node->next = node->previous = node; }
			else { auto* tail = replacement.first->previous; node->previous = tail; node->next = replacement.first; tail->next = node; replacement.first->previous = node; }
			++replacement.count;
		}
		DestroySessionListContents(sessions); *sessions = replacement;
	}

	bool GetNetworkIPAddress(std::uint32_t index, char* destination, std::uint32_t capacity)
	{
		if (destination == nullptr || capacity == 0) return false;
		destination[0] = 0; WSADATA data{}; if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return false;
		char hostname[256]{}; ADDRINFOA hints{}, *addresses = nullptr; hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM;
		bool result = false;
		if (gethostname(hostname, sizeof(hostname)) == 0 && getaddrinfo(hostname, nullptr, &hints, &addresses) == 0)
		{
			for (auto* address = addresses; address != nullptr; address = address->ai_next)
			{
				if (address->ai_family != AF_INET && address->ai_family != AF_INET6) continue;
				if (index-- == 0) { result = getnameinfo(address->ai_addr, static_cast<socklen_t>(address->ai_addrlen), destination, capacity, nullptr, 0, NI_NUMERICHOST) == 0; break; }
			}
			freeaddrinfo(addresses);
		}
		WSACleanup(); return result;
	}
}
