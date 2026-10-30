#include "C2E2StaticDataTerrain.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineDrawingAccess.h"
#include "C2EngineHouseDrawing.h"
#include "C2EngineObjectData.h"
#include "C2GameState.h"
#include "C2HouseAccess.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "XBitmap.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XNumericConstants.h"

#include <cstring>

namespace NC2E2
{
	namespace
	{
		using NC2Lifetime::Read;
		using NC2Lifetime::Write;

		std::int32_t Bits(std::uint32_t value)
		{

			return NXBasics::Runtime_TimeSigned32(value);
		}

		std::int32_t Add(std::int32_t a, std::int32_t b)
		{

			return Bits(static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b));
		}

		std::int32_t Sub(std::int32_t a, std::int32_t b)
		{

			return Bits(static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b));
		}

		std::int32_t Mul(std::int32_t a, std::int32_t b)
		{

			return Bits(static_cast<std::uint32_t>(a) * static_cast<std::uint32_t>(b));
		}

		const std::byte* House(std::uint32_t id)
		{

			return NC2Logic::CHouseArray::mStaticVars + id * NC2Logic::CHouseArray::RuntimeElementSize;
		}

		const std::byte* Human(std::uint32_t id)
		{

			return NC2Logic::CHumanArray::mStaticVars + id * NC2Logic::CHumanArray::RuntimeElementSize;
		}

		const std::byte* Graphic(const void* house)
		{

			return static_cast<const std::byte*>(gStaticHouseEntries) + Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType) * NC2Layout::RecordSize::EngineHouseTypeUnsigned;
		}

		std::uint32_t DemolitionTime(const void* house)
		{

			return static_cast<std::uint32_t>(Mul(Read<std::int16_t>(house, NC2Layout::HouseOffset::DemolitionStepsDone), 255) / Read<std::int16_t>(house, NC2Layout::HouseOffset::DemolitionStepsRequired));
		}

		std::uint32_t ConstructionTime(std::int32_t percentage, std::int32_t first, std::int32_t last)
		{

			return static_cast<std::uint32_t>(Mul(Mul(Sub(percentage, first), 100) / Sub(last, first), 255) / 100);
		}

		// Complete Windows lazy debug-color region 00568B3C..00568B40.
		NXBasics::SColorRGB gHouseDebugColor{ 0, 0, 0 };
		bool gHouseDebugColorInitialized = false;

		// Original function: FUN_0049660e; the registered callback is an empty RET.
		int __cdecl DestroyHouseDebugColor()
		{

			return 0;
		}
	}

	using NC2Lifetime::Read;
	using NC2Lifetime::Write;

	// Original function: FUN_00495b57; completed, building, upgrading and demolition paths share the original effect order.
	void DrawAtom_House_DoDraw(std::uint32_t id, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness, const void* display)
	{
		const auto* house = House(id);
		NC2Logic::House_GetHouseTypeDataPtr(house);
		const auto* graphic = Graphic(house);
		auto* dynamic = gDynamicDataHouses.entries + id * NC2Layout::RecordSize::DynamicHouseUnsigned;
		const auto variant = Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) % Read<std::uint32_t>(graphic, NC2Layout::EngineHouseTypeOffset::PaletteCount);
		const auto paletteId = Read<std::uint32_t>(graphic, NC2Layout::EngineHouseTypeOffset::PaletteIdsUnsigned + variant * NC2Layout::RecordSize::HouseStageValueUnsigned);
		auto* colorData = gStaticPalettes.palettes256 + paletteId;
		auto* manager = Read<void*>(graphic, NC2Layout::EngineHouseTypeOffset::BobManager);
		auto* shadowManager = Read<void*>(graphic, NC2Layout::EngineHouseTypeOffset::ShadowBobManager);
		const auto stage = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage);
		const auto state = Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState);
		const bool deleting = Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) != 0;
		const auto baseBob = Read<std::uint32_t>(graphic, NC2Layout::EngineHouseTypeOffset::StageBobIdsUnsigned + stage * NC2Layout::RecordSize::HouseStageValueUnsigned);
		std::int32_t removedRows = 0;

		if (Read<std::uint8_t>(dynamic, NC2Layout::DynamicHouseOffset::SelectedFlag) != 0)
		{
			brightness = Bits(DrawAtom_Tool_GetSelectedBrightness());
		}
		else if (display != nullptr && Read<std::uint8_t>(display, NC2Layout::WorldDisplayOffset::HighlightingEnabled) == 0)
		{
			brightness = Add(256, Mul(Sub(brightness, 256), Read<std::int32_t>(graphic, NC2Layout::EngineHouseTypeOffset::ShadingFactor256)) / 256);
		}

		if (!deleting && display != nullptr)
		{
			const auto marking = Display_DE_ObjectsToMark_House_GetMarkType(display, id);

			if (marking != 0)
			{
				const auto* color = DrawAtom_Tool_GetMarkingColorRef(marking, Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId));
				S2DEngineDisplaySearchObjectBobData bobs;
				DrawAtom_House_GetBobData(id, &bobs);

				for (std::uint32_t i = 0; i < bobs.count; ++i)
				{
					const auto& bob = bobs.bobs[i];

					for (std::int32_t dy = -1; dy <= 1; ++dy)
					{
						if (state == 3)
						{
							NXBasics::BobManager_PrintBob_FX_DrawOutline(bob.manager, bob.id, bitmap, Add(x, bob.offsetX), Add(Add(y, bob.offsetY), dy), color, 1);
						}
						else
						{
							NXBasics::BobManager_PrintBob_FX_DrawOutline_Lines(bob.manager, bob.id, bitmap, Add(x, bob.offsetX), Add(Add(y, bob.offsetY), dy), color, 1);
						}
					}
				}
			}
		}

		DrawAtom_House_Tool_CreateDestroySmokeParticelBack(id, bitmap, x, y, brightness);

		if (state == 3)
		{
			if (deleting)
			{
				if (manager == nullptr)
					return;

				if (colorData->preshadedPalettes != nullptr)
				{
					auto* palette = EnginePalette_GetBrightnessPalette(colorData, brightness / 2);

					if (NXBasics::BobManager_GetBobType(manager, baseBob) == 4)
						removedRows = NXBasics::BobManager_PrintBob_UsingCollapseTimeMask(manager, baseBob, DemolitionTime(house), bitmap, x, y, palette);
				}
			}
			else
			{
				if (shadowManager != nullptr)
					NXBasics::BobManager_PrintBob_Shadow(shadowManager, baseBob, bitmap, x, y);

				ls_DrawAtom_House_DoDraw_PrintImageBob(manager, baseBob, paletteId, brightness, bitmap, x, y, false);

				for (std::uint32_t i = 0; i < Read<std::uint32_t>(graphic, NC2Layout::EngineHouseTypeOffset::ActionOverlayCount); ++i)
				{
					const auto* overlay = Read<const std::byte*>(graphic, NC2Layout::EngineHouseTypeOffset::ActionOverlays) + i * NC2Layout::RecordSize::EngineHouseActionOverlayUnsigned;
					const auto action = Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::Action);

					if (action == 0 || Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::DevelopmentStage) != stage || Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::FrameCount) == 0)
						continue;

					if (Read<std::uint8_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::RequiredActiveFlag) != static_cast<std::uint8_t>(NC2Logic::House_ActionOverlay_IsActive(house, action)))
						continue;

					const auto frame = (NC2Logic::gGameTime.tick / Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::FrameTicks)) % Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::FrameCount);
					ls_DrawAtom_House_DoDraw_PrintImageBob(manager, Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::BobIdsUnsigned + frame * NC2Layout::RecordSize::HouseActionFrameIdUnsigned), paletteId, brightness, bitmap, Add(x, Read<std::int32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::OffsetX)), Add(y, Read<std::int32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::OffsetY)), false);
				}
				// Windows requires a display for the completed-house debug branches.

				if ((Read<std::uint8_t>(display, NC2Layout::WorldDisplayOffset::DrawFlagsByte2) & 1u) != 0)
				{
					auto* font = Read<NXBasics::CFont*>(display, NC2Layout::WorldDisplayOffset::Font);

					for (std::uint32_t i = 0; i < 5; ++i)
					{
						if (NC2Logic::House_ActionOverlay_IsActive(house, i))
							bitmap->Text_PrintV(font, x, Add(y, static_cast<std::int32_t>(10u + i * 10u)), "ovly %d", i);
					}
				}

				for (std::int32_t i = 0; i < 10; ++i)
				{
					const auto* overlay = graphic + NC2Layout::EngineHouseTypeOffset::LandscapeOverlaysUnsigned + (stage * 10u + static_cast<std::uint32_t>(i)) * 16u;

					if (Read<std::uint8_t>(overlay, NC2Layout::EngineHouseLandscapeOverlayOffset::EnabledFlag) != 0 && NC2Logic::House_LandscapeOverlay_IsActive(house, i))
						EngineGraphic_Draw(Read<std::uint32_t>(overlay, NC2Layout::EngineHouseLandscapeOverlayOffset::LandscapeId), 1, bitmap, Add(x, Read<std::int32_t>(overlay, NC2Layout::EngineHouseLandscapeOverlayOffset::OffsetX)), Add(y, Read<std::int32_t>(overlay, NC2Layout::EngineHouseLandscapeOverlayOffset::OffsetY)), NC2Logic::gGameTime.tick, brightness, -1, false);
				}

				if (Read<std::uint8_t>(graphic, NC2Layout::EngineHouseTypeOffset::DoorOverlaysUnsigned + stage * NC2Layout::RecordSize::EngineHouseDoorOverlayUnsigned) != 0)
				{
					NC2Logic::CHumanIterator iterator;
					auto human = iterator.GetFirstHumanId(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId), house + NC2Layout::HouseOffset::DoorEntryPoint, 1);

					while (human != -1 && NC2Logic::Human_Misc_IsHidden(Human(static_cast<std::uint32_t>(human))))
						human = iterator.GetNextHumanId();

					const auto bob = Read<std::uint32_t>(graphic, (human == -1 ? NC2Layout::EngineHouseTypeOffset::ClosedDoorBobIdsUnsigned : NC2Layout::EngineHouseTypeOffset::OpenDoorBobIdsUnsigned) + stage * NC2Layout::RecordSize::HouseDoorOverlayUnsigned);

					if (bob != NXBasics::IntegerRange::UInt32Maximum)
						ls_DrawAtom_House_DoDraw_PrintImageBob(manager, bob, paletteId, brightness, bitmap, x, y, false);
				}
			}
		}
		else if (state == 1 || state == 2)
		{
			const bool upgrade = state == 2;
			const auto* layers = graphic + NC2Layout::EngineHouseTypeOffset::ConstructionLayersUnsigned + stage * NC2Layout::RecordSize::EngineHouseConstructionLayersUnsigned;
			const auto count = Read<std::int32_t>(layers, NC2Layout::EngineHouseConstructionLayersOffset::Count);
			const auto percentage = NC2Logic::House_Construction_GetPercentage(house);

			if (!deleting)
			{
				if (upgrade && shadowManager != nullptr)
					NXBasics::BobManager_PrintBob_Shadow(shadowManager, baseBob, bitmap, x, y);

				for (std::int32_t i = 0; i < count; ++i)
				{
					const auto* layer = layers + NC2Layout::EngineHouseConstructionLayersOffset::EntriesUnsigned + static_cast<std::uint32_t>(i) * NC2Layout::RecordSize::EngineHouseConstructionLayerUnsigned;
					const auto first = Read<std::int32_t>(layer, NC2Layout::EngineHouseConstructionLayerOffset::FirstPercentage), last = Read<std::int32_t>(layer, NC2Layout::EngineHouseConstructionLayerOffset::LastPercentage);

					if (Read<std::uint8_t>(layer, NC2Layout::EngineHouseConstructionLayerOffset::UpgradeFlag) != static_cast<std::uint8_t>(upgrade) || percentage < first)
						continue;

					EnginePalette_GetBrightnessPalette(colorData, brightness / 2);
					const auto shadow = Read<std::uint32_t>(layer, NC2Layout::EngineHouseConstructionLayerOffset::ShadowBobId);

					if (shadow != NXBasics::IntegerRange::UInt32Maximum)
						NXBasics::BobManager_PrintBob_Shadow_TimeMask(manager, shadow, percentage > last ? 256u : ConstructionTime(percentage, first, last), bitmap, x, y);
				}
			}

			if (upgrade)
			{
				if (deleting)
				{
					if (manager == nullptr)
					{

						return;
					}

					if (colorData->preshadedPalettes != nullptr)
					{
						auto* palette = EnginePalette_GetBrightnessPalette(colorData, brightness / 2);

						if (NXBasics::BobManager_GetBobType(manager, baseBob) == 4)
							removedRows = NXBasics::BobManager_PrintBob_UsingCollapseTimeMask(manager, baseBob, DemolitionTime(house), bitmap, x, y, palette);
					}
				}
				else
				{
					ls_DrawAtom_House_DoDraw_PrintImageBob(manager, baseBob, paletteId, brightness, bitmap, x, y, false);
				}
			}

			for (std::int32_t i = 0; i < count; ++i)
			{
				const auto* layer = layers + NC2Layout::EngineHouseConstructionLayersOffset::EntriesUnsigned + static_cast<std::uint32_t>(i) * NC2Layout::RecordSize::EngineHouseConstructionLayerUnsigned;
				const auto first = Read<std::int32_t>(layer, NC2Layout::EngineHouseConstructionLayerOffset::FirstPercentage), last = Read<std::int32_t>(layer, NC2Layout::EngineHouseConstructionLayerOffset::LastPercentage);

				if (Read<std::uint8_t>(layer, NC2Layout::EngineHouseConstructionLayerOffset::UpgradeFlag) != static_cast<std::uint8_t>(upgrade) || percentage < first)
					continue;

				auto* palette = EnginePalette_GetBrightnessPalette(colorData, brightness / 2);
				const auto bob = Read<std::uint32_t>(layer, NC2Layout::EngineHouseConstructionLayerOffset::BobId);

				if (percentage > last)
				{
					if (deleting)
					{
						removedRows = NXBasics::BobManager_PrintBob_UsingCollapseTimeMask(manager, bob, DemolitionTime(house), bitmap, x, y, palette);
					}
					else
					{
						NXBasics::BobManager_PrintBob_DoubleByteBobNormal(manager, bob, bitmap, x, y, palette);
					}
				}
				else
				{
					const auto time = ConstructionTime(percentage, first, last);

					if (deleting)
					{
						NXBasics::BobManager_PrintBob_UsingRangedCollapseTimeMask(manager, bob, DemolitionTime(house), time, bitmap, x, y, palette);
					}
					else
					{
						NXBasics::BobManager_PrintBob_UsingTimeMask(manager, bob, time, bitmap, x, y, palette);
					}
				}
			}

			auto flagX = Add(x, Read<std::int32_t>(graphic, NC2Layout::EngineHouseStageViewOffset::FlagXUnsigned + stage * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned));
			auto flagY = Add(y, Read<std::int32_t>(graphic, NC2Layout::EngineHouseStageViewOffset::FlagYUnsigned + stage * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned));

			if (display != nullptr)
			{
				std::int32_t origin[2], door[2];
				Display_MapMigToLocalDisplayPixelCoordinates(display, house + NC2Layout::HouseOffset::MapPosition, origin, false);
				Display_MapMigToLocalDisplayPixelCoordinates(display, house + NC2Layout::HouseOffset::DoorEntryPoint, door, false);
				flagX = Sub(door[0], origin[0]);
				flagY = Sub(door[1], origin[1]);
			}

			const auto sign = StaticHumanMisc_InhabitantSign_GetLandscapeId(Read<std::int32_t>(house, NC2Layout::HouseOffset::PlayerId), 8);

			if (sign != -1)
				EngineGraphic_Draw(static_cast<std::uint32_t>(sign), 1, bitmap, Add(x, flagX), Add(y, flagY), NC2Logic::gGameTime.tick, brightness, -1, false);
		}

		const auto* pointsData = graphic + stage * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned;
		// Outlined 0043322F: incense requires a completed house that is not being demolished.

		if (!deleting && state == 3 && Read<std::int32_t>(house, NC2Layout::HouseOffset::HomeQualityEnhancer2Value) > 0)
		{
			const auto* points = Read<const std::byte*>(pointsData, NC2Layout::EngineHouseStageViewOffset::HolyFirePoints);

			for (std::uint32_t i = 0; i < Read<std::uint32_t>(pointsData, NC2Layout::EngineHouseStageViewOffset::HolyFirePointCount); ++i)
			{
				EngineGraphic_Draw(static_cast<std::uint32_t>(gStaticLandscapeStorage.incenseFireIds[(id + i) % 5u]), 1, bitmap, Add(x, Read<std::int32_t>(points, i * NC2Layout::RecordSize::EnginePixelPointUnsigned)), Add(y, Read<std::int32_t>(points, i * NC2Layout::RecordSize::EnginePixelPointUnsigned + NC2Layout::EnginePixelPointOffset::YUnsigned)), NC2Logic::gGameTime.tick, brightness, -1, false);
			}
		}

		if (state == 3 || state == 2)
		{
			DrawAtom_House_Tool_UpdateFirePoints(id);

			if (Read<std::uint32_t>(dynamic, NC2Layout::DynamicHouseOffset::ActiveFirePointCount) != 0)
			{
				const auto count = Read<std::uint32_t>(pointsData, NC2Layout::EngineHouseTypeOffset::StagePoints);
				const auto* points = Read<const std::byte*>(pointsData, NC2Layout::EngineHouseStageViewOffset::FirePoints);

				for (std::uint32_t i = 0; i < count; ++i)
				{
					if (i >= 200 || Read<std::uint8_t>(dynamic, NC2Layout::DynamicHouseOffset::FirePointFlagsUnsigned + i) == 0)
						continue;

					const auto pointY = Add(Read<std::int32_t>(points, i * NC2Layout::RecordSize::EnginePixelPointUnsigned + NC2Layout::EnginePixelPointOffset::YUnsigned), removedRows);

					if (pointY > Read<std::int32_t>(points, (count - 1u) * NC2Layout::RecordSize::EnginePixelPointUnsigned + NC2Layout::EnginePixelPointOffset::YUnsigned))
						break;

					EngineGraphic_Draw(static_cast<std::uint32_t>(gStaticLandscapeStorage.houseFireIds[(id + i) % 5u]), 1, bitmap, Add(x, Read<std::int32_t>(points, i * NC2Layout::RecordSize::EnginePixelPointUnsigned)), Add(y, pointY), i, brightness, -1, false);
				}
			}
		}

		if (!deleting)
		{
			S2DEngineDrawAtomHouseSignData signs;
			DrawAtom_House_GetSignData(id, &signs);

			for (std::uint32_t i = 0; i < signs.count; ++i)
			{
				EngineGraphic_Draw(static_cast<std::uint32_t>(signs.signs[i].landscape), 1, bitmap, Add(x, signs.signs[i].x), Add(y, signs.signs[i].y), 0, brightness, -1, false);
			}
		}

		DrawAtom_House_Tool_CreateDestroySmokeParticelForward(id, bitmap, x, y, brightness);

		if (display != nullptr && Read<std::uint8_t>(display, NC2Layout::WorldDisplayOffset::ActiveDisplayFlag) != 0)
			++gWorldMusicState.visiblePlayerCounts[Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId)];

		if ((Read<std::uint8_t>(display, NC2Layout::WorldDisplayOffset::DrawFlagsByte2) & 1u) != 0)
		{
			if (!gHouseDebugColorInitialized)
			{
				gHouseDebugColorInitialized = true;
				gHouseDebugColor = NXBasics::SColorRGB(255, 255, 255);
				NXBasics::Runtime_AtExit(DestroyHouseDebugColor);
			}

			for (std::int32_t dy = -1; dy <= 1; ++dy)
			{
				for (std::int32_t dx = -1; dx <= 1; ++dx)
				{
					NXBasics::BobManager_PrintBob_FX_DrawOutline_Lines(manager, baseBob, bitmap, Add(x, dx), Add(y, dy), &gHouseDebugColor, 1);
				}
			}

			auto* font = Read<NXBasics::CFont*>(display, NC2Layout::WorldDisplayOffset::Font);
			const auto* name = reinterpret_cast<const char*>(graphic);
			bitmap->Text_Print(font, Sub(x, NXBasics::Font_GetPixelWidth(font, name) / 2), Add(y, 10), name);
		}
	}

	// Original function: FUN_0049660f.
	void DrawAtom_House_GetBobData(std::uint32_t id, S2DEngineDisplaySearchObjectBobData* output)
	{
		const auto* house = House(id);
		const auto* data = static_cast<const std::byte*>(gStaticHouseEntries) + Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType) * NC2Layout::RecordSize::EngineHouseTypeUnsigned;
		std::memset(output, 0, sizeof(*output));
		auto* manager = Read<void*>(data, NC2Layout::EngineHouseTypeOffset::BobManager);
		output->AddBob(manager, Read<std::uint32_t>(data, NC2Layout::EngineHouseTypeOffset::StageBobIdsUnsigned + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::HouseStageValueUnsigned), 0, 0);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::EngineHouseTypeOffset::ActionOverlayCount); ++i)
		{
			const auto* overlay = Read<std::byte*>(data, NC2Layout::EngineHouseTypeOffset::ActionOverlays) + i * NC2Layout::RecordSize::EngineHouseActionOverlayUnsigned;
			const auto action = Read<std::int32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::Action);

			if (action == 0 || Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::DevelopmentStage) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) || Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::FrameCount) == 0)
				continue;

			const bool active = NC2Logic::House_ActionOverlay_IsActive(house, static_cast<std::uint32_t>(action));

			if (Read<std::uint8_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::RequiredActiveFlag) != static_cast<std::uint8_t>(active))
				continue;

			const auto frame = (NC2Logic::gGameTime.tick / Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::FrameTicks)) % Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::FrameCount);
			output->AddBob(manager, Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::BobIdsUnsigned + frame * NC2Layout::RecordSize::HouseActionFrameIdUnsigned), Read<std::int32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::OffsetX), Read<std::int32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::OffsetY));
		}
	}

	// Original function: FUN_00496737; DrawAtom_House_GetSignData.
	void DrawAtom_House_GetSignData(std::uint32_t id, S2DEngineDrawAtomHouseSignData* output)
	{
		output->count = 0;
		const auto* house = House(id);
		auto stage = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage);

		if (Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 2)
			++stage;

		const auto* data = static_cast<const std::byte*>(gStaticHouseEntries) + Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType) * NC2Layout::RecordSize::EngineHouseTypeUnsigned + stage * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned;
		const auto x = Read<std::int32_t>(data, NC2Layout::EngineHouseStageViewOffset::FlagX), baseY = Read<std::int32_t>(data, NC2Layout::EngineHouseStageViewOffset::FlagY);

		if (x == 0 && baseY == 0)
			return;

		if (NC2Logic::House_GetHouseMainType(house) == 2)
		{
			std::uint32_t families[20];
			const auto count = NC2Logic::House_Inhabitants_GetNumberOfFamiliesLivingInHome(house, families);
			bool used[20]{ };

			if (count == 0)
				return;

			auto y = baseY - static_cast<std::int32_t>(count * 25u) + 25;

			for (std::uint32_t slot = 0; slot < 20; ++slot)
			{
				const auto* attachment = static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::Attachments + slot * 16u;

				if ((Read<std::uint8_t>(attachment, NC2Layout::HouseAttachmentOffset::Flags) & 1u) == 0 || families[slot] == NXBasics::IntegerRange::UInt32Maximum || used[families[slot]])
					continue;

				used[families[slot]] = true;
				const auto humanId = Read<std::int32_t>(attachment, NC2Layout::HouseAttachmentOffset::HumanId);
				const auto* human = Human(humanId);
				const auto sign = Read<std::int32_t>(human, NC2Layout::HumanOffset::MotherId) != -1 || Read<std::int32_t>(human, NC2Layout::HumanOffset::FatherId) != -1 || Read<std::int32_t>(human, NC2Layout::HumanOffset::ChildId) != -1 ? 2u : Read<std::int32_t>(human, NC2Layout::HumanOffset::SpouseId) != -1 ? 1u
				                                                                                                                                                                                                                                                                                                      : 0u;
				const auto graphic = StaticHumanMisc_InhabitantSign_GetLandscapeId(Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), sign);

				if (graphic == -1)
					continue;

				output->signs[output->count++] = { graphic, x, y, humanId };

				if (output->count == 30)
					return;

				y += 25;
			}

			return;
		}

		if (NC2Logic::House_Inhabitants_GetNumberOfAttachedHumans(house, 2) == 0)
			return;

		std::int32_t ordinary[20], soldiers[20];
		std::uint32_t ordinaryCount = 0, soldierCount = 0;

		for (std::uint32_t slot = 0; slot < 20; ++slot)
		{
			const auto* attachment = static_cast<const std::byte*>(house) + NC2Layout::HouseOffset::Attachments + slot * 16u;

			if ((Read<std::uint8_t>(attachment, NC2Layout::HouseAttachmentOffset::Flags) & 2u) == 0)
				continue;

			const auto humanId = Read<std::int32_t>(attachment, NC2Layout::HouseAttachmentOffset::HumanId);

			if (NC2Logic::Human_IsJobSoldier(Human(humanId)) && Read<std::uint8_t>(data, NC2Layout::EngineHouseStageViewOffset::SoldierFlagEnabled) != 0)
			{
				soldiers[soldierCount++] = humanId;
			}
			else
			{
				ordinary[ordinaryCount++] = humanId;
			}
		}

		auto y = baseY - static_cast<std::int32_t>(ordinaryCount * 25u) + 25;

		for (std::uint32_t i = 0; i < ordinaryCount; ++i)
		{
			const auto* human = Human(ordinary[i]);
			const auto sign = NC2Logic::Human_IsJobCarrier(human) ? 7u : static_cast<std::uint32_t>(ordinary[i] % 4 + 3);
			const auto graphic = StaticHumanMisc_InhabitantSign_GetLandscapeId(Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId), sign);

			if (graphic == -1)
				continue;

			output->signs[output->count++] = { graphic, x, y, ordinary[i] };

			if (output->count == 30)
			{

				return;
			}

			y += 25;
		}

		if (soldierCount != 0)
		{
			const auto graphic = StaticHumanMisc_SoldierSign_GetLandscapeId(Read<std::int32_t>(Human(soldiers[0]), NC2Layout::HumanOffset::PlayerId), soldierCount);

			if (graphic != -1)
				output->signs[output->count++] = { graphic, Read<std::int32_t>(data, NC2Layout::EngineHouseStageViewOffset::SoldierFlagX), Read<std::int32_t>(data, NC2Layout::EngineHouseStageViewOffset::SoldierFlagY), -1 };
		}
	}

	// Original function: FUN_00496a95; adjust one point per draw and preserve the original RNG call.
	void DrawAtom_House_Tool_UpdateFirePoints(std::uint32_t id)
	{
		const auto* house = House(id);
		const auto* graphic = Graphic(house);
		const auto stage = Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage);
		auto* dynamic = gDynamicDataHouses.entries + id * NC2Layout::RecordSize::DynamicHouseUnsigned;
		const auto maximum = Read<std::uint32_t>(graphic, NC2Layout::EngineHouseTypeOffset::MaximumHitpointsUnsigned + stage * NC2Layout::RecordSize::HouseStageValueUnsigned);
		const auto pointCount = Read<std::uint32_t>(graphic, NC2Layout::EngineHouseTypeOffset::StagePointsUnsigned + stage * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned);
		const auto originalHealth = Read<std::int32_t>(house, NC2Layout::HouseOffset::Hitpoints);
		auto health = originalHealth;

		if (health < 0)
			health = 0;

		auto wanted = ((maximum - static_cast<std::uint32_t>(health)) * pointCount) / maximum;

		if (originalHealth < Bits(maximum) && wanted == 0 && pointCount != 0)
			wanted = 1;

		const auto current = Read<std::uint32_t>(dynamic, NC2Layout::DynamicHouseOffset::ActiveFirePointCount);

		if (wanted < current)
		{
			for (std::uint32_t i = 0; i < pointCount; ++i)
			{
				if (i >= 200 || Read<std::uint8_t>(dynamic, NC2Layout::DynamicHouseOffset::FirePointFlagsUnsigned + i) == 0)
					continue;

				Write<std::uint8_t>(dynamic, NC2Layout::DynamicHouseOffset::FirePointFlagsUnsigned + i, 0);
				Write(dynamic, NC2Layout::DynamicHouseOffset::ActiveFirePointCount, current - 1u);
				return;
			}
		}
		else if (wanted > current)
		{
			const auto pick = static_cast<std::uint32_t>(NXBasics::Runtime_Rand()) % (pointCount - current);
			std::uint32_t freeSeen = 0;

			for (std::uint32_t i = 0; i < pointCount; ++i)
			{
				if (i >= 200 || Read<std::uint8_t>(dynamic, NC2Layout::DynamicHouseOffset::FirePointFlagsUnsigned + i) != 0)
					continue;

				if (freeSeen >= pick)
				{
					Write<std::uint8_t>(dynamic, NC2Layout::DynamicHouseOffset::FirePointFlagsUnsigned + i, 1);
					Write(dynamic, NC2Layout::DynamicHouseOffset::ActiveFirePointCount, current + 1u);

					return;
				}

				++freeSeen;
			}
		}
	}

	// Original function: FUN_00496b79.
	void DrawAtom_House_Tool_CreateDestroySmokeParticelForward(std::uint32_t id, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness)
	{
		const auto* house = House(id);

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0)
		{

			return;
		}

		const auto* data = Graphic(house) + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned;
		const auto* points = Read<const std::byte*>(data, NC2Layout::EngineHouseStageViewOffset::ForwardSmokePoints);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::EngineHouseStageViewOffset::ForwardSmokePointCount); ++i)
		{
			EngineGraphic_Draw(static_cast<std::uint32_t>(gStaticLandscapeStorage.smokeIds[i % 5u]), 1, bitmap, Add(x, Read<std::int32_t>(points, i * NC2Layout::RecordSize::EnginePixelPointUnsigned)), Add(y, Read<std::int32_t>(points, i * NC2Layout::RecordSize::EnginePixelPointUnsigned + NC2Layout::EnginePixelPointOffset::YUnsigned)), i, brightness, -1, false);
		}
	}

	// Original function: FUN_00496c02.
	void DrawAtom_House_Tool_CreateDestroySmokeParticelBack(std::uint32_t id, NXBasics::CBitmap* bitmap, std::int32_t x, std::int32_t y, std::int32_t brightness)
	{
		const auto* house = House(id);

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::DemolitionFlag) == 0)
		{

			return;
		}

		const auto* data = Graphic(house) + Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage) * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned;
		const auto* points = Read<const std::byte*>(data, NC2Layout::EngineHouseStageViewOffset::BackSmokePoints);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(data, NC2Layout::EngineHouseStageViewOffset::BackSmokePointCount); ++i)
		{
			EngineGraphic_Draw(static_cast<std::uint32_t>(gStaticLandscapeStorage.smokeIds[i % 5u]), 1, bitmap, Add(x, Read<std::int32_t>(points, i * NC2Layout::RecordSize::EnginePixelPointUnsigned)), Add(y, Read<std::int32_t>(points, i * NC2Layout::RecordSize::EnginePixelPointUnsigned + NC2Layout::EnginePixelPointOffset::YUnsigned)), i, brightness, -1, false);
		}
	}
}