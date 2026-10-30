#pragma once
#include "XList.h"

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Fixed Windows command/checksum records with separately constructed native runtime queues.
	struct SUserNetworkCommandStorage
	{
		std::uint8_t opcode, playerId;
		std::byte padding02[2];
		std::uint32_t payload[4];
	};

	struct SNetworkCommandPacketHeaderStorage
	{
		std::uint32_t count;
	};

	struct SNetworkChecksumSampleStorage
	{
		std::uint32_t checksum, tick, networkPlayerId;
	};

	struct SNetworkChecksumPlayerStorage
	{
		SNetworkChecksumSampleStorage samples[10];
	};

	struct SUserCommandQueuesStorage
	{
		alignas(NXBasics::CListBase) std::byte system[sizeof(NXBasics::CListBase)];
		alignas(NXBasics::CListBase) std::byte deferred[sizeof(NXBasics::CListBase)];
		alignas(NXBasics::CListBase) std::byte ordinary[sizeof(NXBasics::CListBase)];
	};

	struct SUserNetworkCommandManagerStorage
	{
		SNetworkChecksumPlayerStorage players[20];
		std::uint32_t lastVerifiedTick;
		SUserCommandQueuesStorage queues;
	};

	static_assert(sizeof(SUserNetworkCommandStorage) == 20);
	static_assert(sizeof(SNetworkCommandPacketHeaderStorage) == 4);
	static_assert(sizeof(SNetworkChecksumSampleStorage) == 12);
	static_assert(sizeof(SNetworkChecksumPlayerStorage) == 120);
	static_assert(sizeof(SUserCommandQueuesStorage) == 3 * sizeof(NXBasics::CListBase));
	static_assert(offsetof(SUserCommandQueuesStorage, deferred) == sizeof(NXBasics::CListBase));
	static_assert(offsetof(SUserCommandQueuesStorage, ordinary) == 2 * sizeof(NXBasics::CListBase));
	static_assert(offsetof(SUserNetworkCommandManagerStorage, lastVerifiedTick) == 2400);
	static_assert(offsetof(SUserNetworkCommandManagerStorage, queues) == (sizeof(void*) == 8 ? 2408 : 2404));
	static_assert(sizeof(SUserNetworkCommandManagerStorage) == (sizeof(void*) == 8 ? 2528 : 2464));

	void __fastcall UserNetworkCommandManager_Execute(void* manager, void* unusedEdx, const void* command);
	void __fastcall UserNetworkCommandManager_AddCommand(void* manager, void* unusedEdx, const void* command, bool systemCommand, bool deferred);
	void __fastcall UserNetworkCommandManager_SendCommands(void* manager, void* unusedEdx);
	bool __fastcall UserNetworkCommandManager_ReceiveCommands(void* manager, void* unusedEdx);
}