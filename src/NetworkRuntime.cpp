#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include "XResultIdentifiers.h"
#include <winsock2.h>
#include <Windows.h>

#include "CRC16.h"
#include "CRC32.h"
#include "DirectXErrorText.h"
#include "FileTransferManager.h"
#include "MasterPropertyManager.h"
#include "RemasterLog.h"
#include "NetworkShutdown.h"
#include "NetworkTypes.h"
#include "XTextEncoding.h"
#include "OriginalStrings.h"
#include "XCrtFileAccess.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <new>
#include <algorithm>
#include <cstdio>
#include <string>
#include <mutex>

namespace NNetwork
{
	// Original function: FUN_004DD864; padding is deliberately left untouched.
	void* CommandPacket_Construct(void* packet)
	{
		auto* bytes = static_cast<std::uint8_t*>(packet);
		const std::uint32_t zero = 0;
		std::memcpy(bytes + 4, &zero, 4);
		bytes[8] = 0;
		bytes[0] = 5;
		std::memset(bytes + 9, 0, 200);

		return packet;
	}

	// Original function: FUN_004DEE2D.
	void* ChatPacket_Construct(void* packet)
	{
		auto* bytes = static_cast<std::uint8_t*>(packet);
		bytes[1] = 0;
		bytes[0] = 3;
		std::memset(bytes + 2, 0, 256);

		return packet;
	}

	// Original function: FUN_004DEE4E.
	void* PayloadPacket_Construct(void* packet)
	{
		auto* bytes = static_cast<std::uint8_t*>(packet);
		const std::uint32_t zero = 0;
		std::memcpy(bytes + 4, &zero, 4);
		std::memcpy(bytes + 8, &zero, 4);
		bytes[0] = 4;
		std::memset(bytes + 0x0C, 0, 1024);

		return packet;
	}

	// Original function: FUN_004DFFDF; folded with the sound-device pointer ctor.
	SFileTransferList::SFileTransferList()
	{
		first = nullptr;
	}

	// Original function: FUN_004E0A2C; the original GUID at 004F7FB0 is zero.
	void* FileTransferAnnouncementPacket_Construct(void* packet)
	{
		auto* bytes = static_cast<std::uint8_t*>(packet);
		const std::uint32_t zero = 0;
		std::memcpy(bytes + 0x14, &zero, 4);
		bytes[0] = 11;
		const GUID transferId{ };
		std::memcpy(bytes + 4, &transferId, sizeof(transferId));
		std::memset(bytes + 0x18, 0, 256);

		return packet;
	}
}

// Original data: DAT_00569650.
// Enabled at 004db9c7 after successfully opening the network log.
std::uint8_t gNetworkLogEnabled = 0;

// Original data: DAT_00569658
NNetwork::CDPNet* gNetworkInstance = nullptr;

// Original data: DAT_00569654
NNetwork::SNetLog* gNetworkLog = nullptr;

// Original data: DAT_005697b0.
std::uint8_t gDynamicNetworkUpdateStopped = 0;

// Original data: DAT_005697b4.
NetworkPlayerId gPendingMissingPlayerId = 0;

// Original data: DAT_005697b8.
NetworkPlayerId gLastRecoveryPlayerId = 0;

// Original data: DAT_005697bc.
std::uint32_t gProcessedPlayerChangeCount = 0;

// Original data: DAT_0050aa2c.
// Windows PE bytes are 01 00 00 00; all three exported accesses use the first byte.
std::uint8_t gDynamicNetworkInitialUpdatePending = 1;

namespace
{

	void ReportNetworkSystemError(const char* sourceFile, std::int32_t line, const char* additionalInformation);
	NNetwork::SNetLog* InitializeNetworkLog(NNetwork::SNetLog* object);
	void ReportNetworkError(const char* operation, HRESULT result);
	void ReportNetworkConditionFailure(const char* sourceFile, std::int32_t line, std::int32_t condition, const char* message);

	void ReportNetworkError(const char* operation, HRESULT result)
	{
		OutputDebugStringA(operation);
		OutputDebugStringA(OriginalStrings::DirectPlayErrorSeparator);
		OutputDebugStringA(NXBasics::GetDirectXErrorText(result));
		OutputDebugStringA(OriginalStrings::NewLine);
	}

	// Original function: FUN_004db8a5
	NNetwork::SNetLog* InitializeNetworkLog(NNetwork::SNetLog* object)
	{
		*object = { };
		object->level = 3;

		return object;
	}

	// Original function: FUN_004de237
	void ReportNetworkSystemError(const char* sourceFile, std::int32_t line, const char* additionalInformation)
	{
		const DWORD code = GetLastError();
		char message[1024]{ };
		char system[512]{ };
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0, system, sizeof(system), nullptr);
		std::snprintf(message, sizeof(message), "Network error at %.240s:%d: %.500s (code %lu)\n%.200s\n", sourceFile != nullptr ? sourceFile : "unknown", line, system, code, additionalInformation != nullptr ? additionalInformation : "");
		OutputDebugStringA(message);
	}

}

namespace NNetwork
{

	// Original function: FUN_004dded2; circular list, original count cookie and holes.
	void GetSessionSummaries(SSessionList* list, SSessionSummary** summaries, DWORD* count)
	{
		if (summaries == nullptr || count == nullptr)
			return;

		*summaries = nullptr;
		*count = 0;

		if (list == nullptr || list->first == nullptr || list->count == 0 || list->count > 64)
			return;

		const auto records = list->count;
		auto* allocation = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::size_t>(records) * sizeof(SSessionSummary) + alignof(SSessionSummary), 1));

		if (allocation == nullptr)
		{

			return;
		}

		std::memcpy(allocation, &records, 4);
		auto* result = reinterpret_cast<SSessionSummary*>(allocation + alignof(SSessionSummary));
		std::memset(result, 0, records * sizeof(SSessionSummary));
		auto* node = list->first;

		for (std::uint32_t i = 0; i < records && node != nullptr; ++i, node = node->next)
		{
			wchar_t name[200]{ };
			std::swprintf(name, std::size(name), L"%.99ls (%lu/%lu)", node->description->pwszSessionName, node->description->dwCurrentPlayers, node->description->dwMaxPlayers);
			result[i].name = NXBasics::Runtime_DuplicateWideString(name);

			if (result[i].name == nullptr)
				break;

			result[i].instance = node->description->guidInstance;
			++*count;
		}

		*summaries = result;
	}

	// Original function: FUN_004dea48; reset is followed by the original first update.
	void ResetDynamicNetworkData(SDynamicNetworkData* data)
	{
		WriteNetworkLog(GetNetworkLog(), "Reset logic module", 0);
		ReleaseDynamicNetworkMemory(data);
		data->networkTick = 0;
		data->playerInfoRecords = nullptr;
		data->receivedChatRecords = nullptr;
		data->receivedChatRecordCount = 0;
		data->pendingDataMessageCount = 0;
		data->pendingDataMessages = nullptr;
		data->waitStartTick = 0;
		data->unknown7C = 0;
		data->pendingDataMessages = NXBasics::Runtime_AllocateWithNewHandler(66304, 1);
		ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\dplogic.cpp", 0x8D, data->pendingDataMessages != nullptr, "Memory allocation failed.");

		if (data->pendingDataMessages == nullptr)
		{
			data->active = 0;

			return;
		}

		std::memset(data->pendingDataMessages, 0, 66304);
		UpdateDynamicNetwork(data, true);
	}

	// Original function: FUN_004deb32.
	bool AttachDynamicNetwork(SDynamicNetworkData* data, CDPNet* network)
	{
		if ((network->flags & 8u) != 8u && (network->flags & 2u) != 2u)
		{
			ReleaseDynamicNetworkMemory(data);
			data->active = 0;
			return false;
		}

		if (data->active != 0)
			ShutDownDynamicNetworkData(data);

		ResetDynamicNetworkData(data);
		data->playerCount = 0;

		if (data->playerInfoRecords != nullptr)
		{
			NXBasics::Runtime_Free(data->playerInfoRecords);
			data->playerInfoRecords = nullptr;
		}

		data->active = network->GetPlayerInfo(reinterpret_cast<SPlayerInfo**>(&data->playerInfoRecords), &data->playerCount);

		if (data->active != 0)
		{
			char message[100]{ };
			const auto playerId = network->GetLocalPlayerId();
			NXBasics::Runtime_PrintString(message, "Logic module is attached to the network. Local NetworkPlayerId is 0x%x", playerId);
			WriteNetworkLog(GetNetworkLog(), message, 0);
		}
		else
		{
			WriteNetworkLog(GetNetworkLog(), "Cannot attach logic module to network because player data is unavailable.", 0);
		}

		if (data->active == 0)
			ReleaseDynamicNetworkMemory(data);

		return data->active != 0;
	}
}

