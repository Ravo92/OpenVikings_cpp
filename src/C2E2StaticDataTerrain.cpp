#include "C2E2StaticDataGroups.h"
#include "C2E2StaticDataTerrain.h"
#include "C2EngineLifetimeAccess.h"
#include "C2EngineTerrainData.h"
#include "C2LifetimeGlobals.h"
#include "C2MapArea.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XFile.h"
#include "XIniFileReader.h"
#include "XList.h"
#include "XPicture.h"

#include <cstring>
#include <new>

namespace
{
	// Complete Windows arrays 004F4A58..004F4A8F and 004F4A98..004F4B3F.
	const NXBasics::SToken gPatternTokens[] = {
		{ "editname", SwitchIds::LandscapeToken::EditName }, { "editgroups", SwitchIds::LandscapeToken::EditGroups }, { "logictype", SwitchIds::LandscapeToken::Logictype }, { "gfxtexture", SwitchIds::PatternToken::GfxTexture }, { "gfxcoordsa", SwitchIds::PatternToken::GfxCoordsA }, { "gfxcoordsb", SwitchIds::PatternToken::GfxCoordsB }, { nullptr, 0 }
	};
	const NXBasics::SToken gLandscapeTokens[] = {
		{ "editname", SwitchIds::LandscapeToken::EditName }, { "editgroups", SwitchIds::LandscapeToken::EditGroups }, { "logictype", SwitchIds::LandscapeToken::Logictype }, { "logicmaximumvalency", SwitchIds::LandscapeToken::LogicMaximumValency }, { "logicisworkable", SwitchIds::LandscapeToken::LogicIsWorkable }, { "logicispileableonmap", SwitchIds::LandscapeToken::LogicIsPileableOnMap }, { "logicwalkblockarea", SwitchIds::LandscapeToken::LogicWalkBlockArea }, { "logicbuildblockarea", SwitchIds::LandscapeToken::LogicBuildBlockArea }, { "logicworkarea", SwitchIds::LandscapeToken::Logicworkarea }, { "logicadditionalattachpointarea", SwitchIds::LandscapeToken::LogicAdditionalAttachPointArea }, { "gfxboblibs", SwitchIds::LandscapeToken::Gfxboblibs }, { "gfxframes", SwitchIds::LandscapeToken::GfxFrames }, { "gfxpalette", SwitchIds::LandscapeToken::GfxPalette }, { "gfxstatic", SwitchIds::LandscapeToken::GfxStatic }, { "gfxloopanimation", SwitchIds::LandscapeToken::GfxLoopAnimation }, { "gfxshadingfactor", SwitchIds::LandscapeToken::GfxShadingFactor }, { "gfxtransition", SwitchIds::LandscapeToken::GfxTransition }, { "gfxuserfxmatrix", SwitchIds::LandscapeToken::GfxUserFxMatrix }, { "gfxdynamicbackground", SwitchIds::LandscapeToken::GfxDynamicBackground }, { "gfxdrawvoidever", SwitchIds::LandscapeToken::GfxDrawVoidEver }, { nullptr, 0 }
	};

	struct SPatternTokenState
	{
		std::uint32_t flags;
		const NXBasics::SToken* table;
	};

	SPatternTokenState gPatternTokenState{ };               // 00568A48..00568A4F
	const NXBasics::SToken* gLandscapeTokenTable = nullptr; // 00568A70
	std::uint32_t gLandscapeTokenInitFlags = 0;             // 00568AE8..00568AEB

	// Original function: FUN_0048e568.
	int __cdecl DestroyPatternTokenList()
	{

		return 0;
	}

	// Original function: FUN_00490276.
	int __cdecl DestroyLandscapeTokenList()
	{

		return 0;
	}

	struct SLandscapeFrameWork
	{
		std::uint32_t count;
		std::int32_t frames[150];
	};

	struct SLandscapeAreaWork
	{
		std::uint32_t count;
		NC2Logic::SMapAreaRow rows[80];
	};

	struct SLandscapeTransitionWorkEntry
	{
		std::int32_t type;
		char name[40];
		std::int32_t landscapeId;
	};

	struct SLandscapeTransitionWork
	{
		std::uint32_t count;
		SLandscapeTransitionWorkEntry entries[4];
	};

