#include "C2E2StaticDataHouses.h"
#include "C2E2StaticDataPalettes.h"
#include "C2E2StaticDataTerrain.h"
#include "C2EngineLifetimeAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MapArea.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"

#include <cstring>
#include <new>

namespace
{
	using namespace NC2Lifetime;

	// Complete Windows table 00506BE8..00506CBF, including the dynamically filled tail.
	const NXBasics::SToken gHouseTokens[] = {
		{ "editname", SwitchIds::HouseGraphicsToken::EditName }, { "editgroups", SwitchIds::HouseGraphicsToken::EditGroups }, { "logicmaximumsize", SwitchIds::HouseGraphicsToken::LogicMaximumSize }, { "logictype", SwitchIds::HouseGraphicsToken::Logictype }, { "logicbuildblockarea", SwitchIds::HouseGraphicsToken::LogicBuildBlockArea }, { "logicwalkblockarea", SwitchIds::HouseGraphicsToken::LogicWalkBlockArea }, { "logicconstructionworkarea", SwitchIds::HouseGraphicsToken::Logicconstructionworkarea }, { "logictribetype", SwitchIds::HouseGraphicsToken::LogicTribeType }, { "logicconstructiongoods", SwitchIds::HouseGraphicsToken::Logicconstructiongoods }, { "logicdoorpoint", SwitchIds::HouseGraphicsToken::Logicdoorpoint }, { "logichitpoints", SwitchIds::HouseGraphicsToken::LogicHitpoints }, { "logicadditionalattachpointarea", SwitchIds::HouseGraphicsToken::LogicAdditionalAttachPointArea }, { "gfxboblibs", SwitchIds::HouseGraphicsToken::Gfxboblibs }, { "gfxbobid", SwitchIds::HouseGraphicsToken::Gfxbobid }, { "gfxpalette", SwitchIds::HouseGraphicsToken::GfxPalette }, { "gfxshadingfactor", SwitchIds::HouseGraphicsToken::GfxShadingFactor }, { "gfxbobconstructionlayer", SwitchIds::HouseGraphicsToken::Gfxbobconstructionlayer }, { "gfxdoorbobid", SwitchIds::HouseGraphicsToken::Gfxdoorbobid }, { "GfxOverlay", SwitchIds::HouseGraphicsToken::GfxOverlay }, { "gfxoverlaylandscape", SwitchIds::HouseGraphicsToken::GfxOverlayLandscape }, { "gfxflagpoint", SwitchIds::HouseGraphicsToken::GfxFlagPoint }, { "gfxsoldierflagpoint", SwitchIds::HouseGraphicsToken::Gfxsoldierflagpoint }, { "GfxFirePoint", SwitchIds::HouseGraphicsToken::GfxFirePoint }, { "GfxSmokePoint", SwitchIds::HouseGraphicsToken::GfxSmokePoint }, { "GfxHolyFirePoint", SwitchIds::HouseGraphicsToken::GfxHolyFirePoint }, { "arrowSlotData", SwitchIds::HouseGraphicsToken::ArrowSlotData }, { nullptr, 0 }
	};
	static_assert(sizeof(gHouseTokens) == 27 * sizeof(NXBasics::SToken));

	struct SHouseAreaScratch
	{
		std::uint32_t count;
		NC2Logic::SMapAreaRow rows[100];
	};

	static_assert(sizeof(SHouseAreaScratch) == 804);

	// Original function: FUN_0048f57f; empty CTokenList exit callback.
	int __cdecl DestroyHouseTokenList()
	{

		return 0;
	}

	void* HouseRecord_Construct(void* entry);
	void* HouseLogicData_Construct(void* data);
	void HouseDoorPoints_Construct(void* points);

	// Original function: FUN_0048f580; record constructors precede the native array clear.
	void* HouseRecord_Construct(void* entry)
	{
		HouseLogicData_Construct(static_cast<std::byte*>(entry) + NC2Layout::EngineHouseTypeOffset::LogicTypes);

		return entry;
	}