namespace
{
	// Original function: FUN_004de569
	void ReportNetworkConditionFailure(const char* sourceFile, std::int32_t line, std::int32_t condition, const char* message)
	{
		if (condition != 0)
		{

			return;
		}

		char output[1024]{ };
		std::snprintf(output, sizeof(output), "%.240s:%d\n%.700s", sourceFile != nullptr ? sourceFile : "network", line, message != nullptr ? message : "Invalid network state");
		OutputDebugStringA(output);
	}

}

namespace NNetwork
{

	// Original data: 005097a8; initial bytes verified in the earlier PE review.
	GUID gNetworkApplicationGuid = { 0xAC671273u, 0x85B1u, 0x4C11u, { 0xAC, 0x54, 0x16, 0x6C, 0x39, 0x9C, 0x80, 0x78 } };

	namespace
	{
		// Original data: DAT_005697c0, DAT_005697dc and DAT_005697e4.

		void InitializeEventQueue(NNetwork::SEventQueue* queue);
		SEventQueue* AddEventQueueEntry(SEventQueue* queue, NetworkPlayerId playerId, std::uint32_t timestamp, const wchar_t* playerName, std::uint32_t eventType);
		SSessionNode* ConstructSessionNode(SSessionNode* node);
		char* TransformNetworkEasterEggText(const char* source, char* destination);

		// Original function: FUN_004db429
		void InitializeEventQueue(NNetwork::SEventQueue* queue)
		{
			queue->count = 0;
			queue->last = nullptr;
			queue->first = nullptr;
			queue->changed = 0;
		}

		// Original function: FUN_004db558
		SEventQueue* AddEventQueueEntry(SEventQueue* queue, NetworkPlayerId playerId, std::uint32_t timestamp, const wchar_t* playerName, std::uint32_t eventType)
		{
			if (queue->count == 0x100u)
			{
				SEventNode* oldest = queue->first;
				queue->first = oldest->next;

				NXBasics::Runtime_Free(oldest);
				queue->changed = 1;

				--queue->count;

				if (queue->first == nullptr)
					queue->last = nullptr;
			}

			auto* node = static_cast<SEventNode*>(NXBasics::Runtime_AllocateWithNewHandler(sizeof(SEventNode), 1));

			if (node != nullptr)
			{
				std::memset(node, 0, sizeof(SEventNode));
			}

			ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayBase\\netevent.cpp", 0xAF, node != nullptr, "Memory allocation failed.");

			if (node == nullptr)
				return queue;

			node->playerId = playerId;
			node->timestamp = timestamp;

			std::wcsncpy(node->playerName, playerName != nullptr ? playerName : L"Unknown player", 255);

			node->eventType = eventType;

			if (queue->first == nullptr)
			{
				queue->first = node;
				queue->last = node;
				node->next = nullptr;
			}
			else
			{
				queue->last->next = node;
				queue->last = node;
			}

			++queue->count;

			return queue;
		}

		// Original function: FUN_004ddcd0

		// Original function: FUN_004de0e4
		SSessionNode* ConstructSessionNode(SSessionNode* node)
		{
			node->invalid = 0;
			node->lastResponseTime = 0;
			node->responseTime = 0;
			node->address10 = nullptr;
			node->address0C = nullptr;
			node->description = nullptr;
			node->previous = nullptr;
			node->next = nullptr;

			// The original constructor does not initialize padding bytes.

			return node;
		}

		// Original function: FUN_004dff82
		char* TransformNetworkEasterEggText(const char* source, char* destination)
		{
			while (*source != '\0')
			{
				const auto value = static_cast<unsigned char>(*source++);
				*destination++ = static_cast<char>(value ^ NXBasics::BitMask::ByteExceptBit4);
			}

			*destination = '\0';

			return destination;
		}

	}

	// Original function: FUN_004058fd
	bool Network_StartUp()
	{
		// Original data: 004f3378.
		static constexpr GUID applicationGuid = { 0xA7F93099u, 0x9BCFu, 0x4145u, { 0xAB, 0x6A, 0x57, 0xCD, 0xFC, 0x0A, 0x84, 0x19 } };

		CDPNet* network = GetNetworkInstance();
		network->SetApplicationGuid(&applicationGuid);

		const bool initialized = network->Initialize(nullptr, false);

		// The shared opt-in sink owns <game root>/logs/network.log, including failed startup diagnostics.

		if (NRemaster::Logging::Enabled(NRemaster::Logging::ECategory::Network))
			OpenNetworkLog(GetNetworkLog(), nullptr, 3, 0);

		return initialized;
	}

	// Original function: FUN_004db4a0
	SEventQueue* AppendEventQueueCopy(SEventQueue* destination, const SEventQueue* source)
	{
		destination->changed = source->changed;

		for (const SEventNode* node = source->first; node != nullptr; node = node->next)
		{
			const auto* bytes = reinterpret_cast<const std::byte*>(node);

			NetworkPlayerId playerId;
			std::uint32_t timestamp;
			std::int32_t eventType;

			std::memcpy(&playerId, bytes, sizeof(playerId));
			std::memcpy(&timestamp, bytes + 0x204, sizeof(timestamp));
			std::memcpy(&eventType, bytes + 0x208, sizeof(eventType));

			const auto* playerName = reinterpret_cast<const wchar_t*>(bytes + 4);

			AddEventQueueEntry(destination, playerId, timestamp, playerName, eventType);
		}

		return destination;
	}

	// Original function: FUN_004db43a; distinct from replacement/assignment.
	SEventQueue* ConstructEventQueueAndTakeSource(SEventQueue* destination, SEventQueue* source)
	{
		destination->count = 0;
		destination->last = nullptr;
		destination->first = nullptr;
		destination->changed = 0;
		ClearEventQueue(destination);
		destination->count = 0;
		destination->changed = source->changed;

		for (auto* event = source->first; event != nullptr; event = event->next)
		{
			AddEventQueueEntry(destination, event->playerId, event->timestamp, event->playerName, event->eventType);
		}

		ClearEventQueue(source);

		return destination;
	}

	// Original function: FUN_004db635.
	// Complete thirteen-entry jump table at 004DB754.
	void FormatNetworkEvent(const SEventQueue* queue, const SEventNode* event, char* destination, std::int32_t appendLimit)
	{
		(void)queue;

		if (destination == nullptr || appendLimit <= 0)
			return;

		destination[0] = 0;

		if (event == nullptr)
		{

			return;
		}

		if (event->playerName[0] != L'\0')
		{
			char playerName[64]{ };
			CopyNetworkWideToUtf8(playerName, event->playerName, static_cast<std::int32_t>(std::size(playerName)));
			std::snprintf(destination, appendLimit, "%s", playerName);
		}

		char playerIdText[16] = " (0x";
		NXBasics::Runtime_LongToString(static_cast<std::int32_t>(event->playerId), playerIdText + 4, 16);
		std::strncat(destination, playerIdText, static_cast<std::size_t>(appendLimit) - std::strlen(destination) - 1);
		static constexpr const char* eventText[] = {
			") got an error.",
			") joined the session.",
			") disconnected normally.",
			") was kicked by the host.",
			") lost the connection.",
			") disconnected because the session was terminated.",
			") disconnected but I don't know why...",
			") is new host.",
			") stopped responding.",
			") stopped responding. Connection to server lost.",
			") did either not sent a message or the message could not be received.",
			") has reached a timeout.",
			") lost connection. Waiting for the transport to confirm."
		};
		const std::uint32_t index = event->eventType - 1u;
		const char* text = index < 13 ? eventText[index] : ") generated an unknown event.";
		std::strncat(destination, text, static_cast<std::size_t>(appendLimit) - std::strlen(destination) - 1);
	}

	// Original function: FUN_004db4dd
	SEventQueue* ReplaceEventQueueAndClearSource(SEventQueue* destination, SEventQueue* source)
	{
		ClearEventQueue(destination);
		AppendEventQueueCopy(destination, source);
		ClearEventQueue(source);

		return destination;
	}

	// Original function: FUN_004db788
	void ConvertNetworkUtf8ToWide(wchar_t* destination, const char* source, std::int32_t length)
	{
		if (destination == nullptr || length <= 0)
			return;

		destination[0] = 0;

		if (source == nullptr || length == 1)
		{

			return;
		}

		const auto wide = NXBasics::Text_ToWide(source);
		auto count = (std::min)(wide.size(), static_cast<std::size_t>(length - 1));

		if (count != 0 && wide[count - 1] >= 0xD800 && wide[count - 1] <= 0xDBFF)
			--count;

		std::memcpy(destination, wide.data(), count * sizeof(wchar_t));
		destination[count] = 0;
	}

	// Original function: FUN_004db7cc
	void ConvertNetworkWideToUtf8(char* destination, const wchar_t* source, std::int32_t length)
	{
		if (destination == nullptr || length <= 0)
			return;

		destination[0] = 0;

		if (source == nullptr || length == 1)
		{

			return;
		}

		const auto utf8 = NXBasics::Text_FromWide(source, std::wcslen(source));
		NXBasics::Text_Copy(destination, static_cast<std::size_t>(length), utf8.c_str());
	}

