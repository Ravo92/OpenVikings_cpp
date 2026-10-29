#include "C2GameMapStorage.h"
#include "C2DataIds.h"
#include "C2IoHelper.h"
#include "C2MapSectorAttachment.h"
#include "C2MapSectorGridCursor.h"
#include "C2ObjectLayout.h"
#include "XCrtFileAccess.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace NC2Logic
{
	namespace
	{
		// Original function: FUN_00470374.
		void InitializeSectorAttachmentData(std::byte* sector)
		{
			std::memset(sector, 0, 1344);

			const std::int32_t initialValue = -1;

			for (std::uint32_t group = 0; group < 21u; ++group)
			{
				for (std::uint32_t offset = 0; offset < 0x40u; offset += 8u)
					std::memcpy(sector + group * 0x40u + offset, &initialValue, sizeof(initialValue));
			}
		}
	}

	// Original function: FUN_0044ebd7.
	CGameMapSectorAttachmentManager::CGameMapSectorAttachmentManager()
	{
		static_assert(offsetof(CGameMapSectorAttachmentManager, mGridHeight) == NC2Layout::SectorAttachmentManagerOffset::GridHeight);
		static_assert(offsetof(CGameMapSectorAttachmentManager, mGridWidth) == NC2Layout::SectorAttachmentManagerOffset::GridWidth);
		static_assert(offsetof(CGameMapSectorAttachmentManager, mSectorData) == NC2Layout::SectorAttachmentManagerOffset::SectorData);
		static_assert(offsetof(CGameMapSectorAttachmentManager, mSectorAuxiliaryData) == NC2Layout::SectorAttachmentManagerOffset::SectorAuxiliaryData);
		static_assert(offsetof(CGameMapSectorAttachmentManager, mSectorData) == 2 * sizeof(std::int32_t));
		sTheObjectPtr = this;

		mGridHeight = 0;
		mGridWidth = 0;
		mSectorData = nullptr;
		mSectorAuxiliaryData = nullptr;

		l_InitSizeAndAllocateSectorArray();
	}

	// Original function: FUN_0044ed2f.
	CGameMapSectorAttachmentManager::CGameMapSectorAttachmentManager(CIoHelper& io)
	{
		sTheObjectPtr = this;
		std::memset(this, 0, sizeof(*this));
		l_InitSizeAndAllocateSectorArray();
		if (io.mReadHeader.version != 1u)
			io.IO_File_Chunk_Skip();
		else
		{
			// The original allocates first, then overwrites the saved dimensions.
			io.IO_File_ReadBinary(this, 8);
			io.IO_File_ReadBinary(mSectorData, static_cast<std::uint32_t>(mGridWidth) * static_cast<std::uint32_t>(mGridHeight) * 0x540u);
		}
	}

	// Original function: FUN_0044ebf8.
	CGameMapSectorAttachmentManager::~CGameMapSectorAttachmentManager()
	{
		if (mSectorData != nullptr)
		{
			NXBasics::Runtime_Free(mSectorData);
			mSectorData = nullptr;
		}

		if (mSectorAuxiliaryData != nullptr)
		{
			NXBasics::Runtime_Free(mSectorAuxiliaryData);
			mSectorAuxiliaryData = nullptr;
		}

		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_0044ec28.
	void CGameMapSectorAttachmentManager::l_InitSizeAndAllocateSectorArray()
	{
		std::int16_t macroWidth;
		std::int16_t macroHeight;
		std::memcpy(&macroWidth, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroWidth, sizeof(macroWidth));
		std::memcpy(&macroHeight, gGameMapStaticStorage + NC2Layout::GameMapOffset::MacroHeight, sizeof(macroHeight));

		mGridWidth = static_cast<std::int32_t>(macroWidth) / 10;
		mGridHeight = static_cast<std::int32_t>(macroHeight) / 10;

		// Preserve IDIV followed by a nonzero-remainder test.
		if (macroWidth % 10 != 0)
			++mGridWidth;

		if (macroHeight % 10 != 0)
			++mGridHeight;

		SSectorGridCursor::sGridWidth = mGridWidth;
		SSectorGridCursor::sGridHeight = mGridHeight;

		const std::int32_t sectorCount = mGridWidth * mGridHeight;
		auto* sectorData = static_cast<std::byte*>(NXBasics::Runtime_AllocateArrayWithNewHandler(static_cast<std::uint32_t>(sectorCount), 0x540u));

		if (sectorData != nullptr)
		{
			for (std::int32_t index = 0; index < sectorCount; ++index)
				InitializeSectorAttachmentData(sectorData + static_cast<std::size_t>(index) * 0x540u);
		}

		mSectorData = sectorData;

		mSectorAuxiliaryData = static_cast<std::byte*>(NXBasics::Runtime_AllocateArrayWithNewHandler(static_cast<std::uint32_t>(sectorCount), 0xFCu));
		const std::size_t auxiliaryBytes = static_cast<std::size_t>(static_cast<std::uint32_t>(sectorCount)) * 0xFCu;
		std::memset(mSectorAuxiliaryData, 0, auxiliaryBytes);
	}
	// Original function: FUN_0044ecec.
	void CGameMapSectorAttachmentManager::IO_Savegame_DoSave(CIoHelper& io)
	{
		io.IO_Tool_Chunk_StartCreation(NC2DataIds::LogicMapChunk::SectorAttachments, 1);
		io.IO_Memory_WriteBinary(this, 8);
		io.IO_Memory_WriteBinary(mSectorData, static_cast<std::uint32_t>(mGridWidth) * static_cast<std::uint32_t>(mGridHeight) * 0x540u);
		io.IO_Tool_Chunk_FinishAndWrite();
	}

}
