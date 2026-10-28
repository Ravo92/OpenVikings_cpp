#include "XBobManager.h"
#include "XCrtFileAccess.h"
#include "XFileAccess.h"
#include "XFont.h"
#include "XStorableIO.h"

#include <cstdint>

namespace NXBasics
{
	// Original function: FUN_00439674.
	void* CFont::Delete(std::uint32_t flags)
	{
		auto* result = this;
		this->~CFont();
		if ((flags & 1u) != 0) Runtime_Free(result);
		return result;
	}

	// Original function: FUN_00439690.
	CFont::~CFont()
	{
		if (mBobManager != nullptr)
		{
			mBobManager->Delete(1);
			mBobManager = nullptr;
		}
	}

	// Original function: FUN_004396ad; the three native fills cover +0x04 through +0x17.
	void CFont::l_InitObject()
	{
		static_assert(offsetof(CFont, mSerializedData) == FontStorageOffset::SerializedData);
		static_assert(offsetof(CFont, mCharacterSpacing) == FontStorageOffset::CharacterSpacing);
		static_assert(offsetof(CFont, mBobManager) == FontStorageOffset::BobManager);
		static_assert(offsetof(CFont, mPalette) == FontStorageOffset::Palette);
		Runtime_FillMemory(mSerializedData, 0, sizeof(mSerializedData));
		Runtime_FillMemory(&mCharacterSpacing, 0, sizeof(mCharacterSpacing));
		Runtime_FillMemory(&mBobManager, 0, sizeof(mBobManager) + sizeof(mPalette));
	}

	// Original function: FUN_004397ea.
	CFont::CFont(void* file, std::uint32_t /*version*/)
	{
		l_InitObject();
		File_Read(file, mSerializedData, sizeof(mSerializedData));
		mBobManager = static_cast<CBobManager*>(XB_Storable_LoadObject(file));
	}

	// Original function: FUN_0043981a
	void CFont::Storable_SaveData(void* file)
	{
		File_Write(file, mSerializedData, sizeof(mSerializedData));
		mBobManager->Storable_Save(file);
	}
}