	// Original function: FUN_0048f58f.
	void* HouseLogicData_Construct(void* data)
	{
		HouseDoorPoints_Construct(static_cast<std::byte*>(data) + NC2Layout::EngineHouseLogicViewOffset::DoorPoints);

		return data;
	}

	// Original function: FUN_0048f5a1.
	void HouseDoorPoints_Construct(void* points)
	{
		std::memset(points, 0, 5 * 4);
	}

	void ReadAreaRow(NXBasics::CIniFileReader& reader, SHouseAreaScratch& area)
	{
		if (area.count == 100)
		{

			return;
		}

		auto& row = area.rows[area.count];
		row.x = static_cast<std::int16_t>(reader.GetInt());
		row.y = static_cast<std::int16_t>(reader.GetInt());
		row.width = static_cast<std::uint32_t>(reader.GetInt());
		++area.count;
	}

	void StoreArea(void* entry, std::uint32_t offset, const SHouseAreaScratch& scratch)
	{
		if (scratch.count == 0)
		{

			return;
		}

		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2Logic::CMapArea), 1);
		auto* area = allocation == nullptr ? nullptr : new (allocation) NC2Logic::CMapArea(scratch.count);
		Write(entry, offset, area);
		// The original copies rows and updates bounds even after allocation failure.
		std::memcpy(area->rows, scratch.rows, scratch.count * sizeof(NC2Logic::SMapAreaRow));
		area->UpdateCalculatedVariables();
	}
}

namespace NC2E2
{
	// Original function: FUN_004808d3; -1 remains a sentinel at the entry point.
	std::int32_t Engine2d_EntryPoint_GetNextHouseId(std::int32_t previous, std::uint32_t tribe, std::uint32_t logicType, std::uint32_t* stage)
	{

		return previous == -1 ? -1 : StaticDataHouses_GetNextHouseId(previous, tribe, logicType, stage);
	}

	// Original function: FUN_0048f73e; the output stage is written only for a match.
	std::int32_t StaticDataHouses_GetNextHouseId(std::int32_t previous, std::uint32_t tribe, std::uint32_t logicType, std::uint32_t* stage)
	{
		const auto limit = gStaticHouseCount;
		std::int32_t index = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(previous) + 1u);

		if (index >= limit)
			return -1;

		const auto entries = gStaticHouseEntries;

