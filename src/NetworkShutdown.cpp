#include "DirectXErrorText.h"
#include "FileTransferManager.h"
#include "MasterPropertyManager.h"
#include "NetworkShutdown.h"
#include "OriginalStrings.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"
#include "XTextEncoding.h"
#include <new>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <Windows.h>

// Original data: DAT_0050f6a8.
NNetwork::SDynamicNetworkData gDynamicNetworkData{ };

// Original data: 0050f728..0050f74f.
char gNetworkCallsign[40]{ };

// Original data: 0050f750.
std::uint8_t gNetworkCallsignInitialized = 0;

namespace NNetwork
{
	// Original functions: FUN_004db528 and its tail-jump alias FUN_004db49b.
	SEventQueue* ClearEventQueue(SEventQueue* queue)
	{
		while (queue->first != nullptr)
		{
			SEventNode* node = queue->first;
			queue->first = node->next;

			NXBasics::Runtime_Free(node);
		}

		queue->last = nullptr;
		queue->first = nullptr;
		queue->count = 0;
		queue->changed = 0;

		return queue;
	}

	namespace
	{

		void CloseNetworkLog(SNetLog* log);
		void ReleaseDynamicSlots(SDynamicSlotArray* array);

		// Shared expansion of the original four OutputDebugStringA calls.
		void ReportNetworkError(const char* operation, HRESULT result)
		{
			OutputDebugStringA(operation);
			OutputDebugStringA(OriginalStrings::DirectPlayErrorSeparator);
			OutputDebugStringA(NXBasics::GetDirectXErrorText(result));
			OutputDebugStringA(OriginalStrings::NewLine);
		}

		// Original function: FUN_004db87e
		void DestroyLogSingleton()
		{
			SNetLog* log = gNetworkLog;

			if (log != nullptr)
			{
				CloseNetworkLog(log);
				::operator delete(log);

				gNetworkLog = nullptr;
			}
		}

		// Original function: FUN_004db8c8
		void CloseNetworkLog(SNetLog* log)
		{
			// Non-owning bridge only. The shared application logger drains after transport shutdown.
			log->stream = nullptr;
		}

		// Original function: FUN_004e10fb
		void ReleaseDynamicSlots(SDynamicSlotArray* array)
		{
			if (array->slots != nullptr)
			{
				++array->revision;
			}

			ResetDynamicSlots(array);

			if (array->slots != nullptr)
			{
				NXBasics::Runtime_Free(array->slots);
				array->slots = nullptr;
			}
		}
	}

	// Original function: FUN_004e101b; shared by tick rotation and destruction.
	void ResetDynamicSlots(SDynamicSlotArray* array)
	{
		if (array->slots == nullptr)
		{

			return;
		}

		for (std::uint32_t index = 0; index < array->count; ++index)
		{
			array->slots[index].payloadByteCount = 0;
			std::memset(array->slots[index].data, 0, 200);
		}
	}

	// Original function: FUN_004058dd
	void InitializeGlobalDynamicNetworkData()
	{
		ConstructDynamicNetworkData(&gDynamicNetworkData);

		// Original function: FUN_004058f3
		// The existing reconstructed onexit interface expects an int return value.
		NXBasics::Runtime_AtExit(+[]() -> int
		                         {
			                         DestroyDynamicNetworkData(&gDynamicNetworkData);

			                         return 0;
		                         });
	}

	// Original function: FUN_00405949
	void Network_ShutDown()
	{
		if (auto* manager = CFileTransferManager::GetInstance())
			manager->StopService();

		ShutDownDynamicNetworkData(&gDynamicNetworkData);

		CDPNet* network = GetNetworkInstance();

		if ((network->flags & NXBasics::BitMask::Bit2) != 0)
		{
			network->StopSearchingSessions();
		}

		if ((network->flags & NXBasics::BitMask::Bit3) != 0)
		{
			network->LeaveSession();
		}

		// Read the flags again after LeaveSession().

		if ((network->flags & NXBasics::BitMask::Bit1) != 0)
		{
			network->LeaveSession();
		}

		network->Uninitialize();

		DestroyNetworkSingleton();
		DestroyLogSingleton();
		gNetworkLogEnabled = 0;
	}

