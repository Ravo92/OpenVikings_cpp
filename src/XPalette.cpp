#include "XBasicTypes.h"
#include "XCrtFileAccess.h"
#include "XFileAccess.h"
#include "XPalette.h"
#include "XPaletteAccess.h"

#include <cstdint>
#include <new>

namespace NXBasics
{
	// Original function: FUN_0040f994; constructs all 256 complete color records.
	static void ConstructPaletteColors(std::uint8_t(*colors)[4])
	{
		for (std::uint32_t index = 0; index < 256; ++index) ::new (colors[index]) SColorRGB();
	}

	// Original function: FUN_0040f95e
	CPalette::CPalette() : mColors{ }, mHighColorTable(nullptr), mTrueColorTable(nullptr)
	{
		static_assert(offsetof(CPalette, mColors) == PaletteOffset::Colors);
		static_assert(offsetof(CPalette, mHighColorTable) == PaletteOffset::HighColorTable);
		static_assert(offsetof(CPalette, mTrueColorTable) == PaletteOffset::TrueColorTable);
		ConstructPaletteColors(mColors);
		l_MakeAllColorsBlack();
	}

	// Original function: FUN_0040fbbb; fourth bytes survive; caches are invalidated.
	void CPalette::l_MakeAllColorsBlack()
	{
		for (auto& color : mColors) color[0] = color[1] = color[2] = 0;
		Palette_PaletteChanged(this);
	}

	// Original function: FUN_0040f9b7.
	CPalette::CPalette(const CPalette& source) : mColors{ }, mHighColorTable(nullptr), mTrueColorTable(nullptr)
	{
		std::memmove(mColors, source.mColors, sizeof(mColors));
		Palette_PaletteChanged(this);
	}

	// Original function: FUN_0040fa02; deleting wrapper: LAB_00480e5c.
	CPalette::~CPalette()
	{
		if (mHighColorTable != nullptr)
		{
			Runtime_Free(mHighColorTable);
			mHighColorTable = nullptr;
		}

		if (mTrueColorTable != nullptr)
		{
			Runtime_Free(mTrueColorTable);
			mTrueColorTable = nullptr;
		}
	}

	// Original function: LAB_00480e5c; original array cookie and reverse destruction.
	void* CPalette::DeleteOriginal(CPalette* palette, std::uint32_t flags)
	{
		void* allocation = palette;
		if ((flags & 2) != 0)
		{
			allocation = reinterpret_cast<std::byte*>(palette) - PaletteArrayCookieSize;
			std::uint32_t count;
			std::memcpy(&count, allocation, sizeof(count));
			if (Runtime_TimeSigned32(count - 1u) >= 0)
			{
				std::uintptr_t cursor = reinterpret_cast<std::uintptr_t>(palette) + static_cast<std::size_t>(count) * sizeof(CPalette);
				do
				{
					cursor -= sizeof(CPalette);
					reinterpret_cast<CPalette*>(cursor)->CPalette::~CPalette();
				} while (--count != 0);
			}
		}
		else
		{
			palette->CPalette::~CPalette();
		}
		if ((flags & 1) != 0) Runtime_Free(allocation);
		return allocation;
	}

	// Original function: FUN_0040fa43
	CPalette::CPalette(void* file, std::uint32_t /*version*/) : mColors{ }, mHighColorTable(nullptr), mTrueColorTable(nullptr)
	{
		File_Read(file, mColors, sizeof(mColors));
	}

	// Original function: LAB_0040fa83
	void CPalette::Storable_SaveData(void* file)
	{
		File_Write(file, mColors, sizeof(mColors));
	}

	// Original function: FUN_0040fa98; void assignment, as in the named original.
	void CPalette::operator=(const CPalette& other)
	{
		if (mHighColorTable != nullptr)
		{
			Runtime_Free(mHighColorTable);
			mHighColorTable = nullptr;
		}
		if (mTrueColorTable != nullptr)
		{
			Runtime_Free(mTrueColorTable);
			mTrueColorTable = nullptr;
		}
		std::memcpy(mColors, other.mColors, sizeof(mColors));
		Palette_PaletteChanged(this);
	}

	// Original function: FUN_0040fb90.
	void CPalette::Modify(const SColorModifier& modifier)
	{
		for (auto& entry : mColors)
		{
			SColorRGB color;
			std::memcpy(&color, entry, sizeof(color));
			color.Modify(modifier);
			std::memcpy(entry, &color, sizeof(color));
		}
		Palette_PaletteChanged(this);
	}
}