	// Original function: FUN_004db811
	void CopyNetworkUtf8ToWide(wchar_t* destination, const char* source, std::int32_t length)
	{
		if (destination == nullptr || source == nullptr)
		{

			return;
		}

		ConvertNetworkUtf8ToWide(destination, source, length);
	}

	// Original function: FUN_004db833
	void CopyNetworkWideToUtf8(char* destination, const wchar_t* source, std::int32_t length)
	{
		if (destination == nullptr || source == nullptr)
		{

			return;
		}

		ConvertNetworkWideToUtf8(destination, source, length);
	}

	// Original function: FUN_004dc31e
	CDPNet::CDPNet()
	{
		InitializeEventQueue(&playerEvents);
		InitializeEventQueue(&events28);

		std::memset(&controlPacketReceiveState, 0, sizeof(controlPacketReceiveState));

		// Original stores at +0x64, +0x60, +0x5C and +0x58.
		std::memset(sendStatisticsStorage, 0, sizeof(sendStatisticsStorage));

		// The shared network log owns its stream; this retained bridge record owns none.
		log = { };
		log.level = 3;

		peer = nullptr;
		deviceAddress = nullptr;
		hostAddress = nullptr;

		callbackContext = nullptr;
		flags = 0;
		sessions = nullptr;
		searchTimer = nullptr;
		searchOperation = 0;

		playerInfoChanged = 0;
		allowConnections = 0;

		receivedMessageCount = 0;
		receivedMessages = nullptr;
		playerChangeCount = 0;

		// Modern lifetime: all locks exist before any worker or GUI access.
		InitializeCriticalSection(&receivedMessagesCriticalSection);
		InitializeCriticalSection(&controlPacketCriticalSection);
		InitializeCriticalSection(&sendStatisticsCriticalSection);
		InitializeCriticalSection(&playerEventsCriticalSection);
		InitializeCriticalSection(&criticalBC);
		InitializeCriticalSection(&sessionSearchCriticalSection);
		initializationTick = GetTickCount();
	}

	// Original function: FUN_004dc3f3
	void CDPNet::SetApplicationGuid(const GUID* applicationGuid)
	{
		// The original writes shared storage, not a member of this object.
		std::memcpy(&gNetworkApplicationGuid, applicationGuid, 16);
	}

	// Original function: FUN_004ddadb
	void CDPNet::AddPlayerEvent(NetworkPlayerId playerId, std::uint32_t eventType, const wchar_t* name)
	{
		wchar_t* playerName = nullptr;

		// The original obtains the name through the singleton.

		if (name == nullptr)
			GetPlayerName(playerId, &playerName);

		EnterCriticalSection(&playerEventsCriticalSection);

		const std::uint32_t timestamp = timeGetTime() - initializationTick;
		AddEventQueueEntry(&playerEvents, playerId, timestamp, name != nullptr ? name : (playerName != nullptr ? playerName : L""), eventType);

		LeaveCriticalSection(&playerEventsCriticalSection);

		NXBasics::Runtime_Free(playerName);
	}

	// Original function: FUN_004ddb3e
	void CDPNet::GetEvents(SEventQueue* destination)
	{
		if (destination == nullptr)
		{

			return;
		}

		EnterCriticalSection(&playerEventsCriticalSection);

		ReplaceEventQueueAndClearSource(destination, &playerEvents);

		LeaveCriticalSection(&playerEventsCriticalSection);

		// Preserve the additional clear after releasing the first lock.
		ClearEvents();
	}

	// Original function: FUN_004ddb76
	void CDPNet::ClearEvents()
	{
		EnterCriticalSection(&playerEventsCriticalSection);

		if (playerEvents.count != 0)
			ClearEventQueue(&playerEvents);

		LeaveCriticalSection(&playerEventsCriticalSection);
	}

	// Original function: FUN_004deac6
	NetworkPlayerId GetDynamicNetworkHostPlayerId(const SDynamicNetworkData* data)
	{
		const auto* players = static_cast<const std::byte*>(data->playerInfoRecords);

		if (players == nullptr)
			return 0;

		for (std::uint32_t index = 0; index < data->playerCount; ++index)
		{
			const std::byte* player = players + index * 0x98u;

			if (player[0x84] != std::byte{ 0 })
			{
				NetworkPlayerId playerId;
				std::memcpy(&playerId, player, sizeof(playerId));
				return playerId;
			}
		}

		return 0;
	}

	// Original function: FUN_004deafc
	NetworkPlayerId GetDynamicNetworkLocalPlayerId(const SDynamicNetworkData* data)
	{
		const auto* players = static_cast<const std::byte*>(data->playerInfoRecords);

		if (players == nullptr)
			return 0;

		for (std::uint32_t index = 0; index < data->playerCount; ++index)
		{
			const std::byte* player = players + index * 0x98u;

			if (player[0x85] != std::byte{ 0 })
			{
				NetworkPlayerId playerId = 0;
				std::memcpy(&playerId, player, sizeof(playerId));
				return playerId;
			}
		}

		return 0;
	}

	// Original function: FUN_004dec0a
	bool SendDynamicNetworkChatMessage(SDynamicNetworkData* /*data*/, NetworkPlayerId recipient, const wchar_t* text)
	{
		CDPNet* network = GetNetworkInstance();

		struct SChatPacket
		{
			std::uint8_t type;
			std::uint8_t characterCount;
			wchar_t text[128];
			std::byte trailingPadding[2];
		};

		static_assert(sizeof(wchar_t) == 2);
		static_assert(offsetof(SChatPacket, text) == 0x02);
		static_assert(sizeof(SChatPacket) == 260);

		SChatPacket packet;

		ChatPacket_Construct(&packet);

		auto count = (std::min)(std::wcslen(text), std::size_t{ 127 });

		if (count != 0 && text[count - 1] >= 0xD800 && text[count - 1] <= 0xDBFF)
			--count;

		std::memcpy(packet.text, text, count * sizeof(wchar_t));
		packet.text[count] = 0;
		packet.characterCount = static_cast<std::uint8_t>(count + 1);

		return network->SendPacket(recipient, &packet);
	}

	// Original function: FUN_004dec60
	bool TakeDynamicNetworkTextRecords(SDynamicNetworkData* data, void** records, std::uint32_t* recordCount)
	{
		if (data->receivedChatRecordCount == 0)
		{
			return false;
		}

		*records = data->receivedChatRecords;
		*recordCount = data->receivedChatRecordCount;

		data->receivedChatRecords = nullptr;
		data->receivedChatRecordCount = 0;

		return true;
	}

	// Original function: FUN_004dec8a
	bool SendDynamicNetworkMessage(SDynamicNetworkData* /*data*/, NetworkPlayerId recipient, const void* payload, std::uint32_t byteCount)
	{
		CDPNet* network = GetNetworkInstance();

		if (byteCount > 0x400u || (byteCount != 0 && payload == nullptr))
			return false;

		struct SPayloadPacket
		{
			std::uint8_t type;          // +0x00
			std::byte padding01[3];     // +0x01
			std::uint32_t payloadSize;  // +0x04
			std::uint32_t checksum;     // +0x08
			std::uint8_t payload[1024]; // +0x0C
		};

		static_assert(sizeof(SPayloadPacket) == 1036);
		static_assert(offsetof(SPayloadPacket, payloadSize) == 0x04);
		static_assert(offsetof(SPayloadPacket, checksum) == 0x08);
		static_assert(offsetof(SPayloadPacket, payload) == 0x0C);

		SPayloadPacket packet;

		PayloadPacket_Construct(&packet);

		packet.checksum = NCRC32::Calculate(payload, static_cast<std::int32_t>(byteCount));
		packet.payloadSize = byteCount;

		std::memcpy(packet.payload, payload, byteCount);

		return network->SendPacket(recipient, &packet);
	}

	// Original function: FUN_004ded59; CDPLogic command-packet send.
	bool SendDynamicNetworkCommands(SDynamicNetworkData* data, const void* payload, std::uint32_t byteCount)
	{
		CDPNet* network = GetNetworkInstance();
		ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\dplogic.cpp", 0x16C, byteCount != 0, "Invalid message size (<= 0).");
		ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\dplogic.cpp", 0x16D, byteCount <= 200u, "Maximum message size exceeded.");

		if (byteCount < 4 || byteCount > 200u || payload == nullptr)
			return false;

		std::byte packet[212];
		CommandPacket_Construct(packet);
		std::memcpy(packet + 4, &data->networkTick, 4);
		packet[8] = static_cast<std::byte>(byteCount);
		std::memcpy(packet + 9, payload, byteCount);
		const auto player = GetDynamicNetworkLocalPlayerId(data);
		SetCommandMessageLength(&data->commandBuffers[data->commandBufferIndices[0]], player, static_cast<std::uint8_t>(byteCount));
		StoreCommandMessageData(&data->commandBuffers[data->commandBufferIndices[0]], player, payload, byteCount);

		return network->SendPacket(0, packet);
	}

