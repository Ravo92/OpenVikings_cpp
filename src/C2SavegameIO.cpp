#include "CivilizationBuildings.h"
#include "C2AISavegameAccess.h"
#include "XTextEncoding.h"
#include "C2CallbackManager.h"
#include "C2CleanMapsManager.h"
#include "C2DataIds.h"
#include "C2GameMapIO.h"
#include "C2IoHelper.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicLifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2SavegameIO.h"
#include "C2SavegamesManager.h"
#include "C2SavegameStateAccess.h"
#include "NetworkShutdown.h"
#include "SwitchIdentifiers.h"
#include "XFile.h"
#include "XFileAccess.h"
#include "XMemory.h"
#include "XNumericConstants.h"
#include <Windows.h>

namespace NC2Logic
{
	// Original function: FUN_00425bfb.
	std::int32_t GameIO_Savegame_GetInfoHeader(const char* filename, SSavegameInfoHeaderData* header)
	{
		char fullFilename[260];
		gSavegamesManager->Tool_CreateFullSavegameFilename(filename, fullFilename);
		NXBasics::CFile file(fullFilename, false);
		file.OpenForReading(true, true);
		CIoHelper io;
		io.IO_File_Init(&file);
		NXBasics::CMemory memory(1000u);
		io.IO_Memory_Init(&memory);
		return l_GameIO_Savegame_DoLoad_CheckHeaderAndGetInfos(io, header);
	}

	// Original function: FUN_004264a7.
	std::int32_t l_GameIO_Savegame_DoLoad_CheckHeaderAndGetInfos(CIoHelper& io, SSavegameInfoHeaderData* header)
	{
		std::uint32_t version = 0;
		if (!io.IO_File_Chunk_CheckMarkerAndGetVersion(NC2DataIds::SavegameChunk::Header, &version, nullptr))
			return -1;
		io.IO_File_Chunk_Skip();
		std::uint32_t infoVersion = 0;
		if (!io.IO_File_Chunk_CheckMarkerAndGetVersion(NC2DataIds::SavegameChunk::Info, &infoVersion, nullptr))
			return -1;
		if (header == nullptr)
		{
			io.IO_File_Chunk_Skip();
			return static_cast<std::int32_t>(version);
		}
		std::memset(header, 0, sizeof(*header));
		header->version = version;
		header->slotId = 10000;
		if (infoVersion < 2)
		{
			std::strcpy(header->info, "(no infos found)");
			io.IO_File_Chunk_Skip();
			return static_cast<std::int32_t>(version);
		}
		while (io.IO_File_Chunk_ReadHeader())
		{
			switch (io.mReadHeader.marker)
			{
				case SwitchIds::SavegameInfoChunk::InfoText:
					NXBasics::Text_Copy(header->info, sizeof(header->info), io.IO_File_ReadString());
					break;
				case SwitchIds::SavegameInfoChunk::MapName:
					NXBasics::Text_Copy(header->mapName, sizeof(header->mapName), io.IO_File_ReadString());
					break;
				case SwitchIds::SavegameInfoChunk::MapGuid:
					io.IO_File_ReadBinary(&header->mapGuid, 16);
					break;
				case SwitchIds::SavegameInfoChunk::MultiplayerPlayers:
					{
						header->multiplayer = 1;
						// Original entries end at a negative index, without an upper-bound check.
						for (std::int32_t index = static_cast<std::int32_t>(io.IO_File_ReadLong()); index >= 0; index = static_cast<std::int32_t>(io.IO_File_ReadLong()))
						{
							auto& player = header->players[index];
							player.playerType = io.IO_File_ReadLong();
							NXBasics::Text_Copy(player.name, sizeof(player.name), io.IO_File_ReadString());
							player.colorId = io.IO_File_ReadLong();
						}
						break;
					}
				case SwitchIds::SavegameInfoChunk::CreationTime:
					for (auto& field : header->creationTimeFields)
						field = io.IO_File_ReadLong();
					break;
				case SwitchIds::SavegameInfoChunk::SlotId:
					header->slotId = static_cast<std::int32_t>(io.IO_File_ReadLong());
					break;
				case SwitchIds::SavegameInfoChunk::EndInfoHeader:
					return static_cast<std::int32_t>(version);
				default:
					io.IO_File_Chunk_Skip();
					break;
			}
		}
		return static_cast<std::int32_t>(version);
	}
	// Original function: FUN_004075b0.
	static void Misc_GetInternalNameAndVersionString(char* destination)
	{
		NXBasics::Runtime_PrintString(destination, "%s - Version %d.%2.2d - Build date Nov 29 2003 17:36:55", "Weltwunder", 1, 0);
	}

