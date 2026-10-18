#include "FileTransferManager.h"
#include "NetworkShutdown.h"
#include "XCrtFileAccess.h"
#include "CRC32.h"

#include <process.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>

namespace NNetwork
{
	namespace
	{
		constexpr std::uint32_t MaximumFileSize = 64 * 1024 * 1024 + 28, TransferTimeout = 30000;
		CFileTransferManager* managerInstance = nullptr;
		std::string sharedFile;

		struct Lock
		{
			CRITICAL_SECTION& section;
			explicit Lock(CRITICAL_SECTION& value) : section(value) { EnterCriticalSection(&section); }
			~Lock() { LeaveCriticalSection(&section); }
		};
		std::uint32_t ReadWord(const std::uint8_t* p) { std::uint32_t value; std::memcpy(&value, p, sizeof(value)); return value; }
		void WriteWord(std::uint8_t* p, std::uint32_t value) { std::memcpy(p, &value, sizeof(value)); }
		void Close(SFileTransferNode& node) { if (node.stream != nullptr) { NXBasics::Runtime_CloseStream(node.stream); node.stream = nullptr; } }
		bool Active(const SFileTransferNode& node) { return node.state != 1 && node.state != 14; }
		void Abort(SFileTransferNode& node)
		{
			Close(node);
			if (node.path24 != nullptr) DeleteFileA(node.path24);
			node.state = 14;
		}
		SFileTransferNode* MakeNode(const void* id, const char* path, std::uint32_t player, std::uint32_t state)
		{
			auto* node = static_cast<SFileTransferNode*>(NXBasics::Runtime_AllocateWithNewHandler(sizeof(SFileTransferNode), 1));
			if (node == nullptr) return nullptr;
			*node = {}; std::memcpy(node->transferId, id, 16);
			node->fileName = NXBasics::Runtime_DuplicateString(path); node->peerId = player; node->state = state; node->startTick = GetTickCount();
			if (node->fileName == nullptr) { NXBasics::Runtime_Free(node); return nullptr; }
			return node;
		}
		void Header(std::uint8_t* packet, std::uint8_t type, const SFileTransferNode& node) { packet[0] = type; std::memcpy(packet + 4, node.transferId, 16); }
		bool Send(SFileTransferNode& node, const void* packet)
		{
			auto* network = GetExistingNetworkInstance();
			if (network == nullptr || !network->SendPacket(node.peerId, packet)) { Abort(node); return false; }
			node.startTick = GetTickCount(); return true;
		}
	}

