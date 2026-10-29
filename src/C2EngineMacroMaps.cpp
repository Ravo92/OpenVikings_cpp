#include "ApplicationGlobals.h"
#include "C2AnimalAccess.h"
#include "C2DataIds.h"
#include "C2E2StaticDataHouses.h"
#include "C2E2StaticDataTerrain.h"
#include "C2EngineDisplayAccess.h"
#include "C2EngineLandscapeAccess.h"
#include "C2EngineMacroMaps.h"
#include "C2EngineMapTerrainAccess.h"
#include "C2GameControlStartup.h"
#include "C2GameEngineStartup.h"
#include "C2GameMapAccess.h"
#include "C2GameState.h"
#include "C2HumanInitialization.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicLifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2MapMoveableAccess.h"
#include "C2MapResourceManagers.h"
#include "C2MissionScriptData.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2VehicleAccess.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XDirectoryScanner.h"
#include "XFile.h"
#include "XIniFileReader.h"
#include "XNumericConstants.h"
#include <cstring>
#include <new>

namespace NC2E2
{
	using namespace NC2Lifetime;

	namespace
	{
		struct SMacroMapSection
		{
			const char* command;
			std::uint32_t countOffset, stride;
		};

		// Windows command pointer table 004F46C0..004F46F3; section layouts follow their allocation/parsing instructions.
		const SMacroMapSection gMacroMapSections[] = { { "patname", NC2Layout::MacroMapOffset::PatternNamesCount, NC2Layout::RecordSize::MacroMapName }, { "pat", NC2Layout::MacroMapOffset::PatternsCount, NC2Layout::RecordSize::MacroMapPattern }, { "transname", NC2Layout::MacroMapOffset::TransitionNamesCount, NC2Layout::RecordSize::MacroMapName }, { "trans", NC2Layout::MacroMapOffset::TransitionsCount, NC2Layout::RecordSize::MacroMapTransition }, { "landname", NC2Layout::MacroMapOffset::LandscapeNamesCount, NC2Layout::RecordSize::MacroMapName }, { "land", NC2Layout::MacroMapOffset::LandscapesCount, NC2Layout::RecordSize::MacroMapLandscape }, { "hgt", NC2Layout::MacroMapOffset::HeightsCount, NC2Layout::RecordSize::MacroMapHeight }, { "veco", NC2Layout::MacroMapOffset::VertexColorsCount, NC2Layout::RecordSize::MacroMapVertexColor }, { "road", NC2Layout::MacroMapOffset::RoadsCount, NC2Layout::RecordSize::MacroMapRoad }, { "human", NC2Layout::MacroMapOffset::HumansCount, NC2Layout::RecordSize::MacroMapCreature }, { "animal", NC2Layout::MacroMapOffset::AnimalsCount, NC2Layout::RecordSize::MacroMapCreature }, { "house", NC2Layout::MacroMapOffset::HousesCount, NC2Layout::RecordSize::MacroMapHouse }, { "fishswarm", NC2Layout::MacroMapOffset::FishSwarmsCount, NC2Layout::RecordSize::MacroMapFishSwarm } };
		// Complete Windows token region 00568430..005684BF.
		const NXBasics::SToken gMacroMapTokens[] = { { "version", NC2DataIds::EngineMacroMapsToken::Version }, { "size", NC2DataIds::EngineMacroMapsToken::Size }, { "center", NC2DataIds::EngineMacroMapsToken::Center }, { "patname", NC2DataIds::EngineMacroMapsToken::Patname }, { "pat", NC2DataIds::EngineMacroMapsToken::Pat }, { "transname", NC2DataIds::EngineMacroMapsToken::Transname }, { "trans", NC2DataIds::EngineMacroMapsToken::Trans }, { "landname", NC2DataIds::EngineMacroMapsToken::LandName }, { "land", NC2DataIds::EngineMacroMapsToken::Land }, { "hgt", NC2DataIds::EngineMacroMapsToken::Hgt }, { "veco", NC2DataIds::EngineMacroMapsToken::Veco }, { "road", NC2DataIds::EngineMacroMapsToken::Road }, { "human", NC2DataIds::EngineMacroMapsToken::Human }, { "animal", NC2DataIds::EngineMacroMapsToken::Animal }, { "house", NC2DataIds::EngineMacroMapsToken::House }, { "fishswarm", NC2DataIds::EngineMacroMapsToken::FishSwarm }, { nullptr, 0 } };
		std::uint8_t gMacroMapTokenFlags = 0;
		alignas(NXBasics::CTokenList) std::byte gMacroMapTokenStorage[sizeof(NXBasics::CTokenList)];
		NXBasics::CTokenList* gMacroMapTokenList = nullptr;

		// Original function: FUN_0048823d; CTokenList requires no exit work.
		void __cdecl MacroMap_TokenList_AtExit()
		{
		}
	}

	// Original function: FUN_00487508.
	void* MacroMap_Construct(void* map, const char* filename)
	{
		std::memset(map, 0, NC2Layout::RecordSize::MacroMap);

		if (filename != nullptr)
		{
			if (NXBasics::CFile::FileSystem_Tool_FileExists(filename, true))
			{
				MacroMap_LoadFromIniFile(map, filename);
			}
			else
			{
				std::strcpy(static_cast<char*>(map) + NC2Layout::MacroMapOffset::Filename, filename);
				Write<std::uint8_t>(map, NC2Layout::MacroMapOffset::Initialized, 1);
			}
		}

		return map;
	}

	// Original function: FUN_00487551; distinct destructor forwarding entry.
	void MacroMap_Destruct(void* map)
	{
		MacroMap_l_FreeAllData(map);
	}

	// Original function: FUN_004875fa.
	bool MacroMap_LoadFromIniFile(void* map, const char* filename)
	{
		MacroMap_l_FreeAllData(map);
		NXBasics::CIniFileReader reader(filename, nullptr, false, nullptr, nullptr);

		if (!reader.IsLoaded() || !reader.SearchGroup("map"))
			return false;

		for (const auto& section : gMacroMapSections)
		{
			const auto count = reader.GetNumberOfCommandsInGroup(section.command);
			Write(map, section.countOffset, count);

			if (count == 0)
				continue;

			void* entries = NXBasics::Runtime_AllocateWithNewHandler(count * section.stride, 1);
			Write(map, section.countOffset + NC2Layout::MacroMapSectionOffset::EntriesUnsigned, entries);
			std::memset(entries, 0, count * section.stride);

			if (section.countOffset == NC2Layout::MacroMapOffset::LandscapesCount)
				for (std::uint32_t i = 0; i < count; ++i)
				{
					Write<std::int32_t>(entries, i * NC2Layout::RecordSize::MacroMapLandscapeUnsigned + NC2Layout::MacroMapLandscapeOffset::PlayerIdUnsigned, -1);
				}
		}

		if ((gMacroMapTokenFlags & 1u) == 0)
			gMacroMapTokenFlags |= 1u;

		if ((gMacroMapTokenFlags & 2u) == 0)
		{
			gMacroMapTokenFlags |= 2u;
			gMacroMapTokenList = ::new (gMacroMapTokenStorage) NXBasics::CTokenList(gMacroMapTokens);
			NXBasics::Runtime_AtExit(reinterpret_cast<NXBasics::RuntimeOnExitCallback>(MacroMap_TokenList_AtExit));
		}

		std::uint32_t indices[NC2Layout::MacroMapSectionLimits::Count]{ };
		reader.SetCurrentLineToGroupStart();

		while (reader.GetNextLine() != nullptr)
		{
			if (reader.GetCurrentLineType() != 2)
				break;

			const auto token = gMacroMapTokenList->FindToken(reader.GetCurrentName());

			if (token == NC2DataIds::EngineMacroMapsToken::Version)
			{
				reader.GetInt();
				continue;
			}

			if (token == NC2DataIds::EngineMacroMapsToken::Size)
			{
				Write<std::uint16_t>(map, NC2Layout::MacroMapOffset::Width, static_cast<std::uint16_t>(reader.GetInt()));
				Write<std::uint16_t>(map, NC2Layout::MacroMapOffset::Height, static_cast<std::uint16_t>(reader.GetInt()));
				continue;
			}

			if (token == NC2DataIds::EngineMacroMapsToken::Center)
			{
				Write<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX, static_cast<std::uint16_t>(reader.GetInt()));
				Write<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY, static_cast<std::uint16_t>(reader.GetInt()) & NXBasics::BitMask::WordExceptBit0);
				continue;
			}

			const std::uint32_t sectionTokens[] = { NC2DataIds::EngineMacroMapsToken::Patname, NC2DataIds::EngineMacroMapsToken::Pat, NC2DataIds::EngineMacroMapsToken::Transname, NC2DataIds::EngineMacroMapsToken::Trans, NC2DataIds::EngineMacroMapsToken::LandName, NC2DataIds::EngineMacroMapsToken::Land, NC2DataIds::EngineMacroMapsToken::Hgt, NC2DataIds::EngineMacroMapsToken::Veco, NC2DataIds::EngineMacroMapsToken::Road, NC2DataIds::EngineMacroMapsToken::Human, NC2DataIds::EngineMacroMapsToken::Animal, NC2DataIds::EngineMacroMapsToken::House, NC2DataIds::EngineMacroMapsToken::FishSwarm };
			std::uint32_t section = 0;