	// Original function: FUN_00426229.
	static void l_GameIO_Savegame_DoSave_WriteHeaderAndInfos(CIoHelper* io, const char* info, std::int32_t slotId)
	{
		io->IO_Tool_Chunk_StartCreation(NC2DataIds::SavegameChunk::Header, 1);
		io->IO_Tool_Chunk_FinishAndWrite();
		io->IO_Tool_Chunk_StartCreation(NC2DataIds::SavegameChunk::Info, 2);
		io->IO_Tool_Chunk_StartCreation(SwitchIds::SavegameInfoChunk::InfoText, 1);
		io->IO_Memory_WriteString(info != nullptr && std::strlen(info) != 0 ? info : "(no infos)");
		io->IO_Tool_Chunk_FinishAndWrite();
		io->IO_Tool_Chunk_StartCreation(SwitchIds::SavegameInfoChunk::MapName, 1);
		char mapName[220];
		mapName[0] = '\0';
		if (auto* map = gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(gGameStaticVars.mapGuid)) map->Tool_GetMapName(mapName);
		io->IO_Memory_WriteString(std::strlen(mapName) != 0 ? mapName : "");
		io->IO_Tool_Chunk_FinishAndWrite();
		io->IO_Tool_Chunk_StartCreation(SwitchIds::SavegameInfoChunk::SlotId, 1);
		io->IO_Memory_WriteLong(static_cast<std::uint32_t>(slotId));
		io->IO_Tool_Chunk_FinishAndWrite();
		io->IO_Tool_Chunk_StartCreation(NC2DataIds::SavegameInfoChunk::EndMetadata, 1);
		char version[96];
		Misc_GetInternalNameAndVersionString(version);
		io->IO_Memory_WriteString(version);
		io->IO_Tool_Chunk_FinishAndWrite();
		io->IO_Tool_Chunk_StartCreation(SwitchIds::SavegameInfoChunk::CreationTime, 1);
		SYSTEMTIME time;
		::GetLocalTime(&time);
		for (auto value : { time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds }) io->IO_Memory_WriteLong(value);
		io->IO_Tool_Chunk_FinishAndWrite();
		io->IO_Tool_Chunk_StartCreation(SwitchIds::SavegameInfoChunk::MapGuid, 1);
		io->IO_Memory_WriteBinary(&gGameStaticVars.mapGuid, 16);
		io->IO_Tool_Chunk_FinishAndWrite();
		if (gGameStaticVars.networkGame != 0)
		{
			io->IO_Tool_Chunk_StartCreation(SwitchIds::SavegameInfoChunk::MultiplayerPlayers, 1);
			for (std::uint32_t player = 0; player < 20; ++player)
			{
				const auto offset = player * NC2Layout::RecordSize::GameControlPlayerSigned;
				if (NC2Lifetime::Read<std::uint8_t>(gGameControl, offset) == 0) break;
				const auto type = NC2Lifetime::Read<std::uint32_t>(gGameControl, offset + NC2Layout::GameControlPlayerOffset::Type);
				const auto color = NC2Lifetime::Read<std::uint32_t>(gGameControl, offset + NC2Layout::GameControlPlayerOffset::ColorId);
				char name[216];
				name[0] = '\0';
				if (type == 1 && !NNetwork::Network_GetPlayerName(NC2Lifetime::Read<std::uint32_t>(gGameControl, offset + NC2Layout::GameControlPlayerOffset::NetworkPlayerId), name)) name[0] = '\0';
				io->IO_Memory_WriteLong(player);
				io->IO_Memory_WriteLong(type);
				io->IO_Memory_WriteString(name);
				io->IO_Memory_WriteLong(color);
			}
			io->IO_Memory_WriteLong(NXBasics::IntegerRange::UInt32Maximum);
			io->IO_Tool_Chunk_FinishAndWrite();
		}
		io->IO_Tool_Chunk_StartCreation(SwitchIds::SavegameInfoChunk::EndInfoHeader, 1);
		io->IO_Tool_Chunk_FinishAndWrite();
		io->IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_00425c9d.
	static void l_GameIO_Savegame_DoSave(CIoHelper* io, const char* info, std::int32_t slotId)
	{
		l_GameIO_Savegame_DoSave_WriteHeaderAndInfos(io, info, slotId);
		NRemaster::CivilizationBuildings::WriteSaveManifest(io);
		io->IO_Tool_Chunk_StartCreation(SwitchIds::SavegameChunk::GameState, 1);
		io->IO_Memory_WriteBinary(&gGameStaticVars.mapGuid, 0x98);
		io->IO_Tool_Chunk_FinishAndWrite();
		GameControl_IO_Savegame_DoSave(gGameControl, nullptr, io);
		GameMap_IO_Savegame_DoSave(gGameMap, nullptr, io);
		MissionManager_IO_Savegame_DoSave(gMissionManager, nullptr, io);
		AIManager_IO_Savegame_DoSave(gAIManager, nullptr, io);
		MapMoveableArray_IO_Savegame_DoSave(gMapMoveableArrayStorage.manager, nullptr, io);
		MapPathfinderManager_IO_Savegame_DoSave(gMapPathfinderManager, nullptr, io);
		CHumanArray::sTheObjectPtr->IO_Savegame_DoSave(io);
		HumanMerchantArray_IO_Savegame_DoSave(gHumanMerchantArray, nullptr, io);
		CAnimalArray::sTheObjectPtr->IO_Savegame_DoSave(io);
		CVehicleArray::sTheObjectPtr->IO_Savegame_DoSave(io);
		MarkerArray_IO_Savegame_DoSave(gMarkerArray, nullptr, io);
		CHouseArray::sTheObjectPtr->IO_Savegame_DoSave(io);
		GuideManager_IO_Savegame_DoSave(gGuideManager, nullptr, io);
		PlayerStatisticManager_IO_Savegame_DoSave(gPlayerStatisticManager, nullptr, io);
		DelayedActionManager_IO_Savegame_DoSave(gDelayedActionManager, nullptr, io);
		TributeManager_IO_Savegame_DoSave(gTributeManager, nullptr, io);
		EmbeddedSavegameManager_IO_Savegame_DoSave(gEmbeddedSavegameManager, nullptr, io);
		HappinessManager_IO_Savegame_DoSave(gHappinessManager, nullptr, io);
		MultiPlayerGoalManager_IO_Savegame_DoSave(gMultiPlayerGoalManager, nullptr, io);
		GameAssistantManager_IO_Savegame_DoSave(gGameAssistantManager, nullptr, io);
		MissionOnscreenInfoManager_IO_Savegame_DoSave(gMissionOnscreenInfoManager, nullptr, io);
		io->IO_Tool_Chunk_WriteEndChunk();
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x14, false, static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(io)), 0, 0);
	}

	// Original function: FUN_00425a7b; stack owners destruct in the Windows order.
	static void l_GameIO_Savegame_DoSave(const char* fullFilename, const char* info, std::int32_t slotId)
	{
		CIoHelper io;
		NXBasics::CFile file(fullFilename, false);
		file.OpenForWriting(true);
		io.IO_File_Init(&file);
		NXBasics::CMemory memory(10000);
		io.IO_Memory_Init(&memory);
		NXBasics::CMemory packing(10000);
		io.mPackingBuffer = &packing;
		l_GameIO_Savegame_DoSave(&io, info, slotId);
	}

	// Original function: FUN_00425a35.
	void GameIO_Savegame_Save(const char* filename, const char* info, std::int32_t slotId)
	{
		char fullFilename[260];
		gSavegamesManager->Tool_CreateFullSavegameFilename(filename, fullFilename);
		NXBasics::CFile::FileSystem_Tool_CreateDirectory(gSavegamesManager->Tool_GetCurrentPlayerSavegameDirectoryNamePtr());
		l_GameIO_Savegame_DoSave(fullFilename, info, slotId);
	}

	namespace
	{
		template<class T> void LoadNativeArray(CIoHelper* io)
		{
			if (void* storage = NXBasics::Runtime_AllocateWithNewHandler(sizeof(T), 1)) ::new (storage) T(io);
		}
		void LoadOriginalManager(std::uint32_t size, void* (__fastcall* constructor)(void*, void*, CIoHelper*), CIoHelper* io)
		{
			if (void* storage = NXBasics::Runtime_AllocateWithNewHandler(size, 1)) constructor(storage, nullptr, io);
		}
	}

	// Original function: FUN_00425df1.
	static void l_GameIO_Savegame_DoLoad(CIoHelper* io, bool network)
	{
		if (l_GameIO_Savegame_DoLoad_CheckHeaderAndGetInfos(*io, nullptr) == -1) return;
		Game_ShutDown();
		while (io->IO_File_Chunk_ReadHeader())
		{
			switch (io->mReadHeader.marker)
			{
				case NRemaster::CivilizationBuildings::SaveChunk: NRemaster::CivilizationBuildings::ReadSaveManifest(io); break;
				case SwitchIds::SavegameChunk::GameAssistant: LoadOriginalManager(NC2Layout::ObjectSize::GameAssistantManager, GameAssistantManager_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::Animals: LoadNativeArray<CAnimalArray>(io); break;
				case SwitchIds::SavegameChunk::DelayedActions: LoadOriginalManager(NC2Layout::ObjectSize::DelayedActionManager, DelayedActionManager_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::AI: LoadOriginalManager(NC2Layout::ObjectSize::AIManager, AIManager_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::EmbeddedSavegame: LoadOriginalManager(8, EmbeddedSavegameManager_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::Guides: LoadOriginalManager(NC2Layout::ObjectSize::GuideManager, GuideManager_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::Houses: LoadNativeArray<CHouseArray>(io); break;
				case SwitchIds::SavegameChunk::Happiness: LoadOriginalManager(NC2Layout::ObjectSize::HappinessManager, HappinessManager_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::Humans: LoadNativeArray<CHumanArray>(io); break;
				case SwitchIds::SavegameChunk::Merchants: LoadOriginalManager(NC2Layout::ObjectSize::HumanMerchantArray, HumanMerchantArray_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::MapMoveables: LoadOriginalManager(NC2Layout::ObjectSize::MapMoveableArray, MapMoveableArray_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::Missions: LoadOriginalManager(NC2Layout::ObjectSize::MissionManager, MissionManager_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::Markers: LoadOriginalManager(NC2Layout::ObjectSize::MarkerArray, MarkerArray_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::Pathfinders: LoadOriginalManager(NC2Layout::ObjectSize::MapPathfinderManager, MapPathfinderManager_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::Tributes: LoadOriginalManager(NC2Layout::ObjectSize::TributeManager, TributeManager_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::Vehicles: LoadNativeArray<CVehicleArray>(io); break;
				case SwitchIds::SavegameChunk::GameState:
					{
						io->IO_File_ReadBinary(&gGameStaticVars.mapGuid, 0x98);
						gGameStaticVars.networkGame = static_cast<std::uint8_t>(network);
						auto* map = gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(gGameStaticVars.mapGuid);
						if (map != nullptr && map->userMap != 0)
						{
							void* library = NXBasics::Runtime_AllocateWithNewHandler(20, 1);
							if (library != nullptr) library = NXBasics::Archive_Initialize(library, map->archiveFilename);
							gGameStaticVars.mapFileLibrary = library;
							NXBasics::CFile::FileSystem_FileLibrary_Add(library);
						}
						break;
					}
				case SwitchIds::SavegameChunk::GameControl: LoadOriginalManager(NC2Layout::ObjectSize::GameControl, GameControl_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::GameMap: LoadOriginalManager(1, GameMap_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::MultiplayerGoals: LoadOriginalManager(NC2Layout::ObjectSize::MultiPlayerGoalManager, MultiPlayerGoalManager_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::MissionOnscreenInfo: LoadOriginalManager(NC2Layout::ObjectSize::MissionOnscreenInfoManager, MissionOnscreenInfoManager_ConstructFromSavegame, io); break;
				case SwitchIds::SavegameChunk::PlayerStatistics: LoadOriginalManager(NC2Layout::ObjectSize::PlayerStatisticManager, PlayerStatisticManager_ConstructFromSavegame, io); break;
				default: io->IO_File_Chunk_Skip(); break;
			}
		}
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x15, false, static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(io)), 0, 0);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x16, false, 0, 0, 0);
		const auto now = ::timeGetTime();
		gGameInitialized = 1;
		gGameTime.lastRealTime = static_cast<double>(now);
	}

	// Original function: FUN_00425b68.
	static void GameIO_Savegame_Load_Do(const char* fullFilename, bool network)
	{
		if (!NXBasics::CFile::FileSystem_Tool_FileExists(fullFilename, false)) return;
		NXBasics::CFile file(fullFilename, false);
		file.OpenForReading(true, true);
		CIoHelper io;
		io.IO_File_Init(&file);
		NXBasics::CMemory memory(10000);
		io.IO_Memory_Init(&memory);
		l_GameIO_Savegame_DoLoad(&io, network);
	}

	// Original function: FUN_00425b1a.
	void GameIO_Savegame_Load(const char* filename, bool network)
	{
		gGameStaticVars.retainedMapRoot[0] = '\0';
		gSavegamesManager->SavegameList_Update(network ? 2 : 1, false);
		char fullFilename[260];
		gSavegamesManager->Tool_CreateFullSavegameFilename(filename, fullFilename);
		GameIO_Savegame_Load_Do(fullFilename, network);
	}

	// Original function: FUN_004075cb.
	void System_RestartCurrentMap()
	{
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x2E, true, 0, 0, 0);
		const bool embedded = EmbeddedSavegameManager_Savegame_RestoreEmbedded(gEmbeddedSavegameManager, "sys_temp");
		if (auto* map = gCleanMapsManager->CleanMapList_GetInfoPtrByGuid(gGameStaticVars.mapGuid))
		{
			if (map->archiveFilename[0] == '\0') GameIO_Cleanmap_Load(map->mapRoot, true, true, false);
			else GameIO_Cleanmap_LoadX(nullptr, map->archiveFilename, true, true, false);
		}
		if (embedded)
		{
			EmbeddedSavegameManager_Savegame_DoEmbed(gEmbeddedSavegameManager, "sys_temp");
			char fullFilename[260];
			gSavegamesManager->Tool_CreateFullSavegameFilename("sys_temp", fullFilename);
			NXBasics::Runtime_RemoveFile(fullFilename);
		}
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x2D, true, 0, 0, 0);
	}

	// Original function: FUN_0040768a; save/load return values are not checked by Windows.
	bool SubMap_StartSubMap(std::uint32_t campaignId, std::uint32_t mapId, std::uint32_t pathfinderLimitOverride)
	{
		auto* map = gCleanMapsManager->CleanMapList_GetCampaignMapInfoPtr(campaignId, mapId);
		if (map == nullptr) return false;
		GameIO_Savegame_Save("sys_temp", nullptr, 8000);
		gGameControlRuntimeStorage.pathfinderLimitOverridePending = 1;
		gGameControlRuntimeStorage.pathfinderLimitOverride = pathfinderLimitOverride;
		GameIO_Cleanmap_Load(map->mapRoot, true, true, false);
		// Loading replaces the embedded-savegame manager; obtain its current owner now.
		const bool embedded = EmbeddedSavegameManager_Savegame_DoEmbed(gEmbeddedSavegameManager, "sys_temp");
		char fullFilename[260];
		gSavegamesManager->Tool_CreateFullSavegameFilename("sys_temp", fullFilename);
		NXBasics::Runtime_RemoveFile(fullFilename);
		if (embedded) CCallbackManager::sTheObjectPtr->ExecuteCallback(0x2B, true, 0, 0, 0);
		return embedded;
	}

	// Original function: FUN_0040773a; only an absent embedded save returns false.
	bool SubMap_BackToParent()
	{
		if (!EmbeddedSavegameManager_Savegame_RestoreEmbedded(gEmbeddedSavegameManager, "sys_temp")) return false;
		GameIO_Savegame_Load("sys_temp", false);
		char fullFilename[260];
		gSavegamesManager->Tool_CreateFullSavegameFilename("sys_temp", fullFilename);
		NXBasics::Runtime_RemoveFile(fullFilename);
		CCallbackManager::sTheObjectPtr->ExecuteCallback(0x2C, true, 0, 0, 0);
		return true;
	}

}