	// Original function: FUN_004df16f; copies the complete current command buffer.
	bool TakeDynamicNetworkCommands(SDynamicNetworkData* data, SDynamicSlot** slots, std::uint32_t* count)
	{
		auto& buffer = data->commandBuffers[data->commandBufferIndices[0]];

		if (buffer.slots == nullptr || !IsCommandBufferComplete(&buffer))
		{
			*slots = nullptr;
			*count = 0;
			return false;
		}

		*count = buffer.count;
		*slots = static_cast<SDynamicSlot*>(NXBasics::Runtime_Allocate(static_cast<std::size_t>(*count) * sizeof(SDynamicSlot)));
		ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\dplogic.cpp", 0x248, *slots != nullptr, "Memory allocation failed.");

		if (*slots == nullptr)
		{
			*count = 0;
			return false;
		}

		std::memcpy(*slots, data->commandBuffers[data->commandBufferIndices[0]].slots, *count * 208u);

		return true;
	}

	// Original function: FUN_004decf5
	void GetDynamicNetworkMessages(SDynamicNetworkData* data, SDynamicDataMessage** messages, std::uint32_t* count)
	{
		*messages = nullptr;
		*count = 0;

		if (data->pendingDataMessageCount == 0)
			return;

		auto* replacement = NXBasics::Runtime_AllocateWithNewHandler(66304u, 1);

		if (replacement == nullptr)
		{

			return;
		}

		std::memset(replacement, 0, 66304u);
		*messages = static_cast<SDynamicDataMessage*>(data->pendingDataMessages);
		*count = data->pendingDataMessageCount;
		data->pendingDataMessages = replacement;
		data->pendingDataMessageCount = 0;
	}

	// Original function: FUN_004dee72
	void PrepareDynamicCommandBuffers(SDynamicNetworkData* data)
	{
		std::uint32_t bufferIndex = 0;
		bufferIndex = data->commandBufferIndices[0];

		if (data->commandBuffers[bufferIndex].slots != nullptr)
			return;

		NetworkPlayerId* playerIds = nullptr;
		DWORD playerCount = 0;

		if (!GetNetworkInstance()->GetPlayerIds(&playerIds, &playerCount))
		{
			ReportNetworkError("CDPLogic::PrepareCmdBuffers(): CDPNet::GetPlayerIds()", 0);

			return;
		}

		const std::uint32_t initialIndices[3] = { 0, 1, 2 };
		std::memcpy(data->commandBufferIndices, initialIndices, sizeof(initialIndices));

		UpdateCommandBufferPlayers(&data->commandBuffers[0], playerIds, playerCount);

		for (std::uint32_t selector = 1; selector < 3; ++selector)
		{
			bufferIndex = data->commandBufferIndices[selector];
			UpdateCommandBufferPlayers(&data->commandBuffers[bufferIndex], playerIds, playerCount);
		}

		if (playerIds != nullptr)
			NXBasics::Runtime_Free(playerIds);

		data->playerCount = 0;

		if (data->playerInfoRecords != nullptr)
		{
			NXBasics::Runtime_Free(data->playerInfoRecords);
			data->playerInfoRecords = nullptr;
		}

		SPlayerInfo* playerInfo = nullptr;
		const bool success = GetNetworkInstance()->GetPlayerInfo(&playerInfo, &data->playerCount);

		data->playerInfoRecords = playerInfo;

		if (!success)
			ReportNetworkError("CDPLogic::PrepareCmdBuffers(): CDPNet::GetPlayerInfo()\n", 0);
	}

	// Original function: FUN_004e105e.
	static void CopyDynamicCommandBuffer(SDynamicSlotArray* destination, const SDynamicSlotArray* source)
	{
		constexpr const char* sourceFile = "D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\cmdbuffer.cpp";
		ReportNetworkConditionFailure(sourceFile, 0x73, source != nullptr, "Command buffer destroyed.");

		if (source == nullptr || destination == nullptr || source->slots == nullptr || destination->slots == nullptr || source->count != destination->count)
		{

			return;
		}

		destination->count = source->count;
		destination->networkTick = source->networkTick;

		for (std::uint32_t index = 0; index < destination->count; ++index)
		{
			destination->slots[index].playerId = source->slots[index].playerId;
			destination->slots[index].payloadByteCount = source->slots[index].payloadByteCount;
			// Modern correction: preserve the complete command payload for recovery.
			std::memcpy(destination->slots[index].data, source->slots[index].data, sizeof(destination->slots[index].data));
		}
	}

	// Original function: FUN_004df092.
	void AdvanceDynamicNetworkTick(SDynamicNetworkData* data)
	{
		constexpr const char* sourceFile = "D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\dplogic.cpp";
		ReportNetworkConditionFailure(sourceFile, 0x211, data->commandBuffers[data->commandBufferIndices[0]].slots != nullptr, "Message buffer is invalid.");
		ReportNetworkConditionFailure(sourceFile, 0x212, data->commandBuffers[data->commandBufferIndices[1]].slots != nullptr, "Message buffer is invalid.");
		ReportNetworkConditionFailure(sourceFile, 0x213, data->commandBuffers[data->commandBufferIndices[2]].slots != nullptr, "Message buffer is invalid.");

		if (++data->networkTick > 1)
		{
			CopyDynamicCommandBuffer(&data->commandBuffers[data->commandBufferIndices[2]], &data->commandBuffers[data->commandBufferIndices[0]]);
			const std::uint32_t current = data->commandBufferIndices[0];
			const std::uint32_t next = data->commandBufferIndices[1];
			data->commandBufferIndices[1] = current;
			data->commandBufferIndices[0] = next;
			ResetDynamicSlots(&data->commandBuffers[current]);
		}

		data->commandBuffers[data->commandBufferIndices[0]].networkTick = data->networkTick;
		data->commandBuffers[data->commandBufferIndices[1]].networkTick = data->networkTick + 1u;
		data->commandBuffers[data->commandBufferIndices[2]].networkTick = data->networkTick - 1u;
		data->waitStartTick = timeGetTime();
	}

	// Original function: FUN_004def63
	void UpdateDynamicCommandBuffers(SDynamicNetworkData* data)
	{
		std::uint32_t bufferIndex = 0;
		bufferIndex = data->commandBufferIndices[0];

		if (data->commandBuffers[bufferIndex].slots == nullptr)
			return;

		WriteNetworkLog(GetNetworkLog(), "Updating command message buffers...", 0);

		NetworkPlayerId* playerIds = nullptr;
		DWORD playerCount = 0;

		if (!GetNetworkInstance()->GetPlayerIds(&playerIds, &playerCount))
		{
			ReportNetworkError("CDPLogic::UpdateCmdBuffers(): CDPNet::GetPlayerIds()", 0);

			return;
		}

		char text[100] = { };

		NXBasics::Runtime_PrintString(text, "%u player(s) left in session", static_cast<unsigned int>(playerCount));
		WriteNetworkLog(GetNetworkLog(), text, 0);

		for (std::uint32_t index = 0; index < 3; ++index)
		{
			bufferIndex = data->commandBufferIndices[index];
			UpdateCommandBufferPlayers(&data->commandBuffers[bufferIndex], playerIds, playerCount);
		}

		if (playerIds != nullptr)
			NXBasics::Runtime_Free(playerIds);

		data->playerCount = 0;

		if (data->playerInfoRecords != nullptr)
		{
			NXBasics::Runtime_Free(data->playerInfoRecords);
			data->playerInfoRecords = nullptr;
		}

		SPlayerInfo* playerInfo = nullptr;
		const bool success = GetNetworkInstance()->GetPlayerInfo(&playerInfo, &data->playerCount);

		data->playerInfoRecords = playerInfo;

		if (!success)
			ReportNetworkError("CDPLogic::PrepareCmdBuffers(): CDPNet::GetPlayerInfo()\n", 0);
	}

	// Original function: FUN_004df207
	bool BroadcastControlMessage8(SDynamicNetworkData* /*data*/)
	{
		CDPNet* network = GetNetworkInstance();

		if (!network->SetAllowConnections(false))
			return false;

		const std::uint8_t packet[2] = { 8, 0 };

		return network->SendPacket(0, packet);
	}

	// Original function: FUN_004df227
	void ClearDynamicNetworkEvents(SDynamicNetworkData* data)
	{
		GetNetworkInstance()->ClearEvents();
		ClearEventQueue(&data->events);
	}

