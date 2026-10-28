#include "XPalette.h"
#include "XFont.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

#include "C2EngineDrawingAccess.h"
#include "C2LanguageTools.h"
#include "C2LifetimeGlobals.h"
#include "C2StringAccess.h"
#include "C2StringIO.h"
#include "DSManager.h"
#include "GameProfile.h"
#include "GuiToolsBase.h"
#include "MasterPropertyManager.h"
#include "XBasicTypes.h"
#include "XBitmap.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XUnicodeFont.h"
#include "XTextEncoding.h"
#include "XGuiDesktop.h"
#include "XPaletteAccess.h"
#include "XPicture.h"
#include "XPictureAccess.h"
#include "XStorableIO.h"
#include "XStringArray.h"

#include <windows.h>

#include <cstdarg>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>


namespace
{
	void DestroyGuiStorableResource(void* object)
	{
		if (object == nullptr)
		{
			return;
		}

		// Original deleting-destructor slot: +0x0C, deletion flag 1.
		using TDeletingDestructor = void* (__thiscall*)(void*, unsigned int);

		TDeletingDestructor* methods;
		std::memcpy(&methods, object, sizeof(methods));
		methods[3](object, 1u);
	}
}

namespace NC2GuiToolsBase
{
	// Original function: FUN_004e2bf0; complete seven-byte string at 0050bf48.
	std::int32_t StringToolMisc_StringSortCompare(const char* first, const char* second)
	{
		static constexpr char umlauts[7] = { '\xC4', '\xD6', '\xDC', '\xE4', '\xF6', '\xFC', '\0' };
		if (std::strchr(umlauts, static_cast<signed char>(*first)) == nullptr && std::strchr(umlauts, static_cast<signed char>(*second)) == nullptr) return NXBasics::Runtime_CompareStringsIgnoreCase(first, second);
		char left[100];
		char right[100];
		std::strncpy(left, first, sizeof(left));
		left[99] = '\0';
		std::strncpy(right, second, sizeof(right));
		right[99] = '\0';
		const auto normalize = [] (char& character)
		{
			switch (static_cast<unsigned char>(character))
			{
				case SwitchIds::Latin1Character::UppercaseAUmlaut: character = 'A'; break;
				case SwitchIds::Latin1Character::UppercaseOUmlaut: character = 'O'; break;
				case SwitchIds::Latin1Character::UppercaseUUmlaut: character = 'U'; break;
				case SwitchIds::Latin1Character::LowercaseAUmlaut: character = 'a'; break;
				case SwitchIds::Latin1Character::LowercaseOUmlaut: character = 'o'; break;
				case SwitchIds::Latin1Character::LowercaseUUmlaut: character = 'u'; break;
			}
		};
		normalize(left[0]);
		normalize(right[0]);
		return NXBasics::Runtime_CompareStringsIgnoreCase(left, right);
	}

	// Original function: FUN_004e25ec; alpha Bobs use brightness 128.
	void DrawTool_DrawIcon(std::uint32_t icon, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y)
	{
		const auto& resources = CGuiBaseDataManager::mStaticVars.resources;
		if (NXBasics::BobManager_GetBobType(resources.windowBobManager, icon) == 4) NXBasics::BobManager_PrintBob_UsingShadedAlpha(resources.windowBobManager, icon, bitmap, x, y, resources.backgroundPalette, 128);
		else NXBasics::BobManager_PrintBob(resources.windowBobManager, icon, bitmap, x, y, resources.backgroundPalette);
	}