		for (; index < limit; index = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(index) + 1u))
		{
			const auto record = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(entries) + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::EngineHouseTypeUnsigned + 0x3Cu);

			if (Read<std::uint32_t>(record, NC2Layout::EngineHouseLogicViewOffset::TribeType) != tribe)
				continue;

			for (std::uint32_t item = 0; item < Read<std::uint32_t>(record, NC2Layout::EngineHouseLogicViewOffset::DevelopmentStageCount); ++item)
			{
				if (Read<std::uint32_t>(record, item * NC2Layout::RecordSize::HouseStageValueUnsigned) == logicType)
				{
					*stage = item;
					return index;
				}
			}
		}

		return -1;
	}

	// Original function: FUN_0048e651; CE2StaticDataHousesManager constructor.
	void* __fastcall E2StaticDataHousesManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gE2StaticDataHousesManager = manager;
		std::memset(manager, 0, 1);
		gStaticHouseCount = 0;
		gStaticHouseEntries = nullptr;
		NXBasics::CIniFileReader reader("data\\engine2d\\inis\\houses\\houses.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
			return manager;

		gStaticHouseCount = NXBasics::Runtime_TimeSigned32(reader.GetNumberOfGroups("GfxHouse"));

		if (gStaticHouseCount == 0)
			return manager;

		const auto bytes = static_cast<std::uint32_t>(gStaticHouseCount) * NC2Layout::RecordSize::EngineHouseTypeUnsigned;
		auto* entries = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(bytes, 1));

		if (entries != nullptr)
		{
			for (std::int32_t index = 0; index < gStaticHouseCount; ++index)
			{
				HouseRecord_Construct(entries + index * NC2Layout::RecordSize::EngineHouseTypeUnsigned);
			}
		}

		gStaticHouseEntries = entries;
		std::memset(entries, 0, bytes);
		auto* entry = entries;

		for (char* group = reader.GetFirstGroup("GfxHouse"); group != nullptr; group = reader.GetNextGroup("GfxHouse"), entry += NC2Layout::RecordSize::EngineHouseType)
		{
			SHouseAreaScratch buildArea;
			SHouseAreaScratch walkAreas[5];
			SHouseAreaScratch workAreas[5];
			buildArea.count = 0;

			for (std::uint32_t stage = 0; stage < 5; ++stage)
			{
				walkAreas[stage].count = workAreas[stage].count = 0;
				Write<std::int32_t>(entry, NC2Layout::EngineHouseTypeOffset::OpenDoorBobIds + stage * NC2Layout::RecordSize::EngineHouseDoorOverlay, -1);
				Write<std::int32_t>(entry, NC2Layout::EngineHouseTypeOffset::ClosedDoorBobIds + stage * NC2Layout::RecordSize::EngineHouseDoorOverlay, -1);
			}

			Write<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::ShadingFactor256, 256);
			const auto overlayCount = reader.GetNumberOfCommandsInGroup("GfxOverlay");
			Write(entry, NC2Layout::EngineHouseTypeOffset::ActionOverlayCount, overlayCount);

			if (overlayCount != 0)
			{
				void* overlays = NXBasics::Runtime_AllocateWithNewHandler(overlayCount * NC2Layout::RecordSize::EngineHouseActionOverlayUnsigned, 1);
				Write(entry, NC2Layout::EngineHouseTypeOffset::ActionOverlays, overlays);
				std::memset(overlays, 0, overlayCount * NC2Layout::RecordSize::EngineHouseActionOverlayUnsigned);
			}

			reader.SetCurrentLineToGroupStart();

			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				const auto* name = reader.GetCurrentName();
				std::size_t offset;

				if (NXBasics::Runtime_CompareStringsIgnoreCase(name, "GfxFirePoint") == 0)
				{
					offset = NC2Layout::EngineHouseStageViewOffset::FirePointCountUnsigned + static_cast<std::uint32_t>(reader.GetInt()) * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned;
				}
				else if (NXBasics::Runtime_CompareStringsIgnoreCase(name, "GfxSmokePoint") == 0)
				{
					const auto stage = static_cast<std::uint32_t>(reader.GetInt());
					reader.GetInt();
					reader.GetInt();
					offset = (reader.GetInt() != 0 ? NC2Layout::EngineHouseStageViewOffset::ForwardSmokePointCountUnsigned : NC2Layout::EngineHouseStageViewOffset::BackSmokePointCountUnsigned) + stage * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned;
				}
				else if (NXBasics::Runtime_CompareStringsIgnoreCase(name, "GfxHolyFirePoint") == 0)
				{
					offset = NC2Layout::EngineHouseStageViewOffset::HolyFirePointCountUnsigned + static_cast<std::uint32_t>(reader.GetInt()) * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned;
				}
				else
				{
					continue;
				}
				// Windows deliberately does not validate the stage during this counting pass.
				Write(entry, offset, Read<std::uint32_t>(entry, offset) + 1u);
			}

			for (std::uint32_t stage = 0; stage < 5; ++stage)
			{
				for (std::uint32_t kind = 0; kind < 4; ++kind)
				{
					const auto offset = NC2Layout::EngineHouseStageViewOffset::FirePointCountUnsigned + stage * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned + static_cast<std::size_t>(kind) * sizeof(SHousePointArrayStorage);
					const auto count = Read<std::uint32_t>(entry, offset);

					if (count != 0)
						Write(entry, offset + offsetof(SHousePointArrayStorage, points), NXBasics::Runtime_AllocateWithNewHandler(count * 8u, 1));
				}
			}

			std::uint32_t pointIndices[4][5]{ };
			std::uint32_t overlayIndex = 0;
			reader.SetCurrentLineToGroupStart();

			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				if ((gStaticHouseStorage.tokenFlags & 1u) == 0)
					gStaticHouseStorage.tokenFlags |= 1u;

				if ((gStaticHouseStorage.tokenFlags & 2u) == 0)
				{
					gStaticHouseStorage.tokenFlags |= 2u;
					gStaticHouseStorage.tokens = gHouseTokens;
					NXBasics::Runtime_AtExit(DestroyHouseTokenList);
				}

				NXBasics::CTokenList tokens(gStaticHouseStorage.tokens);
				const auto token = tokens.FindToken(reader.GetCurrentName());

				switch (token)
				{
					case SwitchIds::HouseGraphicsToken::EditName:
						std::strncpy(reinterpret_cast<char*>(entry), reader.GetString(), 29);
						break;
					case SwitchIds::HouseGraphicsToken::EditGroups:
						std::strncpy(reinterpret_cast<char*>(entry) + 0x1E, reader.GetString(), 29);
						break;
					case SwitchIds::HouseGraphicsToken::LogicTribeType:
						Write(entry, NC2Layout::EngineHouseTypeOffset::TribeType, reader.GetInt());
						break;
					case SwitchIds::HouseGraphicsToken::LogicMaximumSize:
					{
						auto size = static_cast<std::uint32_t>(reader.GetInt());

						if (size > 5)
							size = 5;

						Write(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount, size);

						for (std::uint32_t stage = 0; stage < size; ++stage)
						{
							Write<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::MaximumHitpoints + stage * NC2Layout::RecordSize::HouseStageValue, 1000);
						}

						break;
					}
					case SwitchIds::HouseGraphicsToken::Logictype:
					case SwitchIds::HouseGraphicsToken::LogicHitpoints:
					case SwitchIds::HouseGraphicsToken::Gfxbobid:
					{
						const auto stage = static_cast<std::uint32_t>(reader.GetInt());

						if (stage >= Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount))
							break;

						const auto offset = token == SwitchIds::HouseGraphicsToken::Logictype ? NC2Layout::EngineHouseTypeOffset::LogicTypesUnsigned : token == SwitchIds::HouseGraphicsToken::LogicHitpoints ? NC2Layout::EngineHouseTypeOffset::MaximumHitpointsUnsigned
						                                                                                                                                                                                      : NC2Layout::EngineHouseTypeOffset::StageBobIdsUnsigned;
						Write(entry, offset + stage * NC2Layout::RecordSize::HouseStageValue, reader.GetInt());
						break;
					}
					case SwitchIds::HouseGraphicsToken::LogicBuildBlockArea:
						ReadAreaRow(reader, buildArea);
						break;
					case SwitchIds::HouseGraphicsToken::LogicWalkBlockArea:
					case SwitchIds::HouseGraphicsToken::Logicconstructionworkarea:
					{
						const auto stage = static_cast<std::uint32_t>(reader.GetInt());

						if (stage >= Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount))
							break;

						ReadAreaRow(reader, token == SwitchIds::HouseGraphicsToken::LogicWalkBlockArea ? walkAreas[stage] : workAreas[stage]);
						break;
					}
						// This table entry has no matching Windows switch branch.
					case SwitchIds::HouseGraphicsToken::LogicAdditionalAttachPointArea:
						break;
					case SwitchIds::HouseGraphicsToken::Logicconstructiongoods:
					{
						const auto stage = static_cast<std::uint32_t>(reader.GetInt());

						if (stage >= Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount))
							break;

						const auto offset = NC2Layout::EngineHouseTypeOffset::ConstructionGoodsUnsigned + stage * 0x40u;

						while (reader.GetTypeOfNextParameter() == 3)
						{
							const auto count = Read<std::uint32_t>(entry, offset);

							if (count == 15)
								break;

							Write(entry, offset + offsetof(SHouseConstructionGoodsStorage, goods) + count * NC2Layout::RecordSize::HouseConstructionGoodId, reader.GetInt());
							Write(entry, offset, count + 1u);
						}

						break;
					}
					case SwitchIds::HouseGraphicsToken::Logicdoorpoint:
					{
						const auto stage = static_cast<std::uint32_t>(reader.GetInt());

						if (stage >= Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount))
							break;

						const auto x = static_cast<std::int16_t>(reader.GetInt());
						const auto y = static_cast<std::int16_t>(reader.GetInt());
						NC2Logic::MapMigPoint_Construct(entry + NC2Layout::EngineHouseTypeOffset::DoorPoints + stage * 4, x, y, false);
						break;
					}
					case SwitchIds::HouseGraphicsToken::ArrowSlotData:
					{
						const auto count = Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::ArrowSlotCount);
						const auto offset = NC2Layout::EngineHouseTypeOffset::ArrowSlotsUnsigned + count * 12u;
						Write<std::int32_t>(entry, offset, static_cast<std::int16_t>(reader.GetInt()));
						Write<std::int32_t>(entry, offset + NC2Layout::HouseArrowSlotOffset::Y, static_cast<std::int16_t>(reader.GetInt()));
						Write(entry, offset + NC2Layout::HouseArrowSlotOffset::Parameter, reader.GetInt());
						Write(entry, NC2Layout::EngineHouseTypeOffset::ArrowSlotCount, count + 1u);
						break;
					}
					case SwitchIds::HouseGraphicsToken::Gfxboblibs:
					{
						const char* name = reader.GetString();

						if (std::strlen(name) != 0)
							Write(entry, NC2Layout::EngineHouseTypeOffset::BobManager, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, name));

						name = reader.GetString();

						if (std::strlen(name) != 0)
							Write(entry, NC2Layout::EngineHouseTypeOffset::ShadowBobManager, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, name));

						break;
					}
					case SwitchIds::HouseGraphicsToken::GfxPalette:
					{
						while (reader.GetTypeOfNextParameter() == 2)
						{
							const auto count = Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::PaletteCount);

							if (count == 5)
								break;

							const auto palette = E2StaticDataPalettesManager_GetPalette256Id(reader.GetString());

							if (palette == -1)
								break;

							Write(entry, NC2Layout::EngineHouseTypeOffset::PaletteIds + count * NC2Layout::RecordSize::HouseStageValue, palette);
							Write(entry, NC2Layout::EngineHouseTypeOffset::PaletteCount, count + 1u);
						}

						break;
					}
					case SwitchIds::HouseGraphicsToken::GfxShadingFactor:
						Write<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::ShadingFactor256, static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(reader.GetDouble() * 256.0)));
						break;
					case SwitchIds::HouseGraphicsToken::Gfxdoorbobid:
					{
						const auto stage = static_cast<std::uint32_t>(reader.GetInt());

						if (stage >= Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount))
							break;

						const bool open = reader.GetInt() != 0;
						Write(entry, (open ? NC2Layout::EngineHouseTypeOffset::OpenDoorBobIdsUnsigned : NC2Layout::EngineHouseTypeOffset::ClosedDoorBobIdsUnsigned) + stage * NC2Layout::RecordSize::EngineHouseDoorOverlay, reader.GetInt());
						Write<std::uint8_t>(entry, NC2Layout::EngineHouseTypeOffset::DoorOverlays + stage * NC2Layout::RecordSize::EngineHouseDoorOverlay, 1);
						break;
					}
					case SwitchIds::HouseGraphicsToken::Gfxbobconstructionlayer:
					{
						const auto stage = static_cast<std::uint32_t>(reader.GetInt());
						const bool flag = reader.GetInt() != 0;
						const auto first = reader.GetInt();
						const auto second = reader.GetInt();
						const auto third = reader.GetInt();
						const auto fourth = reader.GetInt();

						if (stage >= Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount))
							break;

						const auto offset = NC2Layout::EngineHouseTypeOffset::ConstructionLayersUnsigned + stage * NC2Layout::RecordSize::EngineHouseConstructionLayersUnsigned;
						const auto count = Read<std::uint32_t>(entry, offset);

						if (count == 5)
							break;

						const auto layer = offset + count * 20u;
						Write<std::uint8_t>(entry, layer + NC2Layout::EngineHouseConstructionListViewOffset::FirstUpgradeFlag, flag);
						Write(entry, layer + NC2Layout::EngineHouseConstructionListViewOffset::FirstBobId, first);
						Write(entry, layer + NC2Layout::EngineHouseConstructionListViewOffset::FirstShadowBobId, second);
						Write(entry, layer + NC2Layout::EngineHouseConstructionListViewOffset::FirstPercentage, third);
						Write(entry, layer + NC2Layout::EngineHouseConstructionListViewOffset::LastPercentage, fourth);
						Write(entry, offset, count + 1u);
						break;
					}
					case SwitchIds::HouseGraphicsToken::GfxOverlay:
					{
						if (overlayIndex >= Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::ActionOverlayCount))
							break;

						auto* overlay = Read<std::byte*>(entry, NC2Layout::EngineHouseTypeOffset::ActionOverlays) + overlayIndex * NC2Layout::RecordSize::EngineHouseActionOverlayUnsigned;
						const auto stage = static_cast<std::uint32_t>(reader.GetInt());
						Write(overlay, NC2Layout::EngineHouseActionOverlayOffset::DevelopmentStage, stage);

						if (stage >= Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount))
							break;

						Write(overlay, NC2Layout::EngineHouseActionOverlayOffset::Action, reader.GetInt());
						Write<std::uint8_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::RequiredActiveFlag, reader.GetInt() != 0);
						Write(overlay, NC2Layout::EngineHouseActionOverlayOffset::OffsetX, reader.GetInt());
						Write(overlay, NC2Layout::EngineHouseActionOverlayOffset::OffsetY, reader.GetInt());
						Write(overlay, NC2Layout::EngineHouseActionOverlayOffset::FrameTicks, reader.GetInt());

						while (reader.GetTypeOfNextParameter() == 3)
						{
							const auto count = Read<std::uint32_t>(overlay, NC2Layout::EngineHouseActionOverlayOffset::FrameCount);

							if (count == 50)
								break;

							Write(overlay, NC2Layout::EngineHouseActionOverlayOffset::BobIds + count * NC2Layout::RecordSize::HouseActionFrameId, reader.GetInt());
							Write(overlay, NC2Layout::EngineHouseActionOverlayOffset::FrameCount, count + 1u);
						}

						++overlayIndex;
						break;
					}
					case SwitchIds::HouseGraphicsToken::GfxOverlayLandscape:
					{
						const auto stage = reader.GetInt();

						if (stage < 0 || stage >= Read<std::int32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount))
							break;

						const auto slot = reader.GetInt();

						if (slot < 0 || static_cast<std::uint32_t>(slot) >= 10)
							break;

						const auto offset = NC2Layout::EngineHouseTypeOffset::LandscapeOverlaysUnsigned + (stage * 10u + slot) * 16u;

						if (Read<std::uint8_t>(entry, offset) != 0)
							break;

						Write<std::uint8_t>(entry, offset, 1);
						Write(entry, offset + NC2Layout::EngineHouseLandscapeOverlayOffset::OffsetX, reader.GetInt());
						Write(entry, offset + NC2Layout::EngineHouseLandscapeOverlayOffset::OffsetY, reader.GetInt());
						Write(entry, offset + NC2Layout::EngineHouseLandscapeOverlayOffset::LandscapeId, E2StaticDataLandscapesManager_GetLandscapeId(reader.GetString()));
						break;
					}
					case SwitchIds::HouseGraphicsToken::GfxFlagPoint:
					case SwitchIds::HouseGraphicsToken::Gfxsoldierflagpoint:
					{
						const auto stage = static_cast<std::uint32_t>(reader.GetInt());

						if (stage >= Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount))
							break;

						const auto offset = (token == SwitchIds::HouseGraphicsToken::GfxFlagPoint ? NC2Layout::EngineHouseStageViewOffset::FlagXUnsigned : NC2Layout::EngineHouseStageViewOffset::SoldierFlagXUnsigned) + stage * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned;

						if (token == SwitchIds::HouseGraphicsToken::Gfxsoldierflagpoint)
							Write<std::uint8_t>(entry, NC2Layout::EngineHouseStageViewOffset::SoldierFlagEnabled + stage * NC2Layout::RecordSize::EngineHouseStagePoints, 1);

						Write(entry, offset, reader.GetInt());
						Write(entry, offset + NC2Layout::PixelPointOffset::Y, reader.GetInt());
						break;
					}
					case SwitchIds::HouseGraphicsToken::GfxFirePoint:
					case SwitchIds::HouseGraphicsToken::GfxHolyFirePoint:
					{
						const auto stage = static_cast<std::uint32_t>(reader.GetInt());

						if (stage >= Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount))
							break;

						const auto kind = token == SwitchIds::HouseGraphicsToken::GfxFirePoint ? 0u : 3u;
						const auto offset = NC2Layout::EngineHouseStageViewOffset::FirePointCountUnsigned + stage * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned + static_cast<std::size_t>(kind) * sizeof(SHousePointArrayStorage);
						auto& index = pointIndices[kind][stage];

						if (index >= Read<std::uint32_t>(entry, offset))
							break;

						void* points = Read<void*>(entry, offset + offsetof(SHousePointArrayStorage, points));
						Write(points, index * NC2Layout::RecordSize::PixelPoint, reader.GetInt());
						Write(points, index * NC2Layout::RecordSize::PixelPoint + NC2Layout::PixelPointOffset::Y, reader.GetInt());
						++index;
						break;
					}
					case SwitchIds::HouseGraphicsToken::GfxSmokePoint:
					{
						const auto stage = static_cast<std::uint32_t>(reader.GetInt());

						if (stage >= Read<std::uint32_t>(entry, NC2Layout::EngineHouseTypeOffset::DevelopmentStageCount))
							break;

						const auto x = reader.GetInt();
						const auto y = reader.GetInt();
						const auto kind = reader.GetInt() != 0 ? 1u : 2u;
						const auto offset = NC2Layout::EngineHouseStageViewOffset::FirePointCountUnsigned + stage * NC2Layout::RecordSize::EngineHouseStagePointsUnsigned + static_cast<std::size_t>(kind) * sizeof(SHousePointArrayStorage);
						auto& index = pointIndices[kind][stage];

						if (index >= Read<std::uint32_t>(entry, offset))
							break;

						void* points = Read<void*>(entry, offset + offsetof(SHousePointArrayStorage, points));
						Write(points, index * NC2Layout::RecordSize::PixelPoint, x);
						Write(points, index * NC2Layout::RecordSize::PixelPoint + NC2Layout::PixelPointOffset::Y, y);
						++index;
						break;
					}
				}
			}

			StoreArea(entry, 0x58, buildArea);

			for (std::uint32_t stage = 0; stage < 5; ++stage)
			{
				StoreArea(entry, NC2Layout::EngineHouseTypeOffset::WalkAreas + static_cast<std::size_t>(stage) * sizeof(void*), walkAreas[stage]);
			}

			for (std::uint32_t stage = 0; stage < 5; ++stage)
			{
				StoreArea(entry, NC2Layout::EngineHouseTypeOffset::WorkAreas + static_cast<std::size_t>(stage) * sizeof(void*), workAreas[stage]);
			}
		}

		return manager;
	}
}