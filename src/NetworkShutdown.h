#pragma once

#include "ModernNetwork.h"
#include <Windows.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace NNetwork
{
	class CDPNet;
	void* CommandPacket_Construct(void* packet);
	void* ChatPacket_Construct(void* packet);
	void* PayloadPacket_Construct(void* packet);
	void* FileTransferAnnouncementPacket_Construct(void* packet);
	bool GetNetworkIPAddress(std::uint32_t index, char* destination, std::uint32_t capacity);

	struct SEventNode
	{
		NetworkPlayerId playerId;                           // +0x000
		wchar_t playerName[0x100];                // +0x004
		std::uint32_t timestamp;                  // +0x204
		std::uint32_t eventType;                  // +0x208
		SEventNode* next;                         // +0x20C
	};

	static_assert(sizeof(SEventNode) == 528);
	static_assert(offsetof(SEventNode, playerName) == 0x004);
	static_assert(offsetof(SEventNode, timestamp) == 0x204);
	static_assert(offsetof(SEventNode, eventType) == 0x208);

	struct SEventQueue
	{
		std::uint8_t unknown00;                    // +0x00
		std::uint8_t changed;                      // +0x01
		std::byte unknown02[2];
		std::uint32_t count;                       // +0x04
		SEventNode* first;                         // +0x08
		SEventNode* last;                          // +0x0C
	};

	// Original function: FUN_004db528
	SEventQueue* ClearEventQueue(SEventQueue* queue);

	struct SReceivedMessage
	{
		NetworkPlayerId sender;                             // +0x00
		std::uint32_t size;                        // +0x04
		void* data;                               // +0x08
		NetworkBufferHandle bufferHandle;                    // +0x0C
	};

	struct SSessionNode
	{
		std::uint8_t invalid;                       // +0x00
		std::byte unknown01[3];
		std::uint32_t responseTime;                 // +0x04
		std::uint32_t lastResponseTime;             // +0x08
		NetworkAddress* address0C;              // +0x0C
		NetworkAddress* address10;              // +0x10
		NetworkSessionDescription* description;          // +0x14
		SSessionNode* next;                         // +0x18
		SSessionNode* previous;                     // +0x1C
	};

	struct SSessionList
	{
		std::uint32_t count;                       // +0x00
		std::uint32_t flags;                       // +0x04
		SSessionNode* first;                       // +0x08
	};
	void DestroySessionListContents(SSessionList* list);

	struct SSessionSummary
	{
		wchar_t* name;
		GUID instance;
	};
	static_assert(sizeof(SSessionSummary) == 20);
	void GetSessionSummaries(SSessionList* list, SSessionSummary** summaries, DWORD* count);
	SSessionNode* CopySessionNode(SSessionList* list, const GUID* instance);

	struct SHostSessionData
	{
		std::uint8_t clientServer;
		std::byte padding01[3];
		GUID application;
		DWORD maximumPlayers;
		const wchar_t* sessionName;
		const wchar_t* password;
	};
	static_assert(sizeof(SHostSessionData) == 32);

	// Original function: FUN_004ddd3c

	static_assert(sizeof(NetworkSessionDescription) == 72);
	static_assert(offsetof(NetworkSessionDescription, guidInstance) == 0x08);
	static_assert(offsetof(NetworkSessionDescription, pwszSessionName) == 0x30);

	struct SNetLog
	{
		std::uint8_t error;                         // +0x000
		std::byte unknown001[3];
		std::uint32_t unknown004;                   // +0x004
		std::uint32_t formatFlags;                  // +0x008
		std::uint32_t unknown00C;                   // +0x00C
		std::uint32_t entryLimit;                   // +0x010
		std::uint32_t writtenEntries;               // +0x014
		std::uint32_t sequenceNumber;               // +0x018
		std::int32_t level;                         // +0x01C
		char fileName[256];                       // +0x020
		void* stream;                               // +0x120
	};

	// Only the prefix accessed during destruction is described here.
	struct STimerPrefix
	{
		volatile std::uint8_t stopRequested;        // +0x00; polled by the timer thread.
		std::byte unknown01[3];
		HANDLE thread;                              // +0x04
	};

	struct SDynamicSlot
	{
		std::uint32_t playerId;                    // +0x00
		std::uint8_t payloadByteCount;                         // +0x04: message length; zero means missing
		std::byte data[200];                       // +0x05
		std::byte unknownCD[3];
	};

	struct SDynamicSlotArray
	{
		std::uint32_t slotAllocationCount;         // +0x00; incremented when slot storage is first allocated.
		std::uint32_t revision;                    // +0x04
		std::uint32_t count;                       // +0x08
		std::uint32_t networkTick;                 // +0x0C
		SDynamicSlot* slots;                       // +0x10
	};

	struct SDynamicDataMessage
	{
		NetworkPlayerId sender;                       // +0x000
		std::uint32_t payloadSize;          // +0x004
		std::uint8_t payload[1024];         // +0x008
		std::uint8_t checksumMismatch;      // +0x408
		std::byte padding409[3];            // +0x409
	};

	static_assert(sizeof(SDynamicDataMessage) == 1036);
	static_assert(offsetof(SDynamicDataMessage, payload) == 0x008);
	static_assert(offsetof(SDynamicDataMessage, checksumMismatch) == 0x408);
	static_assert(sizeof(SDynamicDataMessage) * 64 == 66304);

	// Storage required by the original constructor and existing shutdown paths.
	struct SDynamicNetworkData
	{
		std::uint8_t active;                       // +0x00
		std::byte unknown01[3];
		std::uint32_t networkTick;                 // +0x04
		std::uint32_t waitStartTick;               // +0x08; timeGetTime-based timeout reference.
		DWORD playerCount;                         // +0x0C
		void* playerInfoRecords;                        // +0x10
		SEventQueue events;                        // +0x14
		std::uint32_t receivedChatRecordCount;         // +0x24
		void* receivedChatRecords;                        // +0x28
		std::uint32_t pendingDataMessageCount;     // +0x2C
		void* pendingDataMessages;                        // +0x30
		SDynamicSlotArray commandBuffers[3];               // +0x34, +0x48, +0x5C
		std::uint32_t commandBufferIndices[3];     // +0x70
		std::uint32_t unknown7C;                   // +0x7C
	};

	static_assert(sizeof(SDynamicNetworkData) == 128);
	static_assert(offsetof(SDynamicNetworkData, receivedChatRecordCount) == 0x24);
	static_assert(offsetof(SDynamicNetworkData, commandBufferIndices) == 0x70);
	static_assert(offsetof(SDynamicNetworkData, unknown7C) == 0x7C);

	SEventQueue* AppendEventQueueCopy(SEventQueue* destination, const SEventQueue* source);
	SEventQueue* ConstructEventQueueAndTakeSource(SEventQueue* destination, SEventQueue* source);
	void FormatNetworkEvent(const SEventQueue* queue, const SEventNode* event, char* destination, std::int32_t appendLimit);

	SEventQueue* ReplaceEventQueueAndClearSource(SEventQueue* destination, SEventQueue* source);

	NetworkPlayerId GetDynamicNetworkHostPlayerId(const SDynamicNetworkData* data);

	NetworkPlayerId GetDynamicNetworkLocalPlayerId(const SDynamicNetworkData* data);

	bool SendDynamicNetworkChatMessage(SDynamicNetworkData* data, NetworkPlayerId recipient, const wchar_t* text);

	bool TakeDynamicNetworkTextRecords(SDynamicNetworkData* data, void** records, std::uint32_t* recordCount);

	bool SendDynamicNetworkMessage(SDynamicNetworkData* data, NetworkPlayerId recipient, const void* payload, std::uint32_t byteCount);

	void PrepareDynamicCommandBuffers(SDynamicNetworkData* data);
	bool SendDynamicNetworkCommands(SDynamicNetworkData* data, const void* payload, std::uint32_t byteCount);
	bool TakeDynamicNetworkCommands(SDynamicNetworkData* data, SDynamicSlot** slots, std::uint32_t* count);
	void AdvanceDynamicNetworkTick(SDynamicNetworkData* data);
	void ResetDynamicSlots(SDynamicSlotArray* array);
	SDynamicSlotArray* ConstructDynamicSlotArray(SDynamicSlotArray* array);
	void DestroyDynamicSlotArray(SDynamicSlotArray* array);

	void ClearDynamicNetworkEvents(SDynamicNetworkData* data);

	// Extracted initialization and event processing from FUN_004df240.
	// Returns true only when the remaining update stages should execute.
	bool PrepareDynamicNetworkUpdate(SDynamicNetworkData* data, bool reset);

	bool BroadcastControlMessage8(SDynamicNetworkData* data);

	// Extracted type-4 receive branch from FUN_004df240.
	void StoreDynamicDataMessage(SDynamicNetworkData* data, const SReceivedMessage& received);

	void GetDynamicNetworkMessages(SDynamicNetworkData* data, SDynamicDataMessage** messages, std::uint32_t* count);

	inline std::uint32_t gDynamicSlotArrayConstructionCount = 0;

	void InitializeGlobalDynamicNetworkData();

	void ConvertNetworkUtf8ToWide(wchar_t* destination, const char* source, std::int32_t capacity);

	void ConvertNetworkWideToUtf8(char* destination, const wchar_t* source, std::int32_t capacity);

	void CopyNetworkUtf8ToWide(wchar_t* destination, const char* source, std::int32_t capacity);

	void CopyNetworkWideToUtf8(char* destination, const wchar_t* source, std::int32_t capacity);

	SDynamicNetworkData* ConstructDynamicNetworkData(SDynamicNetworkData* data);
	void ReleaseDynamicNetworkMemory(SDynamicNetworkData* data);
	void ShutDownDynamicNetworkData(SDynamicNetworkData* data);
	void ResetDynamicNetworkData(SDynamicNetworkData* data);
	bool AttachDynamicNetwork(SDynamicNetworkData* data, CDPNet* network);

	void DestroyDynamicNetworkData(SDynamicNetworkData* data);

	void UpdateDynamicCommandBuffers(SDynamicNetworkData* data);

	void UpdateDynamicNetwork(SDynamicNetworkData* data, bool reset);

	void HandleSpecialPlayerDepartureEvent(const SEventNode* event);

	void UpdateCommandBufferPlayers(SDynamicSlotArray* buffer, const NetworkPlayerId* playerIds, std::uint32_t playerCount);

	bool IsCommandBufferComplete(const SDynamicSlotArray* buffer);

	std::uint32_t FindCommandBufferPlayerIndex(const SDynamicSlotArray* buffer, NetworkPlayerId playerId);

	NetworkPlayerId GetFirstMissingCommandPlayerId(const SDynamicSlotArray* buffer);

	bool SetCommandMessageLength(SDynamicSlotArray* buffer, NetworkPlayerId playerId, std::uint8_t length);

	bool StoreCommandMessageData(SDynamicSlotArray* buffer, NetworkPlayerId playerId, const void* source, std::uint32_t length);

	std::uint8_t GetCommandMessageLength(const SDynamicSlotArray* buffer, NetworkPlayerId playerId);

	bool CopyCommandMessageData(const SDynamicSlotArray* buffer, NetworkPlayerId playerId, void* destination, std::uint32_t destinationSize);

	// Reconstructed player record populated by FUN_004dd0de.
	struct SPlayerInfo
	{
		NetworkPlayerId playerId;                           // +0x00
		wchar_t playerName[0x40];                 // +0x04
		std::uint8_t isHost;                      // +0x84
		std::uint8_t isLocal;                     // +0x85
		std::byte reserved86[2];                  // +0x86
		std::uint32_t bytesSent;                  // +0x88
		std::uint32_t bytesReceived;              // +0x8C
		std::uint32_t roundTripLatencyMS;         // +0x90
		std::uint32_t throughputBPS;              // +0x94
	};

	static_assert(sizeof(SPlayerInfo) == 152);
	static_assert(offsetof(SPlayerInfo, playerName) == 0x04);
	static_assert(offsetof(SPlayerInfo, isHost) == 0x84);
	static_assert(offsetof(SPlayerInfo, isLocal) == 0x85);
	static_assert(offsetof(SPlayerInfo, bytesSent) == 0x88);
	static_assert(offsetof(SPlayerInfo, bytesReceived) == 0x8C);
	static_assert(offsetof(SPlayerInfo, roundTripLatencyMS) == 0x90);
	static_assert(offsetof(SPlayerInfo, throughputBPS) == 0x94);

	static_assert(sizeof(NetworkConnectionInfo) == 92);
	static_assert(offsetof(NetworkConnectionInfo, dwBytesSentGuaranteed) == 0x10);
	static_assert(offsetof(NetworkConnectionInfo, dwBytesSentNonGuaranteed) == 0x18);
	static_assert(offsetof(NetworkConnectionInfo, dwBytesReceivedGuaranteed) == 0x48);
	static_assert(offsetof(NetworkConnectionInfo, dwBytesReceivedNonGuaranteed) == 0x50);

	// Identified members of the packet-kind-6 receive state at CDPNet +0x48.
	// The value domain and the original writer of awaitingPacket remain unresolved.
	struct SControlPacketReceiveStateStorage
	{
		std::byte unclassified00;
		std::uint8_t awaitingPacket;
		std::byte unclassified02[6];
		std::uint32_t receivedValue;
		std::byte unclassified0C[4];
	};
	static_assert(sizeof(SControlPacketReceiveStateStorage) == 16);
	static_assert(offsetof(SControlPacketReceiveStateStorage, awaitingPacket) == 1);
	static_assert(offsetof(SControlPacketReceiveStateStorage, receivedValue) == 8);

	class CDPNet
	{
	public:
		// Original function: FUN_004dbd68
		void Pump();

		// Original function: FUN_004dc31e.
		CDPNet();

		// Original function: FUN_004dc391
		~CDPNet();

		// Original function: FUN_004dc3f3
		void SetApplicationGuid(const GUID* applicationGuid);

		// Original function: FUN_004dc45f
		bool Initialize(void* context, bool option);

		// Original function: FUN_004dc700.
		bool InitializePeer(void* context, bool option);

		// Original function: FUN_004dc7a3
		bool Uninitialize();

		// Original function: FUN_004dc9cc
		NetworkPlayerId GetLocalPlayerId();

		// Original function: FUN_004dca43
		NetworkPlayerId GetHostPlayerId();

		// Original function: FUN_004dcc9b
		bool LeaveSession();

		// Original function: FUN_004dcedd
		bool StopSearchingSessions();

		// Original function: FUN_004dcf42
		bool GetPlayerName(NetworkPlayerId playerId, wchar_t** playerName);

		// Original function: FUN_004dcfe9
		bool GetPlayerIds(NetworkPlayerId** playerIds, DWORD* playerCount);

		// Original function: FUN_004dd0de
		// Initialize *players to nullptr before calling.
		// Release a successful result with NXBasics::Runtime_Free.
		bool GetPlayerInfo(SPlayerInfo** players, DWORD* playerCount);
		bool GetServiceProviders(NetworkServiceProviderInfo** providers, DWORD* count, bool allProviders);
		bool GetConnectionData(NetworkPlayerId playerId, NetworkConnectionInfo* connection);
		bool HostSession(const GUID* provider, const wchar_t* callsign, const SHostSessionData* description, bool queryAddressing);
		bool JoinSession(const wchar_t* callsign, const SSessionNode* session);
		HRESULT StartSearchingSessions(const GUID* provider, const GUID* application, const wchar_t* hostName);

		// Original function: FUN_004dd5f2
		bool KickPlayer(NetworkPlayerId playerId);
		bool SetAllowConnections(bool allow);

		// Original function: FUN_004dd65e
		bool SendPacket(NetworkPlayerId recipient, const void* packet);

		// Original function: FUN_004dd888
		bool SaveReceivedMessage(NetworkPlayerId sender, const void* packet, std::uint32_t size);

		// Original function: FUN_004dd948
		void GetReceivedMessages(SReceivedMessage** messages, std::uint32_t* count);

		// Original function: FUN_004dd9fe
		void ReleaseReceivedMessages(SReceivedMessage* messages, std::uint32_t count);

		// Original function: FUN_004dda6f
		void GetSendStatistics(std::uint32_t* statistics);

		// Original function: FUN_004ddaa5
		void SetSendStatistics(const std::uint32_t* statistics);

		// Original function: FUN_004ddadb
		void AddPlayerEvent(NetworkPlayerId playerId, std::uint32_t eventType, const wchar_t* name = nullptr);

		// Original function: FUN_004ddb3e
		void GetEvents(SEventQueue* destination);

		// Original function: FUN_004ddb76
		void ClearEvents();

		// Original function: FUN_004ddb9f
		bool HasPlayerConnectionFailed(NetworkPlayerId playerId);

		std::uint32_t flags;                       // +0x000
		std::uint32_t playerChangeCount;           // +0x004
		std::uint8_t allowConnections;             // +0x008
		std::uint8_t playerInfoChanged;            // +0x009
		std::byte unknown00A[2];
		STimerPrefix* searchTimer;                 // +0x00C
		SSessionList* sessions;                    // +0x010
		DWORD initializationTick;                  // +0x014
		SEventQueue playerEvents;                      // +0x018
		SEventQueue events28;                      // +0x028
		std::uint32_t receivedMessageCount;        // +0x038
		SReceivedMessage* receivedMessages;        // +0x03C
		NetworkBufferHandle searchOperation;                 // +0x040
		void* callbackContext;                     // +0x044
		SControlPacketReceiveStateStorage controlPacketReceiveState;                    // +0x048
		std::byte sendStatisticsStorage[16];                    // +0x058
		CModernNetwork* peer;                    // +0x068
		NetworkAddress* hostAddress;                     // +0x06C
		NetworkAddress* deviceAddress;                     // +0x070
		CRITICAL_SECTION controlPacketCriticalSection;               // +0x074
		CRITICAL_SECTION sendStatisticsCriticalSection;               // +0x08C
		CRITICAL_SECTION playerEventsCriticalSection;               // +0x0A4
		CRITICAL_SECTION criticalBC;               // +0x0BC
		CRITICAL_SECTION sessionSearchCriticalSection;               // +0x0D4
		CRITICAL_SECTION receivedMessagesCriticalSection;               // +0x0EC
		SNetLog log;                               // +0x104
	};

	extern GUID gNetworkApplicationGuid;

	bool Network_StartUp();
	void Network_ShutDown();
	std::uint32_t Network_GetFlags();
	bool Network_IsHost();
	bool Network_IsInSession();
	NetworkPlayerId Network_GetLocalPlayerId();
	NetworkPlayerId Network_GetHostPlayerId();

	char* Network_GetCallsign();
	void Network_SetCallsign(const char* callsign);
	void Network_InitializeCallsign();
	std::int32_t Network_GetCallsignBufferSize();
	bool Network_GetPlayerName(NetworkPlayerId playerId, char* destination);
	void DestroyNetworkSingleton();

	static_assert(sizeof(void*) == 4, "This reconstruction requires an x86 build");
	static_assert(sizeof(SEventQueue) == 16);
	static_assert(offsetof(SEventNode, next) == 0x20C);
	static_assert(sizeof(SReceivedMessage) == 16);
	static_assert(offsetof(SReceivedMessage, bufferHandle) == 0x0C);
	static_assert(offsetof(SSessionNode, description) == 0x14);
	static_assert(offsetof(SSessionNode, next) == 0x18);
	static_assert(sizeof(SSessionNode) == 32);
	static_assert(sizeof(SNetLog) == 292);
	static_assert(offsetof(SNetLog, formatFlags) == 0x008);
	static_assert(offsetof(SNetLog, stream) == 0x120);
	static_assert(offsetof(STimerPrefix, thread) == 0x04);
	static_assert(sizeof(SDynamicSlot) == 208);
	static_assert(sizeof(SDynamicSlotArray) == 20);
	static_assert(offsetof(SDynamicSlotArray, slotAllocationCount) == 0);
	static_assert(offsetof(SDynamicNetworkData, events) == 0x14);
	static_assert(offsetof(SDynamicNetworkData, receivedChatRecords) == 0x28);
	static_assert(offsetof(SDynamicNetworkData, commandBuffers) == 0x34);

	static_assert(offsetof(CDPNet, allowConnections) == 0x008);
	static_assert(offsetof(CDPNet, playerInfoChanged) == 0x009);
	static_assert(offsetof(CDPNet, playerEvents) == 0x18);
	static_assert(offsetof(CDPNet, receivedMessages) == 0x3C);
	static_assert(offsetof(CDPNet, controlPacketReceiveState) == 0x48);
	static_assert(offsetof(CDPNet, peer) == 0x68);
	static_assert(offsetof(CDPNet, controlPacketCriticalSection) == 0x74);
	static_assert(offsetof(CDPNet, sendStatisticsCriticalSection) == 0x8C);
	static_assert(offsetof(CDPNet, playerEventsCriticalSection) == 0xA4);
	static_assert(offsetof(CDPNet, criticalBC) == 0xBC);
	static_assert(offsetof(CDPNet, sessionSearchCriticalSection) == 0xD4);
	static_assert(offsetof(CDPNet, receivedMessagesCriticalSection) == 0xEC);
	static_assert(offsetof(CDPNet, log) == 0x104);
	static_assert(sizeof(CDPNet) == 552);
}

