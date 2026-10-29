#include "C2DataIds.h"
#include "C2IoHelper.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XDiagnostics.h"
#include "XMemory.h"
#include "XMemoryChecksum.h"
#include "XNumericConstants.h"
#include "XPack.h"

#include <cstdarg>
#include <cstdint>
#include <cstring>

namespace NC2Logic
{
	bool CIoHelper::Remaster_EnsureMemoryCapacity(std::uint32_t additional)
	{
		auto* memory = mMemory;
		const auto used = mMemoryUsage;

		if (memory == nullptr || additional > UINT32_MAX - used)
			return false;

		const auto required = used + additional;

		if (required <= memory->mSize)
			return required == 0 || memory->mData != nullptr;

		const auto growth = memory->mSize <= UINT32_MAX - 10000u ? memory->mSize + 10000u : UINT32_MAX;

		return memory->ChangeBufferSize(required > growth ? required : growth, false, 0xEE);
	}

	// Original function: FUN_00429f34
	CIoHelper::CIoHelper()
	{
		std::memset(this, 0, sizeof(*this));
	}

	// Original function: FUN_00429f48
	CIoHelper::~CIoHelper()
	{
		if (mStringBuffer != nullptr)
			NXBasics::Runtime_Free(mStringBuffer);
	}

	// Original function: FUN_00429f57
	void CIoHelper::IO_File_Init(NXBasics::CFile* file)
	{
		mFile = file;
		mStringBuffer = static_cast<char*>(NXBasics::Runtime_AllocateWithNewHandler(200u, 1));

		mPayloadPosition = NXBasics::IntegerRange::UInt32Maximum;
		mReadHeader.signature = NC2DataIds::IoChunk::HeaderSignature;
		mReadHeader.marker = 0;
		mReadHeader.version = 0;
		mReadHeader.payloadSize = 0;
	}

	// Original function: FUN_00429f88
	void CIoHelper::IO_File_ReadBinary(void* destination, std::uint32_t byteCount)
	{
		mFile->Read(destination, byteCount);
	}

	// Original function: FUN_00429fb4.
	void CIoHelper::IO_File_ReadAndUnpackBinary(void* destination, std::uint32_t byteCount)
	{
		const auto reject = []()
		{
			NXBasics::ReportFatalError("CIoHelper: invalid or truncated binary data");
		};

		if (mFile == nullptr || (destination == nullptr && byteCount != 0))
		{
			reject();
			return;
		}

		const auto read = [this](void* data, std::uint32_t bytes) -> bool
		{
			const auto position = mFile->GetPosition(), size = mFile->GetSize();

			if (size == UINT32_MAX || position > size || bytes > size - position)
				return false;

			if (bytes != 0)
				mFile->Read(data, bytes);

			return mFile->GetPosition() - position == bytes;
		};
		std::uint8_t packed = 0;
		std::uint32_t serializedBytes = 0;

		if (!read(&packed, 1) || !read(&serializedBytes, 4) || packed > 1u)
		{
			reject();
			return;
		}

		const auto position = mFile->GetPosition(), size = mFile->GetSize();

		if (position > size || serializedBytes > size - position)
		{
			reject();
			return;
		}

		if (packed != 0)
		{
			if (serializedBytes < 16u)
			{
				reject();
				return;
			}

			NXBasics::CMemory buffer(serializedBytes);

			if (buffer.mSize != serializedBytes || buffer.mData == nullptr || !read(buffer.mData, serializedBytes))
			{
				reject();
				return;
			}

			std::uint32_t outputBytes;
			std::memcpy(&outputBytes, buffer.mData + 8, sizeof(outputBytes));

			if (outputBytes != byteCount || !NXBasics::XB_Pack_UnPack(buffer.mData, serializedBytes, destination, byteCount))
			{
				reject();

				return;
			}
		}
		else if (serializedBytes != byteCount || !read(destination, byteCount))
		{
			reject();
		}
	}

	// Original function: FUN_0042a043.
	std::uint8_t CIoHelper::IO_File_ReadByte()
	{
		std::uint8_t value;
		mFile->Read(&value, 1);

		return value;
	}

	// Original function: FUN_0042a02d
	std::uint32_t CIoHelper::IO_File_ReadLong()
	{
		std::uint32_t value;
		mFile->Read(&value, sizeof(value));

		return value;
	}

	// Original function: FUN_0042a059.
	const char* CIoHelper::IO_File_ReadString()
	{
		std::uint8_t length;
		mFile->Read(&length, 1);

		if (length >= 199)
		{
			const std::uint32_t position = mFile->GetPosition();
			mFile->SeekToPosition(position + length + 1u);
			std::strcpy(mStringBuffer, " ");
		}
		else
		{
			mFile->Read(mStringBuffer, static_cast<std::uint32_t>(length) + 1u);
		}

		return mStringBuffer;
	}