	// Original function: FUN_00405999
	std::uint32_t Network_GetFlags()
	{

		return GetNetworkInstance()->flags;
	}

	// Original function: FUN_004059a1
	bool Network_IsHost()
	{

		return (GetNetworkInstance()->flags & NXBasics::BitMask::Bit1) != 0;
	}

	// Original function: FUN_004059b3
	bool Network_IsInSession()
	{
		if (GetExistingNetworkInstance() == nullptr)
		{
			return false;
		}

		return (GetNetworkInstance()->flags & NXBasics::BitMask::Bit1AndBit3) != 0;
	}

	// Original function: FUN_004059cb
	NetworkPlayerId Network_GetLocalPlayerId()
	{

		return GetNetworkInstance()->GetLocalPlayerId();
	}

	// Original function: FUN_004059d7
	NetworkPlayerId Network_GetHostPlayerId()
	{

		return GetNetworkInstance()->GetHostPlayerId();
	}

	// Original function: FUN_004059e3
	char* Network_GetCallsign()
	{

		return gNetworkCallsign;
	}

	// Original function: FUN_004059e9
	void Network_SetCallsign(const char* callsign)
	{
		NXBasics::Text_Copy(gNetworkCallsign, sizeof(gNetworkCallsign), callsign);
	}

	// Original function: FUN_00405a04
	void Network_InitializeCallsign()
	{
		if (gNetworkCallsignInitialized == 0)
		{
			gNetworkCallsign[0] = '\0';
			gNetworkCallsignInitialized = 1;
		}

		if (std::strlen(gNetworkCallsign) != 0)
		{

			return;
		}

		auto* properties = NMasterPropertyManager::CPropertyManager::sTheObjectPtr;

		if (properties->Property_DoesExists("multiplayer_callsign"))
		{
			// Reload the singleton as in the original.
			properties = NMasterPropertyManager::CPropertyManager::sTheObjectPtr;

			NXBasics::Text_Copy(gNetworkCallsign, sizeof(gNetworkCallsign), properties->Property_GetStringValuePtr("multiplayer_callsign"));
		}
		else
		{
			wchar_t userName[100]{ };
			DWORD userNameSize = 100;

			if (!::GetUserNameW(userName, &userNameSize) || userName[0] == 0)
				std::wcscpy(userName, L"Player");

			const auto utf8 = NXBasics::Text_FromWide(userName, std::wcslen(userName));
			NXBasics::Text_Copy(gNetworkCallsign, sizeof(gNetworkCallsign), utf8.c_str());
		}

		gNetworkCallsign[0x27] = '\0';
	}

	// Original function: FUN_00405a87
	std::int32_t Network_GetCallsignBufferSize()
	{

		return 0x28;
	}

	// Original function: FUN_00405a8b
	bool Network_GetPlayerName(NetworkPlayerId playerId, char* destination)
	{
		wchar_t* playerName = nullptr;

		if (!GetNetworkInstance()->GetPlayerName(playerId, &playerName))
		{
			return false;
		}

		ConvertNetworkWideToUtf8(destination, playerName, 40);
		NXBasics::Runtime_Free(playerName);

		return true;
	}

	// Original function: FUN_004dc438
	void DestroyNetworkSingleton()
	{
		CDPNet* object = gNetworkInstance;

		if (object != nullptr)
		{
			object->~CDPNet();
			::operator delete(object);

			gNetworkInstance = nullptr;
		}
	}

	// Original function: FUN_004dda6f
	void CDPNet::GetSendStatistics(std::uint32_t* statistics)
	{
		EnterCriticalSection(&sendStatisticsCriticalSection);

		if (statistics != nullptr)
		{
			std::memcpy(statistics, sendStatisticsStorage, 16);
		}

		LeaveCriticalSection(&sendStatisticsCriticalSection);
	}

	// Original function: FUN_004ddaa5
	void CDPNet::SetSendStatistics(const std::uint32_t* statistics)
	{
		EnterCriticalSection(&sendStatisticsCriticalSection);

		if (statistics != nullptr)
		{
			std::memcpy(sendStatisticsStorage, statistics, 16);
		}

		LeaveCriticalSection(&sendStatisticsCriticalSection);
	}

