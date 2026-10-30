#include "ApplicationGlobals.h"
#include "C2E2LevelOfDetailManager.h"
#include "C2E2StaticDataTerrain.h"
#include "C2EngineDrawingAccess.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "XBasicTypes.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XNumericConstants.h"

#include <windows.h>

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace NC2E2
{
	// Original function: FUN_0048b4e6; ColorIds_GetPlayerShipPalettePtr.
	NXBasics::CPalette* EnginePalette_GetPlayerShipPalette(std::int32_t playerId)
	{
		using NC2Lifetime::Read;

		if (playerId < 0 || playerId >= 20)
			return nullptr;

		const auto* player = static_cast<const std::byte*>(NC2Logic::gGameControl) + static_cast<std::uint32_t>(playerId) * NC2Layout::RecordSize::GameControlPlayer;

		if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0)
			return nullptr;

		auto color = Read<std::int32_t>(player, NC2Layout::GameControlPlayerOffset::ColorId);

		if (color < 0 || color >= 10)
			color = 9;

		const auto paletteId = Read<std::uint32_t>(gStaticHumanColors + color * NC2Layout::RecordSize::StaticHumanColorUnsigned, NC2Layout::StaticHumanColorOffset::ShipPaletteId);

		return gStaticPalettes.palettes256[paletteId].palette;
	}

	// Original function: FUN_0048b532.
	std::int32_t StaticHumanMisc_InhabitantSign_GetLandscapeId(std::int32_t playerId, std::uint32_t sign)
	{
		using NC2Lifetime::Read;

		if (playerId < 0 || playerId >= 20)
			return -1;

		const auto* player = static_cast<const std::byte*>(NC2Logic::gGameControl) + static_cast<std::uint32_t>(playerId) * NC2Layout::RecordSize::GameControlPlayer;

		if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0)
			return -1;

		auto color = Read<std::int32_t>(player, NC2Layout::GameControlPlayerOffset::ColorId);

		if (color < 0 || color >= 10)
			color = 9;

		return Read<std::int32_t>(gStaticHumanColors + static_cast<std::uint32_t>(color) * NC2Layout::RecordSize::StaticHumanColorUnsigned, NC2Layout::StaticHumanColorOffset::InhabitantSigns + sign * NC2Layout::RecordSize::HumanSignGraphicsIdUnsigned);
	}

	// Original function: FUN_0048b574.
	std::int32_t StaticHumanMisc_SoldierSign_GetLandscapeId(std::int32_t playerId, std::uint32_t count)
	{
		using NC2Lifetime::Read;

		if (count == 0 || playerId >= 20)
			return -1;

		const auto* player = static_cast<const std::byte*>(NC2Logic::gGameControl) + static_cast<std::uint32_t>(playerId) * NC2Layout::RecordSize::GameControlPlayer;

		if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0)
			return -1;

		auto color = Read<std::int32_t>(player, NC2Layout::GameControlPlayerOffset::ColorId);

		if (color < 0 || color >= 10)
			color = 9;

		auto index = count - 1u;

		if (index >= 5)
			index = 4;

		return Read<std::int32_t>(gStaticHumanColors + static_cast<std::uint32_t>(color) * NC2Layout::RecordSize::StaticHumanColorUnsigned, NC2Layout::StaticHumanColorOffset::SoldierSignsUnsigned + index * NC2Layout::RecordSize::HumanSignGraphicsIdUnsigned);
	}

	// Original function: FUN_00486f3c
	NXBasics::CPalette* EnginePalette_GetBrightnessPalette(const void* colorData, std::int32_t brightness)
	{
		const std::uint32_t differenceBits = static_cast<std::uint32_t>(brightness) - 128u;
		std::int32_t index = NXBasics::Runtime_TimeSigned32(differenceBits) / 8 + 15;

		if (index < 0)
		{
			index = 0;
		}
		else if (index > 30)
		{
			index = 30;
		}

		// Remaster: all brightness values must stay within the 31 allocated palettes.

		return static_cast<const SE2StaticDataPalette256*>(colorData)->preshadedPalettes + index;
	}

	// Original function: FUN_0048b49a
	NXBasics::CPalette* EnginePalette_GetPlayerPalette(std::int32_t playerId)
	{
		if (playerId < 0 || playerId >= 20)
		{
			return nullptr;
		}

		const auto addressAt = [](const void* base, std::size_t offset) -> const std::byte*
		{
			const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(base) + offset;

			return reinterpret_cast<const std::byte*>(static_cast<std::uintptr_t>(address));
		};

		const std::byte* player = addressAt(NC2Logic::gGameControl, static_cast<std::uint32_t>(playerId) * NC2Layout::RecordSize::GameControlPlayer);

		std::uint8_t active;
		std::memcpy(&active, player, sizeof(active));

		if (active == 0)
		{
			return nullptr;
		}

		std::int32_t colorId;
		std::memcpy(&colorId, addressAt(player, 16u), sizeof(colorId));

		if (colorId < 0 || colorId >= 10)
		{
			colorId = 9;
		}

		std::uint32_t paletteId;
		std::memcpy(&paletteId, gStaticHumanColors + static_cast<std::uint32_t>(colorId) * NC2Layout::RecordSize::StaticHumanColorUnsigned + NC2Layout::StaticHumanColorOffset::PlayerPaletteIdUnsigned, sizeof(paletteId));

		return gStaticPalettes.palettes256[paletteId].palette;
	}

	// Original function: FUN_00496594
	void ls_DrawAtom_House_DoDraw_PrintImageBob(const void* bobManager, std::uint32_t bobId, std::uint32_t colorTableIndex, std::int32_t brightness, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, bool /*drawFlag*/)
	{
		if (bobManager == nullptr)
			return;

		auto* colorData = &gStaticPalettes.palettes256[colorTableIndex];

		if (colorData->preshadedPalettes == nullptr)
		{

			return;
		}

		auto* palette = EnginePalette_GetBrightnessPalette(colorData, brightness / 2);

		if (NXBasics::BobManager_GetBobType(bobManager, bobId) == 1u)
		{
			NXBasics::BobManager_PrintBob(bobManager, bobId, bitmap, x, y, palette);
		}
		else if (NXBasics::BobManager_GetBobType(bobManager, bobId) == 4u)
		{
			NXBasics::BobManager_PrintBob_DoubleByteBobNormal(bobManager, bobId, bitmap, x, y, palette);
		}
	}

	// Original function: FUN_004966da
	void DrawAtom_House_Tool_DoDrawBob(std::int32_t houseId, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness, std::uint32_t bobId)
	{
		const auto* house = NC2Logic::CHouseArray::mStaticVars + static_cast<std::uint32_t>(houseId) * NC2Logic::CHouseArray::RuntimeElementSize;
		NC2Logic::House_GetHouseTypeDataPtr(house);
		const auto houseType = NC2Lifetime::Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType);
		const auto* graphic = static_cast<const std::byte*>(gStaticHouseEntries) + houseType * NC2Layout::RecordSize::EngineHouseType;
		const auto variant = NC2Lifetime::Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) % NC2Lifetime::Read<std::uint32_t>(graphic, NC2Layout::EngineHouseTypeOffset::PaletteCount);
		ls_DrawAtom_House_DoDraw_PrintImageBob(NC2Lifetime::Read<void*>(graphic, NC2Layout::EngineHouseTypeOffset::BobManager), bobId, NC2Lifetime::Read<std::uint32_t>(graphic, NC2Layout::EngineHouseTypeOffset::PaletteIdsUnsigned + variant * NC2Layout::RecordSize::HouseStageValueUnsigned), brightness, bitmap, x, y, false);
	}

	// Original function: FUN_00496c8b
	void DrawAtom_House_Tool_DoDrawEngineHouseType(std::int32_t houseType, std::uint32_t variant, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t colorTableIndex, bool drawShadow, bool drawFlag)
	{
		const auto addressAt = [](const void* base, std::size_t offset) -> const std::byte*
		{
			const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(base) + offset;

			return reinterpret_cast<const std::byte*>(static_cast<std::uintptr_t>(address));
		};

		const auto readWord = [&addressAt](const void* base, std::size_t offset) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, addressAt(base, offset), sizeof(value));

			return value;
		};

		const std::byte* house = addressAt(gStaticHouseEntries, static_cast<std::uint32_t>(houseType) * NC2Layout::RecordSize::EngineHouseType);

		if (colorTableIndex == -1)
		{
			const std::uint32_t defaultIndex = readWord(house, NC2Layout::EngineHouseTypeOffset::PaletteIdsUnsigned);
			std::memcpy(&colorTableIndex, &defaultIndex, sizeof(colorTableIndex));
		}

		const std::uint32_t bobId = readWord(house, NC2Layout::EngineHouseTypeOffset::StageBobIdsUnsigned + variant * sizeof(std::uint32_t));

		if (drawShadow)
		{
			void* shadowManager = NC2Lifetime::Read<void*>(house, NC2Layout::EngineHouseTypeOffset::ShadowBobManager);

			if (shadowManager != nullptr)
			{
				NXBasics::BobManager_PrintBob_Shadow(shadowManager, bobId, bitmap, x, y);
			}
		}

		void* bobManager = NC2Lifetime::Read<void*>(house, NC2Layout::EngineHouseTypeOffset::BobManager);

		ls_DrawAtom_House_DoDraw_PrintImageBob(bobManager, bobId, static_cast<std::uint32_t>(colorTableIndex), 256, bitmap, x, y, drawFlag);
	}

	// Original function: FUN_00496cfd
	void EngineGraphic_Draw(std::uint32_t graphicId, std::uint32_t variant, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::uint32_t animationValue, std::int32_t brightness, std::int32_t parameter24, bool centered)
	{
		const auto* graphic = static_cast<const SE2StaticDataLandscape*>(gStaticLandscapeEntries) + graphicId;

		EngineGraphic_DrawData(graphic, variant, bitmap, x, y, animationValue, brightness, parameter24, centered);
	}

	// Original function: FUN_00496d32
	void EngineGraphic_DrawData(const void* graphic, std::uint32_t variant, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::uint32_t animationValue, std::int32_t brightness, std::int32_t parameter24, bool centered)
	{
		const auto addressAt = [](const void* base, std::size_t offset) -> const std::byte*
		{
			const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(base) + offset;

			return reinterpret_cast<const std::byte*>(static_cast<std::uintptr_t>(address));
		};

		const auto readWord = [&addressAt](const void* base, std::size_t offset) -> std::uint32_t
		{
			std::uint32_t value;
			std::memcpy(&value, addressAt(base, offset), sizeof(value));

			return value;
		};

		const auto readByte = [&addressAt](const void* base, std::size_t offset) -> std::uint8_t
		{
			std::uint8_t value;
			std::memcpy(&value, addressAt(base, offset), sizeof(value));

			return value;
		};

		const auto readPointer = [&addressAt](const void* base, std::size_t offset) -> void*
		{
			void* value;
			std::memcpy(&value, addressAt(base, offset), sizeof(value));

			return value;
		};

		const auto signedValue = [](std::uint32_t bits) -> std::int32_t
		{

			return NXBasics::Runtime_TimeSigned32(bits);
		};

		const std::uint32_t variantCount = readWord(graphic, offsetof(SE2StaticDataLandscape, frameGroupCount));

		if (variantCount == 0)
			return;

		std::uint32_t variantIndex = variantCount - 1u;
		const std::byte* selected = addressAt(readPointer(graphic, offsetof(SE2StaticDataLandscape, frameGroups)), static_cast<std::size_t>(variantIndex) * sizeof(SStaticLandscapeFrames));

		while (signedValue(variantIndex) >= 0)
		{
			if (readWord(selected, 0) <= variant)
				break;

			--variantIndex;
			selected = reinterpret_cast<const std::byte*>(reinterpret_cast<std::uintptr_t>(selected) - sizeof(SStaticLandscapeFrames));
		}

		if (variantIndex == NXBasics::IntegerRange::UInt32Maximum)
			return;

		const std::uint32_t frameCount = readWord(selected, 4);
		std::uint32_t frameValue = 0;

		if (readByte(graphic, offsetof(SE2StaticDataLandscape, hasFireTransition)) != 0)
		{
			frameValue = (frameCount * animationValue) / 100u;

			if (frameValue >= frameCount)
				frameValue = frameCount - 1u;
		}
		else if (readByte(graphic, offsetof(SE2StaticDataLandscape, loopAnimation)) != 0)
		{
			frameValue = NC2Logic::gGameTime.tick + animationValue;

			if (gE2ParticelGeneratorStorage.useRealTimeAnimationClock != 0)
				frameValue = ::timeGetTime() / 83u + animationValue;
		}

		const std::uint32_t frameIndex = frameValue % frameCount;
		const std::uint32_t bobId = readWord(readPointer(selected, 8), static_cast<std::size_t>(frameIndex) * sizeof(std::uint32_t));

		if (centered)
		{
			const void* bobManager = readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers));

			if (bobManager != nullptr)
			{
				const void* area = NXBasics::BobManager_GetBobAreaRectanglePtr(bobManager, bobId);
				const std::int32_t halfWidth = signedValue(readWord(area, 8)) / 2;
				const std::int32_t halfHeight = signedValue(readWord(area, 12)) / 2;

				x = signedValue(static_cast<std::uint32_t>(x) - readWord(area, 0) - static_cast<std::uint32_t>(halfWidth));
				y = signedValue(static_cast<std::uint32_t>(y) - readWord(area, 4) - static_cast<std::uint32_t>(halfHeight));
			}
		}

		const void* shadowManager = readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers) + sizeof(void*));

		if (shadowManager != nullptr && brightness != 0)
			NXBasics::BobManager_PrintBob_Shadow(shadowManager, bobId, bitmap, x, y);

		if (readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)) == nullptr)
			return;

		if (readByte(graphic, offsetof(SE2StaticDataLandscape, userFxMatrix)) != 0)
		{
			if (brightness == 0)
				return;

			if (!CLevelOfDetailManager::GetUseMatrixFXBobsFlag() && readByte(graphic, offsetof(SE2StaticDataLandscape, drawVoidEver)) == 0)
				return;

			NXBasics::BobManager_PrintBob_UsingEffectMatrix(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y);
			return;
		}

		const auto drawWithoutBrightnessAdjustment = [&](NXBasics::CPalette* palette)
		{
			if (NXBasics::BobManager_GetBobType(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId) == 1u)
			{
				NXBasics::BobManager_PrintBob(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y, palette);
			}
			else if (NXBasics::BobManager_GetBobType(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId) == 4u)
			{
				if (CLevelOfDetailManager::GetUseLandscapeAlphaBobsFlag())
				{
					NXBasics::BobManager_PrintBobUsingTransparency(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y, palette);
				}
				else
				{
					NXBasics::BobManager_PrintBob_DoubleByteBobNormal(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y, palette);
				}
			}
		};

		if (brightness == 0)
		{
			drawWithoutBrightnessAdjustment(gStaticPalettes.blackPalette);
			return;
		}

		const std::int32_t graphicType = signedValue(readWord(graphic, offsetof(SE2StaticDataLandscape, logicType)));

		if (graphicType >= 73 && graphicType <= 78)
		{
			NXBasics::SColorRGB glowColor(0, 0, 128);

			constexpr std::int32_t offsetsX[8] = { -4, 4, 0, 0, -2, -2, 2, 2 };
			constexpr std::int32_t offsetsY[8] = { 0, 0, -4, 4, -2, 2, -2, 2 };
			constexpr std::uint32_t strengths[8] = { 40, 40, 40, 40, 64, 64, 64, 64 };

			for (std::uint32_t index = 0; index < 8u; ++index)
			{
				const std::int32_t glowX = signedValue(static_cast<std::uint32_t>(x) + static_cast<std::uint32_t>(offsetsX[index]));
				const std::int32_t glowY = signedValue(static_cast<std::uint32_t>(y) + static_cast<std::uint32_t>(offsetsY[index]));

				NXBasics::BobManager_PrintBob_FX_DrawArea(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, glowX, glowY, &glowColor, strengths[index]);
			}
		}

		const std::uint32_t brightnessProduct = (static_cast<std::uint32_t>(brightness) - 256u) * readWord(graphic, offsetof(SE2StaticDataLandscape, shadingFactor));
		brightness = signedValue(brightnessProduct) / 256 + 256;

		const auto* colorData = &gStaticPalettes.palettes256[readWord(graphic, offsetof(SE2StaticDataLandscape, paletteId))];
		NXBasics::CPalette* palette;

		if (parameter24 != -1)
		{
			palette = EnginePalette_GetPlayerPalette(parameter24);

			if (palette == nullptr)
				palette = gStaticPalettes.blackPalette;
		}
		else if (colorData->preshadedPalettes != nullptr)
		{
			palette = EnginePalette_GetBrightnessPalette(colorData, brightness / 2);
			drawWithoutBrightnessAdjustment(palette);
			return;
		}
		else if (colorData->remapTable != nullptr)
		{
			if (NXBasics::BobManager_GetBobType(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId) == 1u)
			{
				const auto* remap = colorData->remapTable;
				NXBasics::CPalette* remapPalette = EnginePalette_GetBrightnessPalette(colorData->remapTarget, brightness / 2);

				NXBasics::BobManager_PrintBob_Remapped(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y, remapPalette, remap->GetIndices());
			}
			else if (NXBasics::BobManager_GetBobType(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId) == 4u)
			{
				palette = colorData->palette;

				if (CLevelOfDetailManager::GetUseLandscapeAlphaBobsFlag())
				{
					NXBasics::BobManager_PrintBob_UsingShadedAlpha(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y, palette, brightness / 2);
				}
				else
				{
					NXBasics::BobManager_PrintBob_DoubleByteBobNormal(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y, palette);
				}
			}

			return;
		}
		else
		{
			palette = colorData->palette;
		}

		if (NXBasics::BobManager_GetBobType(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId) == 1u)
		{
			if (brightness >= 254 && brightness <= 258)
			{
				NXBasics::BobManager_PrintBob(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y, palette);
			}
			else
			{
				NXBasics::BobManager_PrintBob_Shade_SSE2(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y, palette, brightness / 2);
			}
		}
		else if (NXBasics::BobManager_GetBobType(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId) == 4u)
		{
			if (!CLevelOfDetailManager::GetUseLandscapeAlphaBobsFlag())
			{
				NXBasics::BobManager_PrintBob_DoubleByteBobNormal(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y, palette);
			}
			else if (brightness >= 254 && brightness <= 258)
			{
				NXBasics::BobManager_PrintBobUsingTransparency(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y, palette);
			}
			else
			{
				NXBasics::BobManager_PrintBob_UsingShadedAlpha(readPointer(graphic, offsetof(SE2StaticDataLandscape, bobManagers)), bobId, bitmap, x, y, palette, brightness / 2);
			}
		}
	}
}

