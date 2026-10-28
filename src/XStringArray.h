#pragma once

#include "XMemory.h"
#include "XStorable.h"

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CStringArray final : public CStorable
	{
	public:
		// Original function: FUN_0042ba56
		explicit CStringArray(std::uint8_t useCountForNextId);

		// Original function: FUN_0042bad3
		// Original deleting destructor: FUN_0042bab7
		virtual ~CStringArray();

		CStringArray(const CStringArray&) = delete;
		CStringArray& operator=(const CStringArray&) = delete;

		// Original function: LAB_0040c775
		std::uint32_t Storable_GetVersion() override;

		// Original function: FUN_0042bb02
		CStringArray(void* file, std::uint32_t version);

		// Original function: FUN_0042bb72
		void Storable_SaveData(void* file) override;

		// Original function: LAB_0042bc04
		std::uint32_t Storable_GetId() override;

		// Original function: FUN_0042bc0a
		char* GetStringPtr(std::uint32_t stringId) const;

		// Original function: FUN_0042bc30
		bool InsertString(std::uint32_t stringId, const char* text);

		// Original function: FUN_0042bd12
		std::uint32_t InsertString(const char* text);
		std::uint32_t GetNumberOfStrings() const noexcept
		{
			return mStringCount;
		}

	private:
		friend class CIniFileReader;

		// Original function: FUN_0042bd39
		std::uint32_t l_GetNextFreeStringId() const;

		std::uint8_t mUseCountForNextId; // +0x04
		std::byte mPadding05[3];        // +0x05
		std::uint32_t mStringCount;     // +0x08
		std::uint32_t mIdUpperBound;    // +0x0C
		std::uint32_t mOffsetCapacity;  // +0x10
		std::uint32_t mUsedStringBytes; // +0x14
		CMemory* mOffsets;             // +0x18
		CMemory* mStrings;             // +0x1C
	};

	static_assert(sizeof(CStringArray) == (sizeof(void*) == 8 ? 48 : 32), "Unexpected native string array layout");
}
