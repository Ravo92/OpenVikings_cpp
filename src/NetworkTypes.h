#pragma once

#include <Windows.h>
#include <cstdint>

// In-process bridge layouts retained for the reconstructed game and GUI.
// These records are never serialized by the modern transport.
using NetworkPlayerId = std::uint32_t;
using NetworkBufferHandle = std::uint32_t;

struct NetworkSessionDescription
{
	DWORD dwSize, dwFlags;
	GUID guidInstance, guidApplication;
	DWORD dwMaxPlayers, dwCurrentPlayers;
	wchar_t* pwszSessionName;
	wchar_t* pwszPassword;
	void* pvReservedData;
	DWORD dwReservedDataSize;
	void* pvApplicationReservedData;
	DWORD dwApplicationReservedDataSize;
};

struct NetworkServiceProviderInfo
{
	DWORD dwFlags;
	GUID guid;
	wchar_t* pwszName;
	void* pvReserved;
	DWORD dwReserved;
};

struct NetworkConnectionInfo
{
	DWORD dwSize, dwRoundTripLatencyMS, dwThroughputBPS, dwPeakThroughputBPS;
	DWORD dwBytesSentGuaranteed, dwPacketsSentGuaranteed, dwBytesSentNonGuaranteed, dwPacketsSentNonGuaranteed;
	DWORD dwBytesRetried, dwPacketsRetried, dwBytesDropped, dwPacketsDropped;
	DWORD dwMessagesTransmittedHighPriority, dwMessagesTimedOutHighPriority, dwMessagesTransmittedNormalPriority, dwMessagesTimedOutNormalPriority, dwMessagesTransmittedLowPriority, dwMessagesTimedOutLowPriority;
	DWORD dwBytesReceivedGuaranteed, dwPacketsReceivedGuaranteed, dwBytesReceivedNonGuaranteed, dwPacketsReceivedNonGuaranteed, dwMessagesReceived;
};

namespace NNetwork
{
	inline constexpr GUID TcpIpProvider = { 0xEBFE7BA0u, 0x628Du, 0x11D2u, { 0xAE, 0x0F, 0, 0x60, 0x97, 0xB0, 0x14, 0x11 } };
	inline constexpr std::uint16_t DefaultNetworkPort = 25840;
	// Protocol 8 retains construction escape without rejecting occupied building footprints.
	inline constexpr std::uint32_t NetworkProtocolVersion = 8;
	inline constexpr std::uint8_t FileTransferCancelPacketType = 16;
	inline constexpr std::uint32_t MaximumNetworkPlayers = 10;
}
