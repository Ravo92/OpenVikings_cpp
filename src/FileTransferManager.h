#pragma once

#include <Windows.h>

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	struct RuntimeFormatStream;
}

namespace NNetwork
{
	struct SFileTransferNode
	{
		std::byte transferId[16];               // +0x00
		std::uint32_t state;                      // +0x10
		std::uint32_t replyCode;                  // +0x14
		std::uint32_t peerId;                     // +0x18
		std::uint32_t startTick;                  // +0x1C
		char* fileName;                           // +0x20
		char* path24;                             // +0x24
		std::uint32_t checksum;                   // +0x28
		std::uint32_t expectedChecksum;           // +0x2C
		std::uint32_t transferredBytes;           // +0x30
		std::uint32_t fileSize;                   // +0x34
		NXBasics::RuntimeFormatStream* stream;    // +0x38
		SFileTransferNode* next;                  // +0x3C
	};

	struct SFileTransferList
	{
		SFileTransferList();
		SFileTransferNode* first;                 // +0x00

		// Original function: FUN_004dffe5
		void Clear();

		// Original function: FUN_004e0008
		void Append(SFileTransferNode* node);

		// Original function: FUN_004e002b
		std::uint32_t Count() const;

		// Original function: FUN_004e003a
		SFileTransferNode* GetFirst() const;

		// Original function: FUN_004e003d
		SFileTransferNode* Find(const void* transferId) const;
	};

	struct SFileTransferInfo
	{
		char* fileName;                              // +0x00
		char* path24;                                // +0x04
		std::uint32_t checksum;                      // +0x08
		std::uint32_t expectedChecksum;              // +0x0C
		std::uint32_t transferredBytes;              // +0x10
		std::uint32_t fileSize;                      // +0x14
		NXBasics::RuntimeFormatStream* stream;       // +0x18
	};

	static_assert(sizeof(SFileTransferInfo) == 28);
	static_assert(offsetof(SFileTransferInfo, checksum) == 0x08);
	static_assert(offsetof(SFileTransferInfo, transferredBytes) == 0x10);
	static_assert(offsetof(SFileTransferInfo, stream) == 0x18);
	static_assert(offsetof(SFileTransferNode, fileName) == 0x20);
	static_assert(offsetof(SFileTransferNode, next) == 0x3C);

	class CFileTransferManager
	{
	public:
		// Original function: FUN_004e0062
		CFileTransferManager();

		// Original function: FUN_004e00b5
		~CFileTransferManager();

		// Original function: FUN_004e00de
		static CFileTransferManager* GetInstance();

		// Original function: FUN_004e00e4
		static unsigned __stdcall ServiceThreadProc(void* argument);

		// Original function: FUN_004e0121
		void StartService();

		// Original function: FUN_004e017d
		void StopService();

		// Original function: FUN_004e01d1
		void StartTransfer(const char* fileName, GUID* transferId);

		// Original function: FUN_004e02ac
		SFileTransferInfo* GetTransferInfo(const void* transferId);

		// Original function: FUN_004e03a1
		void ProcessJobList();

		// Original function: FUN_004e0a5d
		void GetTransferList(SFileTransferNode* output, std::uint32_t* count);

		// Original function: FUN_004e0690
		void DispatchMessage(std::uint32_t sender, const std::uint8_t* packet, std::uint32_t size);
		void SetSharedFile(const char* fileName);

		std::uint8_t stopRequested;               // +0x00
		std::byte reserved01[3];                  // +0x01
		HANDLE serviceThread;                     // +0x04
		std::uint32_t serviceState;               // +0x08
		CRITICAL_SECTION critical;               // +0x0C
		SFileTransferList transfers;             // +0x24
	};

	static_assert(sizeof(SFileTransferNode) == 64);
	static_assert(offsetof(SFileTransferNode, state) == 0x10);
	static_assert(offsetof(SFileTransferNode, checksum) == 0x28);
	static_assert(offsetof(SFileTransferNode, transferredBytes) == 0x30);
	static_assert(offsetof(SFileTransferNode, stream) == 0x38);

	static_assert(sizeof(SFileTransferList) == 4);
	static_assert(sizeof(CFileTransferManager) == 40);
	static_assert(offsetof(CFileTransferManager, serviceThread) == 0x04);
	static_assert(offsetof(CFileTransferManager, serviceState) == 0x08);
	static_assert(offsetof(CFileTransferManager, critical) == 0x0C);
	static_assert(offsetof(CFileTransferManager, transfers) == 0x24);
}
