#include "C2DataIds.h"
#include "C2E2StaticDataHumanMisc.h"
#include "C2E2StaticDataPalettes.h"
#include "C2E2StaticDataTerrain.h"
#include "C2EngineLifetimeAccess.h"
#include "C2GameState.h"
#include "C2LanguageTools.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XIniFileReader.h"
#include "XPaletteAccess.h"
#include "XStringArray.h"

#include <cstring>
#include <new>

namespace NC2E2
{
	// Original static region: 00568574..0056857F; one-time token-list lifetime.
	static std::uint8_t gGoodGraphicsTokenFlags = 0;
	static const NXBasics::SToken gGoodGraphicsTokens[] = { { "logicgood", NC2DataIds::HumanMiscToken::LogicGood },{ "graphicshumanrandompalette", NC2DataIds::HumanMiscToken::GraphicsHumanRandomPalette },{ nullptr, 0 } };
	static NXBasics::CTokenList* gGoodGraphicsTokenList = nullptr;
	alignas(NXBasics::CTokenList) static std::byte gGoodGraphicsTokenStorage[sizeof(NXBasics::CTokenList)];

	// Original function: FUN_0048acaf; the original CTokenList destructor is empty.
	static void __cdecl E2StaticDataGoodManager_TokenList_AtExit()
	{}