	// Original function: FUN_004e2376.
	void DrawTool_FillArea(void* context, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle, std::int32_t mode)
	{
		if (mode == 0) return;
		const auto& resources = CGuiBaseDataManager::mStaticVars.dynamic;
		NXBasics::CBitmap* tile = resources.buttonBackground;
		if (mode == 1) tile = resources.background;
		else if (mode == 3) tile = resources.selectedBackground;
		else if (mode == 4) tile = resources.highlightedButton;
		else if (mode == 5) tile = resources.headlineBackground;
		NXBasics::SRectangle area = *rectangle;
		if (context != nullptr)
		{
			std::memcpy(&area, static_cast<std::byte*>(context) + 4, sizeof(area));
			area.x = NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(rectangle->x));
			area.y = NXBasics::Runtime_TimeSigned32(0u - static_cast<std::uint32_t>(rectangle->y));
		}
		std::uint32_t x = 0u - static_cast<std::uint32_t>(bitmap->GetSourceX());
		std::uint32_t y = 0u - static_cast<std::uint32_t>(bitmap->GetSourceY());
		NXBasics::CBitmap* source = bitmap->GetSourceBitmap();
		while (source != nullptr && source->GetSourceBitmap() != nullptr)
		{
			x -= static_cast<std::uint32_t>(source->GetSourceX());
			y -= static_cast<std::uint32_t>(source->GetSourceY());
			source = source->GetSourceBitmap();
		}
		const auto width = static_cast<std::uint32_t>(tile->GetRectangle().width);
		const auto height = static_cast<std::uint32_t>(tile->GetRectangle().height);
		while (NXBasics::Runtime_TimeSigned32(x) < NXBasics::Runtime_TimeSigned32(0u - width)) x += width;
		while (NXBasics::Runtime_TimeSigned32(y) < NXBasics::Runtime_TimeSigned32(0u - height)) y += height;
		bitmap->FillWithBitmap(area, tile, NXBasics::Runtime_TimeSigned32(x), NXBasics::Runtime_TimeSigned32(y));
	}

	// Original function: FUN_004e2445
	void DrawTool_DrawFrame(void* /*context*/, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* /*rectangle*/, std::int32_t mode)
	{
		if (mode == 0)
			return;

		const auto signedValue = [] (std::uint32_t value) -> std::int32_t
		{
			return NXBasics::Runtime_TimeSigned32(value);
		};

		const auto bobDimension = [] (const void* manager, std::uint32_t bobId, std::size_t offset) -> std::uint32_t
		{
			const void* rectangle = NXBasics::BobManager_GetBobAreaRectanglePtr(manager, bobId);

			std::uint32_t dimension;
			std::memcpy(&dimension, static_cast<const std::byte*>(rectangle) + offset, sizeof(dimension));
			return dimension;
		};

		const auto& guiResources = CGuiBaseDataManager::mStaticVars.resources;

		auto* bobManager = guiResources.windowBobManager;
		auto* edgePalette = guiResources.backgroundPalette;
		const std::uint32_t edgeId = mode == 1 ? 5u : 13u;

		std::uint32_t position = 0;

		while (signedValue(position) < bitmap->GetRectangle().width)
		{
			NXBasics::BobManager_PrintBob(bobManager, edgeId, bitmap, signedValue(position), 0, edgePalette);

			const std::int32_t bottom = signedValue(static_cast<std::uint32_t>(bitmap->GetRectangle().height) - 2u);

			NXBasics::BobManager_PrintBob(bobManager, edgeId + 2u, bitmap, signedValue(position), bottom, edgePalette);

			position += bobDimension(bobManager, edgeId, 8);
		}

		position = 0;

		while (signedValue(position) < bitmap->GetRectangle().height)
		{
			NXBasics::BobManager_PrintBob(bobManager, edgeId + 1u, bitmap, 0, signedValue(position), edgePalette);

			const std::int32_t right = signedValue(static_cast<std::uint32_t>(bitmap->GetRectangle().width) - 2u);

			NXBasics::BobManager_PrintBob(bobManager, edgeId + 3u, bitmap, right, signedValue(position), edgePalette);

			// The original always takes the vertical step from Bob 6.
			position += bobDimension(bobManager, 6, 12);
		}

		auto* cornerPalette = guiResources.framePalette;
		NXBasics::CBitmap* destination = bitmap;
		NXBasics::CBitmap* source = destination->GetSourceBitmap();

		std::uint32_t sourceX = 0;
		std::uint32_t sourceY = 0;

		while (source != nullptr)
		{
			sourceX += static_cast<std::uint32_t>(destination->GetSourceX());
			sourceY += static_cast<std::uint32_t>(destination->GetSourceY());

			destination = source;
			source = destination->GetSourceBitmap();
		}

		std::uint32_t cornerId = 9;

		if (mode == 1)
			cornerId = 0;
		else if (mode == 5)
			cornerId = 21;

		NXBasics::BobManager_PrintBobUsingTransparency(bobManager, cornerId, destination, signedValue(sourceX), signedValue(sourceY), cornerPalette);

		std::uint32_t right = sourceX + static_cast<std::uint32_t>(bitmap->GetRectangle().width) - 1u;

		NXBasics::BobManager_PrintBobUsingTransparency(bobManager, cornerId + 1u, destination, signedValue(right), signedValue(sourceY), cornerPalette);

		right = sourceX + static_cast<std::uint32_t>(bitmap->GetRectangle().width) - 1u;
		std::uint32_t bottom = sourceY + static_cast<std::uint32_t>(bitmap->GetRectangle().height) - 1u;

		NXBasics::BobManager_PrintBobUsingTransparency(bobManager, cornerId + 2u, destination, signedValue(right), signedValue(bottom), cornerPalette);

		bottom = sourceY + static_cast<std::uint32_t>(bitmap->GetRectangle().height) - 1u;

		NXBasics::BobManager_PrintBobUsingTransparency(bobManager, cornerId + 3u, destination, signedValue(sourceX), signedValue(bottom), cornerPalette);
	}

	// Original function: FUN_004e25ba
	void DrawTool_DrawFrame(NXBasics::CBitmap* bitmap, const NXBasics::SRectangle& rectangle, std::int32_t mode)
	{
		NXBasics::CBitmap frameBitmap(bitmap, rectangle);
		DrawTool_DrawFrame(nullptr, &frameBitmap, &rectangle, mode);
	}

	// Original function: FUN_004e2645.
	NXBasics::CFont* DrawTool_GetFont10Ptr()
	{
		return CGuiBaseDataManager::mStaticVars.resources.font10;
	}

	// Original function: FUN_004e264b; the last color of the white palette.
	const NXBasics::SColorRGB* DrawTool_GetFontColorRef()
	{
		return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 255u * 4u);
	}

	// Original function: FUN_004e2656; palette color at byte offset 0x194.
	const NXBasics::SColorRGB* DrawTool_GetDarkFontColorRef()
	{
		return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette) + NXBasics::PaletteOffset::Colors + 100u * 4u);
	}

	// Remaster replaces native scratch region 0056988C with complete text runs,
	// so the old 100-byte word limit cannot split a UTF-8 sequence.

	// Original function: FUN_004e29d9.
	std::int32_t DrawTool_PrintStringWrapping(NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t width, NXBasics::CFont* font, const NXBasics::SColorRGB* color, const char* text)
	{
		if (text == nullptr) return 0;
		std::string legacy;
		if (std::strchr(text, '{') == nullptr && !NXBasics::Font_TryLegacyText(font, text, legacy))
		{
			const auto available = static_cast<std::int64_t>(width) - x;
			if (available <= 0) return 0;
			const auto wrap = static_cast<std::int32_t>((std::min)(available, std::int64_t{8192}));
			NXBasics::UnicodeFont_Print(font, bitmap, color, x, y, text, wrap);
			return NXBasics::UnicodeFont_Measure(font, text, true, wrap);
		}
		std::uint32_t fontHeight;
		std::memcpy(&fontHeight, reinterpret_cast<const std::byte*>(font) + NXBasics::FontStorageOffset::Height, sizeof(fontHeight));
		const std::uint32_t lineHeight = fontHeight + 2u;
		std::uint32_t currentX = static_cast<std::uint32_t>(x), currentY = static_cast<std::uint32_t>(y);
		std::uint32_t lastHeight = lineHeight;
		const char* cursor = text;
		for (;;)
		{
			const auto character = static_cast<std::uint8_t>(*cursor++);
			if (character == 0) return NXBasics::Runtime_TimeSigned32(lastHeight - static_cast<std::uint32_t>(y) + currentY);
			if (character == '\t' || character == ' ')
			{
				currentX += static_cast<std::uint32_t>(NXBasics::Font_GetCharacterWidth(font, 'i'));
				continue;
			}
			if (character == '\n' || character == '\r')
			{
				if (*cursor == '\n') ++cursor;
				currentY += lastHeight;
				currentX = static_cast<std::uint32_t>(x);
				continue;
			}
			if (character == '{')
			{
				const char* end = std::strchr(cursor, '}');
				if (end == nullptr) continue;
				if (NXBasics::Runtime_CompareStringsIgnoreCaseN(cursor, "newline", 7) == 0)
				{
					currentY += fontHeight + 2u;
					currentX = static_cast<std::uint32_t>(x);
				}
				cursor = end + 1;
				continue;
			}
			if (character < ' ') continue;
			--cursor;
			const char* start = cursor;
			while (static_cast<std::uint8_t>(*cursor) > ' ' && *cursor != '{') ++cursor;
			const std::string word(start, cursor);
			lastHeight = static_cast<std::uint32_t>(NXBasics::Font_GetPixelWidth(font, word.c_str()));
			if (NXBasics::Runtime_TimeSigned32(lastHeight + currentX) >= width)
			{
				currentY += lineHeight;
				currentX = static_cast<std::uint32_t>(x);
			}
			bitmap->Text_Print(font, color, NXBasics::Runtime_TimeSigned32(currentX), NXBasics::Runtime_TimeSigned32(currentY), word.c_str());
			currentX += lastHeight;
			lastHeight = lineHeight;
		}
	}

	// Original function: FUN_004e2b45; callers use the wrapping height retained in EAX.
	std::int32_t __cdecl DrawTool_PrintStringWrappingV(NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t width, NXBasics::CFont* font, const NXBasics::SColorRGB* color, const char* format, ...)
	{
		char text[1000];
		va_list arguments;
		va_start(arguments, format);
		NXBasics::Runtime_FormatString(text, sizeof(text), format, arguments);
		va_end(arguments);
		text[999] = '\0';
		return DrawTool_PrintStringWrapping(bitmap, x, y, width, font, color, text);
	}

	// Original function: FUN_004e2994; terminate the original 1000-byte formatting buffer.
	void __cdecl DrawTool_PrintStringXV(NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, const void* font, const NXBasics::SColorRGB* color, const char* format, ...)
	{
		char text[1000];
		va_list arguments;
		va_start(arguments, format);
		NXBasics::Runtime_FormatString(text, sizeof(text), format, arguments);
		va_end(arguments);
		text[999] = '\0';
		DrawTool_PrintStringX(bitmap, x, y, font, color, text);
	}

	// Original function: FUN_004e2661
	void DrawTool_PrintStringX(NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, const void* font, const NXBasics::SColorRGB* color, const char* text)
	{
		if (text == nullptr)
			return;

		NXBasics::SColorRGB currentColor = *color;
		std::uint32_t currentX = static_cast<std::uint32_t>(x);
		std::uint32_t currentY = static_cast<std::uint32_t>(y);
		const char* cursor = text;

		const auto signedValue = [] (std::uint32_t bits) -> std::int32_t
		{
			return NXBasics::Runtime_TimeSigned32(bits);
		};

		for (;;)
		{
			const std::uint8_t character = static_cast<std::uint8_t>(*cursor++);

			if (character == 0)
				return;

			if (character == '\t' || character == ' ')
			{
				currentX += static_cast<std::uint32_t>(NXBasics::Font_GetCharacterWidth(font, 'i'));
				continue;
			}

			if (character == '\n' || character == '\r')
			{
				// The original consumes a following LF after either newline byte.
				if (*cursor == '\n')
					++cursor;

				std::uint32_t fontHeight;
				std::memcpy(&fontHeight, static_cast<const std::byte*>(font) + NXBasics::FontStorageOffset::Height, sizeof(fontHeight));

				currentY += fontHeight + 2u;
				currentX = static_cast<std::uint32_t>(x);
				continue;
			}

			if (character == '{')
			{
				const char* end = std::strchr(cursor, '}');

				// Without a closing brace, only the opening brace is discarded.
				if (end == nullptr)
					continue;

				const auto matches = [cursor] (const char* name, std::uint32_t length) -> bool
				{
					return NXBasics::Runtime_CompareStringsIgnoreCaseN(cursor, name, length) == 0;
				};

				const auto argumentAfter = [end] (const char* start, char delimiter) -> const char*
				{
					const char* separator = std::strchr(start, delimiter);

					if (separator == nullptr || separator >= end)
						return nullptr;

					return separator + 1;
				};

				if (matches("gfxgood", 7))
				{
					const char* first = argumentAfter(cursor, ':');

					if (first != nullptr)
					{
						const std::int32_t goodId = NXBasics::Runtime_Atoi(first);
						const char* second = argumentAfter(first, ',');

						if (second != nullptr)
						{
							const std::int32_t variant = NXBasics::Runtime_Atoi(second);
							const std::uint32_t baseAddress = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(NC2Logic::gStaticGoodTypeData));
							const std::uint32_t address = baseAddress + static_cast<std::uint32_t>(goodId) * 0xD0u;
							const auto* good = reinterpret_cast<const std::byte*>(static_cast<std::uintptr_t>(address));

							std::uint32_t graphicId;
							std::memcpy(&graphicId, good + 0x24, sizeof(graphicId));

							if (graphicId != NXBasics::IntegerRange::UInt32Maximum)
							{
								const std::uint32_t frame = ::timeGetTime() / 83u;
								NC2E2::EngineGraphic_Draw(graphicId, static_cast<std::uint32_t>(variant), bitmap, signedValue(currentX + 12u), signedValue(currentY + 8u), frame, 256, -1, true);
								currentX += 24u;
							}
						}
					}
				}
				else if (matches("gfxhouse", 8))
				{
					const char* first = argumentAfter(cursor, ':');

					if (first != nullptr)
					{
						const std::int32_t houseId = NXBasics::Runtime_Atoi(first);
						const char* second = argumentAfter(first, ',');

						if (second != nullptr)
						{
							const std::int32_t variant = NXBasics::Runtime_Atoi(second);
							NC2E2::DrawAtom_House_Tool_DoDrawEngineHouseType(houseId, static_cast<std::uint32_t>(variant), bitmap, signedValue(currentX), signedValue(currentY), -1, true, false);
						}
					}
				}
				else if (matches("setx", 4))
				{
					const char* argument = argumentAfter(cursor, ':');

					if (argument != nullptr)
						currentX = static_cast<std::uint32_t>(x) + static_cast<std::uint32_t>(NXBasics::Runtime_Atoi(argument));
				}
				else if (matches("sety", 4))
				{
					const char* argument = argumentAfter(cursor, ':');

					if (argument != nullptr)
						currentY = static_cast<std::uint32_t>(y) + static_cast<std::uint32_t>(NXBasics::Runtime_Atoi(argument));
				}
				else if (matches("setfontcolor", 12))
				{
					const char* first = argumentAfter(cursor, ':');

					if (first != nullptr)
					{
						const std::int32_t red = NXBasics::Runtime_Atoi(first);
						const char* second = argumentAfter(first, ',');

						if (second != nullptr)
						{
							const std::int32_t green = NXBasics::Runtime_Atoi(second);
							const char* third = argumentAfter(second, ',');

							if (third != nullptr)
							{
								const std::int32_t blue = NXBasics::Runtime_Atoi(third);
								currentColor.red = static_cast<std::uint8_t>(red);
								currentColor.green = static_cast<std::uint8_t>(green);
								currentColor.blue = static_cast<std::uint8_t>(blue);
							}
						}
					}
				}

				cursor = end + 1;
				continue;
			}

			if (character < 0x20u)
				continue;

			// Shape a complete text run, preserving joins, combining marks and bidi.
			const char* start = cursor - 1;
			while (*cursor != 0 && *cursor != '{' && *cursor != '\r' && *cursor != '\n') ++cursor;
			const std::string run(start, cursor);
			const std::int32_t width = NXBasics::Font_GetPixelWidth(font, run.c_str());
			std::string legacy;
			if (NXBasics::Font_TryLegacyText(font, run.c_str(), legacy))
			{
				for (const auto byte : legacy)
				{
					const auto value = static_cast<std::uint8_t>(byte);
					if (value == ' ' || value == '\t') currentX += static_cast<std::uint32_t>(NXBasics::Font_GetCharacterWidth(font, 'i'));
					else if (value >= 0x20)
					{
						NXBasics::Font_PrintCharacter(font, &currentColor, bitmap, value, signedValue(currentX), signedValue(currentY));
						currentX += static_cast<std::uint32_t>(NXBasics::Font_GetCharacterWidth(font, value));
					}
				}
			}
			else
			{
				NXBasics::UnicodeFont_Print(font, bitmap, &currentColor, signedValue(currentX), signedValue(currentY), run.c_str());
				currentX += static_cast<std::uint32_t>(width);
			}
		}
	}

	// Original function: FUN_004e2b8d.
	void FXTool_ClickConfirm()
	{
		auto* manager = NDSManager::CManager::sTheObjectPtr;
		if (manager != nullptr) manager->FireAndForget_LoadAndPlay("data\\engine2d\\bin\\sounds\\gui\\click_confirm.wav", 100u, NXBasics::IntegerRange::UInt32Maximum, 1u, false, false, 100u);
	}

	// Original function: FUN_004e2bae; original failure feedback sound.
	void FXTool_Fail()
	{
		auto* manager = NDSManager::CManager::sTheObjectPtr;
		if (manager != nullptr) manager->FireAndForget_LoadAndPlay("data\\engine2d\\bin\\sounds\\gui\\click_fail.wav", 100u, NXBasics::IntegerRange::UInt32Maximum, 1u, false, false, 100u);
	}

	// Original function: FUN_004e2bcf
	void FXTool_Chat()
	{
		auto* manager = NDSManager::CManager::sTheObjectPtr;

		if (manager != nullptr)
		{
			manager->FireAndForget_LoadAndPlay("data\\engine2d\\bin\\sounds\\gui\\chat_incoming.wav", 100u, NXBasics::IntegerRange::UInt32Maximum, 1u, false, false, 100u);
		}
	}

	// Original function: FUN_004e1d39
	CGuiBaseStringsManager::CGuiBaseStringsManager()
	{
		// Original pointer table: 0050BBBC for Weltwunder, 0050BD74 for Die Saga.
		// Die Saga appends the saga table as entry thirteen.
		static constexpr const char* fileNamePatterns[kGuiStringTableCapacity] =
		{
			"data\\text\\%s\\strings\\mainmenu\\mainmenu.ini",
			"data\\text\\%s\\strings\\ingamegui\\ingameguimain.ini",
			"data\\text\\%s\\strings\\ingamegui\\ingameguimisc.ini",
			"data\\text\\%s\\strings\\ingamegui\\ingameguihumanlistwindow.ini",
			"data\\text\\%s\\strings\\ingamegui\\ingameguimiscwindow.ini",
			"data\\text\\%s\\strings\\ingamegui\\ingameguimisclogic.ini",
			"data\\text\\%s\\strings\\ingamegui\\ingameguimessages.ini",
			"data\\text\\%s\\strings\\ingamegui\\ingameguihumanwindow.ini",
			"data\\text\\%s\\strings\\ingamegui\\ingameguihousewindow.ini",
			"data\\text\\%s\\strings\\ingamegui\\ingameguivehiclewindow.ini",
			"data\\text\\%s\\strings\\updates\\update103.ini",
			"data\\text\\%s\\strings\\odin\\odin001.ini",
			"data\\text\\%s\\strings\\wonders\\wonders001.ini",
			"data\\text\\%s\\strings\\saga\\saga001.ini"
		};

		sTheObjectPtr = this;
		mState = 0;
		mStaticVars = { };

		char fileName[260];

		for (std::size_t index = 0; index < NRemaster::GetGuiStringTableCount(); ++index)
		{
			NC2Logic::LanguageTool_GetFilename(fileNamePatterns[index], fileName, true);
			mStaticVars.stringArrays[index] = NC2Logic::StringIO_CreateOutOfIniFile(fileName);
		}
	}

	// Original function: FUN_004e1da1
	CGuiBaseStringsManager::~CGuiBaseStringsManager()
	{
		// Clears the whole capacity so a later variant selection cannot keep stale handles.
		for (std::size_t index = 0; index < kGuiStringTableCapacity; ++index)
		{
			DestroyGuiStorableResource(mStaticVars.stringArrays[index]);
			mStaticVars.stringArrays[index] = nullptr;
		}

		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_004e1dcb
	char* CGuiBaseStringsManager::GetStringPtr(std::int32_t arrayId, std::uint32_t stringId)
	{
		// Remaster: an unavailable optional table uses the existing diagnostic text.
		// The native bound is the per-game table count (Die Saga 004E15B3 rejects a1 > 0xD).
		if (arrayId >= 0 && arrayId < static_cast<std::int32_t>(NRemaster::GetGuiStringTableCount()) && mStaticVars.stringArrays[arrayId] != nullptr)
		{
			char* text = mStaticVars.stringArrays[arrayId]->GetStringPtr(stringId);

			if (text != nullptr)
				return text;
		}

		NXBasics::Runtime_PrintString(mStaticVars.undefinedString, "<%d:%d:NOT DEFINED!>", arrayId, NXBasics::Runtime_TimeSigned32(stringId));
		return mStaticVars.undefinedString;
	}

	// Original function: FUN_004E1E09; retain the signed table identifier for the native fallback.
	char* StringTool_GetStringPtr(std::int32_t arrayId, std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(arrayId, stringId);
	}

	// Original function: FUN_004E1E6D.
	char* StringTool_GetGameGuiHumanSelectedWindowStringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(7, stringId);
	}

	// Original function: FUN_004E1E7B.
	char* StringTool_GetGameGuiHouseSelectedWindowStringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(8, stringId);
	}

	// Original function: FUN_004E1E89.
	char* StringTool_GetGameGuiVehicleSelectedWindowStringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(9, stringId);
	}

	// Original function: FUN_004E1EB3.
	char* StringTool_GetWonders001StringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(12, stringId);
	}

	// Original function: FUN_004e1e19
	char* StringTool_GetMainMenuStringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(0, stringId);
	}

	// Original function: FUN_004e1e27.
	char* StringTool_GetGameGuiMainStringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(1, stringId);
	}

	// Original function: FUN_004e1ec1; complete Windows jump-table region 004E1FD8..004E20B7.
	const char* StringTool_GetLogicUserCommandsForHumanTypeNamePtr(std::uint32_t type)
	{
		constexpr std::uint8_t strings[56]{ 0,49,1,4,6,8,10,14,15,16,17,18,31,32,49,49,23,24,25,26,27,28,29,30,19,20,21,22,33,34,35,36,37,49,49,38,39,40,41,42,43,44,45,46,47,48,49,49,2,3,5,7,9,11,13,12 };
		return CGuiBaseStringsManager::GetStringPtr(5, type < 56 ? strings[type] : 49);
	}
	// Original function: FUN_004e20b8; complete sixteen-entry Windows table 004E2114..004E2153.
	const char* StringTool_GetLogicUserCommandsForHouseTypeNamePtr(std::uint32_t type)
	{
		return CGuiBaseStringsManager::GetStringPtr(5, type <= 15 ? type + 100u : 120u);
	}
	// Original function: FUN_004e2154; complete 15-entry Windows table 004E21DC..004E2217.
	const char* StringTool_GetLogicUserCommandsForVehicleTypeNamePtr(std::uint32_t type)
	{
		return CGuiBaseStringsManager::GetStringPtr(5, type <= 14 ? type + 140 : 155);
	}

	// Original function: FUN_004e231c; fallback literal verified at 00508354.
	const char* StringTool_GetLogicGoodEquipmentTypeNamePtr(std::uint32_t type)
	{
		return type <= 6 ? CGuiBaseStringsManager::GetStringPtr(5, type + 220u) : "<UNKNOWN>";
	}

	// Original function: FUN_004e1e5f.
	char* StringTool_GetGameGuiMessagesStringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(6, stringId);
	}

	// Original function: FUN_004e2218.
	void StringTool_GetLogicSpecialItemNamePtr(std::uint32_t type, std::int32_t item, char* output)
	{
		output[0] = '\0';
		if (type == 1 || type == 2)
		{
			std::strcpy(output, CGuiBaseStringsManager::GetStringPtr(5, type + 179u)); return;
		}
		if (type < 3 || type > 7) return;
		const char* format = CGuiBaseStringsManager::GetStringPtr(5, type + 179u);
		const char* name = type == 7 ? NC2Logic::StringAccess_GetGoodNamePtr(static_cast<std::uint32_t>(item), 1) : type == 6 ? NC2Logic::StringAccess_GetJobNamePtr(static_cast<std::uint32_t>(item), 1) : NC2Logic::StringAccess_GetHouseNamePtr(static_cast<std::uint32_t>(item), 1);
		NXBasics::Runtime_PrintString(output, format, name);
	}

	// Original function: FUN_004e22d8. Literal states are verified in Game.exe.
	const char* StringTool_GetLogicDiplomacyStateNamePtr(std::uint32_t state)
	{
		if (state == 0) return "<NONE>";
		if (state == 4) return "<SELF>";
		return state <= 3 ? CGuiBaseStringsManager::GetStringPtr(5, state + 199u) : "";
	}

	// Original function: FUN_004e1e43.
	char* StringTool_GetGameGuiHumanListWindowStringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(3, stringId);
	}

	// Original function: FUN_004e1e51.
	char* StringTool_GetGameGuiMiscWindowStringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(4, stringId);
	}

	// Original function: FUN_004e1e35
	char* StringTool_GetMiscMenuStringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(2, stringId);
	}

	// Original function: FUN_004e1e97
	char* StringTool_GetUpdate103StringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(10, stringId);
	}

	// Original function: FUN_004e1ea5
	char* StringTool_GetOdin001StringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(11, stringId);
	}

	// Original function: FUN_004E169E (Die Saga); table 13 added on top of the Weltwunder set.
	char* StringTool_GetSaga001StringPtr(std::uint32_t stringId)
	{
		return CGuiBaseStringsManager::GetStringPtr(13, stringId);
	}

	// Original function: FUN_004e2d10.
	CGuiBaseDataManager::CGuiBaseDataManager()
	{
		sTheObjectPtr = this;
		mState = 0;
		mStaticVars = { };

		auto& resources = mStaticVars.resources;

		resources.font08 = static_cast<NXBasics::CFont*>(NXBasics::XB_Storable_LoadObject("data\\gui\\fonts\\font08.fnt"));
		resources.font10 = static_cast<NXBasics::CFont*>(NXBasics::XB_Storable_LoadObject("data\\gui\\fonts\\font10.fnt"));
		resources.font12 = static_cast<NXBasics::CFont*>(NXBasics::XB_Storable_LoadObject("data\\gui\\fonts\\font12.fnt"));
		resources.fontDebug = static_cast<NXBasics::CFont*>(NXBasics::XB_Storable_LoadObject("data\\gui\\fonts\\fontdebug.fnt"));

		const auto setCharacterSpacing = [] (NXBasics::CFont* font, std::int32_t spacing)
		{
			std::memcpy(reinterpret_cast<std::byte*>(font) + NXBasics::FontStorageOffset::CharacterSpacing, &spacing, sizeof(spacing));
		};

		setCharacterSpacing(resources.font10, -1);

		if (NMasterPropertyManager::CPropertyManager::sTheObjectPtr->Property_DoesExists("font_space_small"))
		{
			setCharacterSpacing(resources.font08, -1);
			setCharacterSpacing(resources.font10, -2);
			setCharacterSpacing(resources.font12, -2);
		}

		resources.fontWhitePalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\font_white.pcx"));
		resources.fontDimmedPalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\font_dimmed.pcx"));
		resources.fontRedPalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\font_red.pcx"));
		resources.fontDarkPalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\font_dark.pcx"));

		NXBasics::Font_SetPalettePtr(resources.font08, resources.fontWhitePalette);
		NXBasics::Font_SetPalettePtr(resources.font10, resources.fontWhitePalette);
		NXBasics::Font_SetPalettePtr(resources.font12, resources.fontWhitePalette);
		NXBasics::Font_SetPalettePtr(resources.fontDebug, resources.fontWhitePalette);

		char fileName[260];
		NC2Logic::LanguageTool_GetFilename("data\\gui\\lang\\%s\\bobs\\ls_gui_window.bmd", fileName, false);

		resources.windowBobManager = static_cast<NXBasics::CBobManager*>(NXBasics::XB_Storable_LoadObject(fileName));

		resources.framePalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\frame.pcx"));
		resources.backgroundPalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\bg_normal.pcx"));
		resources.papyrusPalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\papyrus.pcx"));
		resources.hitpointsBarPalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\bar_hitpoints.pcx"));
		resources.standardBarPalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\bar_standart.pcx"));
		resources.disabledBarPalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\bar_disabled.pcx"));
		resources.iconsLeftPalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\IconsLeft.pcx"));
		resources.contextPalette = static_cast<NXBasics::CPalette*>(NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\gui\\palettes\\Context.pcx"));
	}

	// Original function: FUN_004e2edf.
	CGuiBaseDataManager::~CGuiBaseDataManager()
	{
		DynamicData_Free();

		auto& resources = mStaticVars.resources;

		DestroyGuiStorableResource(resources.contextPalette);
		DestroyGuiStorableResource(resources.iconsLeftPalette);
		DestroyGuiStorableResource(resources.disabledBarPalette);
		DestroyGuiStorableResource(resources.standardBarPalette);
		DestroyGuiStorableResource(resources.hitpointsBarPalette);
		DestroyGuiStorableResource(resources.papyrusPalette);
		DestroyGuiStorableResource(resources.backgroundPalette);
		DestroyGuiStorableResource(resources.framePalette);
		DestroyGuiStorableResource(resources.windowBobManager);
		DestroyGuiStorableResource(resources.fontDarkPalette);
		DestroyGuiStorableResource(resources.fontRedPalette);
		DestroyGuiStorableResource(resources.fontDimmedPalette);
		DestroyGuiStorableResource(resources.fontWhitePalette);
		DestroyGuiStorableResource(resources.fontDebug);
		DestroyGuiStorableResource(resources.font12);
		DestroyGuiStorableResource(resources.font10);
		DestroyGuiStorableResource(resources.font08);

		// The original clears only the permanent 0x44-byte resource block here.
		resources = { };
		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_004e3012.
	void CGuiBaseDataManager::DynamicData_Load()
	{
		DynamicData_Free();

		auto* desktopBitmap = NXBaseGui::CDesktop::sTheObjectPtr->GetDisplayBitmap();

		std::int32_t depth;
		std::memcpy(&depth, reinterpret_cast<const std::byte*>(desktopBitmap) + 0x04, sizeof(depth));

		const auto loadBitmap = [depth] (const char* fileName, NXBasics::CBitmap*& destination)
		{
			NXBasics::CPicture picture(fileName);

			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::uint32_t>(sizeof(NXBasics::CBitmap)), 1);

			if (allocation != nullptr)
			{
				NXBasics::CBitmap* sourceBitmap = picture.GetBitmap();
				destination = ::new (allocation) NXBasics::CBitmap(sourceBitmap->GetRectangle().width, sourceBitmap->GetRectangle().height, depth);
			}
			else
			{
				destination = nullptr;
			}

			// Reload the picture's bitmap after allocation and construction.
			NXBasics::CBitmap* sourceBitmap = picture.GetBitmap();
			sourceBitmap->CopyIntoBitmap(destination, 0, 0);
		};

		auto& dynamic = mStaticVars.dynamic;

		loadBitmap("data\\gui\\bitmaps\\bg.pcx", dynamic.background);
		loadBitmap("data\\gui\\bitmaps\\bg_button.pcx", dynamic.buttonBackground);
		loadBitmap("data\\gui\\bitmaps\\bg_selected.pcx", dynamic.selectedBackground);
		loadBitmap("data\\gui\\bitmaps\\bg_button_hilite.pcx", dynamic.highlightedButton);
		loadBitmap("data\\gui\\bitmaps\\bg_headline.pcx", dynamic.headlineBackground);

		dynamic.loaded = 1;
	}

	// Original function: FUN_004e3195.
	void CGuiBaseDataManager::DynamicData_Free()
	{
		auto& resources = mStaticVars.resources;

		NXBasics::Palette_PaletteChanged(resources.fontWhitePalette);
		NXBasics::Palette_PaletteChanged(resources.fontDimmedPalette);
		NXBasics::Palette_PaletteChanged(resources.fontRedPalette);
		NXBasics::Palette_PaletteChanged(resources.fontDarkPalette);
		NXBasics::Palette_PaletteChanged(resources.framePalette);
		NXBasics::Palette_PaletteChanged(resources.backgroundPalette);
		NXBasics::Palette_PaletteChanged(resources.papyrusPalette);

		auto& dynamic = mStaticVars.dynamic;

		if (dynamic.loaded == 0)
		{
			return;
		}

		DestroyGuiStorableResource(dynamic.headlineBackground);
		DestroyGuiStorableResource(dynamic.highlightedButton);
		DestroyGuiStorableResource(dynamic.selectedBackground);
		DestroyGuiStorableResource(dynamic.buttonBackground);
		DestroyGuiStorableResource(dynamic.background);

		dynamic = { };
	}
}
