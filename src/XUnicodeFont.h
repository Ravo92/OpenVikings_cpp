#pragma once

#include <cstdint>
#include <string>

namespace NXBasics
{
	class CBitmap;
	struct SColorRGB;
	// Remaster-only rendering; no members are added to serialized CFont/CBitmap.
	bool Font_TryLegacyText(const void* font, const char* text, std::string& legacy);
	std::int32_t UnicodeFont_Measure(const void* font, const char* text, bool height, std::int32_t wrapWidth = 0);
	std::int32_t UnicodeFont_CursorPosition(const void* font, const char* text, std::uint32_t bytePosition);
	bool UnicodeFont_Print(const void* font, CBitmap* bitmap, const SColorRGB* color, std::int32_t x, std::int32_t y, const char* text, std::int32_t wrapWidth = 0);
}