namespace NC2E2
{
	// Original function: FUN_0048b5c1.
	std::int32_t StaticHumanMisc_RoadBuildSign_GetLandscapeId(std::int32_t playerId)
	{
		using NC2Lifetime::Read;

		if (playerId >= 20)
			return -1;

		const auto* player = reinterpret_cast<const std::byte*>(reinterpret_cast<std::uintptr_t>(NC2Logic::gGameControl) + static_cast<std::uint32_t>(playerId) * NC2Layout::RecordSize::GameControlPlayer);

		if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0)
			return -1;

		auto color = Read<std::int32_t>(player, NC2Layout::GameControlPlayerOffset::ColorId);

		if (color < 0 || color >= 10)
			color = 9;
		// Windows 0048B5F4 reads 00568630, color record +0x48, not its RGB value.

		return Read<std::int32_t>(gStaticHumanColors + static_cast<std::uint32_t>(color) * NC2Layout::RecordSize::StaticHumanColorUnsigned, NC2Layout::StaticHumanColorOffset::RoadBuildSignLandscapeId);
	}

	// Original function: FUN_0048b5fb.
	std::int32_t StaticHumanMisc_WallBuildSign_GetLandscapeId(std::int32_t playerId)
	{
		using NC2Lifetime::Read;

		if (playerId >= 20)
			return -1;

		const auto* player = reinterpret_cast<const std::byte*>(reinterpret_cast<std::uintptr_t>(NC2Logic::gGameControl) + static_cast<std::uint32_t>(playerId) * NC2Layout::RecordSize::GameControlPlayer);

		if (Read<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0)
			return -1;

		auto color = Read<std::int32_t>(player, NC2Layout::GameControlPlayerOffset::ColorId);

		if (color < 0 || color >= 10)
			color = 9;
		// Windows 0048B62E reads 00568634, color record +0x4C, not its palette patch.

		return Read<std::int32_t>(gStaticHumanColors + static_cast<std::uint32_t>(color) * NC2Layout::RecordSize::StaticHumanColorUnsigned, NC2Layout::StaticHumanColorOffset::WallBuildSignLandscapeId);
	}
}