	// Original function: FUN_0048ab5c.
	void* __fastcall E2StaticDataGoodManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		using NC2Lifetime::Write;
		gE2StaticDataGoodManager = manager;
		std::memset(manager, 0, 1);
		gStaticGoodData = nullptr;
		gStaticGoodData = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::RecordSize::StaticGoodGraphics, 1);
		std::memset(gStaticGoodData, 0, NC2Layout::RecordSize::StaticGoodGraphics);
		for (std::uint32_t offset = 0; offset < NC2Layout::RecordSize::StaticGoodGraphicsHex; offset += 4) Write<std::int32_t>(gStaticGoodData, offset, -1);
		NXBasics::CIniFileReader reader("data\\engine2d\\inis\\goods\\goodgraphics.ini", nullptr, false, nullptr, nullptr);
		if (reader.IsLoaded() && reader.GetFirstGroup("goodgraphics") != nullptr)
		{
			do
			{
				std::int32_t good = -1;
				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					if ((gGoodGraphicsTokenFlags & 1u) == 0)
					{
						gGoodGraphicsTokenFlags |= 1u;
						gGoodGraphicsTokenList = ::new (gGoodGraphicsTokenStorage) NXBasics::CTokenList(gGoodGraphicsTokens);
						NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(E2StaticDataGoodManager_TokenList_AtExit));
					}
					const auto token = gGoodGraphicsTokenList->FindToken(reader.GetCurrentName());
					if (token == NC2DataIds::HumanMiscToken::LogicGood)
					{
						good = reader.GetInt();
						if (good >= NC2Runtime::HumanAppearanceLimits::Goods) good = -1;
					}
					else if (token == NC2DataIds::HumanMiscToken::GraphicsHumanRandomPalette && good != -1)
					{
						const auto patch = StaticHumanMisc_RandomPalette_GetPatchId(reader.GetString());
						Write(gStaticGoodData, static_cast<std::uint32_t>(good) * NC2Layout::RecordSize::GoodGraphicsUnsigned, patch);
					}
				}
			} while (reader.GetNextGroup("goodgraphics") != nullptr);
		}
		return manager;
	}

	// Original function: FUN_0048b213; the original destructor is an empty RET.
	static void __cdecl RandomPaletteTokenList_AtExit()
	{}
	// Original function: FUN_0048bacd; the original destructor is an empty RET.
	static void __cdecl JobBaseTokenList_AtExit()
	{}
	// Original function: FUN_0048bacc; the original destructor is an empty RET.
	static void __cdecl JobChangeTokenList_AtExit()
	{}
	// Original function: FUN_0048bd65; the original destructor is an empty RET.
	static void __cdecl HumanNameTokenList_AtExit()
	{}
	template<void(__cdecl* AtExit)()> static NXBasics::CTokenList& GetLazyTokenList(const NXBasics::SToken* tokens)
	{
		static bool initialized = false;
		alignas(NXBasics::CTokenList) static std::byte storage[sizeof(NXBasics::CTokenList)];
		if (!initialized)
		{
			initialized = true;
			::new (storage) NXBasics::CTokenList(tokens);
			NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(AtExit));
		}
		return *reinterpret_cast<NXBasics::CTokenList*>(storage);
	}

	namespace
	{
		using NC2Lifetime::Read;
		using NC2Lifetime::Write;

		// Complete Windows tables 00506468..0050647F (after lazy initialization),
		// 004F47D0..004F4857, and sign patterns 00506438..0050645F.
		constexpr NXBasics::SToken gPatchTokens[] = { { "name", SwitchIds::HumanNameToken::Name },{ "patch", NC2DataIds::HumanMiscToken::Patch },{ nullptr, 0 } };
		constexpr NXBasics::SToken gJobBaseTokens[] = { { "logictribe", SwitchIds::HumanJobChangeToken::LogicTribe },{ "logicjob", SwitchIds::HumanJobChangeToken::LogicJob },{ "gfxbobmanagerbody", SwitchIds::HumanJobChangeToken::Gfxbobmanagerbody },{ "gfxbobmanagerhead", SwitchIds::HumanJobChangeToken::Gfxbobmanagerhead },{ "gfxpalettebasebody", SwitchIds::HumanJobGraphicsToken::Gfxpalettebasebody },{ "gfxpalettebasehead", SwitchIds::HumanJobGraphicsToken::Gfxpalettebasehead },{ "gfxpaletterandom", SwitchIds::HumanJobChangeToken::GfxPaletteRandom },{ nullptr, 0 } };
		constexpr NXBasics::SToken gJobChangeTokens[] = { { "logictribe", SwitchIds::HumanJobChangeToken::LogicTribe },{ "logicjob", SwitchIds::HumanJobChangeToken::LogicJob },{ "gfxbobmanagerbody", SwitchIds::HumanJobChangeToken::Gfxbobmanagerbody },{ "gfxbobmanagerhead", SwitchIds::HumanJobChangeToken::Gfxbobmanagerhead },{ "gfxpaletterandom", SwitchIds::HumanJobChangeToken::GfxPaletteRandom },{ nullptr, 0 } };
		constexpr NXBasics::SToken gNameTokens[] = { { "logictribe", SwitchIds::HumanJobChangeToken::LogicTribe },{ "name", SwitchIds::HumanNameToken::Name },{ nullptr, 0 } };
		constexpr const char* gSignPatterns[] = { "player%2.2d residence sign 02", "player%2.2d residence sign 01", "player%2.2d residence sign 03", "player%2.2d sign 01", "player%2.2d sign 02", "player%2.2d sign 03", "player%2.2d sign 04", "player%2.2d sign 05", "player%2.2d construction sign", "player%2.2d work extern 01" };

		std::byte* AllocateCleared(std::uint32_t bytes)
		{
			auto* data = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(bytes, 1));
			std::memset(data, 0, bytes);
			return data;
		}

		// Original function: FUN_0048afae; l_RandomPaletteStuff_Load.
		void RandomPaletteStuff_Load()
		{
			NXBasics::CIniFileReader reader("data\\engine2d\\inis\\humans\\randompalette.ini", nullptr, false, nullptr, nullptr);
			if (!reader.IsLoaded()) return;
			const auto count = reader.GetNumberOfGroups("randompalette");
			Write(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::RandomPaletteSetCount, count);
			if (count == 0) return;
			auto* record = AllocateCleared(count * NC2Layout::RecordSize::RandomPaletteSetUnsigned);
			Write(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::RandomPaletteSets, record);
			for (char* group = reader.GetFirstGroup("randompalette"); group != nullptr; group = reader.GetNextGroup("randompalette"), record += NC2Layout::RecordSize::RandomPaletteSet)
			{
				const auto patches = reader.GetNumberOfCommandsInGroup("patch");
				Write(record, NC2Layout::RandomPaletteSetOffset::PatchCount, patches);
				reader.SetCurrentLineToGroupStart();
				auto* data = AllocateCleared(patches * NC2Layout::RecordSize::RandomPalettePatchHexUnsigned);
				Write(record, NC2Layout::RandomPaletteSetOffset::Patches, data);
				std::uint32_t index = 0;
				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					const auto token = GetLazyTokenList<RandomPaletteTokenList_AtExit>(gPatchTokens).FindToken(reader.GetCurrentName());
					if (token == SwitchIds::HumanNameToken::Name) std::strncpy(reinterpret_cast<char*>(record), reader.GetString(), 39);
					else if (token == NC2DataIds::HumanMiscToken::Patch && index != patches)
					{
						auto* patch = data + index * NC2Layout::RecordSize::RandomPalettePatchHexUnsigned;
						Write<std::int32_t>(patch, NC2Layout::RandomPalettePatchOffset::Palette16Id, -1);
						Write<std::int32_t>(patch, NC2Layout::RandomPalettePatchOffset::SourceRange, -1);
						const auto destinationRange = static_cast<std::uint32_t>(reader.GetInt());
						Write(patch, NC2Layout::RandomPalettePatchOffset::DestinationRange, destinationRange);
						if (reader.GetTypeOfNextParameter() == 2) Write(patch, NC2Layout::RandomPalettePatchOffset::Palette16Id, E2StaticDataPalettesManager_GetPalette16Id(reader.GetString()));
						else Write(patch, NC2Layout::RandomPalettePatchOffset::SourceRange, reader.GetInt());
						Write(patch, NC2Layout::RandomPalettePatchOffset::Weight, reader.GetInt());
						const auto rangeCount = Read<std::uint32_t>(record, NC2Layout::RandomPaletteSetOffset::DestinationRangeCount);
						std::uint32_t found = 0;
						while (found < rangeCount && Read<std::uint32_t>(record, NC2Layout::RandomPaletteSetOffset::DestinationRanges + found * NC2Layout::RecordSize::PaletteDestinationRangeUnsigned) != destinationRange) ++found;
						if (found == rangeCount)
						{
							Write(record, NC2Layout::RandomPaletteSetOffset::DestinationRanges + rangeCount * NC2Layout::RecordSize::PaletteDestinationRangeUnsigned, destinationRange);
							Write(record, NC2Layout::RandomPaletteSetOffset::DestinationRangeCount, rangeCount + 1u);
						}
						++index;
					}
				}
			}
		}

		// Original function: FUN_0048b635; l_GraphicsPerJob_Load.
		void GraphicsPerJob_Load()
		{
			NXBasics::CIniFileReader reader("data\\engine2d\\inis\\humans\\jobgraphics.ini", nullptr, false, nullptr, nullptr);
			if (!reader.IsLoaded()) return;
			const auto baseCount = reader.GetNumberOfGroups("jobbasegraphics");
			Write(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::JobBaseGraphicsCount, baseCount);
			if (baseCount != 0)
			{
				auto* record = AllocateCleared(baseCount * NC2Layout::RecordSize::HumanJobBaseGraphicsUnsigned);
				Write(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::JobBaseGraphics, record);
				for (char* group = reader.GetFirstGroup("jobbasegraphics"); group != nullptr; group = reader.GetNextGroup("jobbasegraphics"), record += NC2Layout::RecordSize::HumanJobBaseGraphics)
					while (reader.GetNextCommandLineInGroup() != nullptr)
					{
						const auto token = GetLazyTokenList<JobBaseTokenList_AtExit>(gJobBaseTokens).FindToken(reader.GetCurrentName());
						switch (token)
						{
							case SwitchIds::HumanJobGraphicsToken::LogicTribe: Write(record, NC2Layout::HumanJobGraphicsOffset::Tribe, reader.GetInt()); break;
							case SwitchIds::HumanJobGraphicsToken::LogicJob: Write(record, NC2Layout::HumanJobGraphicsOffset::Job, reader.GetInt()); break;
							case SwitchIds::HumanJobGraphicsToken::Gfxbobmanagerbody:
								{
									const auto count = Read<std::uint32_t>(record, NC2Layout::HumanJobGraphicsOffset::BodyCount);
									if (count == NC2Runtime::HumanAppearanceLimits::GraphicsChoices) break;
									const auto offset = NC2Layout::HumanJobGraphicsOffset::BodyChoices + count * NC2Layout::RecordSize::HumanBodyGraphicsUnsigned;
									Write(record, offset, reader.GetInt());
									Write(record, offset + NC2Layout::HumanBodyGraphicsOffset::BodyBobManager, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, reader.GetString()));
									Write(record, offset + NC2Layout::HumanBodyGraphicsOffset::ShadowBobManager, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, reader.GetString()));
									Write(record, NC2Layout::HumanJobGraphicsOffset::BodyCount, count + 1u);
									break;
								}
							case SwitchIds::HumanJobGraphicsToken::Gfxbobmanagerhead:
								{
									const auto count = Read<std::uint32_t>(record, NC2Layout::HumanJobGraphicsOffset::HeadCount);
									if (count == NC2Runtime::HumanAppearanceLimits::GraphicsChoices) break;
									Write(record, NC2Layout::HumanJobGraphicsOffset::HeadChoices + count * NC2Layout::RecordSize::HumanHeadGraphicsUnsigned, reader.GetInt());
									Write(record, NC2Layout::HumanJobGraphicsOffset::HeadBobManagers + count * NC2Layout::RecordSize::HumanHeadGraphicsUnsigned, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, reader.GetString()));
									Write(record, NC2Layout::HumanJobGraphicsOffset::HeadCount, count + 1u);
									break;
								}
							case SwitchIds::HumanJobGraphicsToken::Gfxpalettebasebody:
							case SwitchIds::HumanJobGraphicsToken::Gfxpalettebasehead:
							case SwitchIds::HumanJobGraphicsToken::GfxPaletteRandom:
								{
									const auto countOffset = token == SwitchIds::HumanJobGraphicsToken::Gfxpalettebasebody ? NC2Layout::HumanJobBaseGraphicsOffset::BodyPaletteCountUnsigned : token == SwitchIds::HumanJobGraphicsToken::Gfxpalettebasehead ? NC2Layout::HumanJobBaseGraphicsOffset::HeadPaletteCountUnsigned : NC2Layout::HumanJobBaseGraphicsOffset::RandomPatchCountUnsigned;
									const auto count = Read<std::uint32_t>(record, countOffset);
									if (count == NC2Runtime::HumanAppearanceLimits::GraphicsChoices) break;
									const char* name = reader.GetString();
									const auto id = token == SwitchIds::HumanJobChangeToken::GfxPaletteRandom ? StaticHumanMisc_RandomPalette_GetPatchId(name) : E2StaticDataPalettesManager_GetPalette256Id(name);
									Write(record, countOffset + NC2Layout::CountedArrayOffset::Entries + count * NC2Layout::RecordSize::PaletteChoiceIdUnsigned, id);
									Write(record, countOffset, count + 1u);
									break;
								}
						}
					}
			}
			const auto changeCount = reader.GetNumberOfGroups("jobchangegraphics");
			Write(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::JobChangeGraphicsCount, changeCount);
			if (changeCount == 0) return;
			auto* record = AllocateCleared(changeCount * NC2Layout::RecordSize::HumanJobChangeGraphicsUnsigned);
			Write(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::JobChangeGraphics, record);
			for (char* group = reader.GetFirstGroup("jobchangegraphics"); group != nullptr; group = reader.GetNextGroup("jobchangegraphics"), record += NC2Layout::RecordSize::HumanJobChangeGraphics)
			{
				Write<std::int32_t>(record, NC2Layout::HumanJobChangeGraphicsOffset::RandomPatchId, -1);
				while (reader.GetNextCommandLineInGroup() != nullptr)
					switch (GetLazyTokenList<JobChangeTokenList_AtExit>(gJobChangeTokens).FindToken(reader.GetCurrentName()))
					{
						case SwitchIds::HumanJobChangeToken::LogicTribe: Write(record, NC2Layout::HumanJobGraphicsOffset::Tribe, reader.GetInt()); break;
						case SwitchIds::HumanJobChangeToken::LogicJob: Write(record, NC2Layout::HumanJobGraphicsOffset::Job, reader.GetInt()); break;
						case SwitchIds::HumanJobChangeToken::Gfxbobmanagerbody:
							{
								const auto count = Read<std::uint32_t>(record, NC2Layout::HumanJobGraphicsOffset::BodyCount);
								if (count == NC2Runtime::HumanAppearanceLimits::GraphicsChoices) break;
								const auto offset = NC2Layout::HumanJobGraphicsOffset::BodyChoices + count * NC2Layout::RecordSize::HumanBodyGraphicsUnsigned;
								Write(record, offset, reader.GetInt());
								Write(record, offset + NC2Layout::HumanBodyGraphicsOffset::BodyBobManager, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, reader.GetString()));
								Write(record, offset + NC2Layout::HumanBodyGraphicsOffset::ShadowBobManager, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, reader.GetString()));
								Write(record, NC2Layout::HumanJobGraphicsOffset::BodyCount, count + 1u);
								break;
							}
						case SwitchIds::HumanJobChangeToken::Gfxbobmanagerhead:
							{
								const auto count = Read<std::uint32_t>(record, NC2Layout::HumanJobGraphicsOffset::HeadCount);
								if (count == NC2Runtime::HumanAppearanceLimits::GraphicsChoices) break;
								Write(record, NC2Layout::HumanJobGraphicsOffset::HeadChoices + count * NC2Layout::RecordSize::HumanHeadGraphicsUnsigned, reader.GetInt());
								Write(record, NC2Layout::HumanJobGraphicsOffset::HeadBobManagers + count * NC2Layout::RecordSize::HumanHeadGraphicsUnsigned, E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, reader.GetString()));
								Write(record, NC2Layout::HumanJobGraphicsOffset::HeadCount, count + 1u);
								break;
							}
						case SwitchIds::HumanJobChangeToken::GfxPaletteRandom: Write(record, NC2Layout::HumanJobChangeGraphicsOffset::RandomPatchId, StaticHumanMisc_RandomPalette_GetPatchId(reader.GetString())); break;
					}
			}
		}

		// Original function: FUN_0048bbed; l_TribeHumanNames_Load.
		void TribeHumanNames_Load()
		{
			char fileName[264];
			NC2Logic::LanguageTool_GetFilename("data\\text\\%s\\strings\\gameobjects\\humannames.ini", fileName, true);
			NXBasics::CIniFileReader reader(fileName, nullptr, false, nullptr, nullptr);
			if (!reader.IsLoaded()) return;
			std::int32_t tribe = -1;
			for (char* group = reader.GetFirstGroup("names"); group != nullptr; group = reader.GetNextGroup("names"))
				while (reader.GetNextCommandLineInGroup() != nullptr)
					switch (GetLazyTokenList<HumanNameTokenList_AtExit>(gNameTokens).FindToken(reader.GetCurrentName()))
					{
						case SwitchIds::HumanNameToken::LogicTribe: tribe = reader.GetInt(); if (tribe < 0 || tribe >= 8) tribe = -1; break;
						case SwitchIds::HumanNameToken::Name:
							if (tribe >= 0)
							{
								const bool female = reader.GetInt() != 0;
								const char* name = reader.GetString();
								const auto offset = NC2Layout::StaticHumanMiscOffset::TribeNameArraysUnsigned + static_cast<std::uint32_t>(tribe) * NC2Layout::RecordSize::TribeHumanNameArraysUnsigned + (female ? NC2Layout::TribeHumanNameArraysOffset::FemaleUnsigned : NC2Layout::TribeHumanNameArraysOffset::MaleUnsigned);
								auto* names = Read<NXBasics::CStringArray*>(gStaticHumanMiscData, offset);
								if (names == nullptr)
								{
									void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CStringArray), 1);
									names = allocation != nullptr ? ::new (allocation) NXBasics::CStringArray(1) : nullptr;
									Write(gStaticHumanMiscData, offset, names);
								}
								names->InsertString(name);
							}
							break;
					}
		}

		// Original function: FUN_0048bafd; l_JobGraphicsBase_GetDataPtr.
		const void* JobGraphicsBase_Find(std::uint32_t tribe, std::uint32_t job)
		{
			const auto* entries = Read<std::byte*>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::JobBaseGraphics);
			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::JobBaseGraphicsCount); ++index)
			{
				const auto* record = entries + index * NC2Layout::RecordSize::HumanJobBaseGraphicsUnsigned;
				if (Read<std::uint32_t>(record, NC2Layout::HumanJobGraphicsOffset::Tribe) == tribe && Read<std::uint32_t>(record, NC2Layout::HumanJobGraphicsOffset::Job) == job) return record;
			}
			return nullptr;
		}

		// Original function: FUN_0048bb6a; l_JobGraphicsChange_GetDataPtr.
		const void* JobGraphicsChange_Find(std::uint32_t tribe, std::uint32_t job)
		{
			const auto* entries = Read<std::byte*>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::JobChangeGraphics);
			for (std::uint32_t index = 0; index < Read<std::uint32_t>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::JobChangeGraphicsCount); ++index)
			{
				const auto* record = entries + index * NC2Layout::RecordSize::HumanJobChangeGraphicsUnsigned;
				if (Read<std::uint32_t>(record, NC2Layout::HumanJobGraphicsOffset::Tribe) == tribe && Read<std::uint32_t>(record, NC2Layout::HumanJobGraphicsOffset::Job) == job) return record;
			}
			return nullptr;
		}
	}

	// Original function: FUN_0048acfb; load order and ten color records follow Windows.
	void* __fastcall E2StaticDataHumanMiscManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gE2StaticDataHumanMiscManager = manager;
		std::memset(manager, 0, 1);
		std::memset(gStaticHumanMiscData, 0, sizeof(gStaticHumanMiscData));
		RandomPaletteStuff_Load();
		GraphicsPerJob_Load();
		TribeHumanNames_Load();
		for (std::uint32_t color = 0; color < 10; ++color)
		{
			auto* record = gStaticHumanColors + color * NC2Layout::RecordSize::StaticHumanColorUnsigned;
			char name[80];
			NXBasics::Runtime_PrintString(name, "player_%2.2d", static_cast<int>(color));
			const auto patchId = StaticHumanMisc_RandomPalette_GetPatchId(name);
			Write(record, NC2Layout::StaticHumanColorOffset::MaleRandomPatchId, patchId);
			if (patchId != -1)
			{
				const auto* patchRecord = Read<std::byte*>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::RandomPaletteSets) + static_cast<std::uint32_t>(patchId) * NC2Layout::RecordSize::RandomPaletteSetUnsigned;
				const auto count = Read<std::uint32_t>(patchRecord, NC2Layout::RandomPaletteSetOffset::PatchCount);
				const auto* patches = Read<std::byte*>(patchRecord, NC2Layout::RandomPaletteSetOffset::Patches);
				std::uint32_t index = 0;
				while (index < count && Read<std::uint32_t>(patches, index * NC2Layout::RecordSize::RandomPalettePatchUnsigned) != 10) ++index;
				if (index == count) continue;
				const auto paletteId = Read<std::int32_t>(patches, index * NC2Layout::RecordSize::RandomPalettePatchUnsigned + NC2Layout::RandomPalettePatchOffset::Palette16Id);
				const auto& palette16 = gStaticPalettes.palettes16[paletteId];
				const auto* palette = gStaticPalettes.palettes256[palette16.palette256Id].palette;
				Write(record, NC2Layout::StaticHumanColorOffset::Color, Read<std::uint32_t>(palette, palette16.colorRange * NC2Runtime::HumanPaletteRange::BytesUnsigned + NC2Layout::PaletteOffset::PlayerColorSampleWithinRange));
			}
			NXBasics::Runtime_PrintString(name, "woman_%2.2d", static_cast<int>(color));
			Write(record, NC2Layout::StaticHumanColorOffset::FemaleRandomPatchId, StaticHumanMisc_RandomPalette_GetPatchId(name));
			for (std::uint32_t sign = 0; sign < 10; ++sign)
			{
				NXBasics::Runtime_PrintString(name, gSignPatterns[sign], static_cast<int>(color + 1u));
				Write(record, NC2Layout::StaticHumanColorOffset::InhabitantSigns + sign * NC2Layout::RecordSize::HumanSignGraphicsIdUnsigned, E2StaticDataLandscapesManager_GetLandscapeId(name));
			}
			for (std::uint32_t soldier = 0; soldier < 5; ++soldier)
			{
				NXBasics::Runtime_PrintString(name, "player%2.2d soldier %2.2d", static_cast<int>(color + 1u), static_cast<int>(soldier + 1u));
				Write(record, NC2Layout::StaticHumanColorOffset::SoldierSigns + soldier * NC2Layout::RecordSize::HumanSignGraphicsIdUnsigned, E2StaticDataLandscapesManager_GetLandscapeId(name));
			}
			NXBasics::Runtime_PrintString(name, "player%2.2d marker road", static_cast<int>(color + 1u));
			Write(record, NC2Layout::StaticHumanColorOffset::RoadBuildSignLandscapeId, E2StaticDataLandscapesManager_GetLandscapeId(name));
			NXBasics::Runtime_PrintString(name, "player%2.2d marker wall", static_cast<int>(color + 1u));
			Write(record, NC2Layout::StaticHumanColorOffset::WallBuildSignLandscapeId, E2StaticDataLandscapesManager_GetLandscapeId(name));
			NXBasics::Runtime_PrintString(name, "human_player%2.2d", static_cast<int>(color + 1u));
			Write(record, NC2Layout::StaticHumanColorOffset::PlayerPaletteId, E2StaticDataPalettesManager_GetPalette256Id(name));
			NXBasics::Runtime_PrintString(name, "human_ship%2.2d", static_cast<int>(color + 1u));
			Write(record, NC2Layout::StaticHumanColorOffset::ShipPaletteId, E2StaticDataPalettesManager_GetPalette256Id(name));
		}
		return manager;
	}

	// Original function: FUN_0048b214; RandomPalette_GetPatchId.
	std::int32_t StaticHumanMisc_RandomPalette_GetPatchId(const char* name)
	{
		if (name == nullptr) return -1;
		const auto* records = NC2Lifetime::Read<std::byte*>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::RandomPaletteSets);
		for (std::uint32_t index = 0; index < NC2Lifetime::Read<std::uint32_t>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::RandomPaletteSetCount); ++index)
			if (std::strcmp(reinterpret_cast<const char*>(records + index * NC2Layout::RecordSize::RandomPaletteSetUnsigned), name) == 0) return static_cast<std::int32_t>(index);
		return -1;
	}

	// Original function: FUN_0048b258; weights and comparisons retain DWORD arithmetic.
	void StaticHumanMisc_RandomPalette_Execute(std::uint32_t patchId, NXBasics::CPalette* body, NXBasics::CPalette* head, bool deterministic)
	{
		using NC2Lifetime::Read;
		if (patchId >= Read<std::uint32_t>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::RandomPaletteSetCount)) return;
		const auto* record = Read<std::byte*>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::RandomPaletteSets) + patchId * NC2Layout::RecordSize::RandomPaletteSetUnsigned;
		const auto count = Read<std::uint32_t>(record, NC2Layout::RandomPaletteSetOffset::PatchCount);
		const auto* patches = Read<std::byte*>(record, NC2Layout::RandomPaletteSetOffset::Patches);
		for (std::uint32_t index = 0; index < Read<std::uint32_t>(record, NC2Layout::RandomPaletteSetOffset::DestinationRangeCount); ++index)
		{
			const auto destinationRange = Read<std::uint32_t>(record, NC2Layout::RandomPaletteSetOffset::DestinationRanges + index * NC2Layout::RecordSize::PaletteDestinationRangeUnsigned);
			std::uint32_t totalWeight = 0;
			for (std::uint32_t patch = 0; patch < count; ++patch)
			{
				if (Read<std::uint32_t>(patches, patch * NC2Layout::RecordSize::RandomPalettePatchUnsigned) == destinationRange)
					totalWeight += Read<std::uint32_t>(patches, patch * NC2Layout::RecordSize::RandomPalettePatchUnsigned + NC2Layout::RandomPalettePatchOffset::Weight);
			}
			if (totalWeight == 0) continue;
			const auto choice = deterministic ? 0u : NC2Logic::Logic_EntryPoint_Random_GetNumber() % totalWeight;
			std::uint32_t accumulatedWeight = 0;
			const auto* selected = patches;
			for (std::uint32_t patch = 0; patch < count; ++patch, selected += NC2Layout::RecordSize::RandomPalettePatch)
			{
				if (Read<std::uint32_t>(selected, NC2Layout::RandomPalettePatchOffset::DestinationRange) != destinationRange) continue;
				accumulatedWeight += Read<std::uint32_t>(selected, NC2Layout::RandomPalettePatchOffset::Weight);
				if (NXBasics::Runtime_TimeSigned32(choice) < NXBasics::Runtime_TimeSigned32(accumulatedWeight)) break;
			}
			const auto palette16Id = Read<std::int32_t>(selected, NC2Layout::RandomPalettePatchOffset::Palette16Id);
			const std::byte* source;
			if (palette16Id != -1)
			{
				const auto& palette16 = gStaticPalettes.palettes16[palette16Id];
				source = reinterpret_cast<const std::byte*>(gStaticPalettes.palettes256[palette16.palette256Id].palette) + NC2Layout::PaletteOffset::Colors + palette16.colorRange * NC2Runtime::HumanPaletteRange::BytesUnsigned;
			}
			else
			{
				const auto sourceRange = Read<std::int32_t>(selected, NC2Layout::RandomPalettePatchOffset::SourceRange);
				if (sourceRange == -1) continue;
				const auto* palette = sourceRange < NC2Runtime::HumanPaletteRange::FirstHeadRange ? body : head;
				if (palette == nullptr) continue;
				source = reinterpret_cast<const std::byte*>(palette) + NC2Layout::PaletteOffset::Colors + (static_cast<std::uint32_t>(sourceRange) & NC2Runtime::HumanPaletteRange::LocalRangeMask) * NC2Runtime::HumanPaletteRange::BytesUnsigned;
			}
			if (source == nullptr) continue;
			auto* destination = destinationRange < NC2Runtime::HumanPaletteRange::FirstHeadRange ? body : head;
			if (destination == nullptr) continue;
			std::memcpy(reinterpret_cast<std::byte*>(destination) + NC2Layout::PaletteOffset::Colors + (destinationRange & NC2Runtime::HumanPaletteRange::LocalRangeMask) * NC2Runtime::HumanPaletteRange::BytesUnsigned, source, NC2Runtime::HumanPaletteRange::Bytes);
			NXBasics::Palette_PaletteChanged(destination);
		}
	}

	// Original function: FUN_0048b44f; callers must supply a nonnegative player ID.
	std::int32_t StaticHumanMisc_ColorIds_GetPlayerRandomPatchId(std::int32_t player, bool female)
	{
		using NC2Lifetime::Read;
		if (player >= 20) return -1;
		const auto offset = static_cast<std::uint32_t>(player) * NC2Layout::RecordSize::GameControlPlayer;
		if (Read<std::uint8_t>(NC2Logic::gGameControl, offset) == 0) return -1;
		auto color = Read<std::int32_t>(NC2Logic::gGameControl, offset + NC2Layout::GameControlPlayerOffset::ColorId);
		if (color < 0 || color >= 10) color = 9;
		return Read<std::int32_t>(gStaticHumanColors, static_cast<std::uint32_t>(color) * NC2Layout::RecordSize::StaticHumanColorUnsigned + (female ? NC2Layout::StaticHumanColorOffset::FemaleRandomPatchIdUnsigned : NC2Layout::StaticHumanColorOffset::MaleRandomPatchIdUnsigned));
	}

	// Original function: FUN_0048bace; parent-job fallback, then the first base record.
	const void* StaticHumanMisc_JobGraphicsBase_GetDataPtr(std::uint32_t tribe, std::uint32_t job)
	{
		while (true)
		{
			if (const void* record = JobGraphicsBase_Find(tribe, job)) return record;
			job = NC2Lifetime::Read<std::uint32_t>(NC2Logic::gStaticJobTypeData, job * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::BaseJob);
			if (job == 0) return NC2Lifetime::Read<void*>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::JobBaseGraphics);
		}
	}

	// Original function: FUN_0048bb3e; parent-job fallback without a base-record default.
	const void* StaticHumanMisc_JobGraphicsChange_GetDataPtr(std::uint32_t tribe, std::uint32_t job)
	{
		while (true)
		{
			if (const void* record = JobGraphicsChange_Find(tribe, job)) return record;
			job = NC2Lifetime::Read<std::uint32_t>(NC2Logic::gStaticJobTypeData, job * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::BaseJob);
			if (job == 0) return nullptr;
		}
	}

	// Original function: FUN_0048bba5; the Windows fallback string is "<noname>".
	const char* StaticHumanMisc_TribeHumanNames_GetRandomNamePtr(std::uint32_t tribe, bool female)
	{
		using NC2Lifetime::Read;
		if (NXBasics::Runtime_TimeSigned32(tribe) >= 8) return "<noname>";
		auto* names = Read<NXBasics::CStringArray*>(gStaticHumanMiscData, NC2Layout::StaticHumanMiscOffset::TribeNameArraysUnsigned + tribe * NC2Layout::RecordSize::TribeHumanNameArraysUnsigned + (female ? NC2Layout::TribeHumanNameArraysOffset::FemaleUnsigned : NC2Layout::TribeHumanNameArraysOffset::MaleUnsigned));
		if (names == nullptr || names->GetNumberOfStrings() == 0) return "<noname>";
		return names->GetStringPtr(NC2Logic::Logic_EntryPoint_Random_GetNumber() % names->GetNumberOfStrings());
	}
}