			while (section < NC2Layout::MacroMapSectionLimits::Count && sectionTokens[section] != token)
				++section;

			if (section == NC2Layout::MacroMapSectionLimits::Count)
				continue;

			const auto& schema = gMacroMapSections[section];

			if (indices[section] == Read<std::uint32_t>(map, schema.countOffset))
				continue;

			auto* entry = static_cast<std::byte*>(Read<void*>(map, schema.countOffset + NC2Layout::MacroMapSectionOffset::EntriesUnsigned)) + indices[section] * schema.stride;

			if (section == SwitchIds::MacroMapSection::PatternNames || section == SwitchIds::MacroMapSection::TransitionNames || section == SwitchIds::MacroMapSection::LandscapeNames)
			{
				std::strncpy(reinterpret_cast<char*>(entry), reader.GetString(), 39);
				const auto id = section == SwitchIds::MacroMapSection::PatternNames ? E2StaticDataPatternManager_GetPatternId(reinterpret_cast<char*>(entry)) : section == SwitchIds::MacroMapSection::TransitionNames ? E2StaticDataTransitionsManager_GetTransitionId(reinterpret_cast<char*>(entry))
				                                                                                                                                                                                                       : E2StaticDataLandscapesManager_GetLandscapeId(reinterpret_cast<char*>(entry));
				Write(entry, NC2Layout::MacroMapNameOffset::StaticDataId, id);
			}
			else if (section == SwitchIds::MacroMapSection::Houses)
			{
				const auto player = reader.GetInt(), x = reader.GetInt(), y = reader.GetInt();
				const char* name = reader.GetString();
				const auto stage = reader.GetInt();
				const bool completed = reader.GetInt() != 0;
				Write(entry, NC2Layout::MacroMapHouseOffset::PlayerId, player);
				Write<std::uint16_t>(entry, NC2Layout::MacroMapHouseOffset::MapX, static_cast<std::uint16_t>(x));
				Write<std::uint16_t>(entry, NC2Layout::MacroMapHouseOffset::MapY, static_cast<std::uint16_t>(y));
				Write(entry, NC2Layout::MacroMapHouseOffset::HouseType, StaticDataHouses_GetHouseId(name));
				Write(entry, NC2Layout::MacroMapHouseOffset::DevelopmentStage, stage);
				Write<std::uint8_t>(entry, NC2Layout::MacroMapHouseOffset::Completed, completed);
			}
			else
			{
				const auto x = reader.GetInt(), y = reader.GetInt();

				if (section == SwitchIds::MacroMapSection::Humans || section == SwitchIds::MacroMapSection::Animals)
				{
					const auto type = section == SwitchIds::MacroMapSection::Humans && reader.GetTypeOfNextParameter() == 2 ? NC2Logic::Logic_EntryPoint_GetJobType(reader.GetString()) : static_cast<std::uint32_t>(reader.GetInt());
					const auto tribe = reader.GetInt(), player = reader.GetInt();
					Write<std::uint16_t>(entry, NC2Layout::MacroMapCreatureOffset::MapX, static_cast<std::uint16_t>(x));
					Write<std::uint16_t>(entry, NC2Layout::MacroMapCreatureOffset::MapY, static_cast<std::uint16_t>(y));
					Write(entry, NC2Layout::MacroMapCreatureOffset::Type, type);
					Write(entry, NC2Layout::MacroMapCreatureOffset::TribeType, tribe);
					Write(entry, NC2Layout::MacroMapCreatureOffset::PlayerId, player);
				}
				else if (section == SwitchIds::MacroMapSection::Roads)
				{
					Write<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapX, static_cast<std::uint16_t>(x));
					Write<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapY, static_cast<std::uint16_t>(y));
				}
				else
				{
					const auto value = reader.GetInt();

					if (section == SwitchIds::MacroMapSection::Patterns)
					{
						const bool triangleA = reader.GetInt() != 0;
						Write<std::uint16_t>(entry, NC2Layout::MacroMapPatternOffset::MapX, static_cast<std::uint16_t>(x));
						Write<std::uint16_t>(entry, NC2Layout::MacroMapPatternOffset::MapY, static_cast<std::uint16_t>(y));
						Write(entry, NC2Layout::MacroMapPatternOffset::NameIndex, value);
						Write<std::uint8_t>(entry, NC2Layout::MacroMapPatternOffset::TriangleA, triangleA);
					}
					else if (section == SwitchIds::MacroMapSection::Transitions)
					{
						const bool triangleA = reader.GetInt() != 0;
						const auto nameId = reader.GetInt();
						Write(entry, NC2Layout::MacroMapTransitionOffset::Layer, value);
						Write<std::uint16_t>(entry, NC2Layout::MacroMapTransitionOffset::MapX, static_cast<std::uint16_t>(x));
						Write<std::uint16_t>(entry, NC2Layout::MacroMapTransitionOffset::MapY, static_cast<std::uint16_t>(y));
						Write<std::uint8_t>(entry, NC2Layout::MacroMapTransitionOffset::TriangleA, triangleA);
						Write(entry, NC2Layout::MacroMapTransitionOffset::NameIndex, nameId);
					}
					else if (section == SwitchIds::MacroMapSection::Landscapes)
					{
						const auto valency = static_cast<std::uint8_t>(reader.GetInt());
						const auto player = reader.GetTypeOfNextParameter() == 3 ? reader.GetInt() : -1;
						Write<std::uint16_t>(entry, NC2Layout::MacroMapLandscapeOffset::MapY, static_cast<std::uint16_t>(y));
						Write(entry, NC2Layout::MacroMapLandscapeOffset::NameIndex, value);
						Write<std::uint16_t>(entry, NC2Layout::MacroMapLandscapeOffset::MapX, static_cast<std::uint16_t>(x));
						Write<std::uint32_t>(entry, NC2Layout::MacroMapLandscapeOffset::Valency, valency);
						Write(entry, NC2Layout::MacroMapLandscapeOffset::PlayerId, player);
					}
					else
					{
						Write<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapX, static_cast<std::uint16_t>(x));
						Write<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapY, static_cast<std::uint16_t>(y));

						if (section == SwitchIds::MacroMapSection::FishSwarms)
						{
							Write(entry, NC2Layout::MacroMapFishSwarmOffset::FishCount, value);
						}
						else
						{
							Write<std::uint8_t>(entry, NC2Layout::MacroMapByteValueOffset::Value, static_cast<std::uint8_t>(value));
						}
					}
				}
			}

			++indices[section];
		}

		NXBasics::CFile::FileSystem_Tool_GetRealFilename(filename, static_cast<char*>(map) + NC2Layout::MacroMapOffset::Filename);
		Write<std::uint8_t>(map, NC2Layout::MacroMapOffset::Initialized, 1);

		return true;
	}

	// Original function: FUN_00489de0.
	void* __fastcall MacroMapManager_Construct(void* manager, void* /*unusedEdx*/)
	{
		gMacroMapManager = manager;
		std::memset(manager, 0, NC2Layout::RecordSize::MacroMapManager);
		MacroMapManager_RescanMacroMaps(manager);

		return manager;
	}

	// Original function: FUN_00489e0e; scan INI first, falling back to CIF only for zero matches.
	void MacroMapManager_RescanMacroMaps(void* manager)
	{
		MacroMapManager_l_FreeAllMacroMaps(manager);
		const char* extension = "ini";
		auto count = NXBasics::DirectoryScanner_GetNumberOfMatchingFiles("data\\edit\\macromaps\\", extension, false);
		Write(manager, NC2Layout::MacroMapManagerOffset::Count, count);

		if (count == 0)
		{
			extension = "cif";
			count = NXBasics::DirectoryScanner_GetNumberOfMatchingFiles("data\\edit\\macromaps\\", extension, false);
			Write(manager, NC2Layout::MacroMapManagerOffset::Count, count);
		}

		const bool pickedExists = NXBasics::CFile::FileSystem_Tool_FileExists("data\\edit\\macromaps\\z_system_picked.ini", true);

		if (!pickedExists)
		{
			count = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(count) + 1u);
			Write(manager, NC2Layout::MacroMapManagerOffset::Count, count);
		}

		if (count == 0)
		{

			return;
		}

		void* entries = NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::uint32_t>(count) * NC2Layout::RecordSize::MacroMapPointerUnsigned, 1);
		Write(manager, NC2Layout::MacroMapManagerOffset::Entries, entries);
		std::memset(entries, 0, Read<std::uint32_t>(manager, NC2Layout::MacroMapManagerOffset::Count) * NC2Layout::RecordSize::MacroMapPointerUnsigned);
		NXBasics::SDirectoryScannerState scanner;
		NXBasics::DirectoryScanner_Initialize(&scanner);
		std::uint32_t index = 0;
		bool found = NXBasics::DirectoryScanner_GetFirstMatchingFile(&scanner, "data\\edit\\macromaps\\", extension, false);

		while (found)
		{
			const char* name = NXBasics::DirectoryScanner_GetCurrentNamePtr(&scanner);

			if (name == nullptr)
			{
				found = NXBasics::DirectoryScanner_GetNextMatchingFile(&scanner);
				continue;
			}

			void* map = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::RecordSize::MacroMap, 1);

			if (map != nullptr)
				MacroMap_Construct(map, name);

			Write(Read<void*>(manager, NC2Layout::MacroMapManagerOffset::Entries), index++ * NC2Layout::RecordSize::MacroMapPointerUnsigned, map);
			found = NXBasics::DirectoryScanner_GetNextMatchingFile(&scanner);
		}

		NXBasics::DirectoryScanner_Destroy(&scanner);

		if (!pickedExists)
		{
			void* map = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::RecordSize::MacroMap, 1);

			if (map != nullptr)
				MacroMap_Construct(map, "data\\edit\\macromaps\\z_system_picked.ini");

			Write(Read<void*>(manager, NC2Layout::MacroMapManagerOffset::Entries), index * NC2Layout::RecordSize::MacroMapPointerUnsigned, map);
		}
	}

	// Original function: FUN_00481036.
	std::int32_t Engine2d_EntryPoint_Edit_SetHouse(std::uint32_t player, std::int32_t engineType, std::uint32_t stage, const void* point, bool completed)
	{
		if (engineType == -1)
		{
			NC2Logic::Logic_EntryPoint_House_Remove(point);
			return -1;
		}

		return NC2Logic::Logic_EntryPoint_House_Set(player, static_cast<std::uint32_t>(engineType), stage, point, completed, 0);
	}

	// Original function: FUN_0048963e.
	void MacroMap_SetPatternToGameMap(void* map, const void* root, MacroMapPositionCallback callback)
	{

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::PatternsCount); ++i)
		{
			const auto entry = static_cast<const std::byte*>(Read<void*>(map, NC2Layout::MacroMapOffset::Patterns)) + i * NC2Layout::RecordSize::MacroMapPatternUnsigned;
			std::uint16_t point[2];
			NC2Logic::MapMagPoint_Construct(point, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapPatternOffset::MapX) - Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX) + Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::X)), static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapPatternOffset::MapY) - Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY) + Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::Y)), false);

			if ((Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::Y) & 1) != 0 && (Read<std::uint16_t>(entry, NC2Layout::MacroMapPatternOffset::MapY) & 1) != 0)
				++point[0];

			const auto triangle = Read<std::uint8_t>(entry, NC2Layout::MacroMapPatternOffset::TriangleA);

			if (callback == nullptr || callback(point, triangle))
				EngineMap_MapMacroPoint_TrianglePattern_SetType(point, triangle != 0, Read<std::uint32_t>(Read<void*>(map, NC2Layout::MacroMapOffset::PatternNames), Read<std::uint32_t>(entry, NC2Layout::MacroMapPatternOffset::NameIndex) * NC2Layout::RecordSize::MacroMapNameUnsigned + NC2Layout::MacroMapNameOffset::StaticDataId));
		}
	}

	// Original function: FUN_004896fc.
	void MacroMap_SetTransitionsToGameMap(void* map, const void* root, MacroMapPositionCallback callback)
	{

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::TransitionsCount); ++i)
		{
			const auto entry = static_cast<const std::byte*>(Read<void*>(map, NC2Layout::MacroMapOffset::Transitions)) + i * NC2Layout::RecordSize::MacroMapTransitionUnsigned;
			std::uint16_t point[2];
			NC2Logic::MapMagPoint_Construct(point, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapTransitionOffset::MapX) - Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX) + Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::X)), static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapTransitionOffset::MapY) - Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY) + Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::Y)), false);

			if ((Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::Y) & 1) != 0 && (Read<std::uint16_t>(entry, NC2Layout::MacroMapTransitionOffset::MapY) & 1) != 0)
				++point[0];

			const auto triangle = Read<std::uint8_t>(entry, NC2Layout::MacroMapTransitionOffset::TriangleA);

			if (callback == nullptr || callback(point, triangle))
				EngineMap_Transition_Set(point, Read<std::int32_t>(Read<void*>(map, NC2Layout::MacroMapOffset::TransitionNames), Read<std::uint32_t>(entry, NC2Layout::MacroMapTransitionOffset::NameIndex) * NC2Layout::RecordSize::MacroMapNameUnsigned + NC2Layout::MacroMapNameOffset::StaticDataId), Read<std::uint32_t>(entry, NC2Layout::MacroMapTransitionOffset::Layer), triangle != 0);
		}
	}

	// Original function: FUN_004897bd.
	void MacroMap_SetLandscapesToGameMap(void* map, const void* root, MacroMapPositionCallback callback, bool preserveWalkingBlock)
	{
		std::uint16_t base[2], centerMacro[2], center[2];
		NC2Logic::MapMigPoint_ConstructFromMag(base, root, false);
		NC2Logic::MapMagPoint_Construct(centerMacro, Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX), Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY), false);
		NC2Logic::MapMigPoint_ConstructFromMag(center, centerMacro, false);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::LandscapesCount); ++i)
		{
			const auto entry = static_cast<const std::byte*>(Read<void*>(map, NC2Layout::MacroMapOffset::Landscapes)) + i * NC2Layout::RecordSize::MacroMapLandscapeUnsigned;
			std::uint16_t point[2];
			NC2Logic::MapMigPoint_Construct(point, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapLandscapeOffset::MapX) - center[0] + base[0]), static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapLandscapeOffset::MapY) - center[1] + base[1]), false);

			if (callback == nullptr || callback(point, 1))
				EngineMap_MapMicroPoint_Landscape_SetType(point, Read<std::int32_t>(Read<void*>(map, NC2Layout::MacroMapOffset::LandscapeNames), Read<std::uint32_t>(entry, NC2Layout::MacroMapLandscapeOffset::NameIndex) * NC2Layout::RecordSize::MacroMapNameUnsigned + NC2Layout::MacroMapNameOffset::StaticDataId), Read<std::uint32_t>(entry, NC2Layout::MacroMapLandscapeOffset::Valency), preserveWalkingBlock, Read<std::int32_t>(entry, NC2Layout::MacroMapLandscapeOffset::PlayerId));
		}
	}

	// Original function: FUN_0048988a.
	void MacroMap_SetHeightsToGameMap(void* map, const void* root, std::int32_t mode, MacroMapPositionCallback callback)
	{
		std::uint16_t base[2], centerMacro[2], center[2];
		NC2Logic::MapMigPoint_ConstructFromMag(base, root, false);
		NC2Logic::MapMagPoint_Construct(centerMacro, Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX), Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY), false);
		NC2Logic::MapMigPoint_ConstructFromMag(center, centerMacro, false);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::HeightsCount); ++i)
		{
			const auto entry = static_cast<const std::byte*>(Read<void*>(map, NC2Layout::MacroMapOffset::Heights)) + i * NC2Layout::RecordSize::MacroMapHeightUnsigned;
			std::uint16_t point[2];
			NC2Logic::MapMagPoint_Construct(point, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapHeightOffset::MapX) - Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX) + Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::X)), static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapHeightOffset::MapY) - Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY) + Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::Y)), false);

			if ((Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::Y) & 1) != 0 && (Read<std::uint16_t>(entry, NC2Layout::MacroMapHeightOffset::MapY) & 1) != 0)
				++point[0];

			if (callback != nullptr)
			{
				std::uint16_t microPoint[2];
				NC2Logic::MapMigPoint_ConstructFromMag(microPoint, point, false);

				if (!callback(microPoint, 1))
					continue;
			}

			const auto height = Read<std::uint8_t>(entry, NC2Layout::MacroMapHeightOffset::Height);

			if (mode == NC2Runtime::MacroMapHeightMode::Set)
			{
				NC2Logic::GameMap_MapMacroPoint_Height_Set(point, height);
			}
			else if (mode == NC2Runtime::MacroMapHeightMode::Add)
			{
				NC2Logic::GameMap_MapMacroPoint_Height_Modify(point, height);
			}
			else if (mode == NC2Runtime::MacroMapHeightMode::Subtract)
			{
				NC2Logic::GameMap_MapMacroPoint_Height_Modify(point, -static_cast<std::int32_t>(height));
			}
		}
	}

	// Original function: FUN_00489998.
	void MacroMap_SetVertexColorsToGameMap(void* map, const void* root, MacroMapPositionCallback callback)
	{
		std::uint16_t base[2], centerMacro[2], center[2];
		NC2Logic::MapMigPoint_ConstructFromMag(base, root, false);
		NC2Logic::MapMagPoint_Construct(centerMacro, Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX), Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY), false);
		NC2Logic::MapMigPoint_ConstructFromMag(center, centerMacro, false);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::VertexColorsCount); ++i)
		{
			const auto entry = static_cast<const std::byte*>(Read<void*>(map, NC2Layout::MacroMapOffset::VertexColors)) + i * NC2Layout::RecordSize::MacroMapVertexColorUnsigned;
			std::uint16_t point[2];
			NC2Logic::MapMagPoint_Construct(point, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapVertexColorOffset::MapX) - Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX) + Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::X)), static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapVertexColorOffset::MapY) - Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY) + Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::Y)), false);

			if ((Read<std::uint16_t>(root, NC2Layout::MapCoordinatePairOffset::Y) & 1) != 0 && (Read<std::uint16_t>(entry, NC2Layout::MacroMapVertexColorOffset::MapY) & 1) != 0)
				++point[0];

			if (callback != nullptr)
			{
				std::uint16_t microPoint[2];
				NC2Logic::MapMigPoint_ConstructFromMag(microPoint, point, false);

				if (!callback(microPoint, 1))
					continue;
			}

			EngineMap_VertexColorIndex_Set(point, Read<std::uint8_t>(entry, NC2Layout::MacroMapVertexColorOffset::ColorIndex));
		}
	}

	// Original function: FUN_00489a80.
	void MacroMap_SetRoadsToGameMap(void* map, const void* root, MacroMapPositionCallback callback)
	{
		std::uint16_t base[2], centerMacro[2], center[2];
		NC2Logic::MapMigPoint_ConstructFromMag(base, root, false);
		NC2Logic::MapMagPoint_Construct(centerMacro, Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX), Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY), false);
		NC2Logic::MapMigPoint_ConstructFromMag(center, centerMacro, false);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::RoadsCount); ++i)
		{
			const auto entry = static_cast<const std::byte*>(Read<void*>(map, NC2Layout::MacroMapOffset::Roads)) + i * NC2Layout::RecordSize::MacroMapRoadUnsigned;
			std::uint16_t point[2];
			NC2Logic::MapMigPoint_Construct(point, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapRoadOffset::MapX) - center[0] + base[0]), static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapRoadOffset::MapY) - center[1] + base[1]), false);

			if (callback == nullptr || callback(point, 1))
				NC2Logic::GameMap_MapMicroPoint_Road_Set(point, true);
		}
	}

	// Original function: FUN_00489b28.
	void MacroMap_SetHumansToGameMap(void* map, const void* root)
	{
		std::uint16_t base[2], centerMacro[2], center[2];
		NC2Logic::MapMigPoint_ConstructFromMag(base, root, false);
		NC2Logic::MapMagPoint_Construct(centerMacro, Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX), Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY), false);
		NC2Logic::MapMigPoint_ConstructFromMag(center, centerMacro, false);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::HumansCount); ++i)
		{
			const auto entry = static_cast<const std::byte*>(Read<void*>(map, NC2Layout::MacroMapOffset::Humans)) + i * NC2Layout::RecordSize::MacroMapCreatureUnsigned;

			if (Read<std::int32_t>(entry, NC2Layout::MacroMapCreatureOffset::Type) == -1)
				continue;

			std::uint16_t point[2];
			NC2Logic::MapMigPoint_Construct(point, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapCreatureOffset::MapX) - center[0] + base[0]), static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapCreatureOffset::MapY) - center[1] + base[1]), false);
			NC2Logic::Logic_EntryPoint_Add_Human(Read<std::uint32_t>(entry, NC2Layout::MacroMapCreatureOffset::PlayerId), Read<std::uint32_t>(entry, NC2Layout::MacroMapCreatureOffset::TribeType), Read<std::uint32_t>(entry, NC2Layout::MacroMapCreatureOffset::Type), point, 0, 0);
		}
	}

	// Original function: FUN_00489bd9.
	void MacroMap_SetAnimalsToGameMap(void* map, const void* root)
	{
		std::uint16_t base[2], centerMacro[2], center[2];
		NC2Logic::MapMigPoint_ConstructFromMag(base, root, false);
		NC2Logic::MapMagPoint_Construct(centerMacro, Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX), Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY), false);
		NC2Logic::MapMigPoint_ConstructFromMag(center, centerMacro, false);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::AnimalsCount); ++i)
		{
			const auto entry = static_cast<const std::byte*>(Read<void*>(map, NC2Layout::MacroMapOffset::Animals)) + i * NC2Layout::RecordSize::MacroMapCreatureUnsigned;
			std::uint16_t point[2];
			NC2Logic::MapMigPoint_Construct(point, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapCreatureOffset::MapX) - center[0] + base[0]), static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapCreatureOffset::MapY) - center[1] + base[1]), false);
			NC2Logic::Logic_EntryPoint_Add_Animal(Read<std::uint32_t>(entry, NC2Layout::MacroMapCreatureOffset::PlayerId), Read<std::uint32_t>(entry, NC2Layout::MacroMapCreatureOffset::TribeType), Read<std::uint32_t>(entry, NC2Layout::MacroMapCreatureOffset::Type), point, 0, 0);
		}
	}

	// Original function: FUN_00489c84.
	void MacroMap_SetHousesToGameMap(void* map, const void* root)
	{
		std::uint16_t base[2], centerMacro[2], center[2];
		NC2Logic::MapMigPoint_ConstructFromMag(base, root, false);
		NC2Logic::MapMagPoint_Construct(centerMacro, Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX), Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY), false);
		NC2Logic::MapMigPoint_ConstructFromMag(center, centerMacro, false);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::HousesCount); ++i)
		{
			const auto entry = static_cast<const std::byte*>(Read<void*>(map, NC2Layout::MacroMapOffset::Houses)) + i * NC2Layout::RecordSize::MacroMapHouseUnsigned;
			std::uint16_t point[2];
			NC2Logic::MapMigPoint_Construct(point, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapHouseOffset::MapX) - center[0] + base[0]), static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapHouseOffset::MapY) - center[1] + base[1]), false);
			// Windows constructs a translated point but places the house at its stored coordinates.
			std::uint16_t storedPoint[2];
			NC2Logic::MapMigPoint_Construct(storedPoint, Read<std::uint16_t>(entry, NC2Layout::MacroMapHouseOffset::MapX), Read<std::uint16_t>(entry, NC2Layout::MacroMapHouseOffset::MapY), false);
			Engine2d_EntryPoint_Edit_SetHouse(Read<std::uint32_t>(entry, NC2Layout::MacroMapHouseOffset::PlayerId), Read<std::int32_t>(entry, NC2Layout::MacroMapHouseOffset::HouseType), Read<std::uint32_t>(entry, NC2Layout::MacroMapHouseOffset::DevelopmentStage), storedPoint, Read<std::uint8_t>(entry, NC2Layout::MacroMapHouseOffset::Completed) != 0);
		}
	}

	// Original function: FUN_00489d46.
	void MacroMap_SetFishSwarmsToGameMap(void* map, const void* root)
	{
		std::uint16_t base[2], centerMacro[2], center[2];
		NC2Logic::MapMigPoint_ConstructFromMag(base, root, false);
		NC2Logic::MapMagPoint_Construct(centerMacro, Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX), Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY), false);
		NC2Logic::MapMigPoint_ConstructFromMag(center, centerMacro, false);

		for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::FishSwarmsCount); ++i)
		{
			const auto entry = static_cast<const std::byte*>(Read<void*>(map, NC2Layout::MacroMapOffset::FishSwarms)) + i * NC2Layout::RecordSize::MacroMapFishSwarmUnsigned;
			std::uint16_t point[2];
			NC2Logic::MapMigPoint_Construct(point, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapFishSwarmOffset::MapX) - center[0] + base[0]), static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapFishSwarmOffset::MapY) - center[1] + base[1]), false);
			static_cast<NC2Logic::CMapFishManager*>(NC2Logic::gMapFishManager)->AddFishAtMapPosition(point, Read<std::uint32_t>(entry, NC2Layout::MacroMapFishSwarmOffset::FishCount), 7);
		}
	}

	// Original function: FUN_0048823e; all thirteen sections and both mirror modes.
	void MacroMap_CreateFromGameMap(void* map, const char* filename, bool includeCreatures, std::int16_t minX, std::int16_t minY, std::int16_t maxX, std::int16_t maxY, bool mirrorX, bool mirrorY)
	{
		MacroMap_l_FreeAllData(map);
		const auto align = [&](std::int16_t value)
		{
			auto bits = (static_cast<std::uint32_t>(value) + 1u) & ~1u;

			if (mirrorX || mirrorY)
				bits = (bits + 3u) & ~3u;

			return static_cast<std::int16_t>(bits);
		};
		minX = align(minX);
		minY = align(minY);
		maxX = align(maxX);
		maxY = align(maxY);

		if (minX < 0)
			minX = 0;

		if (minY < 0)
			minY = 0;

		if (maxX == 0 || maxX > static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroWidth()))
			maxX = static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroWidth());

		if (maxY == 0 || maxY > static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroHeight()))
			maxY = static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroHeight());

		if (minX > maxX)
		{
			const auto swap = minX;
			minX = maxX;
			maxX = swap;
		}

		if (minY > maxY)
		{
			const auto swap = minY;
			minY = maxY;
			maxY = swap;
		}

		if (minX == maxX || minY == maxY)
		{

			return;
		}

		Write<std::uint16_t>(map, NC2Layout::MacroMapOffset::Width, static_cast<std::uint16_t>(maxX - minX));
		Write<std::uint16_t>(map, NC2Layout::MacroMapOffset::Height, static_cast<std::uint16_t>(maxY - minY));
		const auto microMinX = static_cast<std::int16_t>(minX * 2), microMinY = static_cast<std::int16_t>(minY * 2);
		const auto microMaxX = static_cast<std::int16_t>(maxX * 2), microMaxY = static_cast<std::int16_t>(maxY * 2);
		const auto allocateSection = [&](std::uint32_t countOffset, std::uint32_t stride, bool clear)
		{
			const auto count = Read<std::uint32_t>(map, countOffset);

			if (count == 0)
			{

				return;
			}

			void* entries = NXBasics::Runtime_AllocateWithNewHandler(count * stride);
			Write(map, countOffset + NC2Layout::MacroMapSectionOffset::Entries, entries);

			if (clear)
				std::memset(entries, 0, Read<std::uint32_t>(map, countOffset) * stride);
		};
		const auto increment = [&](std::uint32_t offset)
		{
			Write(map, offset, Read<std::uint32_t>(map, offset) + 1u);
		};
		const auto entryAt = [&](std::uint32_t countOffset, std::uint32_t index, std::uint32_t stride)
		{

			return static_cast<std::byte*>(Read<void*>(map, countOffset + NC2Layout::MacroMapSectionOffset::Entries)) + index * stride;
		};
		const auto mirrorTriangles = [&](std::uint32_t countOffset, std::uint32_t stride, std::uint32_t triangleOffset)
		{
			if (mirrorX)
			{
				for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, countOffset); ++i)
				{
					auto* entry = entryAt(countOffset, i, stride);
					auto x = static_cast<std::uint16_t>(Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::Width) - Read<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapX));
					const bool triangle = Read<std::uint8_t>(entry, triangleOffset) != 0;

					if ((Read<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapY) & 1u) == 0)
					{
						if (triangle)
							++x;
					}
					else if (!triangle)
					{
						--x;
					}

					Write(entry, NC2Layout::MacroMapPointOffset::MapX, x);
				}
			}

			if (mirrorY)
			{
				for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, countOffset); ++i)
				{
					auto* entry = entryAt(countOffset, i, stride);
					const bool triangle = Read<std::uint8_t>(entry, triangleOffset) != 0;

					if (triangle)
						Write<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapX, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapX) - 1));

					Write<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapY, static_cast<std::uint16_t>(Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::Height) - Read<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapY)));
					Write<std::uint8_t>(entry, triangleOffset, !triangle);
				}
			}
		};
		const auto mirrorPoints = [&](std::uint32_t countOffset, std::uint32_t stride, std::uint32_t positionOffset, bool micro)
		{
			if (mirrorX)
			{
				for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, countOffset); ++i)
				{
					auto* entry = entryAt(countOffset, i, stride) + positionOffset;
					auto x = static_cast<std::uint16_t>(Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::Width) * (micro ? 2u : 1u) - Read<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapX));

					if ((Read<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapY) & 1u) != 0)
						--x;

					x = static_cast<std::uint16_t>(x + (micro ? 2u : 1u));
					Write(entry, NC2Layout::MacroMapPointOffset::MapX, x);
				}
			}

			if (mirrorY)
			{
				for (std::uint32_t i = 0; i < Read<std::uint32_t>(map, countOffset); ++i)
				{
					auto* entry = entryAt(countOffset, i, stride) + positionOffset;

					if (micro || (Read<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapY) & 1u) == 0)
						Write<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapX, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapX) - 1));

					const auto height = Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::Height);
					Write<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapY, static_cast<std::uint16_t>((micro ? static_cast<std::uint16_t>(height + 1u) * 2u : height + 1u) - Read<std::uint16_t>(entry, NC2Layout::MacroMapPointOffset::MapY)));
				}
			}
		};
		// Windows pattern capture: count occurrences, assign names in static-data order, then copy records.
		void* patternUsage = NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::uint32_t>(gStaticPatternCount) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned);
		std::memset(patternUsage, 0, static_cast<std::uint32_t>(gStaticPatternCount) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned);

		for (std::int32_t y = minY; y < maxY; ++y)
		{
			for (std::int32_t x = minX; x < maxX; ++x)
			{
				const auto engine = EngineMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
				const auto logic = NC2Logic::GameMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));

				for (std::uint32_t triangle = 0; triangle < 2; ++triangle)
				{
					if (Read<std::uint8_t>(logic, triangle) == 0)
						continue;

					const auto id = Read<std::uint16_t>(engine, triangle * NC2Layout::RecordSize::EngineTrianglePatternId);

					if (Read<std::uint32_t>(patternUsage, id * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned) == 0)
						increment(NC2Layout::MacroMapOffset::PatternNamesCount);

					Write(patternUsage, id * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned, Read<std::uint32_t>(patternUsage, id * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned) + 1u);
					increment(NC2Layout::MacroMapOffset::PatternsCount);
				}
			}
		}

		if (Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::PatternNamesCount) != 0)
		{
			allocateSection(NC2Layout::MacroMapOffset::PatternNamesCount, NC2Layout::RecordSize::MacroMapName, false);
			std::uint32_t nameIndex = 0;

			for (std::int32_t id = 0; id < gStaticPatternCount; ++id)
			{
				if (Read<std::uint32_t>(patternUsage, static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned) == 0)
				{
					Write<std::int32_t>(patternUsage, static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned + NC2Layout::MacroMapCaptureUsageOffset::NameIndexUnsigned, -1);
					continue;
				}

				Write(patternUsage, static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned + NC2Layout::MacroMapCaptureUsageOffset::NameIndexUnsigned, nameIndex);
				auto* name = entryAt(NC2Layout::MacroMapOffset::PatternNamesCount, nameIndex++, NC2Layout::RecordSize::MacroMapName);
				NXBasics::Runtime_CopyString(reinterpret_cast<char*>(name), reinterpret_cast<const char*>(gStaticPatternEntries) + static_cast<std::uint32_t>(id) * 112u);
				Write(name, NC2Layout::MacroMapNameOffset::StaticDataId, id);
			}

			allocateSection(NC2Layout::MacroMapOffset::PatternsCount, NC2Layout::RecordSize::MacroMapPattern, true);
			std::uint32_t index = 0;

			for (std::int32_t y = minY; y < maxY; ++y)
			{
				for (std::int32_t x = minX; x < maxX; ++x)
				{
					const auto engine = EngineMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
					const auto logic = NC2Logic::GameMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));

					for (std::uint32_t triangle = 0; triangle < 2; ++triangle)
					{
						if (Read<std::uint8_t>(logic, triangle) == 0)
							continue;

						auto* entry = entryAt(NC2Layout::MacroMapOffset::PatternsCount, index++, NC2Layout::RecordSize::MacroMapPattern);
						Write<std::uint16_t>(entry, NC2Layout::MacroMapPatternOffset::MapX, static_cast<std::uint16_t>(x - minX));
						Write<std::uint16_t>(entry, NC2Layout::MacroMapPatternOffset::MapY, static_cast<std::uint16_t>(y - minY));
						Write<std::uint8_t>(entry, NC2Layout::MacroMapPatternOffset::TriangleA, triangle == 0);
						Write(entry, NC2Layout::MacroMapPatternOffset::NameIndex, Read<std::uint32_t>(patternUsage, Read<std::uint16_t>(engine, triangle * NC2Layout::RecordSize::EngineTrianglePatternId) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned + NC2Layout::MacroMapCaptureUsageOffset::NameIndexUnsigned));
					}
				}
			}

			mirrorTriangles(NC2Layout::MacroMapOffset::PatternsCount, 12, 4);
		}

		NXBasics::Runtime_Free(patternUsage);
		// Windows transition capture: count occurrences, assign names in static-data order, then copy records.
		void* transitionUsage = NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::uint32_t>(gStaticTransitionStorage.count) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned);
		std::memset(transitionUsage, 0, static_cast<std::uint32_t>(gStaticTransitionStorage.count) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned);

		for (std::int32_t y = minY; y < maxY; ++y)
		{
			for (std::int32_t x = minX; x < maxX; ++x)
			{
				const auto engine = EngineMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));

				for (std::uint32_t layer = 0; layer < 2; ++layer)
				{
					for (std::uint32_t triangle = 0; triangle < 2; ++triangle)
					{
						const auto value = Read<std::uint8_t>(engine, NC2Layout::EngineMapMacroPointOffset::TransitionsUnsigned + layer * NC2Runtime::EngineMapEncoding::TrianglesPerLayerUnsigned + triangle);

						if (value == NXBasics::IntegerRange::UInt8Maximum)
							continue;

						const auto id = value / 6u;

						if (Read<std::uint32_t>(transitionUsage, id * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned) == 0)
							increment(NC2Layout::MacroMapOffset::TransitionNamesCount);

						Write(transitionUsage, id * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned, Read<std::uint32_t>(transitionUsage, id * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned) + 1u);
						increment(NC2Layout::MacroMapOffset::TransitionsCount);
					}
				}
			}
		}

		if (Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::TransitionNamesCount) != 0)
		{
			allocateSection(NC2Layout::MacroMapOffset::TransitionNamesCount, NC2Layout::RecordSize::MacroMapName, false);
			std::uint32_t nameIndex = 0;

			for (std::int32_t id = 0; id < gStaticTransitionStorage.count; ++id)
			{
				if (Read<std::uint32_t>(transitionUsage, static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned) == 0)
				{
					Write<std::int32_t>(transitionUsage, static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned + NC2Layout::MacroMapCaptureUsageOffset::NameIndexUnsigned, -1);
					continue;
				}

				Write(transitionUsage, static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned + NC2Layout::MacroMapCaptureUsageOffset::NameIndexUnsigned, nameIndex);
				auto* name = entryAt(NC2Layout::MacroMapOffset::TransitionNamesCount, nameIndex++, NC2Layout::RecordSize::MacroMapName);
				NXBasics::Runtime_CopyString(reinterpret_cast<char*>(name), reinterpret_cast<const char*>(gStaticTransitionStorage.entries) + static_cast<std::uint32_t>(id) * 340u);
				Write(name, NC2Layout::MacroMapNameOffset::StaticDataId, id);
			}

			allocateSection(NC2Layout::MacroMapOffset::TransitionsCount, NC2Layout::RecordSize::MacroMapTransition, true);
			std::uint32_t index = 0;

			for (std::int32_t y = minY; y < maxY; ++y)
			{
				for (std::int32_t x = minX; x < maxX; ++x)
				{
					const auto engine = EngineMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));

					for (std::uint32_t layer = 0; layer < 2; ++layer)
					{
						for (std::uint32_t triangle = 0; triangle < 2; ++triangle)
						{
							const auto value = Read<std::uint8_t>(engine, NC2Layout::EngineMapMacroPointOffset::TransitionsUnsigned + layer * NC2Runtime::EngineMapEncoding::TrianglesPerLayerUnsigned + triangle);

							if (value == NXBasics::IntegerRange::UInt8Maximum)
								continue;

							auto* entry = entryAt(NC2Layout::MacroMapOffset::TransitionsCount, index++, NC2Layout::RecordSize::MacroMapTransition);
							Write(entry, NC2Layout::MacroMapTransitionOffset::Layer, layer);
							Write<std::uint8_t>(entry, NC2Layout::MacroMapTransitionOffset::TriangleA, triangle == 0);
							Write<std::uint16_t>(entry, NC2Layout::MacroMapTransitionOffset::MapX, static_cast<std::uint16_t>(x - minX));
							Write<std::uint16_t>(entry, NC2Layout::MacroMapTransitionOffset::MapY, static_cast<std::uint16_t>(y - minY));
							Write(entry, NC2Layout::MacroMapTransitionOffset::NameIndex, Read<std::uint32_t>(transitionUsage, (value / 6u) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned + NC2Layout::MacroMapCaptureUsageOffset::NameIndexUnsigned));
						}
					}
				}
			}

			mirrorTriangles(NC2Layout::MacroMapOffset::TransitionsCount, 16, 8);
		}

		NXBasics::Runtime_Free(transitionUsage);
		// Windows landscape capture: count occurrences, assign names in static-data order, then copy records.
		void* landscapeUsage = NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::uint32_t>(gStaticLandscapeCount) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned);
		std::memset(landscapeUsage, 0, static_cast<std::uint32_t>(gStaticLandscapeCount) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned);

		for (std::int32_t y = microMinY; y < microMaxY; ++y)
		{
			for (std::int32_t x = microMinX; x < microMaxX; ++x)
			{
				const auto engine = EngineMap_GetConstMapMicroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
				const auto id = Read<std::uint16_t>(engine, NC2Layout::EngineMapMicroPointOffset::LandscapeId);

				if (id == NXBasics::IntegerRange::UInt16Maximum)
					continue;

				if (Read<std::uint32_t>(landscapeUsage, id * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned) == 0)
					increment(NC2Layout::MacroMapOffset::LandscapeNamesCount);

				Write(landscapeUsage, id * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned, Read<std::uint32_t>(landscapeUsage, id * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned) + 1u);
				increment(NC2Layout::MacroMapOffset::LandscapesCount);
			}
		}

		if (Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::LandscapeNamesCount) != 0)
		{
			allocateSection(NC2Layout::MacroMapOffset::LandscapeNamesCount, NC2Layout::RecordSize::MacroMapName, false);
			std::uint32_t nameIndex = 0;

			for (std::int32_t id = 0; id < gStaticLandscapeCount; ++id)
			{
				if (Read<std::uint32_t>(landscapeUsage, static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned) == 0)
				{
					Write<std::int32_t>(landscapeUsage, static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned + NC2Layout::MacroMapCaptureUsageOffset::NameIndexUnsigned, -1);
					continue;
				}

				Write(landscapeUsage, static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned + NC2Layout::MacroMapCaptureUsageOffset::NameIndexUnsigned, nameIndex);
				auto* name = entryAt(NC2Layout::MacroMapOffset::LandscapeNamesCount, nameIndex++, NC2Layout::RecordSize::MacroMapName);
				NXBasics::Runtime_CopyString(reinterpret_cast<char*>(name), reinterpret_cast<const char*>(gStaticLandscapeEntries) + static_cast<std::size_t>(static_cast<std::uint32_t>(id)) * sizeof(SE2StaticDataLandscape));
				Write(name, NC2Layout::MacroMapNameOffset::StaticDataId, id);
			}

			allocateSection(NC2Layout::MacroMapOffset::LandscapesCount, NC2Layout::RecordSize::MacroMapLandscape, true);
			std::uint32_t index = 0;

			for (std::int32_t y = microMinY; y < microMaxY; ++y)
			{
				for (std::int32_t x = microMinX; x < microMaxX; ++x)
				{
					const auto engine = EngineMap_GetConstMapMicroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
					const auto logic = NC2Logic::GameMap_GetConstMapMicroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
					const auto id = Read<std::uint16_t>(engine, NC2Layout::EngineMapMicroPointOffset::LandscapeId);

					if (id == NXBasics::IntegerRange::UInt16Maximum)
						continue;

					auto* entry = entryAt(NC2Layout::MacroMapOffset::LandscapesCount, index++, NC2Layout::RecordSize::MacroMapLandscape);
					Write<std::uint16_t>(entry, NC2Layout::MacroMapLandscapeOffset::MapX, static_cast<std::uint16_t>(x - microMinX));
					Write<std::uint16_t>(entry, NC2Layout::MacroMapLandscapeOffset::MapY, static_cast<std::uint16_t>(y - microMinY));
					Write(entry, NC2Layout::MacroMapLandscapeOffset::NameIndex, Read<std::uint32_t>(landscapeUsage, id * NC2Layout::RecordSize::MacroMapCaptureUsageUnsigned + NC2Layout::MacroMapCaptureUsageOffset::NameIndexUnsigned));
					Write<std::uint32_t>(entry, NC2Layout::MacroMapLandscapeOffset::Valency, Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::LandscapeValency) : 0);
					Write<std::int32_t>(entry, NC2Layout::MacroMapLandscapeOffset::PlayerId, Read<std::uint8_t>(logic, NC2Layout::MapMicroPointOffset::ObjectType) == 1 ? Read<std::int8_t>(logic, NC2Layout::MapMicroPointOffset::LandscapePlayerId) : -1);
				}
			}

			mirrorPoints(NC2Layout::MacroMapOffset::LandscapesCount, 16, 0, true);
		}

		NXBasics::Runtime_Free(landscapeUsage);

		for (std::int32_t y = minY; y < maxY; ++y)
		{
			for (std::int32_t x = minX; x < maxX; ++x)
			{
				if (Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y)), NC2Layout::MapMacroPointOffset::Height) != 0)
					increment(NC2Layout::MacroMapOffset::HeightsCount);
			}
		}

		if (Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::HeightsCount) != 0)
		{
			allocateSection(NC2Layout::MacroMapOffset::HeightsCount, NC2Layout::RecordSize::MacroMapHeight, true);
			std::uint32_t index = 0;

			for (std::int32_t y = minY; y < maxY; ++y)
			{
				for (std::int32_t x = minX; x < maxX; ++x)
				{
					const auto data = NC2Logic::GameMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));

					if (Read<std::uint8_t>(data, NC2Layout::MapMacroPointOffset::Height) == 0)
						continue;

					auto* entry = entryAt(NC2Layout::MacroMapOffset::HeightsCount, index++, NC2Layout::RecordSize::MacroMapHeight);
					Write<std::uint16_t>(entry, NC2Layout::MacroMapHeightOffset::MapX, static_cast<std::uint16_t>(x - minX));
					Write<std::uint16_t>(entry, NC2Layout::MacroMapHeightOffset::MapY, static_cast<std::uint16_t>(y - minY));
					Write(entry, NC2Layout::MacroMapHeightOffset::Height, Read<std::uint8_t>(data, NC2Layout::MapMacroPointOffset::Height));
				}
			}

			mirrorPoints(NC2Layout::MacroMapOffset::HeightsCount, 6, 0, false);
		}

		for (std::int32_t y = minY; y < maxY; ++y)
		{
			for (std::int32_t x = minX; x < maxX; ++x)
			{
				if (Read<std::uint8_t>(EngineMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y)), NC2Layout::EngineMapMacroPointOffset::VertexColorIndex) != 0)
					increment(NC2Layout::MacroMapOffset::VertexColorsCount);
			}
		}

		if (Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::VertexColorsCount) != 0)
		{
			allocateSection(NC2Layout::MacroMapOffset::VertexColorsCount, NC2Layout::RecordSize::MacroMapVertexColor, true);
			std::uint32_t index = 0;

			for (std::int32_t y = minY; y < maxY; ++y)
			{
				for (std::int32_t x = minX; x < maxX; ++x)
				{
					const auto data = EngineMap_GetConstMapMacroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));

					if (Read<std::uint8_t>(data, NC2Layout::EngineMapMacroPointOffset::VertexColorIndex) == 0)
						continue;

					auto* entry = entryAt(NC2Layout::MacroMapOffset::VertexColorsCount, index++, NC2Layout::RecordSize::MacroMapVertexColor);
					Write<std::uint16_t>(entry, NC2Layout::MacroMapVertexColorOffset::MapX, static_cast<std::uint16_t>(x - minX));
					Write<std::uint16_t>(entry, NC2Layout::MacroMapVertexColorOffset::MapY, static_cast<std::uint16_t>(y - minY));
					Write(entry, NC2Layout::MacroMapVertexColorOffset::ColorIndex, Read<std::uint8_t>(data, NC2Layout::EngineMapMacroPointOffset::VertexColorIndex));
				}
			}

			mirrorPoints(NC2Layout::MacroMapOffset::VertexColorsCount, 6, 0, false);
		}

		for (std::int32_t y = microMinY; y < microMaxY; ++y)
		{
			for (std::int32_t x = microMinX; x < microMaxX; ++x)
			{
				if ((Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y)), NC2Layout::MapMicroPointOffset::TerrainFlags) & 2u) != 0)
					increment(NC2Layout::MacroMapOffset::RoadsCount);
			}
		}

		if (Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::RoadsCount) != 0)
		{
			allocateSection(NC2Layout::MacroMapOffset::RoadsCount, NC2Layout::RecordSize::MacroMapRoad, true);
			std::uint32_t index = 0;

			for (std::int32_t y = microMinY; y < microMaxY; ++y)
			{
				for (std::int32_t x = microMinX; x < microMaxX; ++x)
				{
					if ((Read<std::uint8_t>(NC2Logic::GameMap_GetConstMapMicroPointPtr(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y)), NC2Layout::MapMicroPointOffset::TerrainFlags) & 2u) == 0)
						continue;

					auto* entry = entryAt(NC2Layout::MacroMapOffset::RoadsCount, index++, NC2Layout::RecordSize::MacroMapRoad);
					Write<std::uint16_t>(entry, NC2Layout::MacroMapRoadOffset::MapX, static_cast<std::uint16_t>(x - microMinX));
					Write<std::uint16_t>(entry, NC2Layout::MacroMapRoadOffset::MapY, static_cast<std::uint16_t>(y - microMinY));
				}
			}

			mirrorPoints(NC2Layout::MacroMapOffset::RoadsCount, 4, 0, true);
		}

		const auto inside = [&](const void* point)
		{
			const auto x = Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::X), y = Read<std::int16_t>(point, NC2Layout::MapCoordinatePairOffset::Y);

			return x >= microMinX && x < microMaxX && y >= microMinY && y < microMaxY;
		};

		if (includeCreatures)
		{
			for (std::int32_t id = 0; id < NC2Logic::gMapMoveableArrayStorage.manager->mCapacity; ++id)
			{
				const auto moveable = NC2Logic::gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

				if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag) != 0 && Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0 && inside(moveable + NC2Layout::MapMoveableOffset::MapPosition) && Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) == 1)
					increment(NC2Layout::MacroMapOffset::HumansCount);
			}

			if (Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::HumansCount) != 0)
			{
				allocateSection(NC2Layout::MacroMapOffset::HumansCount, NC2Layout::RecordSize::MacroMapCreature, true);
				std::uint32_t index = 0;

				for (std::int32_t id = 0; id < NC2Logic::gMapMoveableArrayStorage.manager->mCapacity; ++id)
				{
					const auto moveable = NC2Logic::gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

					if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag) == 0 || Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0 || !inside(moveable + NC2Layout::MapMoveableOffset::MapPosition) || Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) != 1)
						continue;

					const auto object = NC2Logic::CHumanArray::mStaticVars + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId) * 3160u;
					auto* entry = entryAt(NC2Layout::MacroMapOffset::HumansCount, index++, NC2Layout::RecordSize::MacroMapCreature);
					Write<std::uint16_t>(entry, NC2Layout::MacroMapCreatureOffset::MapX, static_cast<std::uint16_t>(Read<std::uint16_t>(NC2Logic::Human_GetMapPositionRef(object), NC2Layout::MapCoordinatePairOffset::X) - microMinX));
					Write<std::uint16_t>(entry, NC2Layout::MacroMapCreatureOffset::MapY, static_cast<std::uint16_t>(Read<std::uint16_t>(NC2Logic::Human_GetMapPositionRef(object), NC2Layout::MapCoordinatePairOffset::Y) - microMinY));
					Write(entry, NC2Layout::MacroMapCreatureOffset::Type, Read<std::uint32_t>(object, NC2Layout::HumanOffset::JobType));
					Write(entry, NC2Layout::MacroMapCreatureOffset::PlayerId, Read<std::uint32_t>(object, NC2Layout::HumanOffset::PlayerId));
					Write(entry, NC2Layout::MacroMapCreatureOffset::TribeType, Read<std::uint32_t>(object, NC2Layout::HumanOffset::TribeType));
				}

				mirrorPoints(NC2Layout::MacroMapOffset::HumansCount, 16, 0, true);
			}

			for (std::int32_t id = 0; id < NC2Logic::gMapMoveableArrayStorage.manager->mCapacity; ++id)
			{
				const auto moveable = NC2Logic::gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

				if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag) != 0 && Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0 && inside(moveable + NC2Layout::MapMoveableOffset::MapPosition) && Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) == 2)
					increment(NC2Layout::MacroMapOffset::AnimalsCount);
			}

			if (Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::AnimalsCount) != 0)
			{
				allocateSection(NC2Layout::MacroMapOffset::AnimalsCount, NC2Layout::RecordSize::MacroMapCreature, true);
				std::uint32_t index = 0;

				for (std::int32_t id = 0; id < NC2Logic::gMapMoveableArrayStorage.manager->mCapacity; ++id)
				{
					const auto moveable = NC2Logic::gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;

					if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AllocatedFlag) == 0 || Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) == 0 || !inside(moveable + NC2Layout::MapMoveableOffset::MapPosition) || Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectType) != 2)
						continue;

					const auto object = NC2Logic::CAnimalArray::mStaticVars + Read<std::uint32_t>(moveable, NC2Layout::MapMoveableOffset::ObjectId) * 152u;
					auto* entry = entryAt(NC2Layout::MacroMapOffset::AnimalsCount, index++, NC2Layout::RecordSize::MacroMapCreature);
					Write<std::uint16_t>(entry, NC2Layout::MacroMapCreatureOffset::MapX, static_cast<std::uint16_t>(Read<std::uint16_t>(NC2Logic::Animal_GetMapPositionRef(object), NC2Layout::MapCoordinatePairOffset::X) - microMinX));
					Write<std::uint16_t>(entry, NC2Layout::MacroMapCreatureOffset::MapY, static_cast<std::uint16_t>(Read<std::uint16_t>(NC2Logic::Animal_GetMapPositionRef(object), NC2Layout::MapCoordinatePairOffset::Y) - microMinY));
					Write(entry, NC2Layout::MacroMapCreatureOffset::Type, Read<std::uint32_t>(object, NC2Layout::AnimalOffset::AnimalType));
					Write(entry, NC2Layout::MacroMapCreatureOffset::PlayerId, Read<std::uint32_t>(object, NC2Layout::AnimalOffset::PlayerId));
					Write(entry, NC2Layout::MacroMapCreatureOffset::TribeType, Read<std::uint32_t>(object, NC2Layout::AnimalOffset::TribeType));
				}

				mirrorPoints(NC2Layout::MacroMapOffset::AnimalsCount, 16, 0, true);
			}

			for (std::int32_t id = 0; id < NC2Logic::CHouseArray::sTheObjectPtr->mCapacity; ++id)
			{
				const auto house = NC2Logic::CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * 0x7E0u;

				if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 && inside(house + NC2Layout::HouseOffset::MapPosition))
					increment(NC2Layout::MacroMapOffset::HousesCount);
			}

			if (Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::HousesCount) != 0)
			{
				allocateSection(NC2Layout::MacroMapOffset::HousesCount, NC2Layout::RecordSize::MacroMapHouse, true);
				std::uint32_t index = 0;

				for (std::int32_t id = 0; id < NC2Logic::CHouseArray::sTheObjectPtr->mCapacity; ++id)
				{
					const auto house = NC2Logic::CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * 0x7E0u;

					if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0 || !inside(house + NC2Layout::HouseOffset::MapPosition))
						continue;

					auto* entry = entryAt(NC2Layout::MacroMapOffset::HousesCount, index++, NC2Layout::RecordSize::MacroMapHouse);
					Write(entry, NC2Layout::MacroMapHouseOffset::PlayerId, Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId));
					Write<std::uint16_t>(entry, NC2Layout::MacroMapHouseOffset::MapX, static_cast<std::uint16_t>(Read<std::uint16_t>(house, NC2Layout::HouseOffset::MapPositionX) - microMinX));
					Write<std::uint16_t>(entry, NC2Layout::MacroMapHouseOffset::MapY, static_cast<std::uint16_t>(Read<std::uint16_t>(house, NC2Layout::HouseOffset::MapPositionY) - microMinY));
					Write(entry, NC2Layout::MacroMapHouseOffset::HouseType, Read<std::uint32_t>(house, NC2Layout::HouseOffset::EngineType));
					Write(entry, NC2Layout::MacroMapHouseOffset::DevelopmentStage, Read<std::uint32_t>(house, NC2Layout::HouseOffset::DevelopmentStage));
					Write<std::uint8_t>(entry, NC2Layout::MacroMapHouseOffset::Completed, Read<std::uint32_t>(house, NC2Layout::HouseOffset::ConstructionState) == 3);
				}

				mirrorPoints(NC2Layout::MacroMapOffset::HousesCount, 20, 4, true);
			}
		}
		// Windows retains the total swarm count; out-of-region slots stay zero rather than being compacted.
		Write(map, NC2Layout::MacroMapOffset::FishSwarmsCount, static_cast<NC2Logic::CMapFishManager*>(NC2Logic::gMapFishManager)->mSwarmCount);

		if (Read<std::uint32_t>(map, NC2Layout::MacroMapOffset::FishSwarmsCount) != 0)
		{
			allocateSection(NC2Layout::MacroMapOffset::FishSwarmsCount, NC2Layout::RecordSize::MacroMapFishSwarm, true);

			for (std::int32_t id = 0; id < Read<std::int32_t>(map, NC2Layout::MacroMapOffset::FishSwarmsCount); ++id)
			{
				const auto* swarm = &static_cast<NC2Logic::CMapFishManager*>(NC2Logic::gMapFishManager)->mSwarms[id];

				if (!inside(swarm))
					continue;

				auto* entry = entryAt(NC2Layout::MacroMapOffset::FishSwarmsCount, static_cast<std::uint32_t>(id), NC2Layout::RecordSize::MacroMapFishSwarm);
				Write<std::uint16_t>(entry, NC2Layout::MacroMapFishSwarmOffset::MapX, static_cast<std::uint16_t>(Read<std::uint16_t>(swarm, NC2Layout::FishSwarmOffset::MapX) - microMinX));
				Write<std::uint16_t>(entry, NC2Layout::MacroMapFishSwarmOffset::MapY, static_cast<std::uint16_t>(Read<std::uint16_t>(swarm, NC2Layout::FishSwarmOffset::MapY) - microMinY));
				Write(entry, NC2Layout::MacroMapFishSwarmOffset::FishCount, Read<std::uint32_t>(swarm, NC2Layout::FishSwarmOffset::FishCount));

				if (mirrorX)
				{
					auto x = static_cast<std::uint16_t>(Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::Width) * 2u - Read<std::uint16_t>(entry, NC2Layout::MacroMapFishSwarmOffset::MapX));

					if ((Read<std::uint16_t>(entry, NC2Layout::MacroMapFishSwarmOffset::MapY) & 1u) != 0)
						--x;

					Write<std::uint16_t>(entry, NC2Layout::MacroMapFishSwarmOffset::MapX, static_cast<std::uint16_t>(x + 2u));
				}

				if (mirrorY)
				{
					Write<std::uint16_t>(entry, NC2Layout::MacroMapFishSwarmOffset::MapX, static_cast<std::uint16_t>(Read<std::uint16_t>(entry, NC2Layout::MacroMapFishSwarmOffset::MapX) - 1));
					Write<std::uint16_t>(entry, NC2Layout::MacroMapFishSwarmOffset::MapY, static_cast<std::uint16_t>(static_cast<std::uint16_t>(Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::Height) + 1u) * 2u - Read<std::uint16_t>(entry, NC2Layout::MacroMapFishSwarmOffset::MapY)));
				}
			}
		}

		if (filename != nullptr)
			NXBasics::Runtime_CopyString(static_cast<char*>(map) + NC2Layout::MacroMapOffset::Filename, filename);

		Write<std::uint8_t>(map, NC2Layout::MacroMapOffset::Initialized, 1);
	}

	// Original function: FUN_00493472; MapIO_NewMap; the Mac symbol supplies the name only.
	void MapIO_NewMap(void* map, bool enableTime, std::int16_t width, std::int16_t height, bool initializeAllPlayers)
	{
		if (Read<std::uint8_t>(map, NC2Layout::MacroMapOffset::Initialized) == 0)
			return;

		if (width == 0)
			width = Read<std::int16_t>(map, NC2Layout::MacroMapOffset::Width);

		if (height == 0)
			height = Read<std::int16_t>(map, NC2Layout::MacroMapOffset::Height);

		if (width == 0 || height == 0)
		{

			return;
		}

		NC2Logic::Game_ShutDown();
		NC2Logic::gGameControlStartupData.Reset();
		NC2Logic::Game_StartUp(width, height, enableTime, nullptr, false);

		if (initializeAllPlayers)
		{
			for (std::uint32_t offset = 0; offset < NC2Layout::RecordSize::GameControlPlayerArray; offset += NC2Layout::RecordSize::GameControlPlayer)
			{
				auto* player = static_cast<std::byte*>(NC2Logic::gGameControl) + offset;
				Write<std::uint8_t>(player, NC2Layout::GameControlPlayerOffset::ActiveFlag, 1);
				Write<std::uint32_t>(player, NC2Layout::GameControlPlayerOffset::Type, 1);
				Write<std::uint32_t>(player, NC2Layout::GameControlPlayerOffset::TribeType, 1);
			}

			NC2Logic::GameControl_MiscSystem_PlayerDynamicData_InitStructures(NC2Logic::gGameControl);
		}

		NC2Logic::GameMapModifyManager_StartGameMapModify();
		std::uint16_t root[2];
		NC2Logic::MapMagPoint_Construct(root, Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterX), Read<std::uint16_t>(map, NC2Layout::MacroMapOffset::CenterY), false);
		MacroMap_SetPatternToGameMap(map, root, nullptr);
		MacroMap_SetTransitionsToGameMap(map, root, nullptr);
		MacroMap_SetLandscapesToGameMap(map, root, nullptr, true);
		MacroMap_SetHeightsToGameMap(map, root, 0, nullptr);
		MacroMap_SetVertexColorsToGameMap(map, root, nullptr);
		MacroMap_SetRoadsToGameMap(map, root, nullptr);
		NC2Logic::GameMapModifyManager_ForceUpdate();
		MacroMap_SetHousesToGameMap(map, root);
		MacroMap_SetHumansToGameMap(map, root);
		MacroMap_SetAnimalsToGameMap(map, root);
		MacroMap_SetFishSwarmsToGameMap(map, root);
		NC2Logic::GameMapModifyManager_StopGameMapModify();
	}

	// Original function: FUN_004935ba; the map GUID survives the complete recreation.
	void MapIO_RebuildCurrentMapViaMacroMap(std::int16_t width, std::int16_t height)
	{
		std::byte guid[16];
		std::memcpy(guid, &NC2Logic::gGameStaticVars.mapGuid, 16);
		NC2Logic::GameMapModifyManager_StartGameMapModify();
		std::byte map[NC2Layout::RecordSize::MacroMap];
		MacroMap_Construct(map, nullptr);
		MacroMap_CreateFromGameMap(map, nullptr, false, 0, 0, static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroWidth()), static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroHeight()), false, false);

		if (width == 0)
			width = static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroWidth());

		if (height == 0)
			height = static_cast<std::int16_t>(NC2Logic::GameMap_GetMacroHeight());

		MapIO_NewMap(map, false, width, height, false);
		NC2Logic::GameMapModifyManager_StopGameMapModify();
		std::memcpy(&NC2Logic::gGameStaticVars.mapGuid, guid, 16);
		MacroMap_Destruct(map);
	}

	// Original function: FUN_00483e58; the flag is cleared only after rebuilding.
	void EngineMap_PostLoad_RebuildIfRequired()
	{
		if (Read<std::uint8_t>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MissingResourceFlag) == 0)
		{

			return;
		}

		MapIO_RebuildCurrentMapViaMacroMap(0, 0);
		Write<std::uint8_t>(gEngineMapStaticStorage, NC2Layout::EngineMapStaticOffset::MissingResourceFlag, 0);
	}
}