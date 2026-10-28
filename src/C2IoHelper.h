#pragma once

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CFile;
	class CMemory;
}

namespace NC2Logic
{
	struct SIoChunkHeader
	{
		std::uint32_t signature;           // +0x00: chunk-header signature
		std::uint32_t marker;              // +0x04: chunk identifier
		std::uint32_t version;             // +0x08: chunk version
		std::uint32_t payloadSize;         // +0x0C: payload size excluding the header
		std::uint32_t nestingDepth;        // +0x10: creation nesting depth
		std::uint32_t checksum;            // +0x14: payload checksum
		std::uint32_t unknown18;           // +0x18: meaning unresolved
		std::uint32_t unknown1C;           // +0x1C: meaning unresolved
	};

	static_assert(sizeof(SIoChunkHeader) == 32);
	static_assert(offsetof(SIoChunkHeader, payloadSize) == 0x0C);
	static_assert(offsetof(SIoChunkHeader, checksum) == 0x14);

	class CIoHelper
	{
	public:
		// Original function: FUN_00429f34
		CIoHelper();
		CIoHelper(const CIoHelper&) = delete;
		CIoHelper& operator=(const CIoHelper&) = delete;

		// Original function: FUN_00429f48
		~CIoHelper();

		// Original function: FUN_00429f57
		void IO_File_Init(NXBasics::CFile* file);

		// Original function: FUN_00429f88
		void IO_File_ReadBinary(void* destination, std::uint32_t byteCount);
		void IO_File_Skip(std::uint32_t byteCount);
		void IO_File_ReadAndUnpackBinary(void* destination, std::uint32_t byteCount);
		std::uint8_t IO_File_ReadByte();

		// Original function: FUN_0042a02d
		std::uint32_t IO_File_ReadLong();
		const char* IO_File_ReadString();

		// Original function: FUN_0042a13e
		bool IO_File_Chunk_CheckMarkerAndGetVersion(std::uint32_t marker, std::uint32_t* version, std::uint32_t* payloadSize);
		bool IO_File_Chunk_CheckMarkerAndVersion(std::uint32_t marker, std::uint32_t version);
		bool IO_File_Chunk_ReadHeader();

		// Original function: FUN_0042a1ce
		void IO_File_Chunk_Skip();

		// Original function: FUN_0042a1fc
		void IO_Memory_Init(NXBasics::CMemory* memory);

		// Original function: FUN_0042a244
		void IO_Memory_WriteLong(std::uint32_t value);
		void IO_Memory_WriteByte(std::uint8_t value);
		void IO_Memory_WriteString(const char* value);
		void IO_Memory_PackAndWriteBinary(const void* data, std::uint32_t byteCount, std::uint32_t packingType);
		void IO_File_Chunk_WriteMarkerAndVersion(std::uint32_t marker, std::uint32_t version, std::uint32_t payloadSize);
		void IO_Tool_Chunk_WriteEndChunk();

		// Original function: FUN_0042a2e4
		void IO_Memory_WriteBinary(const void* data, std::uint32_t byteCount);

		// Original function: FUN_0042a3c6
		void IO_Tool_Chunk_StartCreation(std::uint32_t marker, std::uint32_t version);

		// Original function: FUN_0042a431
		void IO_Tool_Chunk_FinishAndWrite();

		NXBasics::CFile* mFile;             // +0x00: borrowed file object
		char* mStringBuffer;                // +0x04: owned 200-byte string buffer
		SIoChunkHeader mReadHeader;         // +0x08: last header read from the file
		std::uint32_t mPayloadPosition;     // +0x28: file position immediately after that header
		NXBasics::CMemory* mMemory;         // +0x2C: borrowed output buffer
		std::uint32_t mMemoryUsage;         // +0x30: bytes used in the output buffer
		NXBasics::CMemory* mPackingBuffer;  // +0x34: borrowed compression buffer owned by the save wrapper
		std::uint32_t mChunkDepth;          // +0x38: number of open output chunks
		std::uint32_t mChunkOffsets[5];     // +0x3C: output-header offsets by nesting level

	private:
		bool Remaster_EnsureMemoryCapacity(std::uint32_t additional);
	};

	static_assert(sizeof(CIoHelper) == (sizeof(void*) == 8 ? 104 : 80));
	static_assert(offsetof(CIoHelper, mFile) == 0);
	static_assert(offsetof(CIoHelper, mStringBuffer) == sizeof(void*));
	static_assert(offsetof(CIoHelper, mReadHeader) == 2 * sizeof(void*));
	static_assert(offsetof(CIoHelper, mPayloadPosition) == (sizeof(void*) == 8 ? 48 : 40));
	static_assert(offsetof(CIoHelper, mMemory) == (sizeof(void*) == 8 ? 56 : 44));
	static_assert(offsetof(CIoHelper, mMemoryUsage) == (sizeof(void*) == 8 ? 64 : 48));
	static_assert(offsetof(CIoHelper, mPackingBuffer) == (sizeof(void*) == 8 ? 72 : 52));
	static_assert(offsetof(CIoHelper, mChunkDepth) == (sizeof(void*) == 8 ? 80 : 56));
	static_assert(offsetof(CIoHelper, mChunkOffsets) == (sizeof(void*) == 8 ? 84 : 60));
}
