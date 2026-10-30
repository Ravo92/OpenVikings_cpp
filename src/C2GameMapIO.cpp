#include "C2CallbackManager.h"
#include "C2CleanMapsManager.h"
#include "C2DataIds.h"
#include "C2EngineLifetimeAccess.h"
#include "C2GameEngineStartup.h"
#include "C2GameMapAccess.h"
#include "C2GameMapIO.h"
#include "C2IoHelper.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MissionScriptData.h"
#include "C2ObjectLayout.h"
#include "XFileAccess.h"
#include "XIniFileReader.h"

#include <cstring>
#include <new>
#include <Windows.h>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Script managers retain their separate native dispatch entries.
	extern bool __fastcall AIManager_IO_InitFromScript(void*, void* unusedEdx, NXBasics::CIniFileReader*);
	extern void __fastcall HumanMerchantArray_CleanMap_LoadTradeAgreements(void*, void* unusedEdx, NXBasics::CIniFileReader*);
	extern void __fastcall MultiPlayerGoalManager_IO_LoadGoals(void*, void* unusedEdx, NXBasics::CIniFileReader*);

	namespace
	{
#ifdef _DEBUG
		// Reconstruction diagnostics only; do not select frames or consume random state.
		void DebugReportLoadedHumans(const char* mapRoot, const char* archiveFilename)
		{
			struct SCounts
			{
				std::uint32_t allocated = 0, attached = 0, hidden = 0, bodies = 0, heads = 0, explored = 0, turning = 0;
			};

			SCounts counts[20];

			for (auto id = CHumanArray::sTheObjectPtr->mFirstScanIndex; id <= CHumanArray::sTheObjectPtr->mLastScanIndex; ++id)
			{
				const auto* human = CHumanArray::mStaticVars + static_cast<std::uint32_t>(id) * CHumanArray::RuntimeElementSize;

				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0)
					continue;

				const auto player = Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId);

				if (player >= 20u)
					continue;

				auto& count = counts[player];
				++count.allocated;

				if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideHouseFlag) != 0 || Read<std::uint8_t>(human, NC2Layout::HumanOffset::InsideVehicleFlag) != 0)
					++count.hidden;

				const auto moveableId = Read<std::int32_t>(human, NC2Layout::HumanOffset::MapMoveableId);

				if (moveableId != -1)
				{
					const auto* moveable = gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(moveableId) * NC2Layout::RecordSize::MapMoveable;

					if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::TurningFlag) != 0)
						++count.turning;

					if (Read<std::uint8_t>(moveable, NC2Layout::MapMoveableOffset::AttachedToMapFlag) != 0)
					{
						++count.attached;

						if (player < 16u && (Read<std::uint16_t>(GameMap_GetConstMapMicroPointPtr(moveable + NC2Layout::MapMoveableOffset::MapPosition), NC2Layout::MapMicroPointOffset::ExplorationMask) & (1u << player)) != 0)
							++count.explored;
					}
				}

				const auto graphics = NC2E2::Remaster_DynamicHuman_GetBobPresence(static_cast<std::uint32_t>(id));

				if ((graphics & 1u) != 0)
					++count.bodies;

				if ((graphics & 2u) != 0)
					++count.heads;
			}

			::OutputDebugStringA("[OpenVikings] Loaded human overview: ");
			::OutputDebugStringA(mapRoot != nullptr ? mapRoot : archiveFilename);
			::OutputDebugStringA("\n");

			for (std::uint32_t player = 0; player < 20u; ++player)
			{
				const auto* data = static_cast<const std::byte*>(gGameControl) + player * NC2Layout::RecordSize::GameControlPlayer;

				if (counts[player].allocated == 0 && Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag) == 0)
					continue;

				const auto& c = counts[player];
				char line[256];
				NXBasics::Runtime_PrintString(line, "[OpenVikings] player=%u active=%u type=%u tribe=%u humans=%u attached=%u hidden=%u bodies=%u heads=%u explored=%u turning=%u\n", player, static_cast<std::uint32_t>(Read<std::uint8_t>(data, NC2Layout::GameControlPlayerOffset::ActiveFlag)), Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::Type), Read<std::uint32_t>(data, NC2Layout::GameControlPlayerOffset::TribeType), c.allocated, c.attached, c.hidden, c.bodies, c.heads, c.explored, c.turning);
				::OutputDebugStringA(line);
			}
		}