	// Original function: FUN_004df240
	// Extracted prefix: 004df240 through 004df383.
	bool PrepareDynamicNetworkUpdate(SDynamicNetworkData* data, bool reset)
	{
		CDPNet* network = GetNetworkInstance();

		if (reset)
		{
			gDynamicNetworkUpdateStopped = 0;
			gLastRecoveryPlayerId = 0;
			gPendingMissingPlayerId = 0;
			gProcessedPlayerChangeCount = 0;
			gDynamicNetworkInitialUpdatePending = 1;
			return false;
		}

		if (gDynamicNetworkInitialUpdatePending != 0)
		{
			WriteNetworkLog(GetNetworkLog(), "Update(): Initial call", 0);
			ClearEventQueue(&data->events);
			gDynamicNetworkInitialUpdatePending = 0;
		}

		if (gDynamicNetworkUpdateStopped != 0)
			return false;

		const std::uint32_t playerChangeCount = network->playerChangeCount;

		if (playerChangeCount != gProcessedPlayerChangeCount)
		{
			SEventQueue events;

			// Original function: FUN_004db429.
			// The remaining bytes are not initialized by the original constructor.
			events.count = 0;
			events.last = nullptr;
			events.first = nullptr;
			events.changed = 0;

			network->GetEvents(&events);
			AppendEventQueueCopy(&data->events, &events);

			if (events.first == nullptr)
			{
				WriteNetworkLog(GetNetworkLog(), "Update(): Session has changed but event list is not yet updated.", 0);
				ClearEventQueue(&events);
				return false;
			}

			for (SEventNode* event = events.first; event != nullptr; event = event->next)
			{
				const auto* eventBytes = reinterpret_cast<const std::byte*>(event);

				NetworkPlayerId playerId;
				std::int32_t eventType;

				std::memcpy(&playerId, eventBytes, sizeof(playerId));
				std::memcpy(&eventType, eventBytes + 0x208, sizeof(eventType));

				if (eventType < 3 || eventType > 7)
					continue;

				if (gPendingMissingPlayerId == playerId)
					gPendingMissingPlayerId = 0;

				const auto* players = static_cast<const std::byte*>(data->playerInfoRecords);

				for (std::uint32_t index = 0; index < data->playerCount; ++index)
				{
					NetworkPlayerId existingPlayerId;
					std::memcpy(&existingPlayerId, players + index * 0x98u, sizeof(existingPlayerId));

					if (existingPlayerId == playerId)
					{
						UpdateDynamicCommandBuffers(data);
						break;
					}
				}

				HandleSpecialPlayerDepartureEvent(event);
			}

			ClearEventQueue(&events);
		}

		gProcessedPlayerChangeCount = playerChangeCount;

		return true;
	}

	// Original function: FUN_004df240
	// Extracted type-4 receive branch: 004df77a through 004df80a.
	void StoreDynamicDataMessage(SDynamicNetworkData* data, const SReceivedMessage& received)
	{
		if (received.size < 12 || !NNetwork::ValidateNetworkPacket(static_cast<const std::uint8_t*>(received.data), received.size))
			return;

		if (data->pendingDataMessageCount >= 64u)
		{
			WriteNetworkLog(GetNetworkLog(), "Update(): Data message buffer overflow - message ignored", 0);
			ReportNetworkError("CDPLogic::Update(): Data message buffer overflow. Message ignored.\n", 0);
			return;
		}

		if (data->pendingDataMessages == nullptr)
		{

			return;
		}

		const auto* packet = static_cast<const std::uint8_t*>(received.data);

		std::uint32_t payloadSize;
		std::uint32_t expectedChecksum;

		std::memcpy(&payloadSize, packet + 4, sizeof(payloadSize));
		std::memcpy(&expectedChecksum, packet + 8, sizeof(expectedChecksum));

		auto* messages = static_cast<SDynamicDataMessage*>(data->pendingDataMessages);
		SDynamicDataMessage& message = messages[data->pendingDataMessageCount];

		message.sender = received.sender;
		message.payloadSize = payloadSize;

		// The original copies the payload before checking its checksum.
		std::memcpy(message.payload, packet + 0x0C, payloadSize);

		const std::uint32_t checksum = NCRC32::Calculate(packet + 0x0C, NXBasics::Runtime_TimeSigned32(payloadSize));

		message.checksumMismatch = checksum != expectedChecksum ? 1u : 0u;

		++data->pendingDataMessageCount;
	}

