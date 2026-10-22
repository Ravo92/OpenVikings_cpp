#include "XBobManager.h"
#include "XCrtFileAccess.h"
#include "XFileAccess.h"
#include "XMemory.h"
#include "XStorableIO.h"

#include <cstring>
#include <memory>
#include <new>

namespace NXBasics
{
	// Original function: FUN_004398e9.
	void* CBobManager::Delete(std::uint32_t flags)
	{
		auto* result = this;
		this->~CBobManager();
		if ((flags & 1u) != 0) Runtime_Free(result);
		return result;
	}

	// Original function: FUN_00439905.
	CBobManager::~CBobManager()
	{
		if (mBobDataMemory != nullptr)
		{
			mBobDataMemory->Delete(1);
			mBobDataMemory = nullptr;
		}

		if (mPackedLineDataMemory != nullptr)
		{
			mPackedLineDataMemory->Delete(1);
			mPackedLineDataMemory = nullptr;
		}

		if (mLineControlMemory != nullptr)
		{
			mLineControlMemory->Delete(1);
			mLineControlMemory = nullptr;
		}

		l_System_InitObject();
	}

	// Original function: FUN_0043994d
	void CBobManager::l_System_InitObject()
	{
		std::memset(&mSerialized, 0, sizeof(mSerialized));

		mUnknown20 = 0;
		mBobData = nullptr;
		mPackedLineData = nullptr;
		mLineControlData = nullptr;
		mBobDataMemory = nullptr;
		mPackedLineDataMemory = nullptr;
		mLineControlMemory = nullptr;

		mStaticVars = nullptr;
	}

	// Original function: FUN_0043997d
	CBobManager::CBobManager(void* file, std::uint32_t /*version*/)
	{
		l_System_InitObject();

		File_Read(file, &mSerialized, sizeof(mSerialized));

		if (mSerialized.bobCount != 0)
		{
			mBobDataMemory = static_cast<CMemory*>(XB_Storable_LoadObject(file));
			mPackedLineDataMemory = static_cast<CMemory*>(XB_Storable_LoadObject(file));
			mLineControlMemory = static_cast<CMemory*>(XB_Storable_LoadObject(file));
		}

		l_System_SetRawPointer();
	}

	// Remaster-only construction; allocation and destruction use the native CMemory owners.
	CBobManager::CBobManager(std::uint32_t firstBobId, std::uint32_t bobCount, const void* bobData, std::uint32_t bobDataSize, const void* packedLineData, std::uint32_t packedLineDataSize, const void* lineControlData, std::uint32_t lineControlDataSize)
	{
		l_System_InitObject();
		mSerialized.firstBobId = firstBobId;
		mSerialized.bobCount = bobCount;
		mSerialized.packedLineDataSize = packedLineDataSize;
		auto records = std::make_unique<CMemory>(bobDataSize);
		auto pixels = std::make_unique<CMemory>(packedLineDataSize);
		auto lines = std::make_unique<CMemory>(lineControlDataSize);
		if (bobDataSize != 0) std::memcpy(records->mData, bobData, bobDataSize);
		if (packedLineDataSize != 0) std::memcpy(pixels->mData, packedLineData, packedLineDataSize);
		if (lineControlDataSize != 0) std::memcpy(lines->mData, lineControlData, lineControlDataSize);
		mBobDataMemory = records.release(); mPackedLineDataMemory = pixels.release(); mLineControlMemory = lines.release();
		l_System_SetRawPointer();
	}

	// Original function: FUN_004399cf
	void CBobManager::Storable_SaveData(void* file)
	{
		l_Generate_ResizePackedLineDataBuffer(mSerialized.packedLineDataSize);

		File_Write(file, &mSerialized, sizeof(mSerialized));

		if (mSerialized.bobCount != 0)
		{
			mBobDataMemory->Storable_Save(file);
			mPackedLineDataMemory->Storable_Save(file);
			mLineControlMemory->Storable_Save(file);
		}
	}

	// Original function: FUN_00439a12
	void CBobManager::l_Generate_ResizePackedLineDataBuffer(std::uint32_t size)
	{
		if (mPackedLineDataMemory != nullptr)
		{
			mPackedLineDataMemory->ChangeBufferSize(size, true, 0);
		}
		else
		{
			mPackedLineDataMemory = new (std::nothrow) CMemory(size, static_cast<std::uint8_t>(0));
		}

		l_System_SetRawPointer();
	}

	// Original function: FUN_00439a56
	void CBobManager::l_System_SetRawPointer()
	{
		mBobData = mBobDataMemory != nullptr ? mBobDataMemory->mData : nullptr;
		mPackedLineData = mPackedLineDataMemory != nullptr ? mPackedLineDataMemory->mData : nullptr;
		mLineControlData = mLineControlMemory != nullptr ? mLineControlMemory->mData : nullptr;
	}
}