	static_assert(sizeof(gPatternTokens) == 7 * sizeof(NXBasics::SToken));
	static_assert(sizeof(gLandscapeTokens) == 21 * sizeof(NXBasics::SToken));
	static_assert(sizeof(SPatternTokenState) == (sizeof(void*) == 8 ? 16 : 8));
	static_assert(sizeof(SLandscapeFrameWork) == 604);
	static_assert(sizeof(SLandscapeAreaWork) == 644);
	static_assert(sizeof(SLandscapeTransitionWorkEntry) == 48);
	static_assert(sizeof(SLandscapeTransitionWork) == 196);

	// Source-level helper for the original word stores from CIniFileReader::GetInt.
	std::int16_t ReadAreaCoordinate(NXBasics::CIniFileReader& reader)
	{
		const std::uint16_t bits = static_cast<std::uint16_t>(reader.GetInt());
		std::int16_t coordinate;
		std::memcpy(&coordinate, &bits, sizeof(coordinate));

		return coordinate;
	}
}

namespace NC2E2
{
	// Original function: FUN_0048e24e; CE2StaticDataPatternManager constructor.
	void* __fastcall E2StaticDataPatternManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gE2StaticDataPatternManager = manager;
		std::memset(manager, 0, 4004);
		std::memset(&gStaticPatternStorage, 0, sizeof(gStaticPatternStorage));
		NXBasics::CIniFileReader reader("data\\engine2d\\inis\\patterns\\pattern.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
			return manager;

		gStaticPatternCount = reader.GetNumberOfGroups("GfxPattern");

		if (gStaticPatternCount != 0)
		{
			const std::size_t bytes = static_cast<std::size_t>(gStaticPatternCount) * sizeof(SE2StaticDataPatternData);
			gStaticPatternEntries = NXBasics::Runtime_AllocateArrayWithNewHandler(gStaticPatternCount, sizeof(SE2StaticDataPatternData));
			// Original embedded texture constructor 0048E569 only returns this.
			std::memset(gStaticPatternEntries, 0, bytes);
			auto* entry = static_cast<SE2StaticDataPatternData*>(gStaticPatternEntries);

			for (char* group = reader.GetFirstGroup("GfxPattern"); group != nullptr; group = reader.GetNextGroup("GfxPattern"), ++entry)
			{
				while (reader.GetNextCommandLineInGroup() != nullptr)
				{
					if ((gPatternTokenState.flags & 1u) == 0)
					{
						gPatternTokenState.flags |= 1u;
						gPatternTokenState.table = gPatternTokens;
						NXBasics::Runtime_AtExit(DestroyPatternTokenList);
					}

					NXBasics::CTokenList tokens(gPatternTokenState.table);

					switch (tokens.FindToken(reader.GetCurrentName()))
					{
						case SwitchIds::PatternToken::EditName:
							std::strncpy(entry->name, reader.GetString(), 39);
							break;
						case SwitchIds::PatternToken::EditGroups:
							while (reader.GetTypeOfNextParameter() == 2 && entry->groupCount != 3)
							{
								const std::int32_t id = StaticDataGroups_AddName(manager, reader.GetString());
								entry->groupIds[entry->groupCount++] = id;
							}

							break;
						case SwitchIds::PatternToken::Logictype:
							entry->logicType = reader.GetInt();
							break;
						case SwitchIds::PatternToken::GfxTexture:
							entry->textureId = E2StaticDataTexturesManager_GetTextureId(gE2StaticDataTexturesManager, nullptr, reader.GetString());
							break;
						case SwitchIds::PatternToken::GfxCoordsA:
							for (std::int32_t& coordinate : entry->coordinatesA)
							{
								coordinate = reader.GetInt();
							}

							break;
						case SwitchIds::PatternToken::GfxCoordsB:
							for (std::int32_t& coordinate : entry->coordinatesB)
							{
								coordinate = reader.GetInt();
							}

							break;
					}
				}
			}
		}

		gStaticPatternStorage.borderIds[0] = E2StaticDataPatternManager_GetPatternId("border");
		gStaticPatternStorage.borderIds[1] = E2StaticDataPatternManager_GetPatternId("border");
		gStaticPatternStorage.roadOverlayId = E2StaticDataPatternManager_GetPatternId("overlay road");
		gStaticPatternStorage.houseOverlayId = E2StaticDataPatternManager_GetPatternId("overlay house");

		return manager;
	}