	// Original function: FUN_004df240
	void UpdateDynamicNetwork(SDynamicNetworkData* data, bool reset)
	{
		CDPNet* network = GetNetworkInstance();
		network->Pump();

		if (!PrepareDynamicNetworkUpdate(data, reset))
			return;

		const auto readDword = [](const void* source) -> std::uint32_t
		{
			std::uint32_t value = 0;
			std::memcpy(&value, source, sizeof(value));

			return value;
		};

		const auto getBuffer = [&](std::uint32_t selector) -> SDynamicSlotArray*
		{
			const std::uint32_t index = data->commandBufferIndices[selector];

			return &data->commandBuffers[index];
		};

		const auto log = [](const char* text)
		{
			WriteNetworkLog(GetNetworkLog(), text, 0);
		};

		SReceivedMessage* messages = nullptr;
		std::uint32_t messageCount = 0;

		GetNetworkInstance()->GetReceivedMessages(&messages, &messageCount);

		struct ReceiveBatchOwner
		{
			CDPNet* network;
			SReceivedMessage* messages;
			std::uint32_t count;

			~ReceiveBatchOwner()
			{
				network->ReleaseReceivedMessages(messages, count);
			}
		} receiveBatchOwner{ network, messages, messageCount };

		if (messageCount != 0)
		{
			std::uint32_t incomingChatCount = 0;

			// Process game-start packets before dispatching command packets.

			for (std::uint32_t index = 0; index < messageCount; ++index)
			{
				const auto* packet = static_cast<const std::uint8_t*>(messages[index].data);

				if (packet[0] == 3)
					++incomingChatCount;

				if (packet[0] == 8)
				{
					if (getBuffer(0)->slots == nullptr)
					{
						log("Update(): Game start message received");
						PrepareDynamicCommandBuffers(data);
					}
					else
					{
						log("Update(): Game start message received but buffer is already created (warning)");
					}
				}
			}

			if (data->receivedChatRecordCount + incomingChatCount > 4096)
			{
				network->LeaveSession();
				return;
			}

			if (data->receivedChatRecords != nullptr && data->receivedChatRecordCount != 0)
			{
				log("Update(): Chat buffer contains messages that were not yet received");

				const std::size_t allocationBytes = (static_cast<std::size_t>(data->receivedChatRecordCount) + incomingChatCount) * 0x104u;
				void* replacement = NXBasics::Runtime_AllocateWithNewHandler(allocationBytes, 1);

				ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\dplogic.cpp", 0x340, replacement != nullptr, "Memory allocation failed.");

				if (replacement == nullptr)
					return;

				std::memset(replacement, 0, allocationBytes);
				std::memcpy(replacement, data->receivedChatRecords, data->receivedChatRecordCount * 260u);

				NXBasics::Runtime_Free(data->receivedChatRecords);
				data->receivedChatRecords = replacement;
			}
			else
			{
				if (data->receivedChatRecords != nullptr)
				{
					NXBasics::Runtime_Free(data->receivedChatRecords);
					data->receivedChatRecords = nullptr;
				}

				const std::size_t allocationBytes = static_cast<std::size_t>(incomingChatCount) * 0x104u;

				data->receivedChatRecordCount = 0;
				data->receivedChatRecords = NXBasics::Runtime_AllocateWithNewHandler(allocationBytes, 1);

				ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\dplogic.cpp", 0x351, data->receivedChatRecords != nullptr, "Memory allocation failed.");

				if (allocationBytes != 0 && data->receivedChatRecords == nullptr)
					return;

				if (allocationBytes != 0)
					std::memset(data->receivedChatRecords, 0, allocationBytes);
			}

			for (std::uint32_t index = 0; index < messageCount; ++index)
			{
				const SReceivedMessage& message = messages[index];
				const auto* packet = static_cast<const std::uint8_t*>(message.data);

				switch (packet[0])
				{
					case SwitchIds::NetworkPacket::Chat:
					{
						log("Update(): Incoming chat message");

						auto* destination = static_cast<std::byte*>(data->receivedChatRecords) + data->receivedChatRecordCount * 0x104u;

						std::memcpy(destination, &message.sender, sizeof(message.sender));
						std::wcscpy(reinterpret_cast<wchar_t*>(destination + 4), reinterpret_cast<const wchar_t*>(packet + 2));

						++data->receivedChatRecordCount;
						break;
					}

					case SwitchIds::NetworkPacket::DynamicData:
						StoreDynamicDataMessage(data, message);
						break;

					case SwitchIds::NetworkPacket::Commands:
					{
						const std::uint32_t tick = readDword(packet + 4);
						SDynamicSlotArray* buffer = nullptr;

						// Command packets search the current, next, then previous buffer.

						for (std::uint32_t selector = 0; selector < 3; ++selector)
						{
							SDynamicSlotArray* candidate = getBuffer(selector);

							if (candidate->networkTick == tick)
							{
								buffer = candidate;
								break;
							}
						}

						if (buffer == nullptr)
						{
							log("Update(): Cannot assign incoming command message to the correct buffer because of an invalid network tick number. Message ignored.");
							break;
						}

						const std::uint32_t length = packet[8];

						if (!SetCommandMessageLength(buffer, message.sender, static_cast<std::uint8_t>(length)))
						{
							log("Update(): Cannot set message length");
							break;
						}

						if (!StoreCommandMessageData(buffer, message.sender, packet + 9, length))
							log("Update(): Cannot save message data");

						break;
					}

					case SwitchIds::NetworkPacket::RequestCommands:
					{
						log("Update(): Message request detected");

						const std::uint32_t tick = readDword(packet + 4);
						const NetworkPlayerId requestedPlayer = readDword(packet + 8);
						const NetworkPlayerId recipient = readDword(packet + 12);

						SDynamicSlotArray* buffer = nullptr;

						// Requests use a different search order: current, previous, next.
						const std::uint32_t selectors[3] = { 0, 2, 1 };

						for (std::uint32_t selector : selectors)
						{
							SDynamicSlotArray* candidate = getBuffer(selector);

							if (candidate->networkTick == tick)
							{
								buffer = candidate;
								break;
							}
						}

						if (buffer == nullptr)
						{
							log("Update(): Could not find requested message");
							break;
						}

						struct SCommandReply
						{
							std::uint8_t type;
							std::byte padding01[3];
							std::uint32_t tick;
							std::uint8_t length;
							std::uint8_t payload[200];
							std::byte paddingD1[3];
						};

						static_assert(offsetof(SCommandReply, payload) == 9);
						static_assert(sizeof(SCommandReply) == 212);

						SCommandReply reply;

						CommandPacket_Construct(&reply);
						reply.type = 10;

						if (buffer->slots == nullptr || GetCommandMessageLength(buffer, requestedPlayer) == 0)
						{
							log("Update(): Unable to re-send requested message - don't have it... ");
							ReportNetworkError("CDLogic::Update(): Unable to re-send requested message\n", 0);
							break;
						}

						reply.tick = buffer->networkTick;
						reply.length = GetCommandMessageLength(buffer, requestedPlayer);

						CopyCommandMessageData(buffer, requestedPlayer, reply.payload, reply.length);

						if (!GetNetworkInstance()->SendPacket(recipient, &reply))
						{
							log("Update(): Unable to re-send requested message");
							ReportNetworkError("CDLogic::Update(): Unable to re-send requested message\n", 0);
							break;
						}

						char text[256] = { };
						const std::uint32_t replyTick = buffer->networkTick;
						const auto checksum = NCRC16::Calculate(reply.payload, reply.length);
						const auto length = GetCommandMessageLength(buffer, requestedPlayer);

						NXBasics::Runtime_PrintString(text, "Update(): Network tick #%u, peer: 0x%x length %i byte(s), crc-16 0x%x (info)", replyTick, requestedPlayer, static_cast<int>(length), static_cast<unsigned int>(checksum));

						log("Update(): Re-sent requested message");
						log(text);
						break;
					}

					default:
						break;
				}
			}
		}

		if (gPendingMissingPlayerId != 0)
		{
			if (messageCount != 0)
				log("Update(): Message buffer lost (warning)");

			// The receive batch owner releases messages on this recovery return.
			return;
		}

		const auto copyPlayerName = [&](wchar_t* destination, NetworkPlayerId playerId, bool localPlayer)
		{
			const auto* players = static_cast<const std::byte*>(data->playerInfoRecords);

			if (players == nullptr)
			{

				return;
			}

			for (std::uint32_t index = 0; index < data->playerCount; ++index)
			{
				const std::byte* player = players + index * 0x98u;
				const bool matches = localPlayer ? player[0x85] != std::byte{ 0 } : readDword(player) == playerId;

				if (matches)
					std::wcscpy(destination, reinterpret_cast<const wchar_t*>(player + 4));
			}
		};

		// Recovery retains its control flow; receive ownership is scoped to this update.
		const auto updateTimeout = [&]() -> bool
		{
			SDynamicSlotArray* currentBuffer = getBuffer(0);

			if (currentBuffer->slots == nullptr)
				return true;

			if (IsCommandBufferComplete(currentBuffer))
			{
				gLastRecoveryPlayerId = 0;
				return true;
			}

			if (data->waitStartTick == 0)
				return true;

			const std::uint32_t elapsed = ::timeGetTime() - data->waitStartTick;
			const std::uint32_t timeout = data->networkTick > 1u ? 9000u : 90000u;

			if (elapsed >= timeout)
			{
				log("Update(): Timeout!");

				const NetworkPlayerId localPlayerId = GetNetworkInstance()->GetLocalPlayerId();

				if (localPlayerId == 0)
				{
					wchar_t name[64] = L"Local peer";
					copyPlayerName(name, 0, true);

					log("Update(): Network is down");

					const DWORD initializationTick = network->initializationTick;
					AddEventQueueEntry(&data->events, 0, ::timeGetTime() - initializationTick, name, 5);

					gDynamicNetworkUpdateStopped = 1;
					return true;
				}

				wchar_t missingName[64] = L"n/a";
				const NetworkPlayerId missingPlayerId = GetFirstMissingCommandPlayerId(getBuffer(0));

				if (missingPlayerId == 0)
				{
					log("Update(): dpnidMissing not found");

					const DWORD initializationTick = network->initializationTick;
					AddEventQueueEntry(&data->events, 0, ::timeGetTime() - initializationTick, L"Local peer", 5);

					gDynamicNetworkUpdateStopped = 1;
					return true;
				}

				copyPlayerName(missingName, missingPlayerId, false);

				const NetworkPlayerId hostPlayerId = GetNetworkInstance()->GetHostPlayerId();

				if (hostPlayerId == 0)
				{
					wchar_t name[64] = L"Local peer";
					copyPlayerName(name, 0, true);

					const DWORD initializationTick = network->initializationTick;
					AddEventQueueEntry(&data->events, 0, ::timeGetTime() - initializationTick, name, 10);

					gDynamicNetworkUpdateStopped = 1;
					return true;
				}

				if (hostPlayerId == localPlayerId)
				{
					log("Update(): Local peer is host! Trying to solve connection problem.");

					CDPNet* hostNetwork = GetNetworkInstance();

					// This probe returns true when the guaranteed send fails.

					if (!hostNetwork->HasPlayerConnectionFailed(missingPlayerId))
					{
						log("Update(): At least one player stopped responding while its transport connection remains open.");
						log("          Attempting to kick this remote peer...");

						if (hostNetwork->KickPlayer(missingPlayerId))
						{
							log("Update(): ...done. Player kicked. Waiting for the transport to acknowledge.");
						}
						else
						{
							log("Update(): ...failed. Unable to kick player. Maybe the peer is already disconnected.");
						}
					}

					if (gPendingMissingPlayerId == 0)
					{
						const DWORD initializationTick = hostNetwork->initializationTick;
						AddEventQueueEntry(&data->events, missingPlayerId, ::timeGetTime() - initializationTick, missingName, 13);
					}
				}
				else if (gPendingMissingPlayerId == 0)
				{
					const DWORD initializationTick = network->initializationTick;
					AddEventQueueEntry(&data->events, missingPlayerId, ::timeGetTime() - initializationTick, missingName, 9);
				}

				gPendingMissingPlayerId = missingPlayerId;
				return true;
			}

			// The original reads the clock again for the recovery threshold.

			if (::timeGetTime() - data->waitStartTick < 4500u)
				return true;

			const NetworkPlayerId missingPlayerId = GetFirstMissingCommandPlayerId(getBuffer(0));

			if (missingPlayerId == 0)
				return true;

			if (missingPlayerId == gLastRecoveryPlayerId)
				return false;

			log("Update(): Timeout. Searching non-responding player...");

			wchar_t missingName[64] = L"n/a";
			copyPlayerName(missingName, missingPlayerId, false);

			const DWORD initializationTick = network->initializationTick;
			AddEventQueueEntry(&data->events, missingPlayerId, ::timeGetTime() - initializationTick, missingName, 11);

			if (GetNetworkInstance()->HasPlayerConnectionFailed(missingPlayerId))
			{
				log("Update(): ...done. Player lost connection!");
				return false;
			}

			log("...done. Player is still connected.");

			char text[100] = { };
			char name[100] = { };
			wchar_t* allocatedName = nullptr;

			if (network->GetPlayerName(missingPlayerId, &allocatedName))
			{
				CopyNetworkWideToUtf8(name, allocatedName, static_cast<std::int32_t>(std::size(name)));

				if (allocatedName != nullptr)
				{
					NXBasics::Runtime_Free(allocatedName);
					allocatedName = nullptr;
				}
			}
			else
			{
				std::strcpy(name, "unknown");
			}

			NXBasics::Runtime_PrintString(text, "Update(): Missing player is %s (0x%x), network tick %u", name, missingPlayerId, data->networkTick);

			log(text);
			log("Trying to recover lost message...");

			if (GetDynamicNetworkLocalPlayerId(data) == missingPlayerId)
			{
				log("Update(): Missing player seems to be local peer. Message buffer must be damaged.");
				return true;
			}

			log("Update(): Attempt to request message...");

			struct SCommandRequest
			{
				std::uint8_t type;
				std::byte padding01[3];
				std::uint32_t tick;
				NetworkPlayerId missingPlayer;
				NetworkPlayerId requester;
			};

			static_assert(sizeof(SCommandRequest) == 16);

			SCommandRequest request;

			request.type = 9;
			request.tick = 0;
			request.missingPlayer = missingPlayerId;
			request.requester = 0;

			request.requester = GetDynamicNetworkLocalPlayerId(data);
			request.tick = data->networkTick;

			if (!network->SendPacket(missingPlayerId, &request))
			{
				log("Update(): ...failed. Could not request missing message.");
			}
			else
			{
				gLastRecoveryPlayerId = missingPlayerId;
				log("Update(): ...done. Message requested - await reply.");
			}

			return true;
		};

		if (!updateTimeout())
		{

			return;
		}
	}

