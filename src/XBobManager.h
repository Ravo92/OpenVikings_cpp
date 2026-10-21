#pragma once

#include "XStorable.h"

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	class CMemory;

	class CBobManager : public CStorable
	{
	public:
		// Original address: 0051261C; Mac symbol: CBobManager::mStaticVars.
		// Non-owning alias of the bitmap managed by Inform_DesktopXTructed.
		inline static CBitmap* mStaticVars = nullptr;

		// Original function: FUN_0043997d
		CBobManager(void* file, std::uint32_t version);

		// Remaster asset packs supply validated native data blocks without a temporary file.
		CBobManager(std::uint32_t firstBobId, std::uint32_t bobCount, const void* bobData, std::uint32_t bobDataSize, const void* packedLineData, std::uint32_t packedLineDataSize, const void* lineControlData, std::uint32_t lineControlDataSize);

		// Original function: FUN_00439905.
		virtual ~CBobManager();
		void* Delete(std::uint32_t flags);

		// Original function: LAB_00439be7
		std::uint32_t Storable_GetId() override
		{
			return 0x3F4;
		}

		// Original function: LAB_0040c775
		std::uint32_t Storable_GetVersion() override
		{
			return 0;
		}

		// Original function: FUN_004399cf
		void Storable_SaveData(void* file) override;

	private:
		struct SSerializedData
		{
			std::uint32_t firstBobId;             // Object +0x04
			std::uint32_t bobCount;               // Object +0x08
			std::uint32_t packedLineDataSize;     // Object +0x0C
			std::byte unresolved10[16];         // Object +0x10; semantics unresolved.
		};

		static_assert(sizeof(SSerializedData) == 28);

		// Original function: FUN_0043994d
		void l_System_InitObject();

		// Original function: FUN_00439a12
		void l_Generate_ResizePackedLineDataBuffer(std::uint32_t size);

		// Original function: FUN_00439a56
		void l_System_SetRawPointer();

		SSerializedData mSerialized;              // +0x04
		std::uint32_t mUnknown20;                 // +0x20; semantics unresolved.
		std::uint8_t* mBobData;                   // +0x24; borrowed from mBobDataMemory.
		std::uint8_t* mPackedLineData;            // +0x28; borrowed from mPackedLineDataMemory.
		std::uint8_t* mLineControlData;           // +0x2C; borrowed from mLineControlMemory.
		CMemory* mBobDataMemory;                  // +0x30; owned.
		CMemory* mPackedLineDataMemory;           // +0x34; owned.
		CMemory* mLineControlMemory;              // +0x38; owned.
	};

	static_assert(sizeof(CBobManager) == 60, "CBobManager requires the original x86 layout");
}
