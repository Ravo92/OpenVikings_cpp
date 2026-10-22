#include "XDiagnostics.h"
#include "XFileAccess.h"
#include "XNumericConstants.h"
#include "XStorableIO.h"
#include "XStringArray.h"

#include <cstring>
#include <new>

namespace NXBasics
{
	// Original function: FUN_0042ba56
	CStringArray::CStringArray(std::uint8_t useCountForNextId) : mUseCountForNextId(useCountForNextId), mPadding05{ }, mStringCount(0), mIdUpperBound(1), mOffsetCapacity(100), mUsedStringBytes(0), mOffsets(nullptr), mStrings(nullptr)
	{
		void* allocation = Runtime_AllocateWithNewHandler(sizeof(CMemory), 1);
		mOffsets = allocation != nullptr ? ::new (allocation) CMemory(mOffsetCapacity * std::uint32_t{ 4 }) : nullptr;
		mOffsets->Fill(NXBasics::IntegerRange::UInt8MaximumSigned);
	}

	// Original function: FUN_0042bad3
	// Original deleting destructor: FUN_0042bab7
	CStringArray::~CStringArray()
	{
		if (mOffsets != nullptr)
		{
			delete mOffsets;
			mOffsets = nullptr;
		}

		if (mStrings != nullptr)
		{
			delete mStrings;
			mStrings = nullptr;
		}
	}

	// Original function: FUN_0042bb02
	CStringArray::CStringArray(void* file, std::uint32_t /*version*/) : mUseCountForNextId(0), mPadding05{ }, mStringCount(0), mIdUpperBound(0), mOffsetCapacity(0), mUsedStringBytes(0), mOffsets(nullptr), mStrings(nullptr)
	{
		auto* objectBytes = reinterpret_cast<std::byte*>(this);
		File_Read(file, objectBytes + 0x04, 0x14);

		mOffsets = static_cast<CMemory*>(XB_Storable_LoadObject(file));
		mOffsets->Decrypt(1);

		std::uint8_t hasStrings;
		static_cast<CFile*>(file)->ReadFlag(&hasStrings);

		if (hasStrings != 0)
		{
			mStrings = static_cast<CMemory*>(XB_Storable_LoadObject(file));
			mStrings->Decrypt(1);
		}
		else
		{
			mStrings = nullptr;
		}
	}

	// Original function: LAB_0042bc04
	std::uint32_t CStringArray::Storable_GetId()
	{
		return 0x3FD;
	}

	// Original function: LAB_0040c775
	std::uint32_t CStringArray::Storable_GetVersion()
	{
		return 0;
	}

	// Original function: FUN_0042bb72
	void CStringArray::Storable_SaveData(void* file)
	{
		mOffsetCapacity = mIdUpperBound;
		mOffsets->ChangeBufferSize(mOffsetCapacity * std::uint32_t{ 4 }, false, 0xEE);

		File_Write(file, &mUseCountForNextId, 0x14);

		mOffsets->Encrypt(1);
		mOffsets->Storable_Save(file);
		mOffsets->Decrypt(1);

		if (mStrings != nullptr)
		{
			static_cast<CFile*>(file)->WriteTrueFlag();

			mStrings->ChangeBufferSize(mUsedStringBytes, false, 0xEE);

			mStrings->Encrypt(1);
			mStrings->Storable_Save(file);
			mStrings->Decrypt(1);
		}
		else
		{
			static_cast<CFile*>(file)->WriteFalseFlag();
		}
	}

	// Original function: FUN_0042bc0a
	char* CStringArray::GetStringPtr(std::uint32_t stringId) const
	{
		if (stringId >= mOffsetCapacity)
		{
			return nullptr;
		}

		std::uint32_t offset;
		std::memcpy(&offset, mOffsets->mData + stringId * std::uint32_t{ 4 }, sizeof(offset));

		if (offset == NXBasics::IntegerRange::UInt32Maximum)
		{
			return nullptr;
		}

		return reinterpret_cast<char*>(mStrings->mData + offset);
	}

	// Original function: FUN_0042bc30
	bool CStringArray::InsertString(std::uint32_t stringId, const char* text)
	{
		if (stringId < mOffsetCapacity)
		{
			std::uint32_t offset;
			std::memcpy(&offset, mOffsets->mData + stringId * std::uint32_t{ 4 }, sizeof(offset));

			if (offset != NXBasics::IntegerRange::UInt32Maximum)
			{
				return false;
			}
		}
		else
		{
			if (stringId >= UINT32_C(1000000))
			{
				ReportWarning("NXBasics::CStringArray::InsertString  too much strings! Maximum is %d!", stringId);
				return false;
			}

			mOffsetCapacity = stringId + std::uint32_t{ 100 };
			mOffsets->ChangeBufferSize(mOffsetCapacity * std::uint32_t{ 4 }, true, NXBasics::IntegerRange::UInt8MaximumSigned);
		}

		const std::uint32_t byteCount = Runtime_StringLength(text) + std::uint32_t{ 1 };

		if (mStrings == nullptr)
		{
			void* allocation = Runtime_AllocateWithNewHandler(sizeof(CMemory), 1);
			mStrings = allocation != nullptr ? ::new (allocation) CMemory(byteCount + std::uint32_t{ 10000 }) : nullptr;
		}
		else
		{
			const std::uint32_t requiredSize = mUsedStringBytes + byteCount;

			if (requiredSize > mStrings->mSize)
			{
				mStrings->ChangeBufferSize(requiredSize + std::uint32_t{ 10000 }, false, 0xEE);
			}
		}

		std::memcpy(mOffsets->mData + stringId * std::uint32_t{ 4 }, &mUsedStringBytes, sizeof(mUsedStringBytes));
		Runtime_CopyString(reinterpret_cast<char*>(mStrings->mData + mUsedStringBytes), text);

		mUsedStringBytes += byteCount;
		++mStringCount;

		const std::uint32_t nextId = stringId + std::uint32_t{ 1 };

		if (nextId > mIdUpperBound)
		{
			mIdUpperBound = nextId;
		}

		return true;
	}

	// Original function: FUN_0042bd12
	std::uint32_t CStringArray::InsertString(const char* text)
	{
		const std::uint32_t stringId = l_GetNextFreeStringId();

		if (!InsertString(stringId, text))
		{
			return NXBasics::IntegerRange::UInt32Maximum;
		}

		return stringId;
	}

	// Original function: FUN_0042bd39
	std::uint32_t CStringArray::l_GetNextFreeStringId() const
	{
		if (mStringCount == mOffsetCapacity)
		{
			return mOffsetCapacity;
		}

		if (mUseCountForNextId != 0)
		{
			return mStringCount;
		}

		for (std::uint32_t stringId = 0; stringId < mOffsetCapacity; ++stringId)
		{
			std::uint32_t offset;
			std::memcpy(&offset, mOffsets->mData + stringId * std::uint32_t{ 4 }, sizeof(offset));

			if (offset == NXBasics::IntegerRange::UInt32Maximum)
			{
				return stringId;
			}
		}

		return mOffsetCapacity;
	}
}
