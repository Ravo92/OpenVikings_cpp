#include "C2E2StaticDataPalettes.h"
#include "GameProfile.h"
#include "SwitchIdentifiers.h"
#include "XBasicTypes.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"
#include "XPicture.h"
#include "XPictureAccess.h"

#include <cstring>
#include <new>

namespace
{
	// Complete Windows token arrays 004F45C0..004F45E7 and 004F45E8..004F45FF.
	const NXBasics::SToken gPalette256Tokens[] = {
		{ "editname", SwitchIds::Palette16Token::EditName }, { "gfxfile", SwitchIds::PaletteToken::Gfxfile }, { "gfxpreshade", SwitchIds::PaletteToken::GfxPreshade }, { "gfxremaptopreshaded", SwitchIds::PaletteToken::GfxRemapToPreshaded }, { nullptr, 0 }
	};
	const NXBasics::SToken gPalette16Tokens[] = {
		{ "editname", SwitchIds::Palette16Token::EditName }, { "gfxcolorrange", SwitchIds::Palette16Token::GfxColorRange }, { nullptr, 0 }
	};

	// Complete Windows token-list region 00555100..0055510B.
	struct SPaletteTokenState
	{
		const NXBasics::SToken* table16;
		std::uint32_t flags;
		const NXBasics::SToken* table256;
	};

	SPaletteTokenState gPaletteTokenState{ };
	static_assert(sizeof(gPalette256Tokens) == 5 * sizeof(NXBasics::SToken));
	static_assert(sizeof(gPalette16Tokens) == 3 * sizeof(NXBasics::SToken));
	static_assert(sizeof(SPaletteTokenState) == (sizeof(void*) == 8 ? 24 : 12));
	static_assert(offsetof(SPaletteTokenState, flags) == sizeof(void*));
	static_assert(offsetof(SPaletteTokenState, table256) == 2 * sizeof(void*));

	// Original function: FUN_00480e5a; empty CTokenList exit callback.
	int __cdecl DestroyPalette16TokenList()
	{

		return 0;
	}

	// Original function: FUN_00480e5b; empty CTokenList exit callback.
	int __cdecl DestroyPalette256TokenList()
	{

		return 0;
	}
}