#endif

		void ReleaseIni(NXBasics::CIniFileReader* reader)
		{
			if (reader != nullptr)
			{
				reader->~CIniFileReader();
				NXBasics::Runtime_Free(reader);
			}
		}

		void ReleaseLibrary(void* library)
		{
			if (library != nullptr)
			{
				NXBasics::Archive_Destroy(library);
				NXBasics::Runtime_Free(library);
			}
		}
	}

	// Original function: FUN_00407586.
	void CleanMapGuid_InitWhenNotSet()
	{
		if (!gGameStaticVars.mapGuid.IsValid())
		{
			CCleanMapsManager::Tool_GenerateNewGuid(&gGameStaticVars.mapGuid);
			gCleanMapsManager->CleanMapList_Update();
		}
	}

	// Original function: FUN_0040a68b; distinct directory-loading entry.
	void GameIO_Cleanmap_Load(const char* mapRoot, bool enableTime, bool retainIni, bool networkGame)
	{
		GameIO_Cleanmap_LoadX(mapRoot, nullptr, enableTime, retainIni, networkGame);
	}

	// Original function: FUN_0040a6a6.
	bool GameIO_Cleanmap_LoadX(const char* mapRoot, const char* libraryFilename, bool enableTime, bool retainIni, bool networkGame)
	{
		const bool libraryMode = libraryFilename != nullptr;
		const bool rootMode = !libraryMode && mapRoot != nullptr;

		if (!libraryMode && !rootMode)
			return false;

		NXBasics::CIniFileReader* reader = nullptr;
		void* library = nullptr;

		if (rootMode)
		{
			NXBasics::CFile::FileSystem_SystemPathName_Set("$maproot$", mapRoot);

			if (void* storage = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CIniFileReader), 1))
				reader = ::new (storage) NXBasics::CIniFileReader("$maproot$\\map.ini", nullptr, false, nullptr, nullptr);
		}

		if (libraryMode)
		{
			if (void* storage = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::SArchiveState), 1))
				library = NXBasics::Archive_Initialize(storage, libraryFilename);

			NXBasics::CFile::FileSystem_FileLibrary_Add(library);

			if (void* storage = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CIniFileReader), 1))
				reader = ::new (storage) NXBasics::CIniFileReader("currentusermap\\map.ini", nullptr, false, nullptr, nullptr);
		}

		// Windows dereferences the result even after a failed reader allocation.

		if (!reader->IsLoaded())
		{
			ReleaseIni(reader);
			ReleaseLibrary(library);
			return false;
		}

		if (reader->SearchGroup("logiccontrol") && reader->SearchCommand("version"))
			reader->GetInt();

		if (!reader->SearchGroup("logiccontrol") || !reader->SearchCommand("mapsize"))
		{
			ReleaseIni(reader);
			ReleaseLibrary(library);
			return false;
		}

		const std::int32_t width = reader->GetInt();
		const std::int32_t height = reader->GetInt();
		Game_StartUp(static_cast<std::int16_t>(width), static_cast<std::int16_t>(height), false, reader, networkGame);

		if (reader->SearchGroup("logiccontrol") && reader->SearchCommand("mapguid"))
			for (std::uint32_t index = 0; index < 16; ++index)
			{
				gGameStaticVars.mapGuid.bytes[index] = static_cast<std::uint8_t>(reader->GetInt());
			}

		CleanMapGuid_InitWhenNotSet();

		if (reader->SearchGroup("misc_maptype") && reader->SearchCommand("maptype"))
			gGameStaticVars.mapType = static_cast<std::uint32_t>(reader->GetInt());

		if (retainIni)
		{
			if (mapRoot != nullptr)
				std::strcpy(gGameStaticVars.retainedMapRoot, mapRoot);

			NC2Logic::gGameStaticVars.retainedMapIni = reader;
		}

		{
			char mapDataFilename[260];
			mapDataFilename[0] = '\0';
			std::strcpy(mapDataFilename, rootMode ? "$maproot$\\map.dat" : "currentusermap\\map.dat");
			NXBasics::CFile file(mapDataFilename, true);
			CIoHelper io;
			io.IO_File_Init(&file);
			bool loaded = io.IO_File_Chunk_CheckMarkerAndVersion(NC2DataIds::LogicMapChunk::LogicFileStart, 0) && GameMap_IO_Cleanmap_Load(&io);

			if (loaded)
			{
				CCallbackManager::sTheObjectPtr->ExecuteCallback(0x13, false, reinterpret_cast<std::uintptr_t>(&io), 0, 0);
				loaded = io.IO_File_Chunk_CheckMarkerAndVersion(NC2DataIds::LogicMapChunk::LogicFileEnd, 0);
			}

			if (!loaded)
			{
				// Free these before the stack I/O helper and file, as in Windows.
				ReleaseIni(reader);
				NC2Logic::gGameStaticVars.retainedMapIni = nullptr;
				ReleaseLibrary(library);
				return false;
			}
		}

		if (reader != nullptr)
		{
			MissionManager_LoadMissionScript(gMissionManager, nullptr, reader);
			AIManager_IO_InitFromScript(gAIManager, nullptr, reader);
			HumanMerchantArray_CleanMap_LoadTradeAgreements(gHumanMerchantArray, nullptr, reader);
			MultiPlayerGoalManager_IO_LoadGoals(gMultiPlayerGoalManager, nullptr, reader);
		}

		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x16, false, 1, 0, 0);
#ifdef _DEBUG
		DebugReportLoadedHumans(mapRoot, libraryFilename);
#endif

		if (rootMode)
			NXBasics::CFile::FileSystem_SystemPathName_Clear("$maproot$");

		if (libraryMode)
			NC2Logic::gGameStaticVars.mapFileLibrary = library;

		if (reader != NC2Logic::gGameStaticVars.retainedMapIni)
			ReleaseIni(reader);

		if (enableTime)
			GameTime_Reset(true);

		return true;
	}

	// Original function: FUN_0042fbcc; validates the map envelope before dispatch.
	bool GameMap_IO_Cleanmap_Load(CIoHelper* io)
	{
		std::uint32_t version;

		if (!io->IO_File_Chunk_CheckMarkerAndGetVersion(NC2DataIds::LogicMapChunk::MapData, &version, nullptr))
			return false;

		return GameMap_l_GlobalIO_Load(io);
	}
}