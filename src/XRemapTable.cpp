#include "XFileAccess.h"
#include "XPalette.h"
#include "XPaletteAccess.h"
#include "XRemapTable.h"

namespace NXBasics
{
	// Original function: FUN_0040fd80; inclusive unsigned source range.
	CRemapTable::CRemapTable(const CPalette& source, const CPalette& destination, std::uint32_t first, std::uint32_t last) : mRemapIndices{ }
	{
		static_assert(offsetof(CRemapTable, mRemapIndices) == sizeof(void*));
		if (first > last) return;
		for (std::uint32_t index = first; index <= last; ++index)
		{
			SColorRGB color;
			std::memcpy(&color, source.mColors[index], sizeof(color));
			mRemapIndices[index] = Palette_FindMatchingColor(&destination, color);
		}
	}

	// Original function: LAB_0040fdda
	std::uint32_t CRemapTable::Storable_GetId()
	{
		return 0x3F7;
	}

	// Original function: LAB_0040c775
	std::uint32_t CRemapTable::Storable_GetVersion()
	{
		return 0;
	}

	// Original function: FUN_0040fde0
	CRemapTable::CRemapTable(void* file, std::uint32_t /*version*/) : mRemapIndices{ }
	{
		File_Read(file, mRemapIndices, sizeof(mRemapIndices));
	}

	// Original function: LAB_0040fe12
	void CRemapTable::Storable_SaveData(void* file)
	{
		File_Write(file, mRemapIndices, sizeof(mRemapIndices));
	}
}
