#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XFont.h"
#include "XBobManager.h"
#include "XUnicodeFont.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace NXBasics
{
	// Original function: FUN_004396dc; the font palette is used without a color override.
	void Font_PrintCharacter(const void* font, CBitmap* bitmap, std::uint8_t character, std::int32_t x, std::int32_t y)
	{
		void* manager;
		CPalette* palette;
		std::memcpy(&manager, static_cast<const std::byte*>(font) + FontStorageOffset::BobManager, sizeof(manager));
		std::memcpy(&palette, static_cast<const std::byte*>(font) + FontStorageOffset::Palette, sizeof(palette));
		BobManager_PrintBob(manager, static_cast<std::uint32_t>(character) - 0x20u, bitmap, x, y, palette);
	}

	// Original function: FUN_004396ff
	void Font_PrintCharacter(const void* font, const SColorRGB* colour, CBitmap* bitmap, std::uint8_t character, std::int32_t x, std::int32_t y)
	{
		void* bobManager;
		std::memcpy(&bobManager, static_cast<const std::byte*>(font) + FontStorageOffset::BobManager, sizeof(bobManager));
		const std::uint32_t bobId = static_cast<std::uint32_t>(character) - 0x20u;
		BobManager_PrintBobUsingTransparency(bobManager, bobId, bitmap, x, y, colour);
	}

	// Original function: FUN_00439722
	std::int32_t Font_GetCharacterWidth(const void* font, std::uint8_t character)
	{
		const auto* fontBytes = static_cast<const std::byte*>(font);

		const void* bobManager;
		std::memcpy(&bobManager, fontBytes + FontStorageOffset::BobManager, sizeof(bobManager));

		const std::uint32_t bobId = static_cast<std::uint32_t>(character) - 0x20u;
		const void* rectangle = BobManager_GetBobAreaRectanglePtr(bobManager, bobId);

		if (rectangle == nullptr)
		{
			return 0;
		}

		const auto* rectangleBytes = static_cast<const std::byte*>(rectangle);

		std::uint32_t width;
		std::uint32_t spacing;
		std::uint32_t x;

		std::memcpy(&width, rectangleBytes + 0x08, sizeof(width));
		std::memcpy(&spacing, fontBytes + FontStorageOffset::CharacterSpacing, sizeof(spacing));
		std::memcpy(&x, rectangleBytes, sizeof(x));

		return Runtime_TimeSigned32(width + spacing + x + 1u);
	}

	// Original function: FUN_0043974a
	std::int32_t Font_GetCharacterHeight(const void* font, std::uint8_t character)
	{
		const auto* fontBytes = static_cast<const std::byte*>(font);

		const void* bobManager;
		std::memcpy(&bobManager, fontBytes + FontStorageOffset::BobManager, sizeof(bobManager));

		const std::uint32_t bobId = static_cast<std::uint32_t>(character) - 0x20u;
		const void* rectangle = BobManager_GetBobAreaRectanglePtr(bobManager, bobId);

		if (rectangle == nullptr)
		{
			return 0;
		}

		const auto* rectangleBytes = static_cast<const std::byte*>(rectangle);

		std::uint32_t height;
		std::uint32_t y;

		std::memcpy(&height, rectangleBytes + 0x0C, sizeof(height));
		std::memcpy(&y, rectangleBytes + 0x04, sizeof(y));

		return Runtime_TimeSigned32(height + y + 1u);
	}

	// Original function: FUN_0043976c
	std::int32_t Font_GetPixelWidth(const void* font, const char* text)
	{
		if (font == nullptr || text == nullptr)
			return 0;

		std::string legacy;

		if (!Font_TryLegacyText(font, text, legacy))
			return UnicodeFont_Measure(font, text, false);

		text = legacy.c_str();
		std::uint32_t width = 0;

		for (;;)
		{
			const auto character = static_cast<std::uint8_t>(*text);
			const std::uint8_t measuredCharacter = character == 0x09u || character == 0x20u ? std::uint8_t{ 0x69 } : character;

			width += static_cast<std::uint32_t>(Font_GetCharacterWidth(font, measuredCharacter));

			// The original processes the terminator and reloads the byte after the call.
			const auto processedCharacter = static_cast<std::uint8_t>(*text);
			++text;

			if (processedCharacter == 0)
			{
				break;
			}
		}

		return Runtime_TimeSigned32(width);
	}

	// Original function: FUN_004397a6
	std::int32_t Font_GetPixelHeight(const void* font, const char* text)
	{
		if (font == nullptr || text == nullptr)
			return 0;

		std::string legacy;

		if (!Font_TryLegacyText(font, text, legacy))
			return UnicodeFont_Measure(font, text, true);

		text = legacy.c_str();
		std::int32_t height = 0;

		for (;;)
		{
			const auto character = static_cast<std::uint8_t>(*text);

			if (character != 0x09u && character != 0x20u)
			{
				const std::int32_t characterHeight = Font_GetCharacterHeight(font, character);

				if (characterHeight > height)
				{
					height = characterHeight;
				}
			}

			// The original also measures the terminating zero byte.
			const auto processedCharacter = static_cast<std::uint8_t>(*text);
			++text;

			if (processedCharacter == 0)
			{
				break;
			}
		}

		return height;
	}

	// Original function: FUN_004397e0.
	void Font_SetPalettePtr(void* font, CPalette* palette)
	{
		std::memcpy(static_cast<std::byte*>(font) + FontStorageOffset::Palette, &palette, sizeof(palette));
	}

	// Original function: FUN_00439a8e
	const void* BobManager_GetBobAreaRectanglePtr(const void* bobManager, std::uint32_t bobId)
	{
		const auto* managerBytes = static_cast<const std::byte*>(bobManager);

		std::uint32_t firstBobId;
		std::uint32_t bobCount;

		std::memcpy(&firstBobId, managerBytes + BobManagerOffset::FirstBobId, sizeof(firstBobId));
		std::memcpy(&bobCount, managerBytes + BobManagerOffset::BobCount, sizeof(bobCount));

		const std::uint32_t index = bobId - firstBobId;

		if (index >= bobCount)
		{
			return nullptr;
		}

		const std::byte* entries;
		std::memcpy(&entries, managerBytes + BobManagerOffset::BobData, sizeof(entries));

		const std::uint32_t entryOffset = index * 0x18u;
		const std::byte* entry = entries + entryOffset;

		std::uint32_t entryValue;
		std::memcpy(&entryValue, entry, sizeof(entryValue));

		if (entryValue == 0)
		{
			return nullptr;
		}

		return entry + 0x04;
	}

}