	// Original function: FUN_0042a0f5.
	bool CIoHelper::IO_File_Chunk_CheckMarkerAndVersion(std::uint32_t marker, std::uint32_t version)
	{
		mFile->Read(&mReadHeader, sizeof(mReadHeader));
		mPayloadPosition = mFile->GetPosition();

		if (mReadHeader.signature != NC2DataIds::IoChunk::HeaderSignature)
		{
			mPayloadPosition = NXBasics::IntegerRange::UInt32Maximum;
			return false;
		}

		return mReadHeader.marker == marker && mReadHeader.version == version;
	}

	// Original function: FUN_0042a13e
	bool CIoHelper::IO_File_Chunk_CheckMarkerAndGetVersion(std::uint32_t marker, std::uint32_t* version, std::uint32_t* payloadSize)
	{
		mFile->Read(&mReadHeader, sizeof(mReadHeader));
		mPayloadPosition = mFile->GetPosition();

		if (mReadHeader.signature != NC2DataIds::IoChunk::HeaderSignature)
		{
			mPayloadPosition = NXBasics::IntegerRange::UInt32Maximum;
			return false;
		}

		if (version != nullptr)
			*version = mReadHeader.version;

		if (payloadSize != nullptr)
			*payloadSize = mReadHeader.payloadSize;

		return mReadHeader.marker == marker;
	}

	// Original function: FUN_0042a194.
	bool CIoHelper::IO_File_Chunk_ReadHeader()
	{
		mFile->Read(&mReadHeader, sizeof(mReadHeader));
		mPayloadPosition = mFile->GetPosition();

		if (mReadHeader.signature != NC2DataIds::IoChunk::HeaderSignature)
		{
			mPayloadPosition = NXBasics::IntegerRange::UInt32Maximum;
			return false;
		}

		return mReadHeader.marker != NC2DataIds::IoChunk::End;
	}

