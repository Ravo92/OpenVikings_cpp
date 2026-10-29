#pragma once

#include <cstddef>

namespace NXBasics
{
	class CBitmap;
	class CPalette;
}

namespace NC2E2
{
	struct SE2StaticDataTextureData
	{
		char name[120];
		NXBasics::CBitmap* bitmap;
		NXBasics::CPalette* palette;
	};

	static_assert(sizeof(SE2StaticDataTextureData) == (sizeof(void*) == 8 ? 136 : 128));
	static_assert(offsetof(SE2StaticDataTextureData, bitmap) == 0x78);
	static_assert(offsetof(SE2StaticDataTextureData, palette) == 120 + sizeof(void*));

	// Complete texture static region: 00568AF0..00568AF7.
	// CE2StaticDataTexturesManager owns the arrays populated by GetTextureId.
	struct SStaticDataTextures
	{
		NXBasics::CBitmap** bitmaps;
		NXBasics::CPalette** palettes;
	};

	extern SStaticDataTextures gStaticDataTextures;

	// Shared vertex-color palette ownership at Windows address 005550FC.
	extern NXBasics::CPalette*& gVertexColorsPalette;
	static_assert(offsetof(SStaticDataTextures, bitmaps) == 0);
	static_assert(offsetof(SStaticDataTextures, palettes) == sizeof(void*));
	static_assert(sizeof(SStaticDataTextures) == 2 * sizeof(void*));
}

namespace NC2Logic
{
	struct SStaticTrianglePatternTypeData;
	// Canonical array slot at 005112E0, owned by the native static-data manager.
	extern SStaticTrianglePatternTypeData*& gStaticTrianglePatternTypes;
}