	// Original function: FUN_0048e58a; CE2StaticDataPatternManager::GetPatternId.
	std::int32_t E2StaticDataPatternManager_GetPatternId(const char* name)
	{
		const auto* entries = static_cast<const SE2StaticDataPatternData*>(gStaticPatternEntries);

		for (std::int32_t index = 0; index < static_cast<std::int32_t>(gStaticPatternCount); ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(entries[index].name, name) == 0)
				return index;
		}

		return -1;
	}

	// Original function: FUN_0048f79f; CE2StaticDataLandscapesManager constructor.
	void* __fastcall E2StaticDataLandscapesManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gE2StaticDataLandscapesManager = manager;
		std::memset(manager, 0, 4004);
		std::memset(&gStaticLandscapeStorage, 0, sizeof(gStaticLandscapeStorage));
		NXBasics::CIniFileReader reader("data\\engine2d\\inis\\landscapes\\landscapes.ini", nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded())
			return manager;

		gStaticLandscapeCount = static_cast<std::int32_t>(reader.GetNumberOfGroups("GfxLandscape"));

		if (gStaticLandscapeCount == 0)
			return manager;

		const std::size_t entryBytes = static_cast<std::size_t>(static_cast<std::uint32_t>(gStaticLandscapeCount)) * sizeof(SE2StaticDataLandscape);
		gStaticLandscapeEntries = NXBasics::Runtime_AllocateArrayWithNewHandler(static_cast<std::uint32_t>(gStaticLandscapeCount), sizeof(SE2StaticDataLandscape));
		std::memset(gStaticLandscapeEntries, 0, entryBytes);
		const std::size_t workBytes = static_cast<std::size_t>(static_cast<std::uint32_t>(gStaticLandscapeCount)) * sizeof(SLandscapeTransitionWork);
		auto* transitionWork = static_cast<SLandscapeTransitionWork*>(NXBasics::Runtime_AllocateArrayWithNewHandler(static_cast<std::uint32_t>(gStaticLandscapeCount), sizeof(SLandscapeTransitionWork)));
		std::memset(transitionWork, 0, workBytes);
		auto* entry = static_cast<SE2StaticDataLandscape*>(gStaticLandscapeEntries);
		auto* pending = transitionWork;
		SLandscapeFrameWork frameWork[101];
		SLandscapeAreaWork areaWork[4][12];

		for (char* group = reader.GetFirstGroup("GfxLandscape"); group != nullptr; group = reader.GetNextGroup("GfxLandscape"), ++entry, ++pending)
		{
			for (SLandscapeFrameWork& frames : frameWork)
			{
				frames.count = 0;
			}

			for (auto& areas : areaWork)
			{
				for (SLandscapeAreaWork& area : areas)
				{
					area.count = 0;
				}
			}

			entry->paletteId = -1;
			entry->dynamicForeground = 1;
			entry->shadingFactor = 0x100;
			entry->maximumValency = 1;

			while (reader.GetNextCommandLineInGroup() != nullptr)
			{
				if ((gLandscapeTokenInitFlags & 1u) == 0)
				{
					gLandscapeTokenInitFlags |= 1u;
					gLandscapeTokenTable = gLandscapeTokens;
					NXBasics::Runtime_AtExit(DestroyLandscapeTokenList);
				}

				NXBasics::CTokenList tokens(gLandscapeTokenTable);
				const std::uint32_t command = tokens.FindToken(reader.GetCurrentName());

				switch (command)
				{
					case SwitchIds::LandscapeToken::EditName:
						std::strncpy(entry->name, reader.GetString(), 39);
						break;
					case SwitchIds::LandscapeToken::EditGroups:
						while (reader.GetTypeOfNextParameter() == 2 && entry->groupCount != 3)
						{
							const std::int32_t id = StaticDataGroups_AddName(manager, reader.GetString());
							entry->groupIds[entry->groupCount++] = id;
						}

						break;
					case SwitchIds::LandscapeToken::Logictype:
						entry->logicType = reader.GetInt();
						break;
					case SwitchIds::LandscapeToken::LogicMaximumValency:
						entry->maximumValency = reader.GetInt();
						break;
					case SwitchIds::LandscapeToken::LogicIsWorkable:
						entry->isWorkable = reader.GetInt() != 0;
						break;
					case SwitchIds::LandscapeToken::LogicIsPileableOnMap:
						entry->isPileableOnMap = reader.GetInt() != 0;
						break;
					case SwitchIds::LandscapeToken::LogicWalkBlockArea:
					case SwitchIds::LandscapeToken::LogicBuildBlockArea:
					case SwitchIds::LandscapeToken::Logicworkarea:
					case SwitchIds::LandscapeToken::LogicAdditionalAttachPointArea:
					{
						const std::uint32_t areaGroup = command == SwitchIds::LandscapeToken::LogicWalkBlockArea ? 0u : command == SwitchIds::LandscapeToken::LogicBuildBlockArea ? 1u
						                                                                                            : command == SwitchIds::LandscapeToken::Logicworkarea         ? 2u
						                                                                                                                                                          : 3u;
						const std::uint32_t valency = areaGroup == 3 ? 0u : static_cast<std::uint32_t>(reader.GetInt());

						if (valency >= 12)
							break;

						SLandscapeAreaWork& area = areaWork[areaGroup][valency];

						if (area.count == 80)
							break;

						NC2Logic::SMapAreaRow& row = area.rows[area.count];
						row.x = ReadAreaCoordinate(reader);
						row.y = ReadAreaCoordinate(reader);
						row.width = static_cast<std::uint32_t>(reader.GetInt());
						++area.count;
						break;
					}
					case SwitchIds::LandscapeToken::Gfxboblibs:
					{
						const char* fileName = reader.GetString();

						if (std::strlen(fileName) != 0)
							entry->bobManagers[0] = E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, fileName);

						if (reader.GetTypeOfNextParameter() == 2)
						{
							fileName = reader.GetString();

							if (std::strlen(fileName) != 0)
								entry->bobManagers[1] = E2StaticDataBobManagersManager_GetBobManagerPtr(gE2StaticDataBobManagersManager, nullptr, fileName);
						}

						break;
					}
					case SwitchIds::LandscapeToken::GfxFrames:
					{
						const std::uint32_t valency = static_cast<std::uint32_t>(reader.GetInt());

						if (valency > 100)
							break;

						SLandscapeFrameWork& frames = frameWork[valency];

						while (reader.GetTypeOfNextParameter() == 3 && frames.count != 150)
							frames.frames[frames.count++] = reader.GetInt();

						break;
					}
					case SwitchIds::LandscapeToken::GfxPalette:
						entry->paletteId = E2StaticDataPalettesManager_GetPalette256Id(reader.GetString());
						break;
					case SwitchIds::LandscapeToken::GfxStatic:
						entry->isStatic = reader.GetInt() != 0;

						if (entry->isStatic != 0)
							entry->dynamicForeground = 0;

						break;
					case SwitchIds::LandscapeToken::GfxLoopAnimation:
						entry->loopAnimation = reader.GetInt() != 0;
						break;
					case SwitchIds::LandscapeToken::GfxShadingFactor:
						entry->shadingFactor = static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(reader.GetDouble() * 256.0));
						break;
					case SwitchIds::LandscapeToken::GfxTransition:
					{
						if (pending->count == 4)
							break;

						SLandscapeTransitionWorkEntry& transition = pending->entries[pending->count];
						transition.type = reader.GetInt();
						std::strncpy(transition.name, reader.GetString(), 39);

						if (transition.type == 13)
							entry->hasFireTransition = 1;

						++pending->count;
						break;
					}
					case SwitchIds::LandscapeToken::GfxUserFxMatrix:
						entry->userFxMatrix = reader.GetInt() != 0;
						break;
					case SwitchIds::LandscapeToken::GfxDynamicBackground:
						entry->dynamicBackground = reader.GetInt() != 0;

						if (entry->dynamicBackground != 0)
							entry->dynamicForeground = 0;

						break;
					case SwitchIds::LandscapeToken::GfxDrawVoidEver:
						entry->drawVoidEver = reader.GetInt() != 0;
						break;
				}
			}

			for (const SLandscapeFrameWork& frames : frameWork)
			{
				if (frames.count != 0)
					++entry->frameGroupCount;
			}

			if (entry->frameGroupCount != 0)
			{
				const std::size_t bytes = static_cast<std::size_t>(entry->frameGroupCount) * sizeof(SStaticLandscapeFrames);
				entry->frameGroups = static_cast<SStaticLandscapeFrames*>(NXBasics::Runtime_AllocateArrayWithNewHandler(entry->frameGroupCount, sizeof(SStaticLandscapeFrames)));
				std::memset(entry->frameGroups, 0, bytes);
				auto* destination = entry->frameGroups;

				for (std::uint32_t valency = 0; valency <= 100; ++valency)
				{
					const SLandscapeFrameWork& frames = frameWork[valency];

					if (frames.count == 0)
						continue;

					destination->valency = valency;
					destination->count = frames.count;
					destination->frames = static_cast<std::int32_t*>(NXBasics::Runtime_AllocateArrayWithNewHandler(frames.count, sizeof(std::int32_t)));
					std::memcpy(destination->frames, frames.frames, static_cast<std::size_t>(frames.count) * sizeof(std::int32_t));
					++destination;
				}
			}

			for (std::uint32_t areaGroup = 0; areaGroup < 4; ++areaGroup)
			{
				SStaticLandscapeAreaList& areas = entry->areas[areaGroup];

				for (const SLandscapeAreaWork& area : areaWork[areaGroup])
				{
					if (area.count != 0)
						++areas.count;
				}

				if (areas.count == 0)
					continue;

				areas.entries = static_cast<SStaticLandscapeArea*>(NXBasics::Runtime_AllocateArrayWithNewHandler(areas.count, sizeof(SStaticLandscapeArea)));
				std::memset(areas.entries, 0, static_cast<std::size_t>(areas.count) * sizeof(SStaticLandscapeArea));
				auto* destination = areas.entries;

				for (std::uint32_t valency = 0; valency < 12; ++valency)
				{
					const SLandscapeAreaWork& area = areaWork[areaGroup][valency];

					if (area.count == 0)
						continue;

					destination->valency = valency;
					void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NC2Logic::CMapArea), 1);
					destination->area = allocation != nullptr ? ::new (allocation) NC2Logic::CMapArea(area.count) : nullptr;
					// The original continues unchecked if either allocation failed.
					std::memcpy(destination->area->rows, area.rows, static_cast<std::size_t>(area.count) * sizeof(NC2Logic::SMapAreaRow));
					destination->area->UpdateCalculatedVariables();
					++destination;
				}
			}
		}

		auto* entries = static_cast<SE2StaticDataLandscape*>(gStaticLandscapeEntries);

		for (std::int32_t index = 0; index < gStaticLandscapeCount; ++index)
		{
			SE2StaticDataLandscape& landscape = entries[index];
			SLandscapeTransitionWork& work = transitionWork[index];

			for (std::uint32_t transition = 0; transition < work.count; ++transition)
			{
				auto& target = work.entries[transition];
				target.landscapeId = -1;

				for (std::int32_t candidate = 0; candidate < gStaticLandscapeCount; ++candidate)
				{
					if (NXBasics::Runtime_CompareStringsIgnoreCase(target.name, entries[candidate].name) != 0)
						continue;

					target.landscapeId = candidate;
					++landscape.transitionCount;
					break;
				}
			}

			if (landscape.transitionCount == 0)
				continue;

			landscape.transitions = static_cast<SStaticLandscapeTransition*>(NXBasics::Runtime_AllocateArrayWithNewHandler(landscape.transitionCount, sizeof(SStaticLandscapeTransition)));
			auto* destination = landscape.transitions;

			for (std::uint32_t transition = 0; transition < work.count; ++transition)
			{
				const auto& target = work.entries[transition];

				if (target.landscapeId == -1)
					continue;

				destination->type = target.type;
				destination->landscapeId = target.landscapeId;
				++destination;
			}
		}

		NXBasics::Runtime_Free(transitionWork);
		const char* houseFireNames[] = { "fx fire house 0", "fx fire house 1", "fx fire house 2", "fx fire house 0", "fx fire house 2" };

		for (std::uint32_t index = 0; index < 5; ++index)
		{
			gStaticLandscapeStorage.houseFireIds[index] = E2StaticDataLandscapesManager_GetLandscapeId(houseFireNames[index]);
		}

		for (std::int32_t& id : gStaticLandscapeStorage.smokeIds)
		{
			id = E2StaticDataLandscapesManager_GetLandscapeId("fx smoke");
		}

		for (std::int32_t& id : gStaticLandscapeStorage.incenseFireIds)
		{
			id = E2StaticDataLandscapesManager_GetLandscapeId("fx fire incense");
		}

		char wallName[128];

		for (std::uint32_t index = 0; index < 5; ++index)
		{
			NXBasics::Runtime_PrintString(wallName, "wall_0%d", index + 1u);
			gStaticLandscapeStorage.wallIds[index] = E2StaticDataLandscapesManager_GetLandscapeId(wallName);
		}

		const char* gateNames[] = { "gate_02_open", "gate_02", "gate_01_open", "gate_01", "gate_03_open", "gate_03" };

		for (std::uint32_t index = 0; index < 6; ++index)
		{
			gStaticLandscapeStorage.gateIds[index] = E2StaticDataLandscapesManager_GetLandscapeId(gateNames[index]);
		}

		return manager;
	}

	// Original function: FUN_00490360; CE2StaticDataLandscapesManager::GetLandscapeId.
	std::int32_t E2StaticDataLandscapesManager_GetLandscapeId(const char* name)
	{
		const auto* entries = static_cast<const SE2StaticDataLandscape*>(gStaticLandscapeEntries);

		for (std::int32_t index = 0; index < gStaticLandscapeCount; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(entries[index].name, name) == 0)
				return index;
		}

		return -1;
	}

	// Original function: FUN_00490448; CE2StaticDataTexturesManager::GetTextureId.
	std::int32_t __fastcall E2StaticDataTexturesManager_GetTextureId(void* manager, void* /*unusedEdx*/, const char* fileName)
	{
		auto* list = static_cast<NXBasics::CListBase*>(manager);
		NXBasics::CListBase::Iterator existing(*list);
		std::uint32_t index = 0;

		for (auto* entry = static_cast<SE2StaticDataTextureData*>(existing.First()); entry != nullptr; entry = static_cast<SE2StaticDataTextureData*>(existing.Next()), ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(entry->name, fileName) == 0)
				return static_cast<std::int32_t>(index);
		}

		if (!NXBasics::CFile::FileSystem_Tool_FileExists(fileName, true))
			return -1;

		auto* entry = static_cast<SE2StaticDataTextureData*>(NXBasics::Runtime_AllocateWithNewHandler(sizeof(SE2StaticDataTextureData), 1));
		std::memset(entry, 0, sizeof(SE2StaticDataTextureData));
		std::strncpy(entry->name, fileName, 0x77);
		{
			NXBasics::CPicture picture(fileName);
			entry->bitmap = picture.ExtractBitmap();
			entry->palette = picture.ExtractPalette();
			list->InsertAtEnd(entry);
		}
		NXBasics::Runtime_Free(gStaticDataTextures.bitmaps);
		NXBasics::Runtime_Free(gStaticDataTextures.palettes);
		gStaticDataTextures.bitmaps = static_cast<NXBasics::CBitmap**>(NXBasics::Runtime_AllocateArrayWithNewHandler(list->GetCount(), sizeof(NXBasics::CBitmap*)));
		gStaticDataTextures.palettes = static_cast<NXBasics::CPalette**>(NXBasics::Runtime_AllocateArrayWithNewHandler(list->GetCount(), sizeof(NXBasics::CPalette*)));
		NXBasics::CListBase::Iterator textures(*list);
		index = 0;

		for (auto* texture = static_cast<SE2StaticDataTextureData*>(textures.First()); texture != nullptr; texture = static_cast<SE2StaticDataTextureData*>(textures.Next()), ++index)
		{
			gStaticDataTextures.bitmaps[index] = texture->bitmap;
			gStaticDataTextures.palettes[index] = texture->palette;
		}

		return static_cast<std::int32_t>(list->GetCount() - 1u);
	}

}