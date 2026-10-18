#pragma once

#include "NetworkTypes.h"
#include <memory>
#include <string>
#include <vector>

namespace NNetwork
{
	struct SPlayerInfo;
	struct SModernNetworkState;

	struct NetworkAddress
	{
		std::wstring host;
		std::uint16_t port = DefaultNetworkPort;
		HRESULT Duplicate(NetworkAddress** output) const;
		void Release();
	};

	struct ModernSession
	{
		GUID instance{}, application{};
		std::wstring name;
		NetworkAddress address;
		std::uint32_t maximumPlayers = 0, currentPlayers = 0;
		std::uint64_t lastSeen = 0;
	};

	struct ModernEvent
	{
		enum class Kind { Joined, Left, Packet, Terminated } kind;
		NetworkPlayerId player = 0;
		std::uint32_t reason = 0;
		std::vector<std::uint8_t> packet;
		std::wstring playerName;
	};

	// Socket/thread ownership is private. The worker never calls into game objects.
	class CModernNetwork
	{
	public:
		CModernNetwork();
		~CModernNetwork();
		CModernNetwork(const CModernNetwork&) = delete;
		CModernNetwork& operator=(const CModernNetwork&) = delete;
		bool Ready() const;
		bool Host(const GUID& application, const wchar_t* name, const wchar_t* callsign, std::uint32_t maximumPlayers);
		bool Search(const GUID& application, const wchar_t* address);
		void StopSearch();
		bool Join(const ModernSession& session, const wchar_t* callsign);
		void Leave();
		bool Send(NetworkPlayerId recipient, const void* data, std::uint32_t size, bool noLoopback);
		bool Kick(NetworkPlayerId player);
		void AllowConnections(bool allow);
		NetworkPlayerId LocalPlayer() const;
		NetworkPlayerId HostPlayer() const;
		std::vector<SPlayerInfo> Players() const;
		std::vector<ModernSession> Sessions() const;
		std::vector<ModernEvent> Poll();
		bool ConnectionInfo(NetworkPlayerId player, NetworkConnectionInfo& output) const;
		std::uint32_t LastError() const;
	private:
		std::unique_ptr<SModernNetworkState> state;
	};

	bool ValidateNetworkPacket(const std::uint8_t* packet, std::uint32_t size);
}
