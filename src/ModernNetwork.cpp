#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include "ModernNetwork.h"
#include "NetworkShutdown.h"
#include "GameProfile.h"

#include <algorithm>
#include <array>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <thread>

namespace NNetwork
{
	namespace
	{
		constexpr std::uint32_t Magic = 0x314E564F; // OVN1, little endian
		constexpr std::size_t HeaderSize = 24, MaximumFrame = 8192, MaximumQueue = 4 * 1024 * 1024;
		constexpr std::uint64_t HandshakeTimeout = 5000, ConnectionTimeout = 15000;
		enum class Frame : std::uint32_t { Hello = 1, Welcome = 2, Roster = 3, Data = 4, Depart = 5, Ping = 6, Pong = 7, Bye = 10, Query = 11, Offer = 12 };
		using Bytes = std::vector<std::uint8_t>;

		std::uint32_t Read32(const std::uint8_t* p)
		{
			return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
		}

		void Put32(Bytes& bytes, std::uint32_t value)
		{
			for (unsigned i = 0; i < 4; ++i) bytes.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
		}

		void PutGuid(Bytes& bytes, const GUID& value)
		{
			Put32(bytes, value.Data1);
			bytes.push_back(static_cast<std::uint8_t>(value.Data2)); bytes.push_back(static_cast<std::uint8_t>(value.Data2 >> 8));
			bytes.push_back(static_cast<std::uint8_t>(value.Data3)); bytes.push_back(static_cast<std::uint8_t>(value.Data3 >> 8));
			bytes.insert(bytes.end(), std::begin(value.Data4), std::end(value.Data4));
		}

		bool EqualGuid(const GUID& a, const GUID& b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }
		bool ValidCallsign(const std::wstring& value)
		{
			// The current game's callsign/UI contract is 39 ANSI bytes.
			const auto count = value.empty() ? 0 : WideCharToMultiByte(CP_ACP, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
			return count > 0 && count <= 39;
		}

		// Strings have an explicit UTF-8 byte count; invalid sequences never reach GUI copies.
		bool PutString(Bytes& bytes, const std::wstring& text, std::size_t maximum)
		{
			if (text.size() > maximum) return false;
			const int count = text.empty() ? 0 : WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
			if (!text.empty() && count <= 0) return false;
			Put32(bytes, static_cast<std::uint32_t>(count));
			const auto offset = bytes.size(); bytes.resize(offset + count);
			if (count != 0) WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), reinterpret_cast<char*>(bytes.data() + offset), count, nullptr, nullptr);
			return true;
		}