// Existing reconstruction interfaces, with their original addresses retained.
// Original function: FUN_004d77da
void ReleaseSessionNodeResources(NNetwork::SSessionNode* object);

// Original function: FUN_004db855
NNetwork::SNetLog* GetNetworkLog();

// Original function: FUN_004db8ea
NNetwork::SNetLog* OpenNetworkLog(NNetwork::SNetLog* object, const char* fileName, std::int32_t level, std::uint32_t entryLimit);

// Original function: FUN_004db9f6
NNetwork::SNetLog* WriteNetworkLogHeader(NNetwork::SNetLog* object);

// Original function: FUN_004dbc6d
void* WriteNetworkLog(NNetwork::SNetLog* object, const char* text, std::int32_t indentation);

// Original function: FUN_004dc409
NNetwork::CDPNet* GetNetworkInstance();
NNetwork::CDPNet* GetExistingNetworkInstance();

// Original function: FUN_004ddc42
void WriteNetworkLogLevel(std::int32_t level, const char* text, std::int32_t indentation);

// Original data: 00569654
extern NNetwork::SNetLog* gNetworkLog;
extern std::uint8_t gNetworkLogEnabled;

// Original data: 00569658
extern NNetwork::CDPNet* gNetworkInstance;

// Original data: 0050f6a8.
extern NNetwork::SDynamicNetworkData gDynamicNetworkData;