	// The original 004E0062..004E0A5D API remains the menu's integration boundary.
	CFileTransferManager::CFileTransferManager()
	{
		stopRequested = 0; serviceThread = nullptr; serviceState = 2;
		std::memset(reserved01, 0, sizeof(reserved01)); InitializeCriticalSection(&critical);
		managerInstance = this; sharedFile.clear();
	}
	CFileTransferManager::~CFileTransferManager()
	{
		StopService();
		{ Lock lock(critical); for (auto* node = transfers.first; node != nullptr; node = node->next) { Close(*node); if (node->path24 != nullptr) DeleteFileA(node->path24); } transfers.Clear(); sharedFile.clear(); managerInstance = nullptr; }
		DeleteCriticalSection(&critical);
	}
	CFileTransferManager* CFileTransferManager::GetInstance() { return managerInstance; }
	void CFileTransferManager::SetSharedFile(const char* fileName)
	{
		Lock lock(critical); sharedFile = fileName != nullptr ? fileName : "";
	}
	unsigned __stdcall CFileTransferManager::ServiceThreadProc(void* argument)
	{
		auto* manager = static_cast<CFileTransferManager*>(argument);
		for (;;)
		{
			{ Lock lock(manager->critical); if (manager->stopRequested != 0) break; }
			manager->ProcessJobList(); Sleep(10);
		}
		return 0;
	}
	void CFileTransferManager::StartService()
	{
		Lock lock(critical); if (serviceThread != nullptr) return;
		stopRequested = 0;
		serviceThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, &ServiceThreadProc, this, 0, nullptr));
		serviceState = serviceThread != nullptr ? 1 : 0;
	}
	void CFileTransferManager::StopService()
	{
		HANDLE thread;
		{ Lock lock(critical); stopRequested = 1; thread = serviceThread; }
		if (thread != nullptr) { WaitForSingleObject(thread, INFINITE); CloseHandle(thread); }
		{ Lock lock(critical); serviceThread = nullptr; serviceState = 2; }
	}

	void CFileTransferManager::StartTransfer(const char* fileName, GUID* transferId)
	{
		if (transferId == nullptr) return;
		*transferId = {};
		if (fileName == nullptr || *fileName == 0 || std::strlen(fileName) >= 260) return;
		Lock lock(critical);
		auto* network = GetExistingNetworkInstance();
		if (serviceState != 1 || network == nullptr || network->GetHostPlayerId() == 0 || network->GetLocalPlayerId() == network->GetHostPlayerId() || transfers.Count() >= MaximumNetworkPlayers) return;
		GUID id{}; if (FAILED(CoCreateGuid(&id))) return;
		if (auto* node = MakeNode(&id, fileName, network->GetHostPlayerId(), 0)) { transfers.Append(node); *transferId = id; }
	}

	SFileTransferInfo* CFileTransferManager::GetTransferInfo(const void* transferId)
	{
		struct Snapshot { SFileTransferInfo info{}; std::string name, temporary; };
		thread_local Snapshot snapshot;
		Lock lock(critical);
		auto* node = transferId != nullptr ? transfers.Find(transferId) : nullptr;
		if (node == nullptr || node->state == 14) return nullptr;
		snapshot.name = node->fileName != nullptr ? node->fileName : ""; snapshot.temporary = node->path24 != nullptr ? node->path24 : "";
		snapshot.info = { const_cast<char*>(snapshot.name.c_str()), const_cast<char*>(snapshot.temporary.c_str()), node->checksum, node->expectedChecksum, node->transferredBytes, node->fileSize, nullptr };
		return &snapshot.info;
	}

	void CFileTransferManager::ProcessJobList()
	{
		Lock lock(critical);
		for (auto* node = transfers.first; node != nullptr; node = node->next)
		{
			if (!Active(*node)) continue;
			if (GetTickCount() - node->startTick > TransferTimeout) { Abort(*node); continue; }
			auto* network = GetExistingNetworkInstance();
			if (network == nullptr || network->HasPlayerConnectionFailed(node->peerId)) { Abort(*node); continue; }
			if (node->state == 0)
			{
				std::array<std::uint8_t, 280> packet{}; Header(packet.data(), 11, *node); WriteWord(packet.data() + 20, network->GetLocalPlayerId());
				// The remote request selects only the explicitly shared game file.
				std::memcpy(packet.data() + 24, "game-data", 10);
				if (Send(*node, packet.data())) node->state = 5;
			}
			else if (node->state == 6)
			{
				node->stream = NXBasics::Runtime_OpenStream(node->fileName, "rb");
				if (node->stream == nullptr) { Abort(*node); continue; }
				if (_fseeki64(node->stream->nativeFile, 0, SEEK_END) != 0) { Abort(*node); continue; }
				const auto size = _ftelli64(node->stream->nativeFile);
				if (size <= 0 || size > MaximumFileSize || _fseeki64(node->stream->nativeFile, 0, SEEK_SET) != 0) { Abort(*node); continue; }
				node->fileSize = static_cast<std::uint32_t>(size);
				std::array<std::uint8_t, 24> packet{}; Header(packet.data(), 12, *node); WriteWord(packet.data() + 20, network->GetLocalPlayerId());
				if (Send(*node, packet.data())) node->state = 7;
			}
			else if (node->state == 8)
			{
				std::array<std::uint8_t, 20> packet{}; Header(packet.data(), 13, *node);
				if (Send(*node, packet.data())) node->state = 10;
			}
			else if (node->state == 9)
			{
				std::array<std::uint8_t, 65536> chunk{};
				const auto remaining = node->fileSize - node->transferredBytes;
				const auto count = NXBasics::Runtime_ReadStream(chunk.data(), 1, (std::min)(remaining, static_cast<std::uint32_t>(chunk.size())), node->stream);
				if (count == 0 && remaining != 0) { Abort(*node); continue; }
				for (std::uint32_t i = 0; i < count; ++i) NCRC32::UpdateByte(&node->checksum, chunk[i]);
				node->transferredBytes += count; node->startTick = GetTickCount();
				if (node->transferredBytes == node->fileSize)
				{
					if (NXBasics::Runtime_SeekStream(node->stream, 0, SEEK_SET) != 0) { Abort(*node); continue; }
					std::array<std::uint8_t, 28> packet{}; Header(packet.data(), 14, *node); WriteWord(packet.data() + 20, node->checksum); WriteWord(packet.data() + 24, node->fileSize);
					node->transferredBytes = 0; if (Send(*node, packet.data())) node->state = 13;
				}
			}
			else if (node->state == 13)
			{
				for (unsigned block = 0; block < 16 && node->state == 13; ++block)
				{
					std::array<std::uint8_t, 2072> packet{}; Header(packet.data(), 15, *node);
					const auto requested = (std::min)(2048u, node->fileSize - node->transferredBytes);
					const auto count = NXBasics::Runtime_ReadStream(packet.data() + 24, 1, requested, node->stream);
					if (count != requested) { Abort(*node); break; }
					WriteWord(packet.data() + 20, count);
					if (!Send(*node, packet.data())) break;
					node->transferredBytes += count;
					// Completion is acknowledged only after the receiver verifies and commits.
					if (node->transferredBytes == node->fileSize) { Close(*node); node->state = 11; }
				}
			}
		}
	}

	void CFileTransferManager::DispatchMessage(std::uint32_t sender, const std::uint8_t* packet, std::uint32_t size)
	{
		if (!ValidateNetworkPacket(packet, size) || packet[0] < 11) return;
		Lock lock(critical);
		auto* network = GetExistingNetworkInstance(); if (network == nullptr) return;
		auto* node = transfers.Find(packet + 4);
		if (packet[0] == 11)
		{
			if (!Network_IsHost() || sharedFile.empty() || node != nullptr || transfers.Count() >= MaximumNetworkPlayers || ReadWord(packet + 20) != sender) return;
			for (auto* existing = transfers.first; existing != nullptr; existing = existing->next) if (existing->peerId == sender && Active(*existing)) return;
			if (auto* created = MakeNode(packet + 4, sharedFile.c_str(), sender, 6)) transfers.Append(created);
			return;
		}
		if (node == nullptr || sender != node->peerId) return;
		if (packet[0] == 12 && node->state == 5 && sender == network->GetHostPlayerId() && ReadWord(packet + 20) == sender) { node->replyCode = sender; node->state = 8; }
		else if (packet[0] == 13 && node->state == 7) { node->checksum = UINT32_MAX; node->transferredBytes = 0; node->state = 9; }
		else if (packet[0] == 14 && node->state == 10 && sender == network->GetHostPlayerId())
		{
			node->expectedChecksum = ReadWord(packet + 20); node->fileSize = ReadWord(packet + 24); node->checksum = UINT32_MAX; node->transferredBytes = 0;
			if (node->fileSize == 0) { Abort(*node); return; }
			std::string temporary(node->fileName); temporary += ".ov-";
			constexpr char hex[] = "0123456789abcdef";
			for (const auto value : node->transferId) { const auto byte = std::to_integer<unsigned>(value); temporary += hex[byte >> 4]; temporary += hex[byte & 15]; }
			temporary += ".tmp"; node->path24 = NXBasics::Runtime_DuplicateString(temporary.c_str());
			node->stream = node->path24 != nullptr ? NXBasics::Runtime_OpenStream(node->path24, "wbx") : nullptr;
			if (node->stream == nullptr) { NXBasics::Runtime_Free(node->path24); node->path24 = nullptr; Abort(*node); return; } node->state = 12;
		}
		else if (packet[0] == 15 && node->state == 12)
		{
			const auto count = ReadWord(packet + 20);
			if (count == 0 || count > node->fileSize - node->transferredBytes || NXBasics::Runtime_WriteStream(packet + 24, 1, count, node->stream) != count) { Abort(*node); return; }
			for (std::uint32_t i = 0; i < count; ++i) NCRC32::UpdateByte(&node->checksum, packet[24 + i]);
			node->transferredBytes += count;
			if (node->transferredBytes == node->fileSize)
			{
				const bool verified = node->checksum == node->expectedChecksum && NXBasics::Runtime_FlushStream(node->stream) == 0;
				const auto closeResult = NXBasics::Runtime_CloseStream(node->stream); node->stream = nullptr;
				if (!verified || closeResult != 0 || !MoveFileExA(node->path24, node->fileName, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) { Abort(*node); return; }
				node->state = 1;
				// Type 13 is also the modern receiver's final acknowledgement.
				std::array<std::uint8_t, 20> reply{}; Header(reply.data(), 13, *node); network->SendPacket(node->peerId, reply.data());
			}
		}
		else if (packet[0] == 13 && node->state == 11) node->state = 1;
		else return;
		node->startTick = GetTickCount();
	}

	void CFileTransferManager::GetTransferList(SFileTransferNode* output, std::uint32_t* count)
	{
		if (count == nullptr) return;
		const auto capacity = *count; *count = 0; if (output == nullptr || capacity > 1024) return;
		std::memset(output, 0, capacity * sizeof(SFileTransferNode)); Lock lock(critical);
		for (auto* node = transfers.first; node != nullptr && *count < capacity; node = node->next)
		{
			if (node->state != 1 && node->state != 14 && node->state != 13) continue;
			auto& copy = output[(*count)++]; copy = *node; copy.next = nullptr; copy.stream = nullptr;
			copy.fileName = node->fileName != nullptr ? NXBasics::Runtime_DuplicateString(node->fileName) : nullptr;
			copy.path24 = node->path24 != nullptr ? NXBasics::Runtime_DuplicateString(node->path24) : nullptr;
		}
	}
}