namespace NC2E2
{
	// Original function: FUN_00480a20; CE2StaticDataPalettesManager constructor.
	void* __fastcall E2StaticDataPalettesManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gE2StaticDataPalettesManager = manager;
		std::memset(manager, 0, 1);
		std::memset(&gStaticPalettes, 0, sizeof(gStaticPalettes));
		NXBasics::CIniFileReader reader("data\\engine2d\\inis\\palettes\\palettes.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
			return manager;

		gStaticPalettes.palette256Count = NXBasics::Runtime_TimeSigned32(reader.GetNumberOfGroups("GfxPalette256"));

		if (gStaticPalettes.palette256Count != 0)
		{
			const auto recordCount = static_cast<std::uint32_t>(gStaticPalettes.palette256Count);
			const std::size_t bytes = static_cast<std::size_t>(recordCount) * sizeof(SE2StaticDataPalette256);
			gStaticPalettes.palettes256 = static_cast<SE2StaticDataPalette256*>(NXBasics::Runtime_AllocateArrayWithNewHandler(recordCount, sizeof(SE2StaticDataPalette256)));
			std::memset(gStaticPalettes.palettes256, 0, bytes);
			auto* entry = gStaticPalettes.palettes256;

			for (char* group = reader.GetFirstGroup("GfxPalette256"); group != nullptr; group = reader.GetNextGroup("GfxPalette256"), ++entry)
			{
				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					if ((gPaletteTokenState.flags & 1u) == 0)
					{
						gPaletteTokenState.flags |= 1u;
						gPaletteTokenState.table256 = gPalette256Tokens;
						NXBasics::Runtime_AtExit(DestroyPalette256TokenList);
					}

					NXBasics::CTokenList tokens(gPaletteTokenState.table256);

					switch (tokens.FindToken(reader.GetCurrentName()))
					{
						case SwitchIds::PaletteToken::EditName:
							std::strncpy(entry->name, reader.GetString(), 29);
							break;
						case SwitchIds::PaletteToken::Gfxfile:
						{
							NXBasics::CPicture picture(reader.GetString());
							entry->palette = picture.ExtractPalette();
							break;
						}
						case SwitchIds::PaletteToken::GfxPreshade:
						{
							if (entry->palette == nullptr)
								break;

							void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NXBasics::PaletteArrayCookieSize + 31 * sizeof(NXBasics::CPalette), 1);
							NXBasics::CPalette* palettes = nullptr;

							if (allocation != nullptr)
							{
								const std::uint32_t count = 31;
								std::memcpy(allocation, &count, sizeof(count));
								palettes = reinterpret_cast<NXBasics::CPalette*>(static_cast<std::byte*>(allocation) + NXBasics::PaletteArrayCookieSize);

								for (std::uint32_t index = 0; index < count; ++index)
								{
									new (&palettes[index]) NXBasics::CPalette();
								}
							}

							entry->preshadedPalettes = palettes;
							// The original also enters this loop after a failed array allocation.

							for (std::uint32_t brightness = 8, index = 0; brightness < 256; brightness += 8, ++index)
							{
								entry->preshadedPalettes[index] = *entry->palette;

								if (brightness != 128)
								{
									const float factor = static_cast<float>(brightness) * 0.0078125f;
									NXBasics::SColorModifier modifier(false, factor, factor, factor);
									entry->preshadedPalettes[index].Modify(modifier);
								}
							}

							break;
						}
						case SwitchIds::PaletteToken::GfxRemapToPreshaded:
						{
							if (entry->palette == nullptr)
								break;

							const std::int32_t id = E2StaticDataPalettesManager_GetPalette256Id(reader.GetString());

							if (id == -1)
								break;

							auto* target = &gStaticPalettes.palettes256[id];

							if (target->palette == nullptr || target->preshadedPalettes == nullptr)
								break;

							void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CRemapTable), 1);
							entry->remapTable = allocation == nullptr ? nullptr : new (allocation) NXBasics::CRemapTable(*entry->palette, *target->palette, 0, 255);
							entry->remapTarget = target;
							break;
						}
					}
				}
			}
		}

		gStaticPalettes.palette16Count = NXBasics::Runtime_TimeSigned32(reader.GetNumberOfGroups("GfxPalette16"));

		if (gStaticPalettes.palette16Count != 0)
		{
			const auto recordCount = static_cast<std::uint32_t>(gStaticPalettes.palette16Count);
			const std::size_t bytes = static_cast<std::size_t>(recordCount) * sizeof(SE2StaticDataPalette16);
			gStaticPalettes.palettes16 = static_cast<SE2StaticDataPalette16*>(NXBasics::Runtime_AllocateArrayWithNewHandler(recordCount, sizeof(SE2StaticDataPalette16)));
			std::memset(gStaticPalettes.palettes16, 0, bytes);
			auto* entry = gStaticPalettes.palettes16;

			for (char* group = reader.GetFirstGroup("GfxPalette16"); group != nullptr; group = reader.GetNextGroup("GfxPalette16"), ++entry)
			{
				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					if ((gPaletteTokenState.flags & 2u) == 0)
					{
						gPaletteTokenState.flags |= 2u;
						gPaletteTokenState.table16 = gPalette16Tokens;
						NXBasics::Runtime_AtExit(DestroyPalette16TokenList);
					}

					NXBasics::CTokenList tokens(gPaletteTokenState.table16);

					switch (tokens.FindToken(reader.GetCurrentName()))
					{
						case SwitchIds::Palette16Token::EditName:
							std::strncpy(entry->name, reader.GetString(), 29);
							break;
						case SwitchIds::Palette16Token::GfxColorRange:
						{
							entry->palette256Id = E2StaticDataPalettesManager_GetPalette256Id(reader.GetString());

							if (entry->palette256Id == -1)
								break;

							const std::uint32_t range = static_cast<std::uint32_t>(reader.GetInt());

							if (range < 16)
								entry->colorRange = range;

							break;
						}
					}
				}
			}
		}

		gStaticPalettes.fishPaletteId = E2StaticDataPalettesManager_GetPalette256Id("fishes");
		gStaticPalettes.guiBubblesPaletteId = E2StaticDataPalettesManager_GetPalette256Id("gui_bubbles");
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CPalette), 1);
		gStaticPalettes.blackPalette = allocation == nullptr ? nullptr : new (allocation) NXBasics::CPalette();
		// Cultures2.exe 00473DB0..00473E07 ends after the black palette;
		// only EightWonders 00480E39 adds the vertex-color palette load.

		if (NRemaster::UsesVertexColors())
			gStaticPalettes.vertexColorsPalette = NXBasics::XB_PictureTool_LoadPaletteOutOfPicture("data\\engine2d\\bin\\palettes\\misc\\vertexcolors.pcx");

		return manager;
	}

	// Original function: FUN_00480f6d; CE2StaticDataPalettesManager::GetPalette256Id.
	std::int32_t E2StaticDataPalettesManager_GetPalette256Id(const char* name)
	{
		for (std::int32_t index = 0; index < gStaticPalettes.palette256Count; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(gStaticPalettes.palettes256[index].name, name) == 0)
				return index;
		}

		return -1;
	}

	// Original function: FUN_00480fa8; CE2StaticDataPalettesManager::GetPalette16Id.
	std::int32_t E2StaticDataPalettesManager_GetPalette16Id(const char* name)
	{
		for (std::int32_t index = 0; index < gStaticPalettes.palette16Count; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(gStaticPalettes.palettes16[index].name, name) == 0)
				return index;
		}

		return -1;
	}
}