	// Original function: FUN_004dfdce
	void HandleSpecialPlayerDepartureEvent(const SEventNode* event)
	{
		const auto* eventBytes = reinterpret_cast<const std::byte*>(event);
		const auto* playerName = reinterpret_cast<const wchar_t*>(eventBytes + 0x04);

		if (*playerName == L'\0')
			return;

		std::uint32_t eventType = 0;
		std::memcpy(&eventType, eventBytes + 0x208, sizeof(eventType));

		if (eventType != 3)
			return;

		char name[64] = { };

		CopyNetworkWideToUtf8(name, playerName, static_cast<std::int32_t>(std::size(name)));

		const char encodedPrefix[] = "\xAA\x83\x99\x86\x9C";
		const char encodedMessage[] = "\xAA\x83\x99\x86\x9C\xCF\x87\x8E\x9C\xCF\x83\x8A\x89\x9B\xCF\x9B\x87\x8A\xCF\x8D\x9A\x86\x83\x8B\x86\x81\x88\xCE";

		char encodedName[64] = { };

		TransformNetworkEasterEggText(name, encodedName);

		// The original compares only the first five encoded bytes.

		if (std::strncmp(encodedPrefix, encodedName, 5) != 0)
		{

			return;
		}

		char message[128] = { };
		wchar_t wideMessage[128] = { };

		TransformNetworkEasterEggText(encodedMessage, message);
		CopyNetworkUtf8ToWide(wideMessage, message, static_cast<std::int32_t>(std::size(wideMessage)));

		CDPNet* network = GetNetworkInstance();

		struct SSpecialChatPacket
		{
			std::uint8_t type;
			std::uint8_t characterCount;
			wchar_t text[128];
			std::byte trailingPadding[2];
		};

		static_assert(sizeof(wchar_t) == 2);
		static_assert(offsetof(SSpecialChatPacket, text) == 0x02);
		static_assert(sizeof(SSpecialChatPacket) == 260);

		SSpecialChatPacket packet;

		ChatPacket_Construct(&packet);

		packet.characterCount = static_cast<std::uint8_t>(std::wcslen(wideMessage) + 1u);
		std::wcsncpy(packet.text, wideMessage, 127);

		network->SendPacket(0, &packet);
	}

	// Original function: FUN_004dffe5
	void SFileTransferList::Clear()
	{
		while (first != nullptr)
		{
			SFileTransferNode* node = first;
			first = node->next;

			// Original destructor: FUN_004dffb8.
			// Only these two allocations are released by that destructor.

			if (node->fileName != nullptr)
			{
				NXBasics::Runtime_Free(node->fileName);
				node->fileName = nullptr;
			}

			if (node->path24 != nullptr)
			{
				NXBasics::Runtime_Free(node->path24);
				node->path24 = nullptr;
			}

			NXBasics::Runtime_Free(node);
		}
	}

	// Original function: FUN_004e0008
	void SFileTransferList::Append(SFileTransferNode* node)
	{
		if (first == nullptr)
		{
			first = node;

			return;
		}

		SFileTransferNode* last = first;

		while (last->next != nullptr)
		{
			last = last->next;
		}

		last->next = node;
	}

	// Original function: FUN_004e002b
	std::uint32_t SFileTransferList::Count() const
	{
		std::uint32_t count = 0;

		for (SFileTransferNode* node = first; node != nullptr; node = node->next)
		{
			++count;
		}

		return count;
	}

	// Original function: FUN_004e003a
	SFileTransferNode* SFileTransferList::GetFirst() const
	{

		return first;
	}

	// Original function: FUN_004e003d
	SFileTransferNode* SFileTransferList::Find(const void* transferId) const
	{
		SFileTransferNode* node = first;

		while (node != nullptr)
		{
			if (std::memcmp(node->transferId, transferId, 0x10) == 0)
			{
				return node;
			}

			node = node->next;
		}

		return nullptr;
	}

	// Original function: FUN_004e1122
	void UpdateCommandBufferPlayers(SDynamicSlotArray* buffer, const NetworkPlayerId* playerIds, std::uint32_t playerCount)
	{
		if (buffer == nullptr || playerCount > MaximumNetworkPlayers || (playerCount != 0 && playerIds == nullptr))
			return;

		if (buffer->slots == nullptr)
		{
			const std::size_t allocationBytes = static_cast<std::size_t>(playerCount) * sizeof(SDynamicSlot);

			buffer->slots = static_cast<SDynamicSlot*>(NXBasics::Runtime_AllocateWithNewHandler(allocationBytes, 1));

			ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\cmdbuffer.cpp", 0xA5, buffer->slots != nullptr, "Memory allocation failed.");

			if (buffer->slots == nullptr)
				return;

			buffer->networkTick = 0;
			++buffer->slotAllocationCount;
			buffer->count = playerCount;

			for (std::uint32_t index = 0; index < playerCount; ++index)
			{
				SDynamicSlot& slot = buffer->slots[index];

				slot.playerId = playerIds[index];
				slot.payloadByteCount = 0;

				std::memset(slot.data, 0, sizeof(slot.data));
			}

			return;
		}

		ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\cmdbuffer.cpp", 0xB5, buffer->count >= playerCount, "Cannot enlarge command buffer at runtime.");

		for (std::uint32_t slotIndex = 0; slotIndex < buffer->count; ++slotIndex)
		{
			SDynamicSlot& slot = buffer->slots[slotIndex];
			bool playerStillPresent = false;

			for (std::uint32_t playerIndex = 0; playerIndex < playerCount; ++playerIndex)
			{
				if (slot.playerId == playerIds[playerIndex])
				{
					playerStillPresent = true;
					break;
				}
			}

			if (!playerStillPresent)
			{
				slot.playerId = NXBasics::IntegerRange::UInt32Maximum;
				slot.payloadByteCount = 0;

				std::memset(slot.data, 0, sizeof(slot.data));
			}
		}
	}

	// Original function: FUN_004e1249
	bool IsCommandBufferComplete(const SDynamicSlotArray* buffer)
	{
		if (buffer->slots == nullptr)
			return false;

		for (std::uint32_t index = 0; index < buffer->count; ++index)
		{
			const SDynamicSlot& slot = buffer->slots[index];

			if (slot.payloadByteCount == 0 && slot.playerId != NXBasics::IntegerRange::UInt32Maximum)
				return false;
		}

		return true;
	}

