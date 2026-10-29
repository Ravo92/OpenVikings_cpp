#pragma once

#include "XPalette.h"
#include "XRemapTable.h"

#include <cstddef>
#include <cstdint>

namespace NC2E2
{
	// Original global: 0055510C.
	inline void* gE2StaticDataPalettesManager = nullptr;

	struct SE2StaticDataPalette256
	{
		char name[32];
		NXBasics::CPalette* palette;           // +0x20; owned.
		NXBasics::CPalette* preshadedPalettes; // +0x24; 31 owned objects after an aligned count cookie.
		NXBasics::CRemapTable* remapTable;     // +0x28; owned.
		SE2StaticDataPalette256* remapTarget;  // +0x2C; borrowed record in the same array.
	};

	struct SE2StaticDataPalette16
	{
		char name[32];
		std::int32_t palette256Id;
		std::uint32_t colorRange;
	};

	// Complete Windows static region 005550E0..005550FF.
	struct SStaticDataPalettes
	{
		std::int32_t palette256Count;
		SE2StaticDataPalette256* palettes256;
		std::int32_t palette16Count;
		SE2StaticDataPalette16* palettes16;
		std::int32_t fishPaletteId;
		std::int32_t guiBubblesPaletteId;
		NXBasics::CPalette* blackPalette;
		NXBasics::CPalette* vertexColorsPalette;
	};

	inline SStaticDataPalettes gStaticPalettes{ };

	void* __fastcall E2StaticDataPalettesManager_Construct(void* manager, void* unusedEdx);
	std::int32_t E2StaticDataPalettesManager_GetPalette256Id(const char* name);
	std::int32_t E2StaticDataPalettesManager_GetPalette16Id(const char* name);

	static_assert(sizeof(SE2StaticDataPalette256) == (sizeof(void*) == 8 ? 64 : 48));
	static_assert(offsetof(SE2StaticDataPalette256, palette) == 0x20);
	static_assert(offsetof(SE2StaticDataPalette256, preshadedPalettes) == (sizeof(void*) == 8 ? 40 : 36));
	static_assert(offsetof(SE2StaticDataPalette256, remapTable) == (sizeof(void*) == 8 ? 48 : 40));
	static_assert(offsetof(SE2StaticDataPalette256, remapTarget) == (sizeof(void*) == 8 ? 56 : 44));
	static_assert(sizeof(SE2StaticDataPalette16) == 40);
	static_assert(offsetof(SE2StaticDataPalette16, palette256Id) == 0x20);
	static_assert(offsetof(SE2StaticDataPalette16, colorRange) == 0x24);
	static_assert(sizeof(SStaticDataPalettes) == (sizeof(void*) == 8 ? 56 : 32));
	static_assert(offsetof(SStaticDataPalettes, palettes256) == (sizeof(void*) == 8 ? 8 : 4));
	static_assert(offsetof(SStaticDataPalettes, palette16Count) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(offsetof(SStaticDataPalettes, palettes16) == (sizeof(void*) == 8 ? 24 : 12));
	static_assert(offsetof(SStaticDataPalettes, fishPaletteId) == (sizeof(void*) == 8 ? 32 : 16));
	static_assert(offsetof(SStaticDataPalettes, guiBubblesPaletteId) == (sizeof(void*) == 8 ? 36 : 20));
	static_assert(offsetof(SStaticDataPalettes, blackPalette) == (sizeof(void*) == 8 ? 40 : 24));
	static_assert(offsetof(SStaticDataPalettes, vertexColorsPalette) == (sizeof(void*) == 8 ? 48 : 28));
}