	// Original function: FUN_004E0FFD.
	SDynamicSlotArray* ConstructDynamicSlotArray(SDynamicSlotArray* array)
	{
		++gDynamicSlotArrayConstructionCount;
		array->revision = 0;
		array->slotAllocationCount = 0;
		array->networkTick = 0;
		array->count = 0;
		array->slots = nullptr;

		return array;
	}

	// Original function: FUN_004E1016; distinct tail-jump destructor entry.
	void DestroyDynamicSlotArray(SDynamicSlotArray* array)
	{
		ReleaseDynamicSlots(array);
	}

	// Original function: FUN_004de956.
	SDynamicNetworkData* ConstructDynamicNetworkData(SDynamicNetworkData* data)
	{
		// Original function: FUN_004db429
		data->events.count = 0;
		data->events.last = nullptr;
		data->events.first = nullptr;
		data->events.changed = 0;

		for (std::uint32_t index = 0; index < 3; ++index)
		{
			ConstructDynamicSlotArray(&data->commandBuffers[index]);
		}

		data->active = 0;
		data->networkTick = 0;
		data->playerInfoRecords = nullptr;
		data->receivedChatRecords = nullptr;
		data->receivedChatRecordCount = 0;
		data->pendingDataMessageCount = 0;
		data->pendingDataMessages = nullptr;
		data->waitStartTick = 0;
		data->unknown7C = 0;

		// Original allocation: FUN_004e5ddd -> FUN_004e860e(size, 1).
		data->pendingDataMessages = NXBasics::Runtime_AllocateWithNewHandler(66304u, 1);

		// Leave an absent optional buffer null when allocation fails.

		if (data->pendingDataMessages != nullptr)
			std::memset(data->pendingDataMessages, 0, 66304u);

		return data;
	}

	// Original function: FUN_004de9b1
	void DestroyDynamicNetworkData(SDynamicNetworkData* data)
	{
		ReleaseDynamicNetworkMemory(data);

		// Original destructors: FUN_004e1016 -> FUN_004e10fb.

		for (std::uint32_t index = 3; index != 0; --index)
		{
			DestroyDynamicSlotArray(&data->commandBuffers[index - 1]);
		}

		// Original destructor: FUN_004db49b -> FUN_004db528.
		ClearEventQueue(&data->events);
	}
}

namespace NNetwork
{
	// Original function: FUN_004de9da
	void ReleaseDynamicNetworkMemory(SDynamicNetworkData* data)
	{
		WriteNetworkLog(GetNetworkLog(), "Deleting dynamic memory", 0);

		ClearEventQueue(&data->events);

		if (data->playerInfoRecords != nullptr)
		{
			NXBasics::Runtime_Free(data->playerInfoRecords);
			data->playerInfoRecords = nullptr;
		}

		if (data->receivedChatRecords != nullptr)
		{
			NXBasics::Runtime_Free(data->receivedChatRecords);
			data->receivedChatRecords = nullptr;
		}

		if (data->pendingDataMessages != nullptr)
		{
			NXBasics::Runtime_Free(data->pendingDataMessages);
			data->pendingDataMessages = nullptr;
		}

		ReleaseDynamicSlots(&data->commandBuffers[0]);
		ReleaseDynamicSlots(&data->commandBuffers[1]);
		ReleaseDynamicSlots(&data->commandBuffers[2]);
	}

}

namespace NNetwork
{
	// Original function: FUN_004debfd
	void ShutDownDynamicNetworkData(SDynamicNetworkData* data)
	{
		ReleaseDynamicNetworkMemory(data);
		data->active = 0;
	}

	// Modern list cleanup also resets the public ownership fields.
	void DestroySessionListContents(SSessionList* list)
	{
		if (list == nullptr)
		{

			return;
		}

		auto* node = list->first;
		const auto count = list->count;

		for (std::uint32_t i = 0; node != nullptr && i < count; ++i)
		{
			auto* next = node->next;
			ReleaseSessionNodeResources(node);
			NXBasics::Runtime_Free(node);
			node = next;
		}

		list->first = nullptr;
		list->count = 0;
		list->flags = 1;
	}

}

namespace NNetwork
{

}