		struct Reader
		{
			const Bytes& bytes;
			std::size_t offset = 0;
			bool Integer(std::uint32_t& value) { if (bytes.size() - offset < 4) return false; value = Read32(bytes.data() + offset); offset += 4; return true; }
			bool Guid(GUID& value)
			{
				if (bytes.size() - offset < 16) return false;
				value.Data1 = Read32(bytes.data() + offset); offset += 4;
				value.Data2 = WORD(bytes[offset] | (bytes[offset + 1] << 8)); offset += 2;
				value.Data3 = WORD(bytes[offset] | (bytes[offset + 1] << 8)); offset += 2;
				std::memcpy(value.Data4, bytes.data() + offset, 8); offset += 8; return true;
			}
			bool String(std::wstring& value, std::size_t maximum)
			{
				std::uint32_t count;
				if (!Integer(count) || count > maximum * 4 || count > bytes.size() - offset) return false;
				if (count == 0) { value.clear(); return true; }
				const auto* text = reinterpret_cast<const char*>(bytes.data() + offset);
				const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, count, nullptr, 0);
				if (length <= 0 || static_cast<std::size_t>(length) > maximum || std::memchr(text, 0, count) != nullptr) return false;
				value.resize(length); MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, count, &value[0], length); offset += count; return true;
			}
			bool Done() const { return offset == bytes.size(); }
		};

		Bytes MakeFrame(Frame kind, NetworkPlayerId sender, NetworkPlayerId recipient, const Bytes& payload)
		{
			Bytes bytes; bytes.reserve(HeaderSize + payload.size());
			Put32(bytes, Magic); Put32(bytes, NetworkProtocolVersion); Put32(bytes, static_cast<std::uint32_t>(kind));
			Put32(bytes, sender); Put32(bytes, recipient); Put32(bytes, static_cast<std::uint32_t>(payload.size()));
			bytes.insert(bytes.end(), payload.begin(), payload.end()); return bytes;
		}

		struct Socket
		{
			SOCKET value = INVALID_SOCKET;
			Socket() = default;
			explicit Socket(SOCKET socket) : value(socket) {}
			~Socket() { Reset(); }
			Socket(const Socket&) = delete;
			Socket& operator=(const Socket&) = delete;
			Socket(Socket&& other) noexcept : value(other.value) { other.value = INVALID_SOCKET; }
			Socket& operator=(Socket&& other) noexcept { if (this != &other) { Reset(); value = other.value; other.value = INVALID_SOCKET; } return *this; }
			void Reset() { if (value != INVALID_SOCKET) { closesocket(value); value = INVALID_SOCKET; } }
			explicit operator bool() const { return value != INVALID_SOCKET; }
		};

		bool Nonblocking(SOCKET socket)
		{
			u_long value = 1; return ioctlsocket(socket, FIONBIO, &value) == 0;
		}

		void ConfigureConnection(SOCKET socket)
		{
			const int yes = 1;
			setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&yes), sizeof(yes));
			tcp_keepalive keepalive{ 1, 10000, 2000 }; DWORD returned = 0;
			WSAIoctl(socket, SIO_KEEPALIVE_VALS, &keepalive, sizeof(keepalive), nullptr, 0, &returned, nullptr, nullptr);
		}

		bool ParseAddress(const wchar_t* text, NetworkAddress& result)
		{
			result = NetworkAddress{};
			if (text == nullptr || *text == 0) return true;
			std::wstring input(text), service;
			if (input.size() > 255) return false;
			if (input.front() == L'[')
			{
				const auto end = input.find(L']'); if (end == std::wstring::npos) return false;
				result.host = input.substr(1, end - 1);
				if (end + 1 < input.size()) { if (input[end + 1] != L':') return false; service = input.substr(end + 2); if (service.empty()) return false; }
			}
			else
			{
				const auto colon = input.find(L':');
				if (colon != std::wstring::npos && colon == input.rfind(L':')) { result.host = input.substr(0, colon); service = input.substr(colon + 1); if (service.empty()) return false; }
				else result.host = input;
			}
			if (result.host.empty()) return false;
			if (!service.empty())
			{
				if (service.size() > 5) return false;
				unsigned port = 0;
				for (const auto c : service) { if (c < L'0' || c > L'9') return false; port = port * 10 + (c - L'0'); }
				if (port == 0 || port > 65535) return false;
				result.port = static_cast<std::uint16_t>(port);
			}
			return true;
		}

		struct Endpoint { sockaddr_storage address{}; int size = 0; };

		std::vector<Endpoint> Resolve(const NetworkAddress& address, int type)
		{
			ADDRINFOW hints{}; hints.ai_family = AF_UNSPEC; hints.ai_socktype = type;
			ADDRINFOW* raw = nullptr;
			const auto port = std::to_wstring(address.port);
			std::vector<Endpoint> result;
			if (GetAddrInfoW(address.host.c_str(), port.c_str(), &hints, &raw) != 0) return result;
			std::unique_ptr<ADDRINFOW, decltype(&FreeAddrInfoW)> owner(raw, FreeAddrInfoW);
			for (auto* entry = raw; entry != nullptr && result.size() < 16; entry = entry->ai_next)
			{
				if (entry->ai_addrlen > sizeof(sockaddr_storage)) continue;
				Endpoint endpoint; endpoint.size = static_cast<int>(entry->ai_addrlen);
				std::memcpy(&endpoint.address, entry->ai_addr, entry->ai_addrlen); result.push_back(endpoint);
			}
			return result;
		}

		Socket Bind(int family, int type, std::uint16_t port, bool dualStack)
		{
			Socket socket(::socket(family, type, type == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP));
			if (!socket) return {};
			const int yes = 1;
			if (port != 0) setsockopt(socket.value, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&yes), sizeof(yes));
			if (family == AF_INET6) { const int onlyV6 = dualStack ? 0 : 1; if (setsockopt(socket.value, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&onlyV6), sizeof(onlyV6)) != 0) return {}; }
			else if (type == SOCK_DGRAM) setsockopt(socket.value, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes));
			Endpoint endpoint;
			if (family == AF_INET6) { auto& address = reinterpret_cast<sockaddr_in6&>(endpoint.address); address.sin6_family = AF_INET6; address.sin6_port = htons(port); endpoint.size = sizeof(address); }
			else { auto& address = reinterpret_cast<sockaddr_in&>(endpoint.address); address.sin_family = AF_INET; address.sin_port = htons(port); endpoint.size = sizeof(address); }
			if (bind(socket.value, reinterpret_cast<const sockaddr*>(&endpoint.address), endpoint.size) != 0 || !Nonblocking(socket.value)) return {};
			return socket;
		}

		struct Connection
		{
			Socket socket;
			NetworkPlayerId player = 0;
			bool connecting = false, welcomed = false, dead = false, closing = false;
			std::uint32_t reason = 5;
			std::uint64_t started = GetTickCount64(), lastReceive = started, lastPing = 0, closeStarted = 0;
			NetworkConnectionInfo statistics{};
			Bytes input;
			std::deque<Bytes> output;
			std::size_t sentOffset = 0, queuedBytes = 0;
			bool Queue(const Bytes& bytes)
			{
				if (bytes.size() > MaximumQueue - queuedBytes) { dead = true; return false; }
				output.push_back(bytes); queuedBytes += bytes.size(); return true;
			}
		};
	}

	bool ValidateNetworkPacket(const std::uint8_t* packet, std::uint32_t size)
	{
		if (packet == nullptr || size == 0) return false;
		switch (packet[0])
		{
			case 2: return size == 2 && packet[1] <= 1;
			case 3: return size >= 4 && size <= 258 && size % 2 == 0 && packet[size - 2] == 0 && packet[size - 1] == 0;
			case 4: return size >= 12 && Read32(packet + 4) <= 1024 && size == Read32(packet + 4) + 12;
			case 5:
				if (size < 12 || packet[8] < 4 || packet[8] > 200 || (size != packet[8] + 12u && size != 212)) return false;
				return Read32(packet + 9) <= (packet[8] - 4u) / 20u && packet[8] == 4u + Read32(packet + 9) * 20u;
			case 6: return size == 12;
			case 7: case 8: return size == 2;
			case 9: return size == 16;
			case 11: return size == 280 && std::memchr(packet + 24, 0, 256) != nullptr;
			case 12: return size == 24;
			case 13: return size == 20;
			case 14: return size == 28 && Read32(packet + 24) <= 64 * 1024 * 1024 + 28;
			case 15: return size == 2072 && Read32(packet + 20) <= 2048;
			default: return false;
		}
	}

	struct SModernNetworkState
	{
		mutable std::mutex mutex;
		std::condition_variable wake;
		bool ready = false, stopping = false, hosting = false, searching = false, allow = true, failed = false;
		std::uint32_t error = 0, nextPlayer = 2, maximumPlayers = 10;
		NetworkPlayerId local = 0;
		GUID application{}, instance{};
		std::wstring name, callsign;
		Socket listener, discovery4, discovery6;
		std::vector<Endpoint> targets;
		std::vector<std::unique_ptr<Connection>> connections;
		std::map<NetworkPlayerId, SPlayerInfo> players;
		std::vector<ModernSession> sessions;
		std::vector<ModernEvent> events;
		std::size_t eventBytes = 0;
		std::uint64_t lastQuery = 0;
		std::thread worker;

		bool Event(ModernEvent event)
		{
			if (events.size() >= 4096 || event.packet.size() > MaximumQueue - eventBytes) { failed = true; error = WSAENOBUFS; return false; }
			const auto player = players.find(event.player);
			if (player != players.end()) event.playerName = player->second.playerName;
			eventBytes += event.packet.size(); events.push_back(std::move(event)); return true;
		}

		void AddPlayer(NetworkPlayerId id, const std::wstring& playerName)
		{
			SPlayerInfo player{}; player.playerId = id; player.isHost = id == 1; player.isLocal = id == local;
			std::copy_n(playerName.data(), (std::min)(playerName.size(), std::size_t(63)), player.playerName);
			players[id] = player; Event({ ModernEvent::Kind::Joined, id, 2, {} });
		}

		Bytes Roster() const
		{
			Bytes data; Put32(data, static_cast<std::uint32_t>(players.size()));
			for (const auto& entry : players) { Put32(data, entry.first); PutString(data, entry.second.playerName, 63); }
			return MakeFrame(Frame::Roster, 1, 0, data);
		}

		void Broadcast(const Bytes& bytes, NetworkPlayerId excluded = 0)
		{
			for (auto& connection : connections) if (connection->welcomed && connection->player != excluded && !connection->dead) connection->Queue(bytes);
		}

		bool Route(NetworkPlayerId sender, NetworkPlayerId recipient, const Bytes& bytes, bool noLoopback)
		{
			if (recipient != 0 && players.find(recipient) == players.end()) return false;
			if ((recipient == 0 || recipient == local) && (!noLoopback || sender != local)) Event({ ModernEvent::Kind::Packet, sender, 0, bytes });
			const auto wire = MakeFrame(Frame::Data, sender, recipient, bytes);
			bool result = !failed;
			for (auto& connection : connections)
				if (connection->welcomed && !connection->dead && (recipient == 0 || recipient == connection->player) && (!noLoopback || sender != connection->player)) connection->Queue(wire);
			return result;
		}

		bool Compatible(Reader& reader)
		{
			GUID app{}; std::uint32_t profile;
			return reader.Guid(app) && EqualGuid(app, application) && reader.Integer(profile) && profile == static_cast<std::uint32_t>(NRemaster::gGameVariant);
		}

		void Identity(Bytes& data) const { PutGuid(data, application); Put32(data, static_cast<std::uint32_t>(NRemaster::gGameVariant)); }

		bool Handle(Connection& connection, Frame kind, NetworkPlayerId sender, NetworkPlayerId recipient, const Bytes& bytes)
		{
			Reader reader{ bytes };
			if (!connection.welcomed)
			{
				if (hosting)
				{
					GUID session{}; std::wstring playerName;
					if (kind != Frame::Hello || sender != 0 || recipient != 1 || !Compatible(reader) || !reader.Guid(session) || !EqualGuid(session, instance) || !reader.String(playerName, 63) || !ValidCallsign(playerName) || !reader.Done() || !allow || players.size() >= maximumPlayers || nextPlayer == UINT32_MAX) return false;
					connection.player = nextPlayer++; connection.welcomed = true;
					Bytes welcome; Put32(welcome, connection.player); PutGuid(welcome, instance);
					connection.Queue(MakeFrame(Frame::Welcome, 1, connection.player, welcome));
					AddPlayer(connection.player, playerName); Broadcast(Roster()); return true;
				}
				NetworkPlayerId id; GUID session{};
				if (kind != Frame::Welcome || sender != 1 || !reader.Integer(id) || id < 2 || recipient != id || !reader.Guid(session) || !EqualGuid(session, instance) || !reader.Done()) return false;
				local = id; connection.player = 1; connection.welcomed = true; wake.notify_all(); return true;
			}
			if (hosting && sender != connection.player) return false;
			switch (kind)
			{
				case Frame::Data:
				{
					if (!ValidateNetworkPacket(bytes.data(), static_cast<std::uint32_t>(bytes.size())) || players.find(sender) == players.end()) return false;
					if ((bytes[0] == 2 || bytes[0] == 8) && sender != 1) return false;
					if ((bytes[0] == 11 || bytes[0] == 12) && Read32(bytes.data() + 20) != sender) return false;
					if (hosting) return Route(sender, recipient, bytes, true);
					if (recipient != 0 && recipient != local) return false;
					return Event({ ModernEvent::Kind::Packet, sender, 0, bytes });
				}
				case Frame::Roster:
				{
					std::uint32_t count;
					if (hosting || sender != 1 || recipient != 0 || !reader.Integer(count) || count < 2 || count > MaximumNetworkPlayers) return false;
					std::map<NetworkPlayerId, std::wstring> roster;
					for (std::uint32_t i = 0; i < count; ++i)
					{
						NetworkPlayerId id; std::wstring playerName;
						if (!reader.Integer(id) || id == 0 || id == UINT32_MAX || !reader.String(playerName, 63) || !ValidCallsign(playerName) || !roster.emplace(id, playerName).second) return false;
					}
					if (!reader.Done() || roster.find(1) == roster.end() || roster.find(local) == roster.end()) return false;
					for (const auto& entry : roster) if (players.find(entry.first) == players.end()) AddPlayer(entry.first, entry.second);
					for (auto entry = players.begin(); entry != players.end();) { if (roster.find(entry->first) == roster.end()) { Event({ ModernEvent::Kind::Left, entry->first, 5, {} }); entry = players.erase(entry); } else ++entry; }
					wake.notify_all(); return true;
				}
				case Frame::Depart:
				{
					NetworkPlayerId id; std::uint32_t reason;
					if (hosting || sender != 1 || !reader.Integer(id) || id <= 1 || !reader.Integer(reason) || reason < 3 || reason > 7 || !reader.Done()) return false;
					if (players.find(id) != players.end()) { Event({ ModernEvent::Kind::Left, id, reason, {} }); players.erase(id); }
					if (id == local) { connection.reason = reason; return false; } return true;
				}
				case Frame::Ping: return bytes.size() == 4 && connection.Queue(MakeFrame(Frame::Pong, local, connection.player, bytes));
				case Frame::Pong:
					if (bytes.size() != 4) return false;
					connection.statistics.dwRoundTripLatencyMS = (std::min)(GetTickCount() - Read32(bytes.data()), static_cast<DWORD>(ConnectionTimeout)); return true;
				case Frame::Bye: connection.reason = 3; return false;
				default: return false;
			}
		}

		bool Read(Connection& connection)
		{
			std::array<std::uint8_t, 8192> chunk;
			for (unsigned attempt = 0; attempt < 8; ++attempt)
			{
				const int count = recv(connection.socket.value, reinterpret_cast<char*>(chunk.data()), static_cast<int>(chunk.size()), 0);
				if (count == SOCKET_ERROR) return WSAGetLastError() == WSAEWOULDBLOCK;
				if (count == 0) return false;
				connection.lastReceive = GetTickCount64(); connection.statistics.dwBytesReceivedGuaranteed += count;
				connection.input.insert(connection.input.end(), chunk.begin(), chunk.begin() + count);
				std::size_t consumed = 0;
				while (connection.input.size() - consumed >= HeaderSize)
				{
					const auto* header = connection.input.data() + consumed;
					const auto length = Read32(header + 20);
					if (Read32(header) != Magic || Read32(header + 4) != NetworkProtocolVersion || length > MaximumFrame) return false;
					if (connection.input.size() - consumed < HeaderSize + length) break;
					const auto kind = static_cast<Frame>(Read32(header + 8)); const auto sender = Read32(header + 12), recipient = Read32(header + 16);
					Bytes data(header + HeaderSize, header + HeaderSize + length);
					if (!Handle(connection, kind, sender, recipient, data)) return false;
					++connection.statistics.dwPacketsReceivedGuaranteed; ++connection.statistics.dwMessagesReceived;
					consumed += HeaderSize + length;
				}
				connection.input.erase(connection.input.begin(), connection.input.begin() + consumed);
				if (connection.input.size() > MaximumFrame + HeaderSize) return false;
			}
			return true;
		}

		bool Write(Connection& connection)
		{
			std::size_t budget = 65536;
			while (!connection.output.empty() && budget != 0)
			{
				const auto& front = connection.output.front();
				const auto remaining = (std::min)(front.size() - connection.sentOffset, budget);
				const int count = send(connection.socket.value, reinterpret_cast<const char*>(front.data() + connection.sentOffset), static_cast<int>(remaining), 0);
				if (count == SOCKET_ERROR) return WSAGetLastError() == WSAEWOULDBLOCK;
				if (count <= 0) return false;
				connection.sentOffset += count; connection.queuedBytes -= count; budget -= count; connection.statistics.dwBytesSentGuaranteed += count;
				if (connection.sentOffset == front.size()) { connection.output.pop_front(); connection.sentOffset = 0; ++connection.statistics.dwPacketsSentGuaranteed; }
			}
			return true;
		}

		void Discovery(Socket& socket)
		{
			if (!socket) return;
			std::array<std::uint8_t, 1024> bytes;
			for (unsigned attempt = 0; attempt < 32; ++attempt)
			{
				Endpoint source; source.size = sizeof(source.address);
				const int size = recvfrom(socket.value, reinterpret_cast<char*>(bytes.data()), static_cast<int>(bytes.size()), 0, reinterpret_cast<sockaddr*>(&source.address), &source.size);
				if (size == SOCKET_ERROR) break;
				if (size < static_cast<int>(HeaderSize) || Read32(bytes.data()) != Magic || Read32(bytes.data() + 4) != NetworkProtocolVersion || Read32(bytes.data() + 20) != size - HeaderSize) continue;
				Bytes payload(bytes.begin() + HeaderSize, bytes.begin() + size); Reader reader{ payload };
				if (!Compatible(reader)) continue;
				if (hosting && allow && Read32(bytes.data() + 8) == static_cast<unsigned>(Frame::Query) && reader.Done())
				{
					Bytes data; Identity(data); PutGuid(data, instance); Put32(data, DefaultNetworkPort); Put32(data, maximumPlayers); Put32(data, static_cast<std::uint32_t>(players.size())); PutString(data, name, 99);
					const auto response = MakeFrame(Frame::Offer, 1, 0, data);
					sendto(socket.value, reinterpret_cast<const char*>(response.data()), static_cast<int>(response.size()), 0, reinterpret_cast<sockaddr*>(&source.address), source.size);
				}
				else if (searching && Read32(bytes.data() + 8) == static_cast<unsigned>(Frame::Offer))
				{
					ModernSession session; std::uint32_t port;
					if (!reader.Guid(session.instance) || !reader.Integer(port) || port == 0 || port > 65535 || !reader.Integer(session.maximumPlayers) || session.maximumPlayers > MaximumNetworkPlayers || !reader.Integer(session.currentPlayers) || session.currentPlayers > session.maximumPlayers || !reader.String(session.name, 99) || !reader.Done()) continue;
					wchar_t host[NI_MAXHOST];
					if (GetNameInfoW(reinterpret_cast<sockaddr*>(&source.address), source.size, host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST) != 0) continue;
					session.address.host = host; session.address.port = static_cast<std::uint16_t>(port); session.application = application; session.lastSeen = GetTickCount64();
					const auto found = std::find_if(sessions.begin(), sessions.end(), [&](const ModernSession& entry) { return EqualGuid(entry.instance, session.instance); });
					if (found != sessions.end()) *found = std::move(session); else if (sessions.size() < 64) sessions.push_back(std::move(session));
				}
			}
		}

		void Run()
		{
			std::unique_lock<std::mutex> lock(mutex);
			try
			{
				while (!stopping)
				{
					const auto now = GetTickCount64();
					if (listener)
					{
						for (unsigned i = 0; i < 16; ++i)
						{
							Socket accepted(accept(listener.value, nullptr, nullptr)); if (!accepted) break;
							if (!allow || connections.size() >= MaximumNetworkPlayers + 4 || !Nonblocking(accepted.value)) continue;
							ConfigureConnection(accepted.value);
							auto connection = std::make_unique<Connection>(); connection->socket = std::move(accepted); connections.push_back(std::move(connection));
						}
					}
					Discovery(discovery4); Discovery(discovery6);
					if (searching && now - lastQuery >= 1000)
					{
						Bytes identity; Identity(identity); const auto query = MakeFrame(Frame::Query, 0, 0, identity);
						for (const auto& target : targets)
						{
							const auto socket = target.address.ss_family == AF_INET6 ? discovery6.value : discovery4.value;
							if (socket != INVALID_SOCKET) sendto(socket, reinterpret_cast<const char*>(query.data()), static_cast<int>(query.size()), 0, reinterpret_cast<const sockaddr*>(&target.address), target.size);
						}
						lastQuery = now;
					}
					for (auto& pointer : connections)
					{
						auto& connection = *pointer;
						if (connection.connecting)
						{
							WSAPOLLFD descriptor{ connection.socket.value, POLLWRNORM, 0 };
							if (WSAPoll(&descriptor, 1, 0) > 0 && descriptor.revents != 0)
							{
								int code = 0, size = sizeof(code); getsockopt(connection.socket.value, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&code), &size);
								if (code != 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) { error = code != 0 ? code : WSAECONNREFUSED; connection.dead = true; }
								else connection.connecting = false;
							}
							if (connection.connecting && now - connection.started >= HandshakeTimeout) connection.dead = true;
							if (connection.connecting) continue;
						}
						if ((!connection.welcomed && now - connection.started >= HandshakeTimeout) || now - connection.lastReceive >= ConnectionTimeout) connection.dead = true;
						if (!connection.dead && (!Read(connection) || !Write(connection))) connection.dead = true;
						if (connection.closing && (connection.output.empty() || now - connection.closeStarted >= 250)) connection.dead = true;
						if (!connection.dead && connection.welcomed && now - connection.lastPing >= 2000)
						{
							Bytes ping; Put32(ping, static_cast<std::uint32_t>(now)); connection.lastPing = now;
							connection.Queue(MakeFrame(Frame::Ping, local, connection.player, ping));
						}
					}
					for (auto iterator = connections.begin(); iterator != connections.end();)
					{
						auto& connection = **iterator;
						if (!connection.dead) { ++iterator; continue; }
						if (hosting && connection.welcomed)
						{
							Event({ ModernEvent::Kind::Left, connection.player, connection.reason, {} }); players.erase(connection.player);
							Bytes data; Put32(data, connection.player); Put32(data, connection.reason); Broadcast(MakeFrame(Frame::Depart, 1, 0, data), connection.player);
						}
						if (!hosting) failed = true;
						iterator = connections.erase(iterator); wake.notify_all();
					}
					if (failed)
					{
						for (const auto& entry : players) Event({ ModernEvent::Kind::Left, entry.first, 6, {} });
						players.clear(); local = 0; connections.clear(); listener.Reset(); hosting = false;
						Event({ ModernEvent::Kind::Terminated, 0, 6, {} }); wake.notify_all(); break;
					}
					wake.notify_all();
					wake.wait_for(lock, std::chrono::milliseconds(10));
				}
			}
			catch (...) { error = WSAENOBUFS; failed = true; local = 0; players.clear(); connections.clear(); listener.Reset(); wake.notify_all(); }
		}

		void Start()
		{
			std::lock_guard<std::mutex> lock(mutex);
			stopping = false; failed = false;
			worker = std::thread([this] { Run(); });
		}

		void Stop()
		{
			{ std::lock_guard<std::mutex> lock(mutex); stopping = true; wake.notify_all(); }
			if (worker.joinable()) worker.join();
			std::lock_guard<std::mutex> lock(mutex);
			connections.clear(); listener.Reset(); discovery4.Reset(); discovery6.Reset();
			players.clear(); sessions.clear(); events.clear(); targets.clear(); eventBytes = 0;
			local = 0; hosting = searching = false; failed = false; lastQuery = 0;
		}
	};

	HRESULT NetworkAddress::Duplicate(NetworkAddress** output) const
	{
		if (output == nullptr) return E_POINTER;
		*output = nullptr;
		try { *output = new NetworkAddress(*this); return S_OK; } catch (...) { return E_OUTOFMEMORY; }
	}
	void NetworkAddress::Release() { delete this; }

	CModernNetwork::CModernNetwork() : state(std::make_unique<SModernNetworkState>())
	{
		WSADATA data{}; state->error = WSAStartup(MAKEWORD(2, 2), &data); state->ready = state->error == 0;
	}
	CModernNetwork::~CModernNetwork() { state->Stop(); if (state->ready) WSACleanup(); }
	bool CModernNetwork::Ready() const { return state->ready; }
	std::uint32_t CModernNetwork::LastError() const { std::lock_guard<std::mutex> lock(state->mutex); return state->error; }

	bool CModernNetwork::Host(const GUID& application, const wchar_t* name, const wchar_t* callsign, std::uint32_t maximumPlayers)
	{
		if (!Ready() || name == nullptr || callsign == nullptr || std::wcslen(name) > 99 || !ValidCallsign(callsign) || maximumPlayers < 2 || maximumPlayers > MaximumNetworkPlayers) return false;
		state->Stop();
		try
		{
			state->listener = Bind(AF_INET6, SOCK_STREAM, DefaultNetworkPort, true);
			if (!state->listener) state->listener = Bind(AF_INET, SOCK_STREAM, DefaultNetworkPort, false);
			if (!state->listener || listen(state->listener.value, 16) != 0) { state->error = WSAGetLastError(); state->Stop(); return false; }
			state->discovery4 = Bind(AF_INET, SOCK_DGRAM, DefaultNetworkPort, false);
			state->discovery6 = Bind(AF_INET6, SOCK_DGRAM, DefaultNetworkPort, false);
			if (!state->discovery4 && !state->discovery6) { state->error = WSAGetLastError(); state->Stop(); return false; }
			GUID instance{}; if (FAILED(CoCreateGuid(&instance))) { state->Stop(); return false; }
			{
				std::lock_guard<std::mutex> lock(state->mutex);
				state->application = application; state->instance = instance;
				state->name = name; state->callsign = callsign; state->maximumPlayers = maximumPlayers;
				state->local = 1; state->nextPlayer = 2; state->hosting = true; state->allow = true; state->AddPlayer(1, callsign);
			}
			state->Start(); return true;
		}
		catch (...) { state->Stop(); return false; }
	}

	bool CModernNetwork::Search(const GUID& application, const wchar_t* address)
	{
		if (!Ready()) return false;
		NetworkAddress parsed; if (!ParseAddress(address, parsed)) { state->error = WSAEINVAL; return false; }
		state->Stop();
		try
		{
			state->application = application;
			state->discovery4 = Bind(AF_INET, SOCK_DGRAM, 0, false); state->discovery6 = Bind(AF_INET6, SOCK_DGRAM, 0, false);
			if (!state->discovery4 && !state->discovery6) { state->error = WSAGetLastError(); return false; }
			if (!parsed.host.empty()) state->targets = Resolve(parsed, SOCK_DGRAM);
			else
			{
				NetworkAddress broadcast; broadcast.host = L"255.255.255.255"; state->targets = Resolve(broadcast, SOCK_DGRAM);
				NetworkAddress loopback; loopback.host = L"127.0.0.1";
				const auto local = Resolve(loopback, SOCK_DGRAM); state->targets.insert(state->targets.end(), local.begin(), local.end());
			}
			if (state->targets.empty()) { state->error = WSAHOST_NOT_FOUND; state->Stop(); return false; }
			state->searching = true; state->Start(); return true;
		}
		catch (...) { state->Stop(); return false; }
	}

	void CModernNetwork::StopSearch() { std::lock_guard<std::mutex> lock(state->mutex); state->searching = false; state->targets.clear(); state->sessions.clear(); }

	bool CModernNetwork::Join(const ModernSession& session, const wchar_t* callsign)
	{
		if (!Ready() || callsign == nullptr || !ValidCallsign(callsign)) return false;
		const auto endpoints = Resolve(session.address, SOCK_STREAM);
		const auto deadline = GetTickCount64() + HandshakeTimeout;
		for (const auto& endpoint : endpoints)
		{
			if (GetTickCount64() >= deadline) break;
			state->Stop();
			try
			{
				auto connection = std::make_unique<Connection>();
				connection->socket = Socket(socket(endpoint.address.ss_family, SOCK_STREAM, IPPROTO_TCP));
				if (!connection->socket || !Nonblocking(connection->socket.value)) continue;
				ConfigureConnection(connection->socket.value);
				const int result = connect(connection->socket.value, reinterpret_cast<const sockaddr*>(&endpoint.address), endpoint.size);
				if (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) continue;
				connection->connecting = result == SOCKET_ERROR;
				state->application = session.application; state->instance = session.instance; state->callsign = callsign;
				Bytes hello; state->Identity(hello); PutGuid(hello, session.instance); if (!PutString(hello, callsign, 63)) return false;
				connection->Queue(MakeFrame(Frame::Hello, 0, 1, hello)); state->connections.push_back(std::move(connection)); state->Start();
				std::unique_lock<std::mutex> lock(state->mutex);
				const auto now = GetTickCount64();
				state->wake.wait_for(lock, std::chrono::milliseconds(now < deadline ? deadline - now : 0), [&] { return state->failed || (state->local != 0 && state->players.find(state->local) != state->players.end()); });
				if (!state->failed && state->local != 0 && state->players.find(state->local) != state->players.end()) return true;
			}
			catch (...) { state->error = WSAENOBUFS; }
		}
		state->Stop(); return false;
	}

	void CModernNetwork::Leave()
	{
		{
			std::unique_lock<std::mutex> lock(state->mutex);
			for (auto& connection : state->connections) if (connection->welcomed) connection->Queue(MakeFrame(Frame::Bye, state->local, connection->player, {}));
			state->wake.notify_all();
			state->wake.wait_for(lock, std::chrono::milliseconds(100), [&] { return std::all_of(state->connections.begin(), state->connections.end(), [](const auto& connection) { return connection->output.empty(); }); });
		}
		state->Stop();
	}
	bool CModernNetwork::Send(NetworkPlayerId recipient, const void* data, std::uint32_t size, bool noLoopback)
	{
		if (!ValidateNetworkPacket(static_cast<const std::uint8_t*>(data), size)) return false;
		std::lock_guard<std::mutex> lock(state->mutex);
		if (state->failed || state->local == 0) return false;
		try
		{
			const auto* packet = static_cast<const std::uint8_t*>(data); Bytes bytes(packet, packet + size);
			if (state->hosting) return state->Route(state->local, recipient, bytes, noLoopback);
			if (recipient != 0 && state->players.find(recipient) == state->players.end()) return false;
			if ((recipient == 0 || recipient == state->local) && !noLoopback) state->Event({ ModernEvent::Kind::Packet, state->local, 0, bytes });
			if (recipient == state->local) return !state->failed;
			return !state->connections.empty() && state->connections.front()->Queue(MakeFrame(Frame::Data, state->local, recipient, bytes));
		}
		catch (...) { state->failed = true; return false; }
	}

	bool CModernNetwork::Kick(NetworkPlayerId player)
	{
		std::lock_guard<std::mutex> lock(state->mutex);
		if (!state->hosting || player <= 1) return false;
		for (auto& connection : state->connections)
			if (connection->player == player)
			{
				Bytes data; Put32(data, player); Put32(data, 4);
				connection->Queue(MakeFrame(Frame::Depart, 1, player, data)); connection->reason = 4; connection->closing = true; connection->closeStarted = GetTickCount64(); return true;
			}
		return false;
	}
	void CModernNetwork::AllowConnections(bool allow) { std::lock_guard<std::mutex> lock(state->mutex); if (state->hosting) state->allow = allow; }
	NetworkPlayerId CModernNetwork::LocalPlayer() const { std::lock_guard<std::mutex> lock(state->mutex); return state->local; }
	NetworkPlayerId CModernNetwork::HostPlayer() const { std::lock_guard<std::mutex> lock(state->mutex); return state->players.find(1) == state->players.end() ? 0 : 1; }
	std::vector<SPlayerInfo> CModernNetwork::Players() const
	{
		std::lock_guard<std::mutex> lock(state->mutex); std::vector<SPlayerInfo> result;
		for (const auto& entry : state->players) result.push_back(entry.second);
		return result;
	}
	std::vector<ModernSession> CModernNetwork::Sessions() const
	{
		std::lock_guard<std::mutex> lock(state->mutex); std::vector<ModernSession> result;
		for (const auto& entry : state->sessions) if (GetTickCount64() - entry.lastSeen <= 4000) result.push_back(entry);
		return result;
	}
	std::vector<ModernEvent> CModernNetwork::Poll()
	{
		std::lock_guard<std::mutex> lock(state->mutex); std::vector<ModernEvent> result; result.swap(state->events); state->eventBytes = 0; return result;
	}
	bool CModernNetwork::ConnectionInfo(NetworkPlayerId player, NetworkConnectionInfo& output) const
	{
		std::lock_guard<std::mutex> lock(state->mutex); output = {}; output.dwSize = sizeof(output);
		if (state->players.find(player) == state->players.end()) return false;
		if (player == state->local) return true;
		for (const auto& connection : state->connections)
			if (connection->player == player || !state->hosting)
			{
				output = connection->statistics; output.dwSize = sizeof(output);
				const auto elapsed = (std::max)(std::uint64_t(1), GetTickCount64() - connection->started);
				output.dwThroughputBPS = static_cast<DWORD>((std::uint64_t(output.dwBytesSentGuaranteed) + output.dwBytesReceivedGuaranteed) * 1000 / elapsed); return true;
			}
		return false;
	}
}
