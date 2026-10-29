#pragma once
#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	// Physical Windows command and checksum storage. Queue bytes are constructed separately.
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
		std::byte system[20], deferred[20], ordinary[20];
	};

	struct SUserNetworkCommandManagerStorage
	{
		SNetworkChecksumPlayerStorage players[20];
		std::uint32_t lastVerifiedTick;
		SUserCommandQueuesStorage queues;
	};

	void __fastcall UserNetworkCommandManager_Execute(void* manager, void* unusedEdx, const void* command);
	void __fastcall UserNetworkCommandManager_AddCommand(void* manager, void* unusedEdx, const void* command, bool systemCommand, bool deferred);
	void __fastcall UserNetworkCommandManager_SendCommands(void* manager, void* unusedEdx);
	bool __fastcall UserNetworkCommandManager_ReceiveCommands(void* manager, void* unusedEdx);
}