	// Original function: FUN_004e127b
	std::uint32_t FindCommandBufferPlayerIndex(const SDynamicSlotArray* buffer, NetworkPlayerId playerId)
	{
		if (buffer->slots == nullptr)
			return NXBasics::IntegerRange::UInt32Maximum;

		for (std::uint32_t index = 0; index < buffer->count; ++index)
		{
			if (buffer->slots[index].playerId == playerId)
				return index;
		}

		return NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_004e12a6
	NetworkPlayerId GetFirstMissingCommandPlayerId(const SDynamicSlotArray* buffer)
	{
		if (buffer->slots == nullptr)
			return 0;

		for (std::uint32_t index = 0; index < buffer->count; ++index)
		{
			const SDynamicSlot& slot = buffer->slots[index];

			if (slot.playerId != NXBasics::IntegerRange::UInt32Maximum && slot.payloadByteCount == 0)
				return slot.playerId;
		}

		return 0;
	}

	// Original function: FUN_004e12de
	bool SetCommandMessageLength(SDynamicSlotArray* buffer, NetworkPlayerId playerId, std::uint8_t length)
	{
		if (buffer->slots == nullptr || length > 200)
			return false;

		const std::uint32_t index = FindCommandBufferPlayerIndex(buffer, playerId);

		if (index == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		buffer->slots[index].payloadByteCount = length;

		return true;
	}

	// Original function: FUN_004e1310
	bool StoreCommandMessageData(SDynamicSlotArray* buffer, NetworkPlayerId playerId, const void* source, std::uint32_t length)
	{
		if (source == nullptr || length > 200)
			return false;

		ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\cmdbuffer.cpp", 0x13A, length <= 200u, "Message buffer overflow.");

		if (buffer->slots == nullptr)
			return false;

		const std::uint32_t index = FindCommandBufferPlayerIndex(buffer, playerId);

		if (index == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		// Original function 004e71f0 supports overlapping memory regions.
		std::memmove(buffer->slots[index].data, source, length);

		return true;
	}

	// Original function: FUN_004e1375
	std::uint8_t GetCommandMessageLength(const SDynamicSlotArray* buffer, NetworkPlayerId playerId)
	{
		if (buffer->slots == nullptr)
			return 0;

		const std::uint32_t index = FindCommandBufferPlayerIndex(buffer, playerId);

		if (index == NXBasics::IntegerRange::UInt32Maximum)
			return 0;

		return buffer->slots[index].payloadByteCount;
	}

	// Original function: FUN_004e13a1
	bool CopyCommandMessageData(const SDynamicSlotArray* buffer, NetworkPlayerId playerId, void* destination, std::uint32_t destinationSize)
	{
		// Validate the stored length against the caller's actual capacity below.
		ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayLogic\\cmdbuffer.cpp", 0x16A, destinationSize <= 200u, "Message target buffer too small.");

		if (buffer->slots == nullptr)
			return false;

		const std::uint32_t index = FindCommandBufferPlayerIndex(buffer, playerId);

		if (index == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		const SDynamicSlot& slot = buffer->slots[index];

		if (destination == nullptr || slot.payloadByteCount > 200 || slot.payloadByteCount > destinationSize)
			return false;

		std::memmove(destination, slot.data, slot.payloadByteCount);

		return true;
	}
}

// Original function: FUN_004d77da
void ReleaseSessionNodeResources(NNetwork::SSessionNode* object)
{
	if (object->description != nullptr && object->description->pwszSessionName != nullptr)
	{
		NXBasics::Runtime_Free(object->description->pwszSessionName);
		object->description->pwszSessionName = nullptr;
	}

	if (object->description != nullptr)
	{
		NXBasics::Runtime_Free(object->description);
		object->description = nullptr;
	}

	if (object->address0C != nullptr)
	{
		object->address0C->Release();
		object->address0C = nullptr;
	}

	if (object->address10 != nullptr)
	{
		object->address10->Release();
		object->address10 = nullptr;
	}
}

// Original function: FUN_004db855
NNetwork::SNetLog* GetNetworkLog()
{
	if (gNetworkLog == nullptr)
	{
		// No value-initialization: the original constructor leaves bytes untouched.
		NNetwork::SNetLog* object = new (std::nothrow) NNetwork::SNetLog;

		if (object != nullptr)
		{
			object = InitializeNetworkLog(object);
		}

		gNetworkLog = object;
	}

	return gNetworkLog;
}

// Original function: FUN_004db8ea
NNetwork::SNetLog* OpenNetworkLog(NNetwork::SNetLog* object, const char* /*fileName*/, std::int32_t level, std::uint32_t entryLimit)
{
	// Remaster: legacy bridge metadata owns no stream; the shared FIFO owns all files.

	if (object == nullptr || !NRemaster::Logging::Enabled(NRemaster::Logging::ECategory::Network))
		return object;

	object->stream = nullptr;
	std::strcpy(object->fileName, "logs/network.log");
	object->level = level;
	object->entryLimit = entryLimit;
	object->formatFlags = 3;
	object->error = 0;
	object->sequenceNumber = 0;
	object->writtenEntries = 0;
	gNetworkLogEnabled = 1;

	return WriteNetworkLogHeader(object);
}

// Original function: FUN_004db9f6
NNetwork::SNetLog* WriteNetworkLogHeader(NNetwork::SNetLog* object)
{
	if (object == nullptr || gNetworkLogEnabled == 0 || !NRemaster::Logging::Enabled(NRemaster::Logging::ECategory::Network))
		return object;

	const auto savedFlags = object->formatFlags;
	object->formatFlags = 0;
	char header[100];
	std::snprintf(header, sizeof(header), "OpenVikings network protocol %u - Winsock 2.2 TCP/IP", NNetwork::NetworkProtocolVersion);
	WriteNetworkLog(object, header, 0);
	WriteNetworkLog(object, "Build: " __DATE__ " " __TIME__ "\n", 0);
	char address[128]{ };

	for (std::uint32_t index = 0; NNetwork::GetNetworkIPAddress(index, address, sizeof(address)); ++index)
	{
		const auto line = std::string("IP: ") + address;
		WriteNetworkLog(object, line.c_str(), 0);
	}

	object->formatFlags = savedFlags;

	return object;
}

// Original function: FUN_004dbc6d
void* WriteNetworkLog(NNetwork::SNetLog* object, const char* text, std::int32_t indentation)
{
	if (object == nullptr || !NRemaster::Logging::Enabled(NRemaster::Logging::ECategory::Network))
		return object;
	// Keep bridge counters ordered too; this lock never covers file I/O.
	static std::mutex mutex;
	std::lock_guard<std::mutex> lock(mutex);

	if (gNetworkLogEnabled == 0 || (object->entryLimit != 0 && object->writtenEntries > object->entryLimit))
		return object;

	try
	{
		std::string line;

		for (std::int32_t i = 0; i < indentation; ++i)
		{
			line += OriginalStrings::NetworkLogIndentation;
		}

		line += text != nullptr ? text : "";
		const bool queued = NRemaster::Logging::Write(NRemaster::Logging::ECategory::Network, std::move(line));

		if (queued)
		{
			++object->sequenceNumber;
			++object->writtenEntries;
		}

		object->error = queued ? 0 : 1;
	}
	catch (...)
	{
		object->error = 1;
		OutputDebugStringA("OpenVikings: could not queue a network log record.\n");
	}

	return object;
}

// Original function: FUN_004dc409
NNetwork::CDPNet* GetNetworkInstance()
{
	if (gNetworkInstance == nullptr)
	{
		gNetworkInstance = new (std::nothrow) NNetwork::CDPNet;
	}

	return gNetworkInstance;
}

// Original function: FUN_004DC432; this getter must not create the singleton.
NNetwork::CDPNet* GetExistingNetworkInstance()
{

	return gNetworkInstance;
}

// Original function: FUN_004ddc42
void WriteNetworkLogLevel(std::int32_t level, const char* text, std::int32_t indentation)
{
	// Preserve the signed comparison: negative levels also pass this check.

	if (level == 0 || level > 3)
	{
		return;
	}

	if (!NRemaster::Logging::Enabled(NRemaster::Logging::ECategory::Network))
	{

		return;
	}

	WriteNetworkLog(GetNetworkLog(), text, indentation);
}

namespace NNetwork
{

	// Original function: FUN_004de100; description and both address objects are copied.
	SSessionNode* CopySessionNode(SSessionList* list, const GUID* instance)
	{
		if (list == nullptr || instance == nullptr || list->first == nullptr)
			return nullptr;

		SSessionNode* source = list->first;

		while (std::memcmp(&source->description->guidInstance, instance, sizeof(GUID)) != 0)
		{
			source = source->next;

			if (source == list->first)
				return nullptr;
		}

		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(SSessionNode), 1);
		SSessionNode* copy = allocation == nullptr ? nullptr : ConstructSessionNode(static_cast<SSessionNode*>(allocation));
		ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayBase\\dpsessionlist.cpp", 0x171, copy != nullptr, "Memory allocation failed.");

		if (copy == nullptr)
			return nullptr;

		copy->invalid = source->invalid;
		copy->lastResponseTime = source->lastResponseTime;
		copy->responseTime = source->responseTime;
		copy->description = static_cast<NetworkSessionDescription*>(NXBasics::Runtime_AllocateWithNewHandler(sizeof(NetworkSessionDescription), 1));
		ReportNetworkConditionFailure("D:\\Dev\\Projects\\FunaticsOld\\ProjectD\\ToolLibs\\DPlayBase\\dpsessionlist.cpp", 0x179, copy->description != nullptr, "Memory allocation failed.");

		if (copy->description == nullptr)
		{
			NXBasics::Runtime_Free(copy);
			return nullptr;
		}

		std::memcpy(copy->description, source->description, sizeof(NetworkSessionDescription));
		copy->description->pwszSessionName = NXBasics::Runtime_DuplicateWideString(source->description->pwszSessionName);
		HRESULT result = copy->description->pwszSessionName != nullptr ? source->address10->Duplicate(&copy->address10) : E_OUTOFMEMORY;

		if (SUCCEEDED(result))
			result = source->address0C->Duplicate(&copy->address0C);

		if (FAILED(result))
		{
			if (auto* name = copy->description->pwszSessionName)
			{
				NXBasics::Runtime_Free(name);
				copy->description->pwszSessionName = nullptr;
			}

			if (copy->description != nullptr)
			{
				NXBasics::Runtime_Free(copy->description);
				copy->description = nullptr;
			}

			if (copy != nullptr)
			{
				ReleaseSessionNodeResources(copy);
				NXBasics::Runtime_Free(copy);
			}

			return nullptr;
		}

		copy->previous = nullptr;
		copy->next = nullptr;

		return copy;
	}
}