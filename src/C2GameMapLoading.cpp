#include "C2GameMapStorage.h"
#include "C2CallbackManager.h"
#include "C2DataIds.h"
#include "C2GameMapAccess.h"
#include "C2GameMapIO.h"
#include "C2IoHelper.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2MapResourceManagers.h"
#include "C2MapSectorAttachment.h"
#include "C2MapWalkSectorManager.h"
#include "C2ObjectLayout.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

#include <new>
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Native map-validation callback order is shared by construction and loading.
	extern bool __cdecl GameMap_l_ValidateLandscapes_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl GameMap_l_Continents_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl GameMap_l_ResistanceSet_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl GameMap_l_WalkFlagSet_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	extern bool __cdecl GameMap_l_MaxMoveableSize_Callback(TCallbackTypes, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);

	namespace
	{
		template <class T, class... Args>
		void ConstructMapManager(Args&... args)
		{
			void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(T), 1);

			if (allocation != nullptr)
				new (allocation) T(args...);
		}

		// Original function: FUN_00423611; callback priorities and registration order are retained.
		void GameMap_l_RegisterCallbacks()
		{
			const TCallbackFunction callbacks[] = { GameMap_l_MaxMoveableSize_Callback, GameMap_l_WalkFlagSet_Callback, GameMap_l_ResistanceSet_Callback, GameMap_l_Continents_Callback, GameMap_l_ValidateLandscapes_Callback };
			const std::int32_t priorities[] = { 10, 20, 30, 40, 50 };

			for (std::uint32_t index = 0; index < 5; ++index)
			{
				for (std::uint32_t type : { 0x31u, 0x32u, 0x33u })
				{
					CCallbackManager::sTheObjectPtr->RegisterCallback(type, callbacks[index], 0, priorities[index]);
				}
			}
		}
	}

	// Original function: FUN_004231d7.
	void* __fastcall GameMap_Construct(void* map, void* /*unusedEdx*/, std::int16_t width, std::int16_t height)
	{
		gGameMap = map;
		GameMap_InitAndCreateEmptyMap(width, height);
		ConstructMapManager<CMapWalkSectorManager>();
		ConstructMapManager<CGameMapSectorAttachmentManager>();
		ConstructMapManager<CMapLandscapeAutoTransitionManager>();
		ConstructMapManager<CMapBioRegrowManager>();
		ConstructMapManager<CMapFishManager>();
		ConstructMapManager<CRoadBuildManager>();
		ConstructMapManager<CWallBuildManager>();
		GameMap_l_RegisterCallbacks();
		CCallbackManager::sTheObjectPtr->ExecuteCallback(2, true, static_cast<std::uint32_t>(static_cast<std::int32_t>(width)), static_cast<std::uint32_t>(static_cast<std::int32_t>(height)), 0);

		return map;
	}

	// Original function: FUN_0042fbf4.
	bool GameMap_l_GlobalIO_Load(CIoHelper* io)
	{
		while (io->IO_File_Chunk_ReadHeader())
		{
			const std::uint32_t marker = io->mReadHeader.marker;
			// Each chunk reacquires the current static pointers, as the original does.
			auto* scratch = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::PackingScratchBuffer);
			auto* macroPoints = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointStorage);
			auto* microPoints = Read<std::byte*>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointStorage);
			const std::uint32_t macroCount = Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MacroPointCount);
			const std::uint32_t microCount = Read<std::uint32_t>(gGameMapStaticStorage, NC2Layout::GameMapOffset::MicroPointCount);

			const auto readMacroBytePlane = [&](std::uint32_t offset)
			{
				io->IO_File_ReadAndUnpackBinary(scratch, macroCount);

				for (std::uint32_t index = 0; index < macroCount; ++index)
				{
					macroPoints[index * NC2Layout::RecordSize::MapMacroPointUnsigned + offset] = scratch[index];
				}
			};
			const auto readMicroBytePlane = [&](std::uint32_t offset)
			{
				io->IO_File_ReadAndUnpackBinary(scratch, microCount);

				for (std::uint32_t index = 0; index < microCount; ++index)
				{
					microPoints[index * NC2Layout::RecordSize::MapMicroPointUnsigned + offset] = scratch[index];
				}
			};
			const auto readMicroFlagPlane = [&](std::uint32_t offset, std::uint8_t mask, std::uint32_t shift)
			{
				io->IO_File_ReadAndUnpackBinary(scratch, microCount);

				for (std::uint32_t index = 0; index < microCount; ++index)
				{
					auto* record = microPoints + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
					const std::uint8_t value = static_cast<std::uint8_t>(static_cast<std::uint8_t>(scratch[index]) << shift);
					Write<std::uint8_t>(record, offset, (Read<std::uint8_t>(record, offset) & ~mask) | (value & mask));
				}
			};
			const auto readMicroWordPlane = [&](std::uint32_t offset)
			{
				io->IO_File_ReadAndUnpackBinary(scratch, microCount * 2u);

				for (std::uint32_t index = 0; index < microCount; ++index)
				{
					Write<std::uint16_t>(microPoints + index * NC2Layout::RecordSize::MapMicroPointUnsigned, offset, Read<std::uint16_t>(scratch, index * NC2Layout::RecordSize::SerializedWordUnsigned));
				}
			};

			switch (marker)
			{
				case SwitchIds::LogicMapChunk::Continents:
					io->IO_File_ReadBinary(gGameMapStaticStorage + NC2Layout::GameMapOffset::ContinentCount, NC2Layout::RecordSize::MapContinentRegion);
					break;
				case SwitchIds::LogicMapChunk::CleanFishManager:
					static_cast<CMapFishManager*>(gMapFishManager)->IO_Cleanmap_Load(*io);
					static_cast<CMapFishManager*>(gMapFishManager)->RemoveInvalidSwarms();
					break;
				case SwitchIds::LogicMapChunk::CleanWalkSectors:
					CMapWalkSectorManager::IO_Cleanmap_Load(*io);
					break;
				case SwitchIds::LogicMapChunk::MacroHeight:
					readMacroBytePlane(NC2Layout::MapMacroPointOffset::Height);
					break;
				case SwitchIds::LogicMapChunk::MacroPatternA:
					readMacroBytePlane(NC2Layout::MapMacroPointOffset::PatternA);
					break;
				case SwitchIds::LogicMapChunk::MacroPatternB:
					readMacroBytePlane(NC2Layout::MapMacroPointOffset::PatternB);
					break;
				case SwitchIds::LogicMapChunk::MicroContinent:
					readMicroBytePlane(NC2Layout::MapMicroPointOffset::ContinentId);
					break;
				case SwitchIds::LogicMapChunk::MicroWalkConnections:
					readMicroBytePlane(NC2Layout::MapMicroPointOffset::WalkingDirections);
					break;
				case SwitchIds::LogicMapChunk::MicroMaximumMoveableSize:
					readMicroFlagPlane(NC2Layout::MapMicroPointOffset::WalkingFlags, 0x07, 0);
					break;
				case SwitchIds::LogicMapChunk::MicroPathResistance:
					readMicroFlagPlane(NC2Layout::MapMicroPointOffset::WalkingFlags, 0x78, 3);
					break;
				case SwitchIds::LogicMapChunk::MicroWalkingBlocked:
					readMicroFlagPlane(NC2Layout::MapMicroPointOffset::WalkingFlags, 0x80, 7);
					break;
				case SwitchIds::LogicMapChunk::MicroBuildingBlocked:
					readMicroFlagPlane(NC2Layout::MapMicroPointOffset::TerrainFlags, 1, 0);
					break;
				case SwitchIds::LogicMapChunk::MicroRoad:
					readMicroFlagPlane(NC2Layout::MapMicroPointOffset::TerrainFlags, 0x02, 1);
					break;
				case SwitchIds::LogicMapChunk::MicroSectorBasePoint:
					readMicroFlagPlane(NC2Layout::MapMicroPointOffset::TerrainFlags, 0x10, 4);
					break;
				case SwitchIds::LogicMapChunk::MicroHouseBuildForbidden:
					readMicroFlagPlane(NC2Layout::MapMicroPointOffset::TerrainFlags, 0x20, 5);
					break;
				case SwitchIds::LogicMapChunk::Exploration:
					readMicroWordPlane(NC2Layout::MapMicroPointOffset::ExplorationMask);
					break;
				case SwitchIds::LogicMapChunk::FirstMapMoveableId:
					readMicroWordPlane(NC2Layout::MapMicroPointOffset::FirstMapMoveableId);
					break;
				case SwitchIds::LogicMapChunk::LandscapePlayer:
				case SwitchIds::LogicMapChunk::LandscapeValency:
					io->IO_File_ReadAndUnpackBinary(scratch, microCount);

					for (std::uint32_t index = 0; index < microCount; ++index)
					{
						auto* record = microPoints + index * NC2Layout::RecordSize::MapMicroPointUnsigned;

						if (Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectType) == 1)
							record[marker == SwitchIds::LogicMapChunk::LandscapePlayer ? 4 : 3] = scratch[index];
					}

					break;
				case SwitchIds::LogicMapChunk::LandscapeType:
					io->IO_File_ReadAndUnpackBinary(scratch, microCount);

					for (std::uint32_t index = 0; index < microCount; ++index)
					{
						auto* record = microPoints + index * NC2Layout::RecordSize::MapMicroPointUnsigned;

						if (scratch[index] != std::byte{ 0 })
						{
							record[0] = std::byte{ 1 };
							record[2] = scratch[index];
							record[3] = std::byte{ 1 };
							record[4] = std::byte{ NXBasics::IntegerRange::UInt8MaximumSigned };
						}
					}

					break;
				case SwitchIds::LogicMapChunk::AttachedObjects:
				case SwitchIds::LogicMapChunk::Guides:
				case SwitchIds::LogicMapChunk::Houses:
					io->IO_File_ReadAndUnpackBinary(scratch, microCount * 2u);

					for (std::uint32_t index = 0; index < microCount; ++index)
					{
						auto* record = microPoints + index * NC2Layout::RecordSize::MapMicroPointUnsigned;
						const auto value = Read<std::uint16_t>(scratch, index * NC2Layout::RecordSize::SerializedWordUnsigned);

						if (marker == SwitchIds::LogicMapChunk::AttachedObjects)
						{
							if (value != 0)
							{
								record[0] = std::byte{ 4 };
								Write<std::uint16_t>(record, NC2Layout::MapMicroPointOffset::ObjectData, value);
							}
						}
						else if (value != NXBasics::IntegerRange::UInt16Maximum)
						{
							if (marker == SwitchIds::LogicMapChunk::Guides)
							{
								record[0] = std::byte{ 3 };
								Write<std::uint16_t>(record, NC2Layout::MapMicroPointOffset::ObjectData, value);
							}
							else
							{
								record[0] = std::byte{ 2 };
								Write<std::uint16_t>(record, NC2Layout::MapMicroPointOffset::HouseId, value & NXBasics::BitMask::Low14Bits);
								Write<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectData, (Read<std::uint8_t>(record, NC2Layout::MapMicroPointOffset::ObjectData) & NXBasics::BitMask::ByteExceptBits0Through1) | (value >> 15) | ((value >> 13) & 2u));
							}
						}
					}

					break;
				case SwitchIds::LogicMapChunk::SectorAttachments:
					ConstructMapManager<CGameMapSectorAttachmentManager>(*io);
					break;
				case SwitchIds::LogicMapChunk::BioRegrowth:
					ConstructMapManager<CMapBioRegrowManager>(*io);
					break;
				case SwitchIds::LogicMapChunk::WalkSectors:
					ConstructMapManager<CMapWalkSectorManager>(*io);
					break;
				case SwitchIds::LogicMapChunk::AutoTransitions:
					ConstructMapManager<CMapLandscapeAutoTransitionManager>(*io);
					break;
				case SwitchIds::LogicMapChunk::FishManager:
					ConstructMapManager<CMapFishManager>(*io);
					break;
				case SwitchIds::LogicMapChunk::RoadBuildManager:
					ConstructMapManager<CRoadBuildManager>(*io);
					break;
				case SwitchIds::LogicMapChunk::WallBuildManager:
					ConstructMapManager<CWallBuildManager>(*io);
					break;
				default:
					io->IO_File_Chunk_Skip();
					break;
			}
		}
		// End markers and rejected headers both terminate with success in Windows.

		return true;
	}

	// Original function: FUN_0042fb59; the size chunk is consumed before the map children.
	void* __fastcall GameMap_ConstructFromSavegame(void* map, void* /*unusedEdx*/, CIoHelper* io)
	{
		gGameMap = map;
		std::uint32_t version;

		if (io->IO_File_Chunk_CheckMarkerAndGetVersion(NC2DataIds::LogicMapChunk::MapSize, &version, nullptr))
		{
			const auto width = io->IO_File_ReadLong();
			const auto height = io->IO_File_ReadLong();
			GameMap_InitAndCreateEmptyMap(static_cast<std::int16_t>(width), static_cast<std::int16_t>(height));
			GameMap_l_GlobalIO_Load(io);
			GameMap_l_RegisterCallbacks();
			CCallbackManager::sTheObjectPtr->ExecuteCallback(2, true, static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(width))), static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(height))), 0);
		}

		return map;
	}

	// Original function: FUN_00426a89. Shared by the transition and bio-regrow arrays.
	static void MapBioRegrowManager_FreeAll(void* manager)
	{
		FreeAllocatedRecords(manager, NC2Layout::RecordSize::LandscapeTimer);
	}

	// Original function: FUN_004269a0.
	static void MapBioRegrowManager_Destruct(void* manager)
	{
		MapBioRegrowManager_FreeAll(manager);
		FreeAndClearField(manager, NC2Layout::OwnedArrayHandlerOffset::OwnedEntries);
		gMapBioRegrowManager = nullptr;
		Write<std::uint8_t>(manager, NC2Layout::ArrayHandlerOffset::Initialized, 0);
	}

	// Original function: FUN_00426e9b.
	static void MapLandscapeAutoTransitionManager_Destruct(void* manager)
	{
		MapBioRegrowManager_FreeAll(manager);
		FreeAndClearField(manager, NC2Layout::OwnedArrayHandlerOffset::OwnedEntries);
		gMapLandscapeAutoTransitionManager = nullptr;
		Write<std::uint8_t>(manager, NC2Layout::ArrayHandlerOffset::Initialized, 0);
	}

	// Original function: FUN_00427343.
	static void WallBuildManager_Destruct(void* manager)
	{
		for (std::int32_t index = 0; index < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++index)
		{
			void* data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[index];

			if (data != nullptr)
				NXBasics::Runtime_Free(data);
		}

		FreeField(manager, NC2Layout::BuildManagerOffset::PlayerRecords);
		gWallBuildManager = nullptr;
	}

	// Original function: FUN_00427fb8.
	static void RoadBuildManager_Destruct(void* manager)
	{
		for (std::int32_t index = 0; index < Read<std::int32_t>(manager, NC2Layout::BuildManagerOffset::PlayerCount); ++index)
		{
			void* data = Read<std::byte**>(manager, NC2Layout::BuildManagerOffset::PlayerRecords)[index];

			if (data != nullptr)
				NXBasics::Runtime_Free(data);
		}

		FreeField(manager, NC2Layout::BuildManagerOffset::PlayerRecords);
		gRoadBuildManager = nullptr;
	}

	// Original function: FUN_0044e65b.
	static void MapFishManager_Destruct(void*)
	{
		Unregister(MapFishManager_l_FishManager_Callback, 0x65, { 1, 0x33, 0x32, 0x31 });
		gMapFishManager = nullptr;
	}

	// Original function: FUN_0042329f.
	void GameMap_Destruct(void*)
	{
		CCallbackManager::sTheObjectPtr->ExecuteCallback(2, false, 0, 0, 0);

		for (TCallbackFunction callback : { GameMap_l_ValidateLandscapes_Callback, GameMap_l_Continents_Callback, GameMap_l_ResistanceSet_Callback, GameMap_l_WalkFlagSet_Callback, GameMap_l_MaxMoveableSize_Callback })
		{
			Unregister(callback, 0, { 0x33, 0x32, 0x31 });
		}

		GameMap_ReleaseMapBuffers();
		DestroyOwned(gWallBuildManager, WallBuildManager_Destruct);
		DestroyOwned(gRoadBuildManager, RoadBuildManager_Destruct);
		DestroyOwned(gMapFishManager, MapFishManager_Destruct);
		DestroyOwned(gMapBioRegrowManager, MapBioRegrowManager_Destruct);
		DestroyOwned(gMapLandscapeAutoTransitionManager, MapLandscapeAutoTransitionManager_Destruct);
		DestroyOwned(CGameMapSectorAttachmentManager::sTheObjectPtr, [](CGameMapSectorAttachmentManager* manager)
		             {
			             manager->~CGameMapSectorAttachmentManager();
		             });
		DestroyOwned(CMapWalkSectorManager::sTheObjectPtr, [](CMapWalkSectorManager* manager)
		             {
			             manager->~CMapWalkSectorManager();
		             });
		std::memset(gGameMapStaticStorage, 0, sizeof(gGameMapStaticStorage));
		gMapPointDimensions.macroWidth = 0;
		gMapPointDimensions.macroHeight = 0;
		gMapPointDimensions.microWidth = 0;
		gMapPointDimensions.microHeight = 0;
		gGameMap = nullptr;
	}
}