	// Original function: FUN_0042a1ce
	void CIoHelper::IO_File_Chunk_Skip()
	{
		if (mPayloadPosition == NXBasics::IntegerRange::UInt32Maximum)
		{

			return;
		}

		mFile->SeekToPosition(mPayloadPosition + mReadHeader.payloadSize);

		std::memset(&mReadHeader, 0, sizeof(mReadHeader));
		mPayloadPosition = NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_0042a1fc
	void CIoHelper::IO_Memory_Init(NXBasics::CMemory* memory)
	{
		mMemoryUsage = 0;
		mMemory = memory;
	}

	// Original function: FUN_0042a244
	void CIoHelper::IO_Memory_WriteLong(std::uint32_t value)
	{
		if (!Remaster_EnsureMemoryCapacity(4u))
		{
			NXBasics::ReportFatalError("CIoHelper: output allocation failed");

			return;
		}

		std::memcpy(mMemory->mData + mMemoryUsage, &value, sizeof(value));
		mMemoryUsage += 4u;
	}

	// Original function: FUN_0042a2e4
	void CIoHelper::IO_Memory_WriteBinary(const void* data, std::uint32_t byteCount)
	{
		if ((data == nullptr && byteCount != 0) || !Remaster_EnsureMemoryCapacity(byteCount))
		{
			NXBasics::ReportFatalError("CIoHelper: invalid output size or allocation failed");

			return;
		}

		if (byteCount != 0)
			std::memcpy(mMemory->mData + mMemoryUsage, data, byteCount);

		mMemoryUsage += byteCount;
	}

	// Original function: FUN_0042a3c6
	void CIoHelper::IO_Tool_Chunk_StartCreation(std::uint32_t marker, std::uint32_t version)
	{
		if (mChunkDepth >= 5u)
			return;

		if (mChunkDepth == 0)
			mMemoryUsage = 0;

		if (!Remaster_EnsureMemoryCapacity(sizeof(SIoChunkHeader)))
		{
			NXBasics::ReportFatalError("CIoHelper: chunk allocation failed");

			return;
		}

		mChunkOffsets[mChunkDepth] = mMemoryUsage;

		SIoChunkHeader header = { };
		header.signature = NC2DataIds::IoChunk::HeaderSignature;
		header.marker = marker;
		header.version = version;
		header.payloadSize = 0;
		header.nestingDepth = mChunkDepth;

		IO_Memory_WriteBinary(&header, sizeof(header));

		++mChunkDepth;
	}

	// Original function: FUN_0042a431
	void CIoHelper::IO_Tool_Chunk_FinishAndWrite()
	{
		if (mChunkDepth == 0)
			return;

		--mChunkDepth;

		const std::uint32_t headerOffset = mChunkOffsets[mChunkDepth];

		if (mMemory == nullptr || mMemory->mData == nullptr || headerOffset > mMemoryUsage || mMemoryUsage - headerOffset < sizeof(SIoChunkHeader))
		{
			NXBasics::ReportFatalError("CIoHelper: invalid output chunk");

			return;
		}

		const std::uint32_t currentUsage = mMemory != nullptr ? mMemoryUsage : 0u;
		const std::uint32_t payloadSize = currentUsage - headerOffset - 0x20u;

		const std::uintptr_t bufferAddress = mMemory != nullptr ? reinterpret_cast<std::uintptr_t>(mMemory->mData) : 0u;
		auto* headerBytes = reinterpret_cast<std::uint8_t*>(bufferAddress + headerOffset);

		std::memcpy(headerBytes + 0x0C, &payloadSize, sizeof(payloadSize));

		const std::uint32_t checksum = NXBasics::XB_GetMemoryChecksum(headerBytes + 0x20, payloadSize);
		std::memcpy(headerBytes + 0x14, &checksum, sizeof(checksum));

		if (mChunkDepth == 0)
		{
			const void* data = mMemory != nullptr ? mMemory->mData : nullptr;
			const std::uint32_t byteCount = mMemory != nullptr ? mMemoryUsage : 0u;

			mFile->Write(data, byteCount);
			mMemoryUsage = 0;
		}
	}

	// Original function: FUN_0042a0b1.
	void CIoHelper::IO_File_Chunk_WriteMarkerAndVersion(std::uint32_t marker, std::uint32_t version, std::uint32_t payloadSize)
	{
		SIoChunkHeader header{ };
		header.signature = NC2DataIds::IoChunk::HeaderSignatureSigned;
		header.marker = marker;
		header.version = version;
		header.payloadSize = payloadSize;
		mFile->Write(&header, sizeof(header));
	}

	// Original function: FUN_0042a20a.
	void CIoHelper::IO_Memory_WriteByte(std::uint8_t value)
	{
		if (!Remaster_EnsureMemoryCapacity(1u))
		{
			NXBasics::ReportFatalError("CIoHelper: output allocation failed");

			return;
		}

		mMemory->mData[mMemoryUsage++] = value;
	}

	// Original function: FUN_0042a281; remaster rejects strings beyond the byte-sized format.
	void CIoHelper::IO_Memory_WriteString(const char* value)
	{
		if (value == nullptr)
		{
			NXBasics::ReportFatalError("CIoHelper: null output string");
			return;
		}

		const auto length = std::strlen(value);

		if (length > UINT8_MAX || !Remaster_EnsureMemoryCapacity(static_cast<std::uint32_t>(length) + 2u))
		{
			NXBasics::ReportFatalError("CIoHelper: unrepresentable output string or allocation failed");

			return;
		}

		mMemory->mData[mMemoryUsage] = static_cast<std::uint8_t>(length);
		std::memcpy(mMemory->mData + mMemoryUsage + 1, value, length + 1);
		mMemoryUsage += static_cast<std::uint32_t>(length) + 2u;
	}

	// Original function: FUN_0042a32e.
	void CIoHelper::IO_Memory_PackAndWriteBinary(const void* data, std::uint32_t byteCount, std::uint32_t packingType)
	{
		const bool rle = packingType == NC2DataIds::PackingType::Rle8 || packingType == NC2DataIds::PackingType::Rle16;
		// Invalid methods and odd-sized word input use the already supported raw envelope.

		if (rle && mPackingBuffer != nullptr && (packingType != NC2DataIds::PackingType::Rle16 || (byteCount & 1u) == 0))
		{
			if (data == nullptr && byteCount != 0)
			{
				NXBasics::ReportFatalError("CIoHelper: null packing input");
				return;
			}

			std::uint32_t packedBytes = 0;

			while ((packedBytes = NXBasics::XB_Pack_Pack(packingType, data, byteCount, mPackingBuffer->mData, mPackingBuffer->mSize)) == 0)
			{
				const auto size = mPackingBuffer->mSize;

				if (size > UINT32_MAX - 10000u || !mPackingBuffer->ChangeBufferSize(size + 10000u, false, 0xEE))
				{
					NXBasics::ReportFatalError("CIoHelper: packing allocation failed");
					return;
				}
			}

			if (packedBytes > UINT32_MAX - 5u || !Remaster_EnsureMemoryCapacity(packedBytes + 5u))
			{
				NXBasics::ReportFatalError("CIoHelper: packed output allocation failed");
				return;
			}

			IO_Memory_WriteByte(1);
			IO_Memory_WriteLong(packedBytes);
			IO_Memory_WriteBinary(mPackingBuffer->mData, packedBytes);
		}
		else
		{
			if ((data == nullptr && byteCount != 0) || byteCount > UINT32_MAX - 5u || !Remaster_EnsureMemoryCapacity(byteCount + 5u))
			{
				NXBasics::ReportFatalError("CIoHelper: raw output allocation failed");

				return;
			}

			IO_Memory_WriteByte(0);
			IO_Memory_WriteLong(byteCount);
			IO_Memory_WriteBinary(data, byteCount);
		}
	}

	// Original function: FUN_0042a49e.
	void CIoHelper::IO_Tool_Chunk_WriteEndChunk()
	{
		SIoChunkHeader header{ };
		header.signature = NC2DataIds::IoChunk::HeaderSignatureSigned;
		header.marker = NC2DataIds::IoChunk::EndSigned;
		mFile->Write(&header, sizeof(header));
	}

	// Original function: FUN_00429f9a.
	void CIoHelper::IO_File_Skip(std::uint32_t byteCount)
	{
		mFile->SeekToPosition(mFile->GetPosition() + byteCount);
	}

}