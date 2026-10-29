#pragma once

#include <cstdint>

namespace NXBasics
{
	class CBitmap;
	class CPalette;
	struct SColorRGB;

	void Font_PrintCharacter(const void* font, CBitmap* bitmap, std::uint8_t character, std::int32_t x, std::int32_t y);
	void Font_PrintCharacter(const void* font, const SColorRGB* colour, CBitmap* bitmap, std::uint8_t character, std::int32_t x, std::int32_t y);

	std::int32_t Font_GetCharacterWidth(const void* font, std::uint8_t character);
	std::int32_t Font_GetCharacterHeight(const void* font, std::uint8_t character);
	std::int32_t Font_GetPixelWidth(const void* font, const char* text);
	std::int32_t Font_GetPixelHeight(const void* font, const char* text);

	void Font_SetPalettePtr(void* font, CPalette* palette);

	const void* BobManager_GetBobAreaRectanglePtr(const void* bobManager, std::uint